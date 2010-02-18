// ============================================================================
// VOBParser class
// Copyright © 2002 : Christophe PARIS (christophe.paris@free.fr)
// ============================================================================
#ifndef _VOB_PARSER_H_
#define _VOB_PARSER_H_
// ----------------------------------------------------------------------------
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdexcept>

#include "dvdread/ifo_read.h"
#include "mpegparser/M2VParser.h"

#include <QList>
#include <QFile>
#include <QString>
// ============================================================================

#define VIDEO_STREAM_TYPE		0x01
#define AUDIO_STREAM_TYPE		0x02
#define SUB_STREAM_TYPE			0x04

#define AC3_STREAM_TYPE			(AUDIO_STREAM_TYPE | 0x08)
#define DTS_STREAM_TYPE			(AUDIO_STREAM_TYPE | 0x10)
#define PCM_STREAM_TYPE			(AUDIO_STREAM_TYPE | 0x20)


#define AUDIO_STREAM		0xC0
#define VIDEO_STREAM		0xE0

#define SUBSTREAM_PCI       0x00
#define SUBSTREAM_DSI       0x01

#define SUBSTREAM_SUB_LOW		0x20
#define SUBSTREAM_SUB_HIGH		(SUBSTREAM_SUB_LOW + 32)

#define SUBSTREAM_AC3_LOW		0x80
#define SUBSTREAM_AC3_HIGH		(SUBSTREAM_AC3_LOW + 8)

#define SUBSTREAM_DTS_LOW		0x88
#define SUBSTREAM_DTS_HIGH		(SUBSTREAM_DTS_LOW + 8)

#define SUBSTREAM_PCM_LOW		0xA0
#define SUBSTREAM_PCM_HIGH		(SUBSTREAM_PCM_LOW + 8)

// ============================================================================
// Type
// ============================================================================

typedef struct 
{
	unsigned int vobid;
	unsigned int cellid;
	unsigned int start_frame;
	unsigned int end_frame;
	unsigned int nb_frames;
	uint32_t start_sector;
	uint32_t last_sector;
	double frame_dur;
	uint64_t start_time;
	uint64_t duration;
	bool isStill;
	bool selected;
	bool found;
	//ButtonsListType btt_list;
} CellListElem;

typedef struct {
	uint16_t start_x;				// Starting X position
	uint16_t start_y;				// Starting Y position
	uint16_t end_x;					// Ending X position
	uint16_t end_y;					// Ending Y position
	uint8_t btn_coln;				// Button color table number, 0=none
	uint8_t auto_action_flag;		// Auto Action flag 0=no, 1=yes
	uint8_t up;						// Button number to select if "Up" is pressed
	uint8_t down;					// Button number to select if "Down" is pressed
	uint8_t left;					// Button number to select if "Left" is pressed
	uint8_t right;					// Button number to select if "Right" is pressed
	uint8_t vm_cmd[8];				// One vm command to be executed on "action" of this button
} btni;

typedef struct {
	uint32_t nv_pck_lbn		: 32;	// Logical Block Number (sector) of this block
	uint16_t vobu_cat		: 16;	// Flags, including APS (Analog Protection System)
	uint16_t reserved1		: 16;	// Reserved
	uint32_t vobu_uop_ctl	: 32;	// bitmask for prohibited user operations
	uint32_t vobu_s_ptm		: 32;	// Vobu Start Presentation Time (90KHz clk)
	uint32_t vobu_e_ptm		: 32;	// Vobu End Presentation Time (PTM)
	uint32_t vobu_se_e_ptm	: 32;	// End PTM of VOBU if Sequence_End_Code
	uint32_t c_eltm			: 32;	// Cell elapsed time (BCD)
	uint8_t  vobu_isrc[32];			// International Standard Recording Code (royalty management)
	uint32_t nsml_agli_dsta[9];		// Non-seamless angle 1 relative offset to VOBU for CURRENT ILVU 
	uint16_t hli_ss;				// Highlight status (lower 2 bits only)
	uint32_t hli_s_ptm;				// Highlight start time
	uint32_t hli_e_ptm;				// Highlight end time
	uint32_t btn_sl_e_ptm;			// Button selection end time (ignore user after this)
	uint16_t btn_md;				// 4 nibbles which describe the grouping of the buttons
	uint8_t  btn_sn;				// Starting button number
	uint8_t  btn_ns;				// Number of buttons
	uint8_t  nsl_btn_ns;			// Number of numerically selected buttons
	uint8_t  reserved2;				// Reserved
	uint8_t  fosl_btnn;				// Force select button number
	uint8_t  foac_btnn;				// Force action button number
	uint32_t btn_coli[3][2];		// Selection and action color and contrast values
	btni     btnit[36];
} nav_pci_gi;

typedef struct {
	uint32_t nv_pck_scr		: 32;	// System clock reference
	uint32_t nv_pck_lbn		: 32;	// Logical Block Number (sector) of this block
	uint32_t vobu_ea		: 32;	// VOBU end address - relative offset to last sector of VOBU
	uint32_t vobu_1stref_ea	: 32;	// First reference frame end block, relative - used for fast playing
	uint32_t vobu_2ndref_ea	: 32;	// Second reference frame end block, relative - used for fast playing
	uint32_t vobu_3rdref_ea	: 32;	// Third reference frame end block, relative - used for fast playing
	uint16_t vobu_vob_idn	: 16;	// VOB number
	uint8_t  reserved		: 8;	// Reserved
	uint8_t  vobu_c_idn		: 8;	// CELL number within VOB
	uint32_t c_eltm			: 32;	// Cell elapsed time (BCD)
} nav_dsi_gi;

typedef struct {
	uint8_t PES_scrambling_control : 2;
	uint8_t PES_priority : 1;
	uint8_t data_alignment_indicator : 1;
	uint8_t copyright : 1;
	uint8_t original_or_copy : 1;
	uint8_t PTS_flag : 1;
	uint8_t DTS_flag : 1;
	uint8_t ESCR_flag : 1;
	uint8_t ES_rate_flag : 1;
	uint8_t DSM_trick_mode_flag : 1;
	uint8_t additionnal_copy_info_flag : 1;
	uint8_t PES_CRC_flag : 1;
	uint8_t PES_extension_flag : 1;
	uint8_t PES_header_data_len;
} PES_header_data_content;

typedef struct {
	uint32_t identifier;
	uint64_t scr;
	uint16_t scr_ext;
	uint32_t program_mux_rate;
	int64_t pts;
	int64_t dts;
} packet_info;

// ============================================================================
// Exception
// ============================================================================

class VobParserException : public std::runtime_error
{
public:
	VobParserException(const char* message) : std::runtime_error(message) {}
	virtual ~VobParserException() throw() {}
};

class VobParserFileNotFoundException : public VobParserException
{
public:
	VobParserFileNotFoundException(const char* filename)
		: VobParserException(qPrintable(QString("File not found exception : %1").arg(filename)))
	{
	}
};

class VobParserFileOpenException : public VobParserException
{
public:
	VobParserFileOpenException(const char* filename)
		: VobParserException(qPrintable(QString("Could not open/create file: %1").arg(filename)))
	{
	}
};

class VobParserInvalidPacketException : public VobParserException
{
public:
	VobParserInvalidPacketException(int offset)
		: VobParserException(qPrintable(QString("Invalid packet exception @LBA %1").arg(offset)))
	{
	}
};

// ============================================================================
// Demuxer class
// ============================================================================

class Demuxer
{
public:
	virtual ~Demuxer() {}
	virtual void ProcessStream(uint8_t* buff, uint32_t size, uint32_t start_time, uint32_t end_time, const QString & debug) = 0;
	virtual void SetBoundary(uint32_t start_timecode, uint32_t duration, const CellListElem *cell) = 0;
};

// ----------------------------------------------------------------------------

class Writer : public Demuxer
{
public:
	Writer(const QString& filenamePrefix, const QString& extension, double fps)
		:m_Filename(filenamePrefix)
		,m_fileExtension(extension)
		,m_file(NULL)
		,m_TimecodeFile(NULL)
		,m_fps(fps)
	{
	}

	virtual ~Writer()
	{
		if(m_file)
		{
			fclose(m_file);
		}
		if (m_TimecodeFile)
			fclose(m_TimecodeFile);
	}

	virtual void Write(uint8_t* buff, uint32_t size, uint32_t start_time, uint32_t end_time, const QString& debug)
	{
		m_file = OpenOuputFile();
		WriteTimecodeInfo(start_time, end_time, ftell(m_file), debug);
		fwrite(buff, 1, size, m_file);
	}

	bool FileExists() const {
		return m_file != NULL;
	}

protected:
	inline FILE* GetTimecodeFile()
	{
		if (!m_TimecodeFile)
		{
			QString m_filename = QString("%1_%2.tmc").arg(m_Filename).arg(m_fileExtension);
			m_TimecodeFile = fopen(QFile::encodeName(m_filename),"w");
			
			if (!m_TimecodeFile)
				throw VobParserFileOpenException(QFile::encodeName(m_filename));
			
			fwrite("# timecode format v3\n", 1, 21, m_TimecodeFile);
			fprintf(m_TimecodeFile, "assume %lf\n", m_fps);
		}
		return m_TimecodeFile;
	}

	inline FILE* OpenOuputFile()
	{
		if(!m_file)
		{
			QString m_filename = QString("%1.%2").arg(m_Filename).arg(m_fileExtension);
			m_file = fopen(QFile::encodeName(m_filename),"wb");

			if (!m_file)
				throw VobParserFileOpenException(QFile::encodeName(m_filename));
		}
		return m_file;
	}

	QString m_Filename;
	QString m_fileExtension;
	FILE* m_file;
	FILE* m_TimecodeFile;
	double m_fps;

	virtual void WriteTimecodeInfo(uint32_t start_time, uint32_t end_time, uint64_t filepos, const QString& debug) = 0;
};

// ----------------------------------------------------------------------------

class VideoDemuxWriter : public Writer
{
public:
	VideoDemuxWriter(const QString& filenamePrefix, double fps)
		:Writer(filenamePrefix, "m2v", fps) 
		,m_end_timecode(0)
		,m_last_start_timecode(0)
		,m_last_end_timecode(0)
		,m_is_still(false)
		,m_parser(NULL)
	{}
	void ProcessStream(uint8_t* buff, uint32_t size, uint32_t start_time, uint32_t end_time, const QString& debug);
	void SetBoundary(uint32_t start_timecode, uint32_t duration, const CellListElem *cell);
	~VideoDemuxWriter();
protected:
	void WriteTimecodeInfo(uint32_t start_time, uint32_t end_time, uint64_t filepos, const QString& debug);
	uint32_t m_start_timecode;
	uint32_t m_end_timecode;
	uint32_t m_last_start_timecode;
	uint32_t m_last_end_timecode;
	bool m_is_still;

	M2VParser * m_parser;
};

// ----------------------------------------------------------------------------

class AC3DemuxWriter : public Writer
{
public:
	AC3DemuxWriter(const QString& filenamePrefix, const uint8_t streamID)
		:Writer(filenamePrefix, "ac3", 0.0)
		,m_streamID(streamID)
		,m_end_timecode(0)
		,m_last_start_timecode(0)
		,m_last_end_timecode(0)
	{}
	~AC3DemuxWriter();
	void ProcessStream(uint8_t* buff, uint32_t size, uint32_t start_time, uint32_t end_time, const QString& debug)
	{
		Write(buff,size, start_time, end_time, debug);
	}
	void SetBoundary(uint32_t start_timecode, uint32_t duration, const CellListElem *cell);
private:
	uint8_t m_streamID;
protected:
	void WriteTimecodeInfo(uint32_t start_time, uint32_t end_time, uint64_t filepos, const QString& debug);
	uint32_t m_start_timecode;
	uint32_t m_end_timecode;
	uint32_t m_last_start_timecode;
	uint32_t m_last_end_timecode;
};

// ----------------------------------------------------------------------------

class DTSDemuxWriter : public Writer
{
public:
	DTSDemuxWriter(const QString& filenamePrefix, const uint8_t streamID)
		:Writer(filenamePrefix, "dts",0.0)
		,m_streamID(streamID)
		,m_end_timecode(0)
		,m_last_start_timecode(0)
		,m_last_end_timecode(0)
	{}
	~DTSDemuxWriter();
	void ProcessStream(uint8_t* buff, uint32_t size, uint32_t start_time, uint32_t end_time, const QString& debug)
	{
		Write(buff,size, start_time, end_time, debug);
	}
	void SetBoundary(uint32_t start_timecode, uint32_t duration, const CellListElem *cell);
private:
	uint8_t m_streamID;
protected:
	void WriteTimecodeInfo(uint32_t start_time, uint32_t end_time, uint64_t filepos, const QString& debug);
	uint32_t m_start_timecode;
	uint32_t m_end_timecode;
	uint32_t m_last_start_timecode;
	uint32_t m_last_end_timecode;
};

// ----------------------------------------------------------------------------

class WavWriter : public Writer
{
	public:
		WavWriter(const QString& filenamePrefix, double fps, uint32_t sample_rate, uint8_t bit_depth, uint8_t channel_nb)
			:Writer(filenamePrefix, "wav", fps)
			,m_sample_rate(sample_rate)
			,m_bit_depth(bit_depth)
			,m_channel_nb(channel_nb)
		{}
		~WavWriter();
		void Write(uint8_t* buff, uint32_t size, uint32_t start_time, uint32_t end_time, const QString& debug);
	protected:
		uint32_t m_sample_rate;
		uint8_t m_bit_depth, m_channel_nb;
		size_t m_size_position1,m_size_position2, m_size;
};

class LPCMDemuxWriter : public WavWriter
{
public:
	LPCMDemuxWriter(const QString& filenamePrefix, const uint8_t streamID, uint32_t sample_rate, uint8_t bit_depth, uint8_t channel_nb)
		:WavWriter(filenamePrefix,0.0, sample_rate, bit_depth, channel_nb)
		,m_streamID(streamID)
		,m_end_timecode(0)
		,m_last_start_timecode(0)
		,m_last_end_timecode(0)
	{}
	~LPCMDemuxWriter();
	void ProcessStream(uint8_t* buff, uint32_t size, uint32_t start_time, uint32_t end_time, const QString& debug)
	{
		Write(buff,size, start_time, end_time, debug);
	}
	void SetBoundary(uint32_t start_timecode, uint32_t duration, const CellListElem *cell);
private:
	uint8_t m_streamID;
protected:
	void WriteTimecodeInfo(uint32_t start_time, uint32_t end_time, uint64_t filepos, const QString& debug);
	uint32_t m_start_timecode;
	uint32_t m_end_timecode;
	uint32_t m_last_start_timecode;
	uint32_t m_last_end_timecode;
};

// ----------------------------------------------------------------------------

class MPADemuxWriter : public Writer
{
public:
	MPADemuxWriter(const QString& filenamePrefix, const uint8_t streamID) :
	  Writer(filenamePrefix, "mpa",0.0), m_streamID(streamID) {}
	void ProcessStream(uint8_t* buff, uint32_t size, uint32_t start_time, uint32_t end_time, const QString& debug)
	{
		Write(buff,size, start_time, end_time, debug);
	}
	void SetBoundary(uint32_t start_timecode, uint32_t duration, const CellListElem *cell);
private:
	uint8_t m_streamID;
protected:
	void WriteTimecodeInfo(uint32_t start_time, uint32_t end_time, uint64_t filepos, const QString& debug);
};

// ----------------------------------------------------------------------------

class SubDemuxWriter : public Writer
{
public:
	SubDemuxWriter(const QString& filenamePrefix, const uint8_t streamID, uint16_t width, uint16_t height, const uint32_t * palette, uint16_t language, bool forced)
		:Writer(filenamePrefix, "sub",0.0)
		,m_width(width)
		,m_height(height)
		,m_forced(forced)
		,m_palette(palette)
		,m_language(language)
		,m_start_timecode(0)
		,m_end_timecode(0)
	{}
	void ProcessStream(uint8_t* buff, uint32_t size, uint32_t start_time, uint32_t end_time, const QString& debug)
	{
		Write(buff,size, start_time, end_time, debug);
	}
	void SetBoundary(uint32_t start_timecode, uint32_t duration, const CellListElem *cell);
protected:
	void WriteTimecodeInfo(uint32_t start_time, uint32_t end_time, uint64_t filepos, const QString& debug);
private:
	uint16_t m_width;
	uint16_t m_height;
	bool m_forced;
	const uint32_t * m_palette;
	uint16_t m_language;
	uint32_t m_start_timecode;
	uint32_t m_end_timecode;
};

// ----------------------------------------------------------------------------

class BtnDemuxWriter : public Writer
{
public:
	BtnDemuxWriter(const QString& filenamePrefix, uint16_t width, uint16_t height)
		:Writer(filenamePrefix, "btn",0.0)
		,m_width(width)
		,m_height(height)
		,m_end_timecode(0)
		,m_last_start_timecode(0)
		,m_last_end_timecode(0)
	{}
	void ProcessStream(uint8_t* buff, uint32_t size, uint32_t start_time, uint32_t end_time, const QString& debug)
	{
		if (!m_file) {
			m_file = OpenOuputFile();
			fwrite("butonDVD", 8, 1, m_file);
			uint8_t _tmp[4];
			// width & height
			_tmp[0] = m_width >> 8;
			_tmp[1] = m_width & 0xFF;
			_tmp[2] = m_height >> 8;
			_tmp[3] = m_height & 0xFF;
			fwrite(_tmp, 4, 1, m_file);
			// pad to 16 bytes
			_tmp[0] = 0;
			_tmp[1] = 0;
			_tmp[2] = 0;
			_tmp[3] = 0;
			fwrite(_tmp, 4, 1, m_file);
		}
		Write(buff,size, start_time, end_time, debug);
	}
	void SetBoundary(uint32_t start_timecode, uint32_t duration, const CellListElem *cell);
	~BtnDemuxWriter();
protected:
	void WriteTimecodeInfo(uint32_t start_time, uint32_t end_time, uint64_t filepos, const QString& debug);
	uint16_t m_width, m_height;
	uint32_t m_start_timecode;
	uint32_t m_end_timecode;
	uint32_t m_last_start_timecode;
	uint32_t m_last_end_timecode;
};

// ----------------------------------------------------------------------------

class CompositeDemuxWriter
{
public:
	CompositeDemuxWriter();
	~CompositeDemuxWriter();
	bool AddDemuxer(uint8_t streamID, Writer * demuxer, QString& CommandLine);
	void ProcessStream(int streamID, uint8_t* buff, uint32_t size, int32_t start_time, int32_t end_time, const QString& debug);
	void Reset();
	void SetBoundary(uint32_t start_timecode, uint32_t duration, const CellListElem *cell);

	bool FileExists(uint8_t streamID) const {
		return (m_muxers[streamID] != NULL && m_muxers[streamID]->FileExists());
	}

	const QString& GetString(uint8_t streamID) const {
		return m_strings[streamID];
	}

protected:
	Writer * m_muxers[256];
	QString m_strings[256];
};

// ----------------------------------------------------------------------------

struct stream_info_node
{
	uint8_t id;
	uint32_t type;
	stream_info_node* next;
};


// ----------------------------------------------------------------------------

typedef struct 
{
	int x1;
	int y1;
	int x2;
	int y2;
} ButtonListElem;

typedef QList<ButtonListElem *> ButtonListType;


// ----------------------------------------------------------------------------

// ============================================================================
// Main class
// ============================================================================

class IFOFile;
class CellsListType;

class VobParser
{
public:
	VobParser(const char* dirname, int16_t title, bool menu);
	void Reset();
	bool ParseNextPacket(const CellsListType & Cells);
	uint32_t GetPacketCount() const;
	uint32_t GetPacketIndex() const;
	char* GetCurrentPacketData() const;
	inline uint8_t GetVobID() const;
	inline uint8_t GetCellID() const;
	bool IsNewCell();
	virtual ~VobParser();
	inline CompositeDemuxWriter & GetDemuxer()
	{
		return m_demuxer;
	}
	uint32_t GetCellSCR() const;

	nav_dsi_gi m_dsi;
	nav_pci_gi m_pci;
	packet_info pktinfo;
	PES_header_data_content pes_header_data_content;
protected:
	void ParseNavPacket();
	void ParseAudioPacket(int StreamID);
	void ParseVideoPacket();
	void ParsePrivateStream1();

	void ParseSCR();
	void ParsePCI();
	void ParseDSI();
	void ParsePESHeaderDataContentFlag();
	void ParsePESHeaderData();
	uint64_t ParsePTS_DTS();

	// Buffer management
	bool AvailablePacketData() const;
	bool GetNextPacket();
	uint32_t GetNext32Bits();
	uint16_t GetNext16Bits();
	uint8_t GetNext8Bits();
	void SkipNBytes(int n);

private:
	int16_t m_title;
	dvd_reader_t *m_dvdhandle;
	dvd_file_t *m_stream;
	bool m_language;
	uint8_t m_buff[DVD_VIDEO_LB_LEN];
	uint32_t m_index;
	uint32_t m_pktindex;
	uint32_t m_pktcount;
	bool m_bFirstPacket;
	int64_t m_startpts;
	int64_t m_startdts;
	uint16_t m_pci_position;
	uint16_t m_pci_size;
	uint32_t m_pci_vob_timecode_offset;
	
	CompositeDemuxWriter m_demuxer;

	int previous_vobid, previous_cellid;
};

// ----------------------------------------------------------------------------
#endif
// ----------------------------------------------------------------------------
