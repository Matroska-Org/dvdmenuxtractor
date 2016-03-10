// ============================================================================
// VOBParser class
// Copyright © 2002 : Christophe PARIS (christophe.paris@free.fr)
// ============================================================================

#include <QFileInfo>

#include "IFOFile.h"
#include "VobParser.h"
#include "iso/iso_lang.h"

// ----------------------------------------------------------------------------

#define VOB_SLICE			0x00000100
#define PACK_HEADER			0xBA
#define SYSTEM_HEADER		0xBB
#define PROGRAM_STREAM_MAP	0xBC
#define PRIVATE_STREAM1		0xBD
#define PADDING_STREAM		0xBE
#define PRIVATE_STREAM2		0xBF
#define CURRENT_OFFSET		(m_pktindex * DVD_VIDEO_LB_LEN + m_index)

#define INDENT_UNIT 2
unsigned int indent_lvl = 0;
inline void inc_lvl() { indent_lvl += INDENT_UNIT; }
inline void dec_lvl() { indent_lvl -= INDENT_UNIT; }
void debug (const QString& str)
{
	QString debugStr (indent_lvl, ' ');
	debugStr += str;
	qDebug(qPrintable(debugStr));
}

// ----------------------------------------------------------------------------
// CompositeDemuxWriter
// ----------------------------------------------------------------------------

CompositeDemuxWriter::CompositeDemuxWriter()
{
	for (int i=0; i<256; i++)
		m_muxers[i] = NULL;
}

CompositeDemuxWriter::~CompositeDemuxWriter()
{
	Reset();
}

bool CompositeDemuxWriter::AddDemuxer(uint8_t streamID, Writer * demuxer, QString& CommandLine)
{
	if (m_muxers[streamID] != NULL)
		return false;
	m_muxers[streamID] = demuxer;
	m_strings[streamID] = CommandLine;
	return true;
}

void CompositeDemuxWriter::ProcessStream(int streamID, uint8_t* buff, uint32_t size, int32_t start_time, int32_t end_time, const QString& debug)
{
	if (m_muxers[streamID] != NULL)
		m_muxers[streamID]->ProcessStream(buff, size, start_time, end_time, debug);
}

void CompositeDemuxWriter::Reset()
{
	for (int i=0; i<256; i++)
	{
		if (m_muxers[i] != NULL)
		{
			delete m_muxers[i];
			m_muxers[i] = NULL;
		}
	}
}

void CompositeDemuxWriter::SetBoundary(uint32_t start_timecode, uint32_t duration, const CellListElem *cell)
{
	for (int i=0; i<256; i++)
	{
		if (m_muxers[i] != NULL)
		{
			m_muxers[i]->SetBoundary(start_timecode, duration, cell);
		}
	}
}

// ----------------------------------------------------------------------------

VobParser::VobParser(const char* dirname, int16_t title, bool menu)
	:m_title(title)
	,m_dvdhandle(NULL)
	,m_stream(NULL)
	,m_language(menu)
	,m_bFirstPacket(true)
{
	m_pktcount = 0;

	QFileInfo _tmpDirName(dirname);

	// handle all parts of this title (VIDEO_TS.vob or VTS_XX_Y.vob)
	m_dvdhandle = DVDOpen(QFile::encodeName(_tmpDirName.canonicalPath()));

	if (m_dvdhandle)
	{
		if (m_language)
			m_stream = DVDOpenFile(m_dvdhandle, title, DVD_READ_MENU_VOBS);
		else
			m_stream = DVDOpenFile(m_dvdhandle, title, DVD_READ_TITLE_VOBS);
	}

	if(!m_stream)
	{
		throw VobParserFileNotFoundException(dirname);
	}
	else
	{
		m_pktcount = DVDFileSize(m_stream);
	}

	Reset();
}

// ----------------------------------------------------------------------------

VobParser::~VobParser()
{
	if (m_stream)
		DVDCloseFile(m_stream);

	if (m_dvdhandle)
		DVDClose(m_dvdhandle);
}

// ----------------------------------------------------------------------------

uint32_t VobParser::GetPacketCount() const
{
	return m_pktcount;
}

// ----------------------------------------------------------------------------

uint32_t VobParser::GetPacketIndex() const
{
	return m_pktindex;
}

// ----------------------------------------------------------------------------

char* VobParser::GetCurrentPacketData() const
{
	return (char*)m_buff;
}

// ----------------------------------------------------------------------------

bool VobParser::ParseNextPacket(const CellsListType & Cells)
{
	if(GetNextPacket())
	{	
		pktinfo.identifier = GetNext32Bits();
		if((pktinfo.identifier & VOB_SLICE) != VOB_SLICE || (pktinfo.identifier & PACK_HEADER) != PACK_HEADER)
		{
			// Invalid block start code
			throw VobParserInvalidPacketException(CURRENT_OFFSET-4);
		}
		
		ParseSCR(); // System Clock Reference

		// Program Mux Rate (measured in units of 50 bytes/second)
		pktinfo.program_mux_rate = (GetNext8Bits() << 14) | 
			(GetNext8Bits() << 6) | (GetNext8Bits() >> 2);

		// Skip Pack stuffing length
		int stuffing_nb = GetNext8Bits() & 0x07;
		SkipNBytes(stuffing_nb);
		
		uint32_t _Header = GetNext32Bits();
		int _StreamID;
		if ((_Header & VOB_SLICE) == VOB_SLICE)
		{
			_StreamID = _Header & 0xFF;
			if (_StreamID == SYSTEM_HEADER)
			{
				// skip the system header data
				uint16_t _size = GetNext16Bits();
				SkipNBytes(_size);

				while (AvailablePacketData())
				{
					_Header = GetNext32Bits();
					uint8_t _StreamId = _Header & 0xFF;
					if (_StreamId == PRIVATE_STREAM2)
					{
						debug("Navigation pack {\n");
						inc_lvl();
						debug(QString("SCR: %1.%2\n").arg(pktinfo.scr).arg(pktinfo.scr_ext));
						debug(QString("Program mux rate: %1 (%2 bps)\n").arg(pktinfo.program_mux_rate).arg(pktinfo.program_mux_rate * 50 * 8));
						ParseNavPacket();
						dec_lvl();
						debug("}\n");
					}
					else
					{
						// skip these data
						uint16_t _size = GetNext16Bits();
						SkipNBytes(_size);
					}
				}
				if (IsNewCell()) {
					CellListElem* cell = Cells.at(GetVobID(), GetCellID());

					if (cell != NULL)
					{
						cell->found = true;
						if (m_dsi.nv_pck_scr == 0)
							m_pci_vob_timecode_offset = m_pci.vobu_s_ptm / 90;
						m_demuxer.SetBoundary(m_pci.vobu_s_ptm/90 - m_pci_vob_timecode_offset, cell->nb_frames * cell->frame_dur, cell);
					}
				}
				if (m_pci.btn_ns != 0)
				{ // there are some buttons
					uint32_t t3 = m_pci.vobu_s_ptm/90 - m_pci_vob_timecode_offset;
					uint32_t t4 = m_pci.vobu_e_ptm/90 - m_pci_vob_timecode_offset;
					m_demuxer.ProcessStream(SUBSTREAM_PCI, &m_buff[m_pci_position], m_pci_size+2, t3, t4, 
						QString("# %1 -> %2 / %3 / %4 / %5 / %6 / off %7\n")
						.arg(m_pci.vobu_s_ptm/90).arg(m_pci.vobu_e_ptm/90)
						.arg(m_dsi.c_eltm, 0, 16).arg(m_pci.c_eltm, 0, 16)
						.arg(t3).arg(t4).arg(m_pci_vob_timecode_offset));
						
				}
			}
			else if ((_StreamID & VIDEO_STREAM) == VIDEO_STREAM)
			{
				debug("Video pack {}\n");
				ParseVideoPacket();
			}
			else if ((_StreamID & AUDIO_STREAM) == AUDIO_STREAM)
			{
				debug("Audio pack {}\n");
				ParseAudioPacket(_StreamID);
			}
			else if (_StreamID == PRIVATE_STREAM1)
			{
				debug("Private stream 1 pack {\n");
				inc_lvl();
				ParsePrivateStream1();
				dec_lvl();
				debug("}\n");
			}
			else
			{
				uint16_t _size = GetNext16Bits();
				SkipNBytes(_size);
				debug("Unknown slice type\n");
			}

			if (m_bFirstPacket && _StreamID != SYSTEM_HEADER)
			{
				m_bFirstPacket = false;
				m_startdts = pktinfo.dts;
				m_startpts = pktinfo.pts;
			}
		}
		else
		{
			debug("Unknown start code\n");
		}
		
		m_pktindex++;
	}
	else
	{
		return false;
	}

	return true;
}

// ----------------------------------------------------------------------------

bool VobParser::AvailablePacketData() const
{
	return (m_index < DVD_VIDEO_LB_LEN);
}

// ----------------------------------------------------------------------------

bool VobParser::GetNextPacket()
{
	m_index = 0;

	return (DVDReadBlocks(m_stream, m_pktindex, 1, m_buff) == 1);
}

// ----------------------------------------------------------------------------

uint32_t VobParser::GetNext32Bits()
{
	uint32_t result = 0;
	result |= m_buff[m_index++];
	result <<= 8;
	result |= m_buff[m_index++];
	result <<= 8;
	result |= m_buff[m_index++];
	result <<= 8;
	result |= m_buff[m_index++];
    return result;
}

// ----------------------------------------------------------------------------

uint16_t VobParser::GetNext16Bits()
{
	uint32_t result = 0;
	result |= m_buff[m_index++];
	result <<= 8;
	result |= m_buff[m_index++];
	return result;
}

// ----------------------------------------------------------------------------

uint8_t VobParser::GetNext8Bits()
{
	return m_buff[m_index++];
}

// ----------------------------------------------------------------------------

void VobParser::SkipNBytes(int n)
{
	m_index += n;
}

// ----------------------------------------------------------------------------

void VobParser::ParseSCR()
{

/* From http://dvd.sourceforge.net/dvdinfo/packhdr.html

	      Byte 4                      Byte 5
	      +--+--+--+--+--+--+--+--+   +--+--+--+--+--+--+--+--+
          | 7| 6| 5| 4| 3| 2| 1| 0|   | 7| 6| 5| 4| 3| 2| 1| 0|
	      +--+--+--+--+--+--+--+--+   +--+--+--+--+--+--+--+--+
SCR bits  |f0|f1|32|31|30|f1|29|28|   |27|26|25|24|23|22|21|20|
          +--+--+--+--+--+--+--+--+   +--+--+--+--+--+--+--+--+

	      Byte 6                      Byte 7
	      +--+--+--+--+--+--+--+--+   +--+--+--+--+--+--+--+--+
          | 7| 6| 5| 4| 3| 2| 1| 0|   | 7| 6| 5| 4| 3| 2| 1| 0|
	      +--+--+--+--+--+--+--+--+   +--+--+--+--+--+--+--+--+
SCR bits  |19|18|17|16|15|f1|14|13|   |12|11|10| 9| 8| 7| 6| 5|
          +--+--+--+--+--+--+--+--+   +--+--+--+--+--+--+--+--+

	      Byte 8                      Byte 9
	      +--+--+--+--+--+--+--+--+   +--+--+--+--+--+--+--+--+
          | 7| 6| 5| 4| 3| 2| 1| 0|   | 7| 6| 5| 4| 3| 2| 1| 0|
	      +--+--+--+--+--+--+--+--+   +--+--+--+--+--+--+--+--+
SCR bits  | 4| 3| 2| 1| 0|f1|e8|e7|   |e6|e5|e4|e3|e2|f1|e0|c1|
          +--+--+--+--+--+--+--+--+   +--+--+--+--+--+--+--+--+

  fx = bit value fixed to x
  ex = bit x of scr_ext
*/

	uint8_t byte4, byte5, byte6, byte7, byte8, byte9;
	byte4 = GetNext8Bits();	byte5 = GetNext8Bits();
	byte6 = GetNext8Bits();	byte7 = GetNext8Bits();
	byte8 = GetNext8Bits();	byte9 = GetNext8Bits();

	assert((byte4 & 0xc4) == 0x44);
	assert(byte6 & 0x04);
	assert(byte8 & 0x04);
	assert(byte9 & 0x01);
		
	uint64_t scr = 0;
	scr |= ((((byte4 & 0x38) >> 1) | (byte4 & 0x03)) << 28);
	scr |= (byte5 << 20);
	scr |= ((((byte6 & 0xf8) >> 1) | (byte6 & 0x03)) << 13);
	scr |= (byte7 << 5);
	scr |= ((byte8 & 0xf8) >> 3);

	uint16_t scr_ext = 0;
	scr_ext |= ((byte8 & 0x03) << 7);
	scr_ext |= ((byte9 & 0xfe) >> 1);

	pktinfo.scr = scr;
	pktinfo.scr_ext = scr_ext;
}

// ----------------------------------------------------------------------------

void VobParser::ParseNavPacket()
{
	uint32_t endStreamIndex = 0;
	uint32_t position = m_index;
	uint16_t length = GetNext16Bits();
	endStreamIndex = m_index + length;
	uint8_t substreamID = GetNext8Bits();
	
	switch(substreamID)
	{
	case SUBSTREAM_PCI:
		debug("PCI {\n");
		inc_lvl();
		ParsePCI();
		m_pci_position = position-4; // keep the Private Stream 2 header
		m_pci_size = length+4;
		dec_lvl();
		debug("}\n");
		break;
	case SUBSTREAM_DSI:
		debug("DSI {\n");
		inc_lvl();
		ParseDSI();
		dec_lvl();
		debug("}\n");
		break;
	default:
	  debug(QString("ParseNavPacket: unknown substream id @LBA=%1\n").arg(m_pktindex));
	}
	
	m_index = endStreamIndex;
}

// ----------------------------------------------------------------------------

void VobParser::ParsePCI()
{
	int i,j;
	uint8_t onebyte;

	// PCI General Information
	m_pci.nv_pck_lbn = GetNext32Bits();
	m_pci.vobu_cat = GetNext16Bits();
	m_pci.reserved1 = GetNext16Bits();
	m_pci.vobu_uop_ctl = GetNext32Bits();
	m_pci.vobu_s_ptm = GetNext32Bits();
	m_pci.vobu_e_ptm = GetNext32Bits();
	m_pci.vobu_se_e_ptm = GetNext32Bits();
	m_pci.c_eltm = GetNext32Bits();
	memcpy(m_pci.vobu_isrc, &m_buff[m_index],32*1);	
	SkipNBytes(32*1);

	// Non Seamless Angle Information
	memcpy(m_pci.nsml_agli_dsta, &m_buff[m_index], 9*4);
	SkipNBytes(9*4);

	// Highlight General Information 
	m_pci.hli_ss = GetNext16Bits();
	m_pci.hli_s_ptm = GetNext32Bits();
	m_pci.hli_e_ptm = GetNext32Bits();
	m_pci.btn_sl_e_ptm = GetNext32Bits();
	m_pci.btn_md = GetNext16Bits();
	m_pci.btn_sn = GetNext8Bits();
	m_pci.btn_ns = GetNext8Bits();
	m_pci.nsl_btn_ns = GetNext8Bits();
	m_pci.reserved1 = GetNext8Bits();
	m_pci.fosl_btnn = GetNext8Bits();
	m_pci.foac_btnn = GetNext8Bits();
	
	// Button Color Information Table 
	for(i = 0; i < 3; i++)
		for(j = 0; j < 2; j++)
			m_pci.btn_coli[i][j] = GetNext32Bits();

	// Button Information
	for(i = 0; i < 36; i++)
	{
		onebyte = GetNext8Bits();
		m_pci.btnit[i].btn_coln = (onebyte & 0xC0) >> 6;
		m_pci.btnit[i].start_x = (onebyte & 0x3F) << 4;

		onebyte = GetNext8Bits();
		m_pci.btnit[i].start_x |= (onebyte & 0xF0) >> 4;
		m_pci.btnit[i].end_x = (onebyte & 0x03) << 8;

		onebyte = GetNext8Bits();
		m_pci.btnit[i].end_x |= onebyte;

		onebyte = GetNext8Bits();
		m_pci.btnit[i].auto_action_flag = (onebyte & 0xC0) >> 6;
		m_pci.btnit[i].start_y = (onebyte & 0x3F) << 4;
		
		onebyte = GetNext8Bits();
		m_pci.btnit[i].start_y |= (onebyte & 0xF0) >> 4;
		m_pci.btnit[i].end_y = (onebyte & 0x03) << 8;
		
		onebyte = GetNext8Bits();
		m_pci.btnit[i].end_y |= onebyte;

		m_pci.btnit[i].up = GetNext8Bits() & 0x3F;
		m_pci.btnit[i].down = GetNext8Bits() & 0x3F;
		m_pci.btnit[i].left = GetNext8Bits() & 0x3F;
		m_pci.btnit[i].right = GetNext8Bits() & 0x3F;
		
		memcpy(m_pci.btnit[i].vm_cmd, &m_buff[m_index],8*1);
		SkipNBytes(8*1);
	}

	debug(QString("nv_pck_lbn: %1\n").arg(m_pci.nv_pck_lbn));
	debug(QString("vobu_cat: %1\n").arg(m_pci.vobu_cat));
	debug(QString("reserved1: %1\n").arg(m_pci.reserved1));
	debug(QString("vobu_uop_ctl: %1\n").arg(m_pci.vobu_uop_ctl));
	debug(QString("vobu_s_ptm: %1\n").arg(m_pci.vobu_s_ptm));
	debug(QString("vobu_e_ptm: %1\n").arg(m_pci.vobu_e_ptm));
	debug(QString("vobu_se_e_ptm: %1\n").arg(m_pci.vobu_se_e_ptm));
	debug(QString("e_eltm: %1\n").arg(m_pci.c_eltm));
	debug(QString("vobu_isrc: ...\n").toLatin1());
	debug(QString("nsml_agli_dsta: ...\n").toLatin1());
	debug(QString("hli_ss: %1\n").arg(m_pci.hli_ss));
}

// ----------------------------------------------------------------------------

void VobParser::ParseDSI()
{
	m_dsi.nv_pck_scr = GetNext32Bits();
	m_dsi.nv_pck_lbn = GetNext32Bits();
	m_dsi.vobu_ea = GetNext32Bits();
	m_dsi.vobu_1stref_ea = GetNext32Bits();
	m_dsi.vobu_2ndref_ea = GetNext32Bits();
	m_dsi.vobu_3rdref_ea = GetNext32Bits();
	m_dsi.vobu_vob_idn = GetNext16Bits();
	m_dsi.reserved = GetNext8Bits();
	m_dsi.vobu_c_idn = GetNext8Bits();
	m_dsi.c_eltm = GetNext32Bits();

	debug(QString("nv_pck_scr: %1\n").arg(m_dsi.nv_pck_scr));
	debug(QString("nv_pck_lbn: %1\n").arg(m_dsi.nv_pck_lbn));
	debug(QString("vobu_ea: %1\n").arg(m_dsi.vobu_ea));
	debug(QString("vobu_1stref_ea: %1\n").arg(m_dsi.vobu_1stref_ea));
	debug(QString("vobu_2ndref_ea: %1\n").arg(m_dsi.vobu_2ndref_ea));
	debug(QString("vobu_3rdref_ea: %1\n").arg(m_dsi.vobu_3rdref_ea));
	debug(QString("vobu_vob_idn: %1\n").arg(m_dsi.vobu_vob_idn));
	debug(QString("reserved: %1\n").arg(m_dsi.reserved));
	debug(QString("vobu_c_idn: %1\n").arg(m_dsi.vobu_c_idn));
	debug(QString("c_eltm: %1\n").arg(m_dsi.c_eltm));
}

// ----------------------------------------------------------------------------

uint8_t GetBit(uint32_t data, uint8_t n)
{
	return ((data >> n) & 0x01);
}

// ----------------------------------------------------------------------------

void VobParser::ParsePESHeaderDataContentFlag()
{	
	uint8_t byte6 = GetNext8Bits();
	assert((byte6 & 0xC0) == 0x80);
	pes_header_data_content.PES_scrambling_control = (byte6 & 0x30) >> 4;
	pes_header_data_content.PES_priority = GetBit(byte6,3);
	pes_header_data_content.data_alignment_indicator = GetBit(byte6,2);
	pes_header_data_content.copyright = GetBit(byte6,1);
	pes_header_data_content.original_or_copy = GetBit(byte6,0);

	uint8_t byte7 = GetNext8Bits();
	pes_header_data_content.PTS_flag = GetBit(byte7,7);
	pes_header_data_content.DTS_flag = GetBit(byte7,6);
	pes_header_data_content.ESCR_flag = GetBit(byte7,5);
	pes_header_data_content.ES_rate_flag = GetBit(byte7,4);
	pes_header_data_content.DSM_trick_mode_flag = GetBit(byte7,3);
	pes_header_data_content.additionnal_copy_info_flag = GetBit(byte7,2);
	pes_header_data_content.PES_CRC_flag = GetBit(byte7,1);
	pes_header_data_content.PES_extension_flag = GetBit(byte7,0);
	
	pes_header_data_content.PES_header_data_len = GetNext8Bits();
}

// ----------------------------------------------------------------------------

uint64_t VobParser::ParsePTS_DTS()
{
	// PTS and DTS have same format
	uint64_t result = 0;

	uint8_t byte0 = GetNext8Bits();
	uint16_t word0 = GetNext16Bits();
	uint16_t word1 = GetNext16Bits();

	assert(word0 & 0x01);
	assert(word1 & 0x01);

	result |= (((byte0 & 0x0E) >> 1) << 30);
	result |= ((word0 >> 1) << 15);
	result |= (word1 >> 1);

	return result;
}

// ----------------------------------------------------------------------------

void VobParser::ParsePESHeaderData()
{
	uint8_t bytesLeft = pes_header_data_content.PES_header_data_len;
	if(pes_header_data_content.PTS_flag)
	{
		// PTS : Presentation Time Stamp
		pktinfo.pts = ParsePTS_DTS();
		bytesLeft -= 5;
		if (m_bFirstPacket)
		{
			m_startpts = pktinfo.pts;
		}
		pktinfo.pts -= m_startpts;
	}
	if(pes_header_data_content.DTS_flag)
	{
		// DTS : Decoding Time Stamp
		pktinfo.dts = ParsePTS_DTS();
		bytesLeft -= 5;
		if (m_bFirstPacket)
		{
			m_startdts = pktinfo.dts;
		}
		pktinfo.dts -= m_startdts;
	}
	m_bFirstPacket = false;

	// Skip the rest
	SkipNBytes(bytesLeft);
}

// ----------------------------------------------------------------------------

void VobParser::ParseVideoPacket()
{
	uint16_t length = GetNext16Bits();
	ParsePESHeaderDataContentFlag();
	ParsePESHeaderData();

	m_demuxer.ProcessStream(VIDEO_STREAM, &m_buff[m_index],
		length - 3 - pes_header_data_content.PES_header_data_len, pktinfo.dts/90, pktinfo.pts/90,
		QString("DTS %1 PTS %2 - %3 %4\n").arg(m_startdts/90).arg(m_startpts/90).arg(pktinfo.dts/90).arg(pktinfo.pts/90));
}

void VobParser::ParseAudioPacket(int StreamID)
{
	uint16_t length = GetNext16Bits();
	ParsePESHeaderDataContentFlag();
	ParsePESHeaderData();

	m_demuxer.ProcessStream(StreamID, &m_buff[m_index],
		length - 3 - pes_header_data_content.PES_header_data_len, pktinfo.pts/90, pktinfo.dts/90,
		QString("DTS %1 PTS %2 - %3 %4\n").arg(m_startdts/90).arg(m_startpts/90).arg(pktinfo.dts/90).arg(pktinfo.pts/90));
}

// ----------------------------------------------------------------------------

void VobParser::ParsePrivateStream1()
{
	uint16_t length = GetNext16Bits();	
	uint16_t dataStartIndex = m_index;
	// PES : Packetized Elementary Stream
	ParsePESHeaderDataContentFlag();
	ParsePESHeaderData();
	uint8_t substreamID = GetNext8Bits();

	uint32_t t3 = m_pci.vobu_s_ptm/90 - m_pci_vob_timecode_offset;
	uint32_t t4 = m_pci.vobu_e_ptm/90 - m_pci_vob_timecode_offset;
	if(substreamID >= SUBSTREAM_SUB_LOW && substreamID < SUBSTREAM_SUB_HIGH)
	{
		debug(QString("Subtitles streamID = 0x%1\n").arg(substreamID, 0, 16));

		// .sub files the VobSub way (includes the whole packet)
		m_demuxer.ProcessStream(substreamID, m_buff, DVD_VIDEO_LB_LEN, t3, t4, 
		    QString("DTS %1 PTS %2 - %3 %4\n").arg(m_startdts/90).arg(m_startpts/90).arg(pktinfo.dts/90).arg(pktinfo.pts/90));
	}
	else if(substreamID >= SUBSTREAM_AC3_LOW && substreamID < SUBSTREAM_AC3_HIGH)
	{
		debug(QString("AC3 streamID = 0x%1\n").arg(substreamID, 0, 16));

		// Skip frame header number
		SkipNBytes(1);
		// Skip first access unit pointer
		SkipNBytes(2);

		uint16_t ac3DataLen = length - (m_index - dataStartIndex);
		m_demuxer.ProcessStream(substreamID, &m_buff[m_index], ac3DataLen, t3, t4, 
		    QString("DTS %1 PTS %2 - %3 %4\n").arg(m_startdts/90).arg(m_startpts/90).arg(pktinfo.dts/90).arg(pktinfo.pts/90));
	}
	else if(substreamID >= SUBSTREAM_DTS_LOW && substreamID < SUBSTREAM_DTS_HIGH)
	{
		debug(QString("DTS streamID = 0x%1\n").arg(substreamID, 0, 16));
		
		// Skip frame header number
		SkipNBytes(1);
		// Skip first access unit pointer
		SkipNBytes(2);

		uint16_t ac3DataLen = length - (m_index - dataStartIndex);
		m_demuxer.ProcessStream(substreamID, &m_buff[m_index], ac3DataLen, t3, t4, 
		    QString("DTS %1 PTS %2 - %3 %4\n").arg(m_startdts/90).arg(m_startpts/90).arg(pktinfo.dts/90).arg(pktinfo.pts/90));
	}
	else if(substreamID >= SUBSTREAM_PCM_LOW && substreamID < SUBSTREAM_PCM_HIGH)
	{
		debug(QString("LPCM streamID = 0x%1\n").arg(substreamID, 0, 16));

		// Skip unknown data
		SkipNBytes(6);

		uint16_t ac3DataLen = length - (m_index - dataStartIndex);
		m_demuxer.ProcessStream(substreamID, &m_buff[m_index], ac3DataLen, t3, t4, 
			QString("DTS %1 PTS %2 - %3 %4\n").arg(m_startdts/90).arg(m_startpts/90).arg(pktinfo.dts/90).arg(pktinfo.pts/90));
	}
	else
	{
		debug("Unknown\n");
	}
}

// ----------------------------------------------------------------------------

void VobParser::Reset()
{
	memset(&m_dsi, 0, sizeof(m_dsi));
	memset(&pktinfo, 0, sizeof(pktinfo));
	memset(&pes_header_data_content, 0, sizeof(pes_header_data_content));
	previous_vobid = -1;
	previous_cellid = -1;
	m_index = 0;
	m_pktindex = 0;
	DVDFileSeek(m_stream,0);
}

// ----------------------------------------------------------------------------

bool VobParser::IsNewCell()
{
	bool result = (GetVobID() != previous_vobid ||
					GetCellID() != previous_cellid);
	
    previous_vobid = GetVobID();
    previous_cellid = GetCellID();
	if (result)
		m_bFirstPacket = true;

	return result;
}

// ----------------------------------------------------------------------------

inline uint8_t VobParser::GetVobID() const
{
	return m_dsi.vobu_vob_idn;
}

// ----------------------------------------------------------------------------

inline uint8_t VobParser::GetCellID() const
{
	return m_dsi.vobu_c_idn;
}

// ----------------------------------------------------------------------------

uint32_t VobParser::GetCellSCR() const
{
	return m_dsi.nv_pck_scr;
}

// ----------------------------------------------------------------------------
 
void SubDemuxWriter::WriteTimecodeInfo(uint32_t start_time, uint32_t end_time, uint64_t filepos, const QString& debug)
{
	int i;

	if (!m_TimecodeFile)
	{
		QString m_filename (m_Filename);
		m_filename += ".idx";
		m_TimecodeFile = fopen(QFile::encodeName(m_filename),"w");

		fwrite("# VobSub index file, v7 (do not modify this line!)\n#\n", 1, 53, m_TimecodeFile);
		fwrite("# Settings\n\n# Original frame size\nsize: ", 1, 40, m_TimecodeFile);
		fprintf(m_TimecodeFile, "%dx%d\n\n", m_width, m_height);
		fwrite("# Origin, relative to the upper-left corner, can be overloaded by aligment\n"
		       "org: 0, 0\n\n"
		       "# Image scaling (hor,ver), origin is at the upper-left corner or at the alignment coord (x, y)\n"
		       "scale: 100%, 100%\n\n"
		       "# Alpha blending\n"
		       "alpha: 100%\n\n"
		       "# Smoothing for very blocky images (use OLD for no filtering)\n"
		       "smooth: OFF\n\n"
		       "# In millisecs\n"
		       "fadein/out: 50, 50\n\n"
		       "# Force subtitle placement relative to (org.x, org.y)\n"
		       "align: OFF at LEFT TOP\n\n"
		       "# For correcting non-progressive desync. (in millisecs or hh:mm:ss:ms)\n"
		       "# Note: Not effective in DirectVobSub, use \"delay: ... \" instead.\n"
		       "time offset: 0\n\n"
		       "# ON: displays only forced subtitles, OFF: shows everything\n"
               "forced subs: OFF\n\n"
               "# The original palette of the DVD\n"
			   "palette: ", 1, 692, m_TimecodeFile);
		for (i=0; i<15; i++)
		{
			fprintf(m_TimecodeFile, "%06x, ", m_palette[i]);
		}
		fprintf(m_TimecodeFile, "%06x\n\n", m_palette[i]);
		fwrite("# Custom colors (transp idxs and the four colors)\n"
		       "custom colors: OFF, tridx: 0000, colors: 000000, 000000, 000000, 000000\n\n"
		       "# Language index in use\n"
		       "langidx: 0\n\n"
		       "id: ", 1, 163, m_TimecodeFile);
		if (m_language)
			fprintf(m_TimecodeFile, "%c%c", m_language >> 8, m_language);
		else
			fwrite("un", 1, 2, m_TimecodeFile);
		fwrite(", index: 0\n"
		       "# Decomment next line to activate alternative name in DirectVobSub / Windows Media Player 6.x\n"
		       "# alt: ", 1, 112, m_TimecodeFile);
        fprintf(m_TimecodeFile, "%s\n\n", DecodeLanguage(m_language));
	}
	int32_t delay = start_time + m_start_timecode;
	int millisecond = delay % 1000;
	delay /= 1000;
	int second = delay % 60;
	delay /= 60;
	int minute = delay % 60;
	delay /= 60;
	fprintf(m_TimecodeFile, "timestamp: %02d:%02d:%02d:%03d, filepos: %09x\n", delay,minute,second,millisecond,filepos);
}

void BtnDemuxWriter::WriteTimecodeInfo(uint32_t start_time, uint32_t end_time, uint64_t filepos, const QString& debug)
{
//static uint32_t time =0;
	m_TimecodeFile = GetTimecodeFile();

	if (start_time > m_last_end_timecode)
	{
		if (m_last_end_timecode > m_start_timecode)
			fprintf(m_TimecodeFile, "%lf\n", (m_last_end_timecode - m_start_timecode) / 1000.0);
		fprintf(m_TimecodeFile, "%lf\n", (m_last_end_timecode - m_last_start_timecode) / 1000.0);
		fprintf(m_TimecodeFile, "gap,%lf\n", (start_time - m_last_end_timecode) / 1000.0);
		m_start_timecode = start_time;
	}
//fwrite(debug.c_str(), debug.size(), 1, m_TimecodeFile);
//fprintf(m_TimecodeFile, "# %d => %d\n",time, time + end_time - start_time);
//time += end_time - start_time;
	m_last_start_timecode = start_time;
	m_last_end_timecode = end_time;
}

void DTSDemuxWriter::WriteTimecodeInfo(uint32_t start_time, uint32_t end_time, uint64_t filepos, const QString& debug)
{
	m_TimecodeFile = GetTimecodeFile();

	if (start_time > m_last_end_timecode)
	{
		if (m_last_end_timecode > m_start_timecode)
			fprintf(m_TimecodeFile, "%lf\n", (m_last_end_timecode - m_start_timecode) / 1000.0);
		fprintf(m_TimecodeFile, "%lf\n", (m_last_end_timecode - m_last_start_timecode) / 1000.0);
		fprintf(m_TimecodeFile, "gap,%lf\n", (start_time - m_last_end_timecode) / 1000.0);
		m_start_timecode = start_time;
	}
	m_last_start_timecode = start_time;
	m_last_end_timecode = end_time;
}

void LPCMDemuxWriter::WriteTimecodeInfo(uint32_t start_time, uint32_t end_time, uint64_t filepos, const QString& debug)
{
	m_TimecodeFile = GetTimecodeFile();

	if (start_time > m_last_end_timecode)
	{
		if (m_last_end_timecode > m_start_timecode)
			fprintf(m_TimecodeFile, "%lf\n", (m_last_end_timecode - m_start_timecode) / 1000.0);
		fprintf(m_TimecodeFile, "%lf\n", (m_last_end_timecode - m_start_timecode) / 1000.0);
		fprintf(m_TimecodeFile, "gap,%lf\n", (start_time - m_last_end_timecode) / 1000.0);
		m_start_timecode = start_time;
	}
	m_last_start_timecode = start_time;
	m_last_end_timecode = end_time;
}

void MPADemuxWriter::WriteTimecodeInfo(uint32_t start_time, uint32_t end_time, uint64_t filepos, const QString& debug)
{
	m_TimecodeFile = GetTimecodeFile();
}

void AC3DemuxWriter::WriteTimecodeInfo(uint32_t start_time, uint32_t end_time, uint64_t filepos, const QString& debug)
{
	m_TimecodeFile = GetTimecodeFile();

	if (start_time > m_last_end_timecode)
	{
		if (m_last_end_timecode > m_start_timecode)
			fprintf(m_TimecodeFile, "%lf\n", (m_last_end_timecode - m_start_timecode) / 1000.0);
		fprintf(m_TimecodeFile, "%lf\n", (m_last_end_timecode - m_last_start_timecode) / 1000.0);
		fprintf(m_TimecodeFile, "gap,%lf\n", (start_time - m_last_end_timecode) / 1000.0);
		m_start_timecode = start_time;
	}
	m_last_start_timecode = start_time;
	m_last_end_timecode = end_time;
}

void VideoDemuxWriter::WriteTimecodeInfo(uint32_t start_time, uint32_t end_time, uint64_t filepos, const QString& debug)
{
	// TODO detect gaps in the stream
}

// ============================================================================

void VideoDemuxWriter::ProcessStream(uint8_t* buff, uint32_t size, uint32_t start_time, uint32_t end_time, const QString& debug)
{
	if (m_parser == NULL)
		m_parser = new M2VParser;
	
	m_parser->WriteData(buff, size);

	MPEG2ParserState state = m_parser->GetState();

	while (state == MPV_PARSER_STATE_FRAME) 
	{
		delete m_parser->ReadFrame();
		state = m_parser->GetState();
	}

	Write(buff, size, start_time, end_time, debug); 
}

void VideoDemuxWriter::SetBoundary(uint32_t start_timecode, uint32_t duration, const CellListElem *cell)
{
	if (m_parser != NULL)
	{
		m_parser->SetEOS();

		MPEG2ParserState state = m_parser->GetState();

		while (state == MPV_PARSER_STATE_FRAME) 
		{
			MPEGFrame* current_frame = m_parser->ReadFrame();
			if ((current_frame->timecode + current_frame->duration) / 1000000 > m_last_end_timecode)
			{
				m_last_start_timecode = current_frame->timecode / 1000000;
				m_last_end_timecode = (current_frame->timecode + current_frame->duration) / 1000000;
			}
			delete current_frame;

			state = m_parser->GetState();
		}
		
		delete m_parser;
	}
	
	m_parser = new M2VParser();

	m_TimecodeFile = GetTimecodeFile();

	if (m_end_timecode != 0)
	{
		if (m_is_still) {
			fwrite(" (Still)\n", 1, 9, m_TimecodeFile);
			fprintf(m_TimecodeFile, "%lf,%lf\n", (m_end_timecode - m_start_timecode) / 1000.0, 1000.0 / (m_end_timecode - m_start_timecode));
		}
		else if (m_last_end_timecode < m_end_timecode)
		{
			fprintf(m_TimecodeFile, "\n%lf\n", (m_last_end_timecode - m_start_timecode) / 1000.0);
			fprintf(m_TimecodeFile, "gap,%lf\n", (m_end_timecode - m_last_end_timecode) / 1000.0);
		}
		else
		{
			if (m_last_end_timecode > m_end_timecode)
				qWarning("There is more video that indicated in the cell.");
			fprintf(m_TimecodeFile, "\n%lf\n", (m_last_end_timecode - m_start_timecode) / 1000.0);
		}
	}

	m_start_timecode = start_timecode;
	m_end_timecode = start_timecode + duration;
	m_last_end_timecode = start_timecode;
	m_is_still = cell->isStill;

	fprintf(m_TimecodeFile, "\n# VOB %d Cell %d", cell->vobid, cell->cellid);
}

void AC3DemuxWriter::SetBoundary(uint32_t start_timecode, uint32_t duration, const CellListElem *cell)
{
	m_TimecodeFile = GetTimecodeFile();

	// handle ending of the previous cell
	if (m_last_end_timecode < m_end_timecode)
	{
		if (m_last_end_timecode > m_start_timecode)
			fprintf(m_TimecodeFile, "%lf\n", (m_last_end_timecode - m_start_timecode) / 1000.0);
		fprintf(m_TimecodeFile, "gap,%lf\n", (m_end_timecode - m_last_end_timecode) / 1000.0);
	}
	else
	{
		if (m_end_timecode != 0)
		{
			if (m_last_end_timecode > m_end_timecode)
				qWarning("There is more AC3 audio that indicated in the cell.");
			fprintf(m_TimecodeFile, "%lf\n", (m_last_end_timecode - m_start_timecode) / 1000.0);
		}
	}

	m_start_timecode = start_timecode;
	m_end_timecode = start_timecode + duration;
	m_last_end_timecode = start_timecode;
	m_last_start_timecode = start_timecode;

	fprintf(m_TimecodeFile, "\n# VOB %d Cell %d\n", cell->vobid, cell->cellid);
}

void DTSDemuxWriter::SetBoundary(uint32_t start_timecode, uint32_t duration, const CellListElem *cell)
{
	m_TimecodeFile = GetTimecodeFile();

	// handle ending of the previous cell
	if (m_last_end_timecode < m_end_timecode)
	{
		if (m_last_end_timecode > m_start_timecode)
			fprintf(m_TimecodeFile, "%lf\n", (m_last_end_timecode - m_start_timecode) / 1000.0);
		fprintf(m_TimecodeFile, "gap,%lf\n", (m_end_timecode - m_last_end_timecode) / 1000.0);
	}
	else
	{
		if (m_end_timecode != 0)
		{
			if (m_last_end_timecode > m_end_timecode)
				qWarning("There is more DTS audio that indicated in the cell.");
			fprintf(m_TimecodeFile, "%lf\n", (m_last_end_timecode - m_start_timecode) / 1000.0);
		}
	}

	m_start_timecode = start_timecode;
	m_end_timecode = start_timecode + duration;
	m_last_end_timecode = start_timecode;
	m_last_start_timecode = start_timecode;

	fprintf(m_TimecodeFile, "\n# VOB %d Cell %d\n", cell->vobid, cell->cellid);
}

void LPCMDemuxWriter::SetBoundary(uint32_t start_timecode, uint32_t duration, const CellListElem *cell)
{
	m_TimecodeFile = GetTimecodeFile();

	// handle ending of the previous cell
	if (m_last_end_timecode < m_end_timecode)
	{
		if (m_last_end_timecode > m_start_timecode)
			fprintf(m_TimecodeFile, "%lf\n", (m_last_end_timecode - m_start_timecode) / 1000.0);
		fprintf(m_TimecodeFile, "gap,%lf\n", (m_end_timecode - m_last_end_timecode) / 1000.0);
	}
	else
	{
		if (m_end_timecode != 0)
		{
			if (m_last_end_timecode > m_end_timecode)
				qWarning("There is more PCM audio that indicated in the cell.");
			fprintf(m_TimecodeFile, "%lf\n", (m_last_end_timecode - m_start_timecode) / 1000.0);
		}
	}

	m_start_timecode = start_timecode;
	m_end_timecode = start_timecode + duration;
	m_last_end_timecode = start_timecode;
	m_last_start_timecode = start_timecode;

	fprintf(m_TimecodeFile, "\n# VOB %d Cell %d\n", cell->vobid, cell->cellid);
}

void MPADemuxWriter::SetBoundary(uint32_t start_timecode, uint32_t duration, const CellListElem *cell)
{
}

void SubDemuxWriter::SetBoundary(uint32_t start_timecode, uint32_t duration, const CellListElem *cell)
{
	if (start_timecode == 0)
		m_start_timecode = m_end_timecode;
	m_end_timecode += duration;
}

void BtnDemuxWriter::SetBoundary(uint32_t start_timecode, uint32_t duration, const CellListElem *cell)
{
	m_TimecodeFile = GetTimecodeFile();

	// handle ending of the previous cell
	if (m_last_end_timecode < m_end_timecode)
	{
		if (m_last_end_timecode > m_start_timecode)
			fprintf(m_TimecodeFile, "%lf\n", (m_last_end_timecode - m_start_timecode) / 1000.0);
		fprintf(m_TimecodeFile, "gap,%lf\n", (m_end_timecode - m_last_end_timecode) / 1000.0);
	}
	else
	{
		if (m_end_timecode != 0)
		{
			if (m_last_end_timecode > m_end_timecode)
				qWarning("There is more Button data that indicated in the cell.");
			fprintf(m_TimecodeFile, "%lf\n", (m_last_end_timecode - m_start_timecode) / 1000.0);
		}
	}

	m_start_timecode = start_timecode;
	m_end_timecode = start_timecode + duration;
	m_last_end_timecode = start_timecode;
	m_last_start_timecode = start_timecode;

	fprintf(m_TimecodeFile, "\n# VOB %d Cell %d\n", cell->vobid, cell->cellid);
}

VideoDemuxWriter::~VideoDemuxWriter()
{
	try
	{
		m_TimecodeFile = GetTimecodeFile();
	}
	catch (VobParserException&)
	{
		m_TimecodeFile = 0;
	}
	
	if (m_parser != NULL)
	{
		m_parser->SetEOS();

		MPEG2ParserState state = m_parser->GetState();

		while (state == MPV_PARSER_STATE_FRAME) 
		{
			MPEGFrame* current_frame = m_parser->ReadFrame();
			if ((current_frame->timecode + current_frame->duration) / 1000000 > m_last_end_timecode)
			{
				m_last_start_timecode = current_frame->timecode / 1000000;
				m_last_end_timecode = (current_frame->timecode + current_frame->duration) / 1000000;
			}
			delete current_frame;

			state = m_parser->GetState();
		}
		delete m_parser;
	}


	if ((m_end_timecode != 0) && m_TimecodeFile)
	{
		if (m_is_still) {
			fwrite(" (Still)\n", 1, 9, m_TimecodeFile);
			fprintf(m_TimecodeFile, "%lf,%lf\n", (m_end_timecode - m_start_timecode) / 1000.0, 1000.0 / (m_end_timecode - m_start_timecode));
		}
		else if (m_last_end_timecode < m_end_timecode)
		{
			fprintf(m_TimecodeFile, "\n%lf\n", (m_last_end_timecode - m_start_timecode) / 1000.0);
			fprintf(m_TimecodeFile, "gap,%lf\n", (m_end_timecode - m_last_end_timecode) / 1000.0);
		}
		else
			fprintf(m_TimecodeFile, "\n%lf\n", (m_end_timecode - m_start_timecode) / 1000.0);
	}
}

AC3DemuxWriter::~AC3DemuxWriter()
{
	try
	{
		m_TimecodeFile = GetTimecodeFile();
	}
	catch (VobParserException&)
	{
		return;
	}

	// handle ending of the previous cell
	if (m_last_end_timecode < m_end_timecode)
	{
		if (m_last_end_timecode > m_start_timecode)
			fprintf(m_TimecodeFile, "%lf\n", (m_last_end_timecode - m_start_timecode) / 1000.0);
		fprintf(m_TimecodeFile, "gap,%lf\n", (m_end_timecode - m_last_end_timecode) / 1000.0);
	}
	else
	{
		if (m_end_timecode != 0)
			fprintf(m_TimecodeFile, "%lf\n", (m_end_timecode - m_start_timecode) / 1000.0);
	}
}

DTSDemuxWriter::~DTSDemuxWriter()
{
	try
	{
		m_TimecodeFile = GetTimecodeFile();
	}
	catch (VobParserException&)
	{
		return;
	}

	// handle ending of the previous cell
	if (m_last_end_timecode < m_end_timecode)
	{
		if (m_last_end_timecode > m_start_timecode)
			fprintf(m_TimecodeFile, "%lf\n", (m_last_end_timecode - m_start_timecode) / 1000.0);
		fprintf(m_TimecodeFile, "gap,%lf\n", (m_end_timecode - m_last_end_timecode) / 1000.0);
	}
	else
	{
		if (m_end_timecode != 0)
			fprintf(m_TimecodeFile, "%lf\n", (m_end_timecode - m_start_timecode) / 1000.0);
	}
}

LPCMDemuxWriter::~LPCMDemuxWriter()
{
	try
	{
		m_TimecodeFile = GetTimecodeFile();
	}
	catch (VobParserException&)
	{
		return;
	}

	// handle ending of the previous cell
	if (m_last_end_timecode < m_end_timecode)
	{
		if (m_last_end_timecode > m_start_timecode)
			fprintf(m_TimecodeFile, "%lf\n", (m_last_end_timecode - m_start_timecode) / 1000.0);
		fprintf(m_TimecodeFile, "gap,%lf\n", (m_end_timecode - m_last_end_timecode) / 1000.0);
	}
	else
	{
		if (m_end_timecode != 0)
			fprintf(m_TimecodeFile, "%lf\n", (m_end_timecode - m_start_timecode) / 1000.0);
	}
}

BtnDemuxWriter::~BtnDemuxWriter()
{
	try
	{
		m_TimecodeFile = GetTimecodeFile();
	}
	catch (VobParserException&)
	{
		return;
	}

	// handle ending of the previous cell
	if (m_last_end_timecode < m_end_timecode)
	{
		if (m_last_end_timecode > m_start_timecode)
			fprintf(m_TimecodeFile, "%lf\n", (m_last_end_timecode - m_start_timecode) / 1000.0);
		fprintf(m_TimecodeFile, "gap,%lf\n", (m_end_timecode - m_last_end_timecode) / 1000.0);
	}
	else
	{
		if (m_end_timecode != 0)
			fprintf(m_TimecodeFile, "%lf\n", (m_end_timecode - m_start_timecode) / 1000.0);
	}
}

WavWriter::~WavWriter()
{
	if (m_file)
	{
		uint8_t _tinteger[4];

		fseek(m_file, m_size_position2, SEEK_SET);
		_tinteger[0] = m_size & 0xFF;
		_tinteger[1] = (m_size >> 8) & 0xFF;
		_tinteger[2] = (m_size >> 16) & 0xFF;
		_tinteger[3] = (m_size >> 24) & 0xFF;
		fwrite(_tinteger,4,1, m_file);
		
		m_size += 36;
		fseek(m_file, m_size_position1, SEEK_SET);
		_tinteger[0] = m_size & 0xFF;
		_tinteger[1] = (m_size >> 8) & 0xFF;
		_tinteger[2] = (m_size >> 16) & 0xFF;
		_tinteger[3] = (m_size >> 24) & 0xFF;
		fwrite(_tinteger,4,1, m_file);
	}
}

void WavWriter::Write(uint8_t* buff, uint32_t size, uint32_t start_time, uint32_t end_time, const QString& debug)
{
	if (m_bit_depth != 16) // not supported
		return;
	if (m_file == NULL)
	{
		uint8_t _tinteger[4];
		m_file = OpenOuputFile();

		// create the WAV header
		//   RIFF head
		fwrite("RIFF",4,1, m_file);
		m_size_position1 = ftell(m_file);
		fwrite("0000",4,1, m_file);
		fwrite("WAVE",4,1, m_file);
		
		//   Format head
		fwrite("fmt ",4,1, m_file);
		_tinteger[0] = 16;
		_tinteger[1] = 0;
		_tinteger[2] = 0;
		_tinteger[3] = 0;
		fwrite(_tinteger,4,1, m_file);
		
		_tinteger[0] = 1;
		fwrite(_tinteger,2,1, m_file); // PCM
		
		_tinteger[0] = m_channel_nb;
		_tinteger[1] = 0;
		fwrite(_tinteger,2,1, m_file); // Channels
		
		_tinteger[0] = m_sample_rate & 0xFF;
		_tinteger[1] = (m_sample_rate >> 8) & 0xFF;
		_tinteger[2] = (m_sample_rate >> 16) & 0xFF;
		_tinteger[3] = (m_sample_rate >> 24) & 0xFF;
		fwrite(_tinteger,4,1, m_file); // Sampling freq
		
		uint32_t _byte_rate = (m_sample_rate * m_channel_nb * m_bit_depth) >> 3;
		_tinteger[0] = _byte_rate & 0xFF;
		_tinteger[1] = (_byte_rate >> 8) & 0xFF;
		_tinteger[2] = (_byte_rate >> 16) & 0xFF;
		_tinteger[3] = (_byte_rate >> 24) & 0xFF;
		fwrite(_tinteger,4,1, m_file); // Byte Rate
		
		uint16_t _block_align = (m_channel_nb * m_bit_depth) >> 3;
		_tinteger[0] = _block_align & 0xFF;
		_tinteger[1] = (_block_align >> 8) & 0xFF;
		fwrite(_tinteger,2,1, m_file); // Block Align
		
		_tinteger[0] = m_bit_depth;
		_tinteger[1] = 0;
		fwrite(_tinteger,2,1, m_file); // Block Align
		
		//   Data head
		fwrite("data",4,1, m_file);
		m_size_position2 = ftell(m_file);
		fwrite("0000",4,1, m_file);
		m_size = 0;
	}
	uint16_t _tmp;
	for (size_t i=0; i < size; i+=2) {
		_tmp = buff[i];
		_tmp <<= 8;
		_tmp += buff[i+1];
		buff[i] = _tmp & 0xFF;
		buff[i+1] = _tmp >> 8;
	}
	Writer::Write(buff, size, start_time, end_time, debug);
	m_size += size;
}
