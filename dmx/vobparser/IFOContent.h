// ----------------------------------------------------------------------------
#ifndef _IFO_CONTENT_H_
#define _IFO_CONTENT_H_
// ----------------------------------------------------------------------------
#include <QHash>
#include <vector>

#include "VobParser.h"
#include "dvdread/ifo_read.h"
// ----------------------------------------------------------------------------
typedef std::vector<uint8_t> IdArray;

typedef QHash<int, CellListElem *> CellsHashType;
typedef std::vector<const audio_attr_t*> AudioTrackList;
typedef std::vector<const subp_attr_t*> SubtitleTrackList;

#define MAKE_CELLS_KEY(vob_id,cell_id)  ((vob_id << 8) | cell_id)
// ----------------------------------------------------------------------------
class IFOException { };
class IFOFileIOException : public IFOException { };
class IFOInvalidFileFormatException : public IFOException { };
// ----------------------------------------------------------------------------
typedef QList<CellListElem*> CellsListBaseType;
class CellsListType : public CellsListBaseType
{
public:
	void arrange();
	CellListElem* at(uint16_t vob_id, uint8_t cell_id) const;
	const CellListElem* at(const cell_position_t & position) const;

	// comperator for qSort()
	static int cellListElemCompare(const CellListElem *arg1, const CellListElem *arg2);
};
// ----------------------------------------------------------------------------
class IFOContent 
{
public:
	IFOContent(dvd_reader_t *dvdr, int16_t title);

	~IFOContent()
	{
		ifoClose(m_handle);
		m_CellsList.clear();
		m_langCellsList.clear();
	}

	ifo_handle_t& Handle()
	{
		assert(m_handle != NULL);
		return *m_handle;
	}

	IdArray FindSubStream(uint8_t streamID, bool menu) const;
	uint8_t FindAudioStream(uint8_t streamID, bool menu) const;
	
	void GetPGCCells(const pgcit_t & pgcit, const c_adt_t & adt, CellsHashType * CellsHash);

	int16_t m_title;
	AudioTrackList m_langAudio;
	AudioTrackList m_Audio;
	SubtitleTrackList m_langSubs;
	SubtitleTrackList m_Subs;
	CellsListType m_CellsList;
	CellsListType m_langCellsList;

protected:
	ifo_handle_t *m_handle;
};
// ----------------------------------------------------------------------------
#endif
// ----------------------------------------------------------------------------
