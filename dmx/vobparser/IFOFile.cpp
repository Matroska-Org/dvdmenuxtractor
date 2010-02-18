// ----------------------------------------------------------------------------
#include "IFOFile.h"
#include "iso/iso_lang.h"
#include "dvdread/ifo_print.h"
// ----------------------------------------------------------------------------
IFOFile::IFOFile(const QString& filename)
{	
	m_dvd = DVDOpen(QFile::encodeName(filename));
	
	if(!m_dvd) {
		throw IFOInvalidFileFormatException();
	}

	try {
		m_ifos.append(new IFOContent(m_dvd, 0));		
	}
	catch (...)	{
		qCritical("Cannot open VIDEO_TS.IFO.");
		throw;
	}

	IFOContent *_ifo = m_ifos.getIfoContent(0);

	if (_ifo && _ifo->Handle().vmgi_mat)
	{
		for (int i = 1; i <= _ifo->Handle().vmgi_mat->vmg_nr_of_title_sets; i++)
		{
			try {
				m_ifos.append(new IFOContent(m_dvd,i));
			}
			catch (...)	{
			  qCritical(qPrintable(QString("Cannot open VTS_%1_X.IFO").arg(i)));
			}
		}
	}
}
// ----------------------------------------------------------------------------
IFOFile::~IFOFile()
{
	for (int i = 0; i < m_ifos.size(); i++)
		delete m_ifos.at(i);
	
	if (m_dvd)
	{
		DVDClose(m_dvd);
	}
}
// ----------------------------------------------------------------------------
const pgc_t *IFOFile::FirstPlayPGC() const
{
	if (m_ifos.at(0))
		return m_ifos.at(0)->Handle().first_play_pgc;
	else
		return NULL;
}
// ----------------------------------------------------------------------------
const tt_srpt_t *IFOFile::TitleMap() const
{
	if (m_ifos.at(0))
		return m_ifos.at(0)->Handle().tt_srpt;
	else
		return NULL;
}
// ----------------------------------------------------------------------------
const pgci_ut_t *IFOFile::LanguageUnits(unsigned int title) const
{
	IFOContent *_ifo = m_ifos.getIfoContent(title);
	
	if (_ifo)
		return _ifo->Handle().pgci_ut;
		
	return NULL;
}
// ----------------------------------------------------------------------------
const vts_ptt_srpt_t *IFOFile::VtsTitles(unsigned int title) const
{
	IFOContent *_ifo = m_ifos.getIfoContent(title);
	if (_ifo)
	{
		return _ifo->Handle().vts_ptt_srpt;
	}
	return NULL;
}
// ----------------------------------------------------------------------------
const pgcit_t *IFOFile::VtsPGCs(unsigned int title) const
{
	IFOContent *_ifo = m_ifos.getIfoContent(title);
	if (_ifo)
	{
		return _ifo->Handle().vts_pgcit;
	}
	return NULL;
}
// ----------------------------------------------------------------------------
const int16_t IFOFile::NumberOfTitles() const
{
	if (m_ifos.count() && m_ifos.at(0)->Handle().vmgi_mat)
		return m_ifos.at(0)->Handle().vmgi_mat->vmg_nr_of_title_sets;
	return 0;
}
// ----------------------------------------------------------------------------
int CellsListType::cellListElemCompare(const CellListElem *arg1, const CellListElem *arg2)
{
	const CellListElem* cle1 = arg1;
	const CellListElem* cle2 = arg2;

	return (cle1->vobid > cle2->vobid) ? 1 :
		(cle1->vobid < cle2->vobid) ? -1 :
			(cle1->cellid > cle2->cellid) ? 1 : 
				(cle1->cellid < cle2->cellid) ? -1 :
					0;
}
// ----------------------------------------------------------------------------
const CellsListType* IFOFile::GetCellsList(unsigned int title, bool menu)
{
	CellsHashType CellsHash;

	IFOContent *ifoc = m_ifos.getIfoContent(title);
	if (!ifoc)
		return NULL;

	if (menu)
		return &ifoc->m_langCellsList;
	else
		return &ifoc->m_CellsList;
}
// ----------------------------------------------------------------------------
const AudioTrackList & IFOFile::AudioTracks(unsigned int title, bool menu) const
{
	IFOContent *_ifo = m_ifos.getIfoContent(title);
	if (!_ifo)
		throw;

	if (menu)
		return _ifo->m_langAudio;
	else
		return _ifo->m_Audio;
}
// ----------------------------------------------------------------------------
const SubtitleTrackList & IFOFile::SubsTracks(unsigned int title, bool menu) const
{
	IFOContent *_ifo = m_ifos.getIfoContent(title);
	if (!_ifo)
		throw;

	if (menu)
		return _ifo->m_langSubs;
	else
		return _ifo->m_Subs;
}
// ----------------------------------------------------------------------------
const CellsListType & IFOFile::CellsList(unsigned int title, bool menu) const
{
	IFOContent *_ifo = m_ifos.getIfoContent(title);
	if (!_ifo)
		throw;

	if (menu)
		return _ifo->m_langCellsList;
	else
		return _ifo->m_CellsList;
}
// ----------------------------------------------------------------------------
bool IFOFile::VideoSize(unsigned int title, bool menu, uint16_t & width, uint16_t & height, double & fps) const
{
	IFOContent *_ifo = m_ifos.getIfoContent(title);
	assert(_ifo != NULL);
	video_attr_t *_attr = NULL;
	if (menu)
	{
		if (title == 0)
		{
			if (_ifo->Handle().vmgi_mat)
			{
				_attr = &_ifo->Handle().vmgi_mat->vmgm_video_attr;
			}
		}
		else
		{
			if (_ifo->Handle().vtsi_mat)
			{
				_attr = &_ifo->Handle().vtsi_mat->vtsm_video_attr;
			}
		}
	}
	else
	{
		if (_ifo->Handle().vtsi_mat)
		{
			_attr = &_ifo->Handle().vtsi_mat->vts_video_attr;
		}
	}

	if (_attr != NULL)
	{
		height = 480;
		if(_attr->video_format != 0) {
			fps = 25.0;
			height = 576;
		} else {
			fps = 30000.0/1001.0;
		}
		switch(_attr->picture_size) {
			case 0:
				width = 720;
				break;
			case 1:
				width = 704;
				break;
			case 2:
				width = 352;
				break;
			case 3:
				width = 352;
				height >>= 1;
			break;      
		}
		return true;
	}
	return false;
}
// ----------------------------------------------------------------------------
const uint32_t * IFOFile::GetPalette(unsigned int title, bool menu) const
{
	IFOContent *_ifo = m_ifos.getIfoContent(title);
	assert(_ifo != NULL);
	
	if (menu)
		return (_ifo->Handle().pgci_ut->lu->pgcit->pgci_srp->pgc->palette);
	
	return (_ifo->Handle().vts_pgcit->pgci_srp->pgc->palette);
}
// ----------------------------------------------------------------------------
uint8_t IFOFile::GetAudioId(uint8_t streamID, uint8_t title, bool menu) const
{
	IFOContent *_ifo = m_ifos.getIfoContent(title);
	if (!_ifo)
		throw;

	return _ifo->FindAudioStream(streamID, menu);
}
// ----------------------------------------------------------------------------
IdArray IFOFile::GetSubsId(uint8_t streamID, uint8_t title, bool menu) const
{
	IFOContent *_ifo = m_ifos.getIfoContent(title);
	if (!_ifo)
		throw;

	return _ifo->FindSubStream(streamID, menu);
}
// ----------------------------------------------------------------------------
IFOContent * IfoHandleList::getIfoContent(int16_t title) const
{
	for (size_type _index = 0; _index < size(); _index++)
	{
		if (at(_index)->m_title == title)
			return at(_index);
	}
	return NULL;
}
// ----------------------------------------------------------------------------
const CellListElem* CellsListType::at(const cell_position_t & position) const
{
	return at(position.vob_id_nr, position.cell_nr);
}
// ----------------------------------------------------------------------------
CellListElem* CellsListType::at(uint16_t vob_id, uint8_t cell_id) const
{
	CellsListType::const_iterator node = begin();
	while (node != end())
	{
		CellListElem *cell = *node;
		
		if (cell->cellid == cell_id && cell->vobid == vob_id)
			return cell;
		
		++node;
	}
	
	return NULL;
}
// ----------------------------------------------------------------------------
void CellsListType::arrange()
{
	qSort(begin(), end(), &CellsListType::cellListElemCompare);

	// make the timecodes continuous
	int64_t timecode = 0;
	CellsListType::const_iterator node = begin();
	while (node != end())
	{
		CellListElem *cell = *node;
		cell->start_time = timecode;
		cell->duration = int64_t(cell->frame_dur * cell->nb_frames * 1000000.0);
		if (cell->found)
			timecode += cell->duration;
		++node;
	}
}
// ----------------------------------------------------------------------------
