/*****************************************************************************

    MPEG Video Packetizer

    Copyright(C) 2004 John Cannon <spyder@matroska.org>

    This program is free software ; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation ; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY ; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program ; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA

 **/

#include <string.h>
#include "M2VParser.h"

//#include "common.h"
#define safemalloc(x) malloc(x)
#define safefree(x)   free(x)

#define BUFF_SIZE 2*1024*1024

MPEGFrame::MPEGFrame(binary* data, uint32_t size, bool bCopy){
  if(bCopy){
    this->data  = (binary *)safemalloc(size);
    memcpy(this->data, data, size);
  }else{
    this->data = data;
  }
  this->bCopy = bCopy;
  this->size = size;
  firstRef = -1;
  secondRef = -1;
  seqHdrData = NULL;
  seqHdrDataSize = 0;
}

MPEGFrame::~MPEGFrame(){
  safefree(data);
  safefree(seqHdrData);
}

void M2VParser::SetEOS(){
  MPEGChunk * c;
  while((c = mpgBuf->ReadChunk())){
    chunks.push_back(c);
  }
  mpgBuf->ForceFinal();  //Force the last frame out.
  c = mpgBuf->ReadChunk();
  if(c) chunks.push_back(c);
  FillQueues();
  m_eos = true;
}


int32_t M2VParser::WriteData(binary* data, uint32_t dataSize){
  //If we are at EOS
  if(m_eos)
    return -1;

  if(mpgBuf->Feed(data, dataSize)){
    return -1;
  }

  //Fill the chunks buffer
  while(mpgBuf->GetState() == MPEG2_BUFFER_STATE_CHUNK_READY){
    MPEGChunk * c = mpgBuf->ReadChunk();
    if(c) chunks.push_back(c);
  }

  if(needInit){
    if(InitParser() == 0)
      needInit = false;
  }

  FillQueues();
  if(buffers.empty()){
    parserState = MPV_PARSER_STATE_NEED_DATA;
  }else{
    parserState = MPV_PARSER_STATE_FRAME;
  }

  return 0;
}

void M2VParser::DumpQueues(){
  while(!chunks.empty()){
    delete chunks.front();
    chunks.erase(chunks.begin());
  }
  while(!buffers.empty()){
    delete buffers.front();
    buffers.pop();
  }
}

/*Fraction ConvertFramerateToFraction(float frameRate){
  if(frameRate == 24000/1001.0f)
  return Fraction(24000,1001);
  else if(frameRate == 30000/1001.0f)
  return Fraction(30000,1001);
  else
  return Fraction(frameRate);
  }*/

M2VParser::M2VParser(){

  mpgBuf = new MPEGVideoBuffer(BUFF_SIZE);

  notReachedFirstGOP = true;
  currentStampingTime = 0;
  position = 0;
  m_eos = false;
  mpegVersion = 1;
  needInit = true;
  parserState = MPV_PARSER_STATE_NEED_DATA;
  firstRef = -1;
  secondRef = -1;
  nextSkip = -1;
  nextSkipDuration = -1;
  seqHdrChunk = NULL;
  gopChunk = NULL;
  keepSeqHdrsInBitstream = true;
}

int32_t M2VParser::InitParser(){
  //Gotta find a sequence header now
  MPEGChunk* chunk;
  //MPEGChunk* seqHdrChunk;
  for(size_t i = 0; i < chunks.size(); i++){
    chunk = chunks[i];
    if(chunk->GetType() == MPEG_VIDEO_SEQUENCE_START_CODE){
      //Copy the header for later, we must copy because the actual chunk will be deleted in a bit
      binary * hdrData = new binary[chunk->GetSize()];
      memcpy(hdrData, chunk->GetPointer(), chunk->GetSize());
      seqHdrChunk = new MPEGChunk(hdrData, chunk->GetSize()); //Save this for adding as private data...
      ParseSequenceHeader(chunk, m_seqHdr);

      //Look for sequence extension to identify mpeg2
      binary* pData = chunk->GetPointer();
      for(size_t j = 3; j < chunk->GetSize() - 4; j++){
        if(pData[j] == 0x00 && pData[j+1] == 0x00 && pData[j+2] == 0x01 && pData[j+3] == 0xb5 && ((pData[j+4] & 0xF0) == 0x10)){
          mpegVersion = 2;
          break;
        }
      }
      return 0;
    }
  }
  return -1;
}

M2VParser::~M2VParser(){
  DumpQueues();
  delete seqHdrChunk;
  delete gopChunk;
  delete mpgBuf;
}

const MediaTime M2VParser::GetPosition(){
  return position;
}

MediaTime M2VParser::GetFrameDuration(MPEG2PictureHeader picHdr){
  if(m_seqHdr.progressiveSequence){
    if(!picHdr.topFieldFirst && picHdr.repeatFirstField)
      return 4;
    else if(picHdr.topFieldFirst && picHdr.repeatFirstField)
      return 6;
    else
      return 2;
  }
  if(picHdr.pictureStructure != MPEG2_PICTURE_TYPE_FRAME){
    return 1;
  }
  if(picHdr.progressive && picHdr.repeatFirstField){ //TODO: fix this to support progressive sequences also.
    return 3;
  }else{
    return 2;
  }
}

MPEG2ParserState M2VParser::GetState(){
  FillQueues();
  if(!buffers.empty())
    parserState = MPV_PARSER_STATE_FRAME;
  else
    parserState = MPV_PARSER_STATE_NEED_DATA;

  if(m_eos && parserState == MPV_PARSER_STATE_NEED_DATA)
    parserState = MPV_PARSER_STATE_EOS;
  return parserState;
}

MediaTime M2VParser::CountBFrames(){
  //We count after the first chunk.
  MediaTime count = 0;
  if(m_eos) return 0;
  if(notReachedFirstGOP) return 0;
  for(size_t i = 1; i < chunks.size(); i++){
    MPEGChunk* c = chunks[i];
    if(c->GetType() == MPEG_VIDEO_PICTURE_START_CODE){
      MPEG2PictureHeader h;
      ParsePictureHeader(c, h);
      if(h.frameType == MPEG2_B_FRAME){
        count += GetFrameDuration(h);
      }else{
        return count;
      }
    }
  }
  return -1;
}

int32_t M2VParser::QueueFrame(MPEGChunk* chunk, MediaTime timecode, MPEG2PictureHeader picHdr){
  MPEGFrame* outBuf;
  bool bCopy = true;
  binary* pData = chunk->GetPointer();
  uint32_t dataLen = chunk->GetSize();

  if ((seqHdrChunk && keepSeqHdrsInBitstream &&
       (MPEG2_I_FRAME == picHdr.frameType)) || gopChunk) {
    uint32_t pos = 0;
    bCopy = false;
    dataLen +=
      (seqHdrChunk && keepSeqHdrsInBitstream ? seqHdrChunk->GetSize() : 0) +
      (gopChunk ? gopChunk->GetSize() : 0);
    pData = (binary *)safemalloc(dataLen);
    if (seqHdrChunk && keepSeqHdrsInBitstream &&
        (MPEG2_I_FRAME == picHdr.frameType)) {
      memcpy(pData, seqHdrChunk->GetPointer(), seqHdrChunk->GetSize());
      pos += seqHdrChunk->GetSize();
      delete seqHdrChunk;
      seqHdrChunk = NULL;
    }
    if (gopChunk) {
      memcpy(pData + pos, gopChunk->GetPointer(), gopChunk->GetSize());
      pos += gopChunk->GetSize();
      delete gopChunk;
      gopChunk = NULL;
    }
    memcpy(pData + pos, chunk->GetPointer(), chunk->GetSize());
  }

  MediaTime duration = GetFrameDuration(picHdr);

  outBuf = new MPEGFrame(pData, dataLen, bCopy);

  if (seqHdrChunk && !keepSeqHdrsInBitstream &&
      (MPEG2_I_FRAME == picHdr.frameType)) {
    outBuf->seqHdrData = (binary *)safemalloc(seqHdrChunk->GetSize());
    outBuf->seqHdrDataSize = seqHdrChunk->GetSize();
    memcpy(outBuf->seqHdrData, seqHdrChunk->GetPointer(),
           outBuf->seqHdrDataSize);
    delete seqHdrChunk;
    seqHdrChunk = NULL;
  }

  if(picHdr.frameType == MPEG2_I_FRAME){
    outBuf->frameType = 'I';
  }else if(picHdr.frameType == MPEG2_P_FRAME){
    outBuf->frameType = 'P';
  }else{
    outBuf->frameType = 'B';
  }

  outBuf->timecode = (MediaTime)(timecode * (1000000000/(m_seqHdr.frameRate*2)));
  outBuf->duration = (MediaTime)(duration * (1000000000/(m_seqHdr.frameRate*2)));

  if(outBuf->frameType == 'P'){
    outBuf->firstRef = (MediaTime)(firstRef * (1000000000/(m_seqHdr.frameRate*2)));
  }else if(outBuf->frameType == 'B'){
    outBuf->firstRef = (MediaTime)(firstRef * (1000000000/(m_seqHdr.frameRate*2)));
    outBuf->secondRef = (MediaTime)(secondRef * (1000000000/(m_seqHdr.frameRate*2)));
  }
  outBuf->rff = (picHdr.repeatFirstField != 0);
  outBuf->tff = (picHdr.topFieldFirst != 0);
  outBuf->progressive = (picHdr.progressive != 0);
  outBuf->pictureStructure = (uint8_t) picHdr.pictureStructure;
  buffers.push(outBuf);
  return 0;
}

void M2VParser::ShoveRef(MediaTime ref){
  if(firstRef == -1){
    firstRef = ref;
  }else if(secondRef == -1){
    secondRef = ref;
  }else{
    firstRef = secondRef;
    secondRef = ref;
  }
}

//Reads frames until it can timestamp them, usually reads until it has
//an I, P, B+, P...this way it can stamp them all and set references.
//NOTE: references aren't yet used by our system but they are kept up
//with in this plugin.
int32_t M2VParser::FillQueues(){
  if(chunks.empty()){
    return -1;
  }
  bool done = false;
  while(!done){
    MediaTime myTime = currentStampingTime;
    MPEGChunk* chunk = chunks.front();
    while (chunk->GetType() != MPEG_VIDEO_PICTURE_START_CODE) {
      if (chunk->GetType() == MPEG_VIDEO_GOP_START_CODE) {
        if (gopChunk)
          delete gopChunk;
        gopChunk = chunk;

      } else if (chunk->GetType() == MPEG_VIDEO_SEQUENCE_START_CODE) {
        if (seqHdrChunk)
          delete seqHdrChunk;
        ParseSequenceHeader(chunk, m_seqHdr);
        seqHdrChunk = chunk;

      }

      chunks.erase(chunks.begin());
      if (chunks.empty())
        return -1;
      chunk = chunks.front();
    }
    MPEG2PictureHeader picHdr;
    ParsePictureHeader(chunk, picHdr);
    MediaTime bcount;
    if(myTime == nextSkip){
      myTime+=nextSkipDuration;
      currentStampingTime=myTime;
    }
    switch(picHdr.frameType){
      case MPEG2_I_FRAME:
        bcount = CountBFrames();
        if(bcount > 0){ //..BBIBB..
          myTime += bcount;
          nextSkip = myTime;
          nextSkipDuration = GetFrameDuration(picHdr);
        }else{ //IPBB..
          if(bcount == -1 && !m_eos)
            return -1;
          currentStampingTime += GetFrameDuration(picHdr);;
        }
        ShoveRef(myTime);
        QueueFrame(chunk, myTime, picHdr);
        notReachedFirstGOP = false;
        break;
      case MPEG2_P_FRAME:
        bcount = CountBFrames();
        if(firstRef == -1) break;
        if(bcount > 0){ //..PBB..
          myTime += bcount;
          nextSkip = myTime;
          nextSkipDuration = GetFrameDuration(picHdr);
        }else{ //..PPBB..
          if(bcount == -1 && !m_eos) return -1;
          currentStampingTime+=GetFrameDuration(picHdr);
        }
        ShoveRef(myTime);
        QueueFrame(chunk, myTime, picHdr);
        break;
      default: //B-frames
        if(firstRef == -1 || secondRef == -1) break;
        QueueFrame(chunk, myTime, picHdr);
        currentStampingTime+=GetFrameDuration(picHdr);
    }
    chunks.erase(chunks.begin());
    delete chunk;
    if (chunks.empty())
      return -1;
  }
  return 0;
}

MPEGFrame* M2VParser::ReadFrame(){
  //if(m_eos) return NULL;
  if(GetState() != MPV_PARSER_STATE_FRAME){
    return NULL;
  }
  if(buffers.empty()){
    return NULL; // OOPS!
  }
  MPEGFrame* frame = buffers.front();
  buffers.pop();
  return frame;
}

