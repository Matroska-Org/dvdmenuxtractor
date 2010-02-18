// ----------------------------------------------------------------------------
#ifndef _IFO_FILE_H_
#define _IFO_FILE_H_
// ----------------------------------------------------------------------------
#include <QStringList>

#include "IFOContent.h"
// ----------------------------------------------------------------------------
class IfoHandleList: public QList<IFOContent*>
{
public:
	/// look for IfoContent corresponding to the title
	IFOContent* getIfoContent(int16_t title) const;
};
// ----------------------------------------------------------------------------
class IFOFile  
{
public:
	IFOFile(const QString& filename);
	const CellsListType* GetCellsList(unsigned int title, bool menu);
	virtual ~IFOFile();
	const pgc_t *FirstPlayPGC() const;
	const tt_srpt_t *TitleMap() const;
	const pgci_ut_t *LanguageUnits(unsigned int title) const;
	const vts_ptt_srpt_t *VtsTitles(unsigned int title) const;
	const pgcit_t *VtsPGCs(unsigned int title) const;
	const int16_t NumberOfTitles() const;
	const AudioTrackList & AudioTracks(unsigned int title, bool menu) const;
	const SubtitleTrackList & SubsTracks(unsigned int title, bool menu) const;
	const CellsListType & CellsList(unsigned int title, bool menu) const;
	bool VideoSize(unsigned int title, bool menu, uint16_t & width, uint16_t & height, double & fps) const;
	const uint32_t * GetPalette(unsigned int title, bool menu) const;

	uint8_t GetAudioId(uint8_t streamID, uint8_t title, bool menu) const;
	IdArray GetSubsId(uint8_t streamID, uint8_t title, bool menu) const;

private:
	IfoHandleList m_ifos;
	dvd_reader_t* m_dvd;
};
// ----------------------------------------------------------------------------
#endif
// ----------------------------------------------------------------------------
