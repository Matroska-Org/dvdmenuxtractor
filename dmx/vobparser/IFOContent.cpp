// ----------------------------------------------------------------------------
#include "IFOContent.h"
// ----------------------------------------------------------------------------
// Macro to convert Binary Coded Decimal to Decimal
#define BCD2D(__x__) (((__x__ & 0xf0) >> 4) * 10 + (__x__ & 0x0f))
// ----------------------------------------------------------------------------
int dvdtime2frame(dvd_time_t* dtime, double & frame_dur)
{
	double frame_rate;
	double result = 0.0;
	assert((dtime->hour>>4) < 0xa && (dtime->hour&0xf) < 0xa);
	assert((dtime->minute>>4) < 0x7 && (dtime->minute&0xf) < 0xa);
	assert((dtime->second>>4) < 0x7 && (dtime->second&0xf) < 0xa);
	assert((dtime->frame_u&0xf) < 0xa);

	int hh, min, sec, ff;
	hh = BCD2D(dtime->hour);
	min = BCD2D(dtime->minute);
	sec = BCD2D(dtime->second);
	ff = BCD2D(dtime->frame_u & 0x3f);

	frame_rate = ((dtime->frame_u & 0xc0) >> 6) == 1 ? 25.0 : (30000.0/1001.0);
	result = (((((hh * 60) + min) * 60) + sec) * frame_rate) + (ff & 0x3f);
	frame_dur = 1000.0 / frame_rate;

	return (int)result;
}
// ----------------------------------------------------------------------------
IFOContent::IFOContent(dvd_reader_t *dvdr, int16_t title)
	:m_title(title)
{
	m_handle = ifoOpen(dvdr, title);
	if (!m_handle)
		throw IFOInvalidFileFormatException();

	CellsHashType CellsHash;

	if (title == 0)
	{
		if (m_handle->vmgi_mat)
		{
			const vmgi_mat_t &vmgi_mat = *m_handle->vmgi_mat;
			if (vmgi_mat.nr_of_vmgm_audio_streams)
				m_langAudio.push_back(&vmgi_mat.vmgm_audio_attr);
			if (vmgi_mat.nr_of_vmgm_subp_streams)
				m_langSubs.push_back(&vmgi_mat.vmgm_subp_attr);
		}

		if (m_handle->pgci_ut && m_handle->menu_c_adt)
		{
			// add each cell in a list
			// order this list according to vob ID, cell ID
			// for each cell, find the duration (and start time) from all menu PGCs
			for(uint16_t i = 0; i < m_handle->pgci_ut->nr_of_lus; i++)
			{
				pgci_lu_t& lu = m_handle->pgci_ut->lu[i];
				GetPGCCells(*lu.pgcit, *m_handle->menu_c_adt, &CellsHash);
			}

			// Transfer hash map to the list to be sorted
			m_langCellsList.clear();
			
			for(CellsHashType::iterator it = CellsHash.begin(); it != CellsHash.end(); ++it )
			{
				m_langCellsList.append(it.value());
			}

			m_langCellsList.arrange();
		}
	}
	else
	{
		if (m_handle->vtsi_mat)
		{
			const vtsi_mat_t & vtsi_mat = *m_handle->vtsi_mat;
			if (vtsi_mat.nr_of_vtsm_audio_streams)
				m_langAudio.push_back(&vtsi_mat.vtsm_audio_attr);
			if (vtsi_mat.nr_of_vtsm_subp_streams)
				m_langSubs.push_back(&vtsi_mat.vtsm_subp_attr);

			uint8_t _stream;
			for (_stream=0; _stream < vtsi_mat.nr_of_vts_audio_streams; _stream++)
				m_Audio.push_back(&vtsi_mat.vts_audio_attr[_stream]);
			for (_stream=0; _stream < vtsi_mat.nr_of_vts_subp_streams; _stream++)
				m_Subs.push_back(&vtsi_mat.vts_subp_attr[_stream]);
		}

		// handle the list of menu cells with their ID and sectors
		CellsHash.clear();
		if (m_handle->pgci_ut && m_handle->menu_c_adt)
		{
			// add each cell in a list
			// order this list according to vob ID, cell ID
			// for each cell, find the duration (and start time) from all menu PGCs
			for(int i = 0; i < m_handle->pgci_ut->nr_of_lus; i++)
			{
				pgci_lu_t& lu = m_handle->pgci_ut->lu[i];
				GetPGCCells(*lu.pgcit, *m_handle->menu_c_adt, &CellsHash);
			}

			// Transfer hash map to the list to be sorted
			m_langCellsList.clear();
			CellsHashType::iterator it;
			for( it = CellsHash.begin(); it != CellsHash.end(); ++it )
			{
				m_langCellsList.append(it.value());
			}

			m_langCellsList.arrange();
		}

		// handle the list of cells with their ID and sectors
		CellsHash.clear();
		if (m_handle->vts_pgcit && m_handle->vts_c_adt)
		{
			// add each cell in a list
			// order this list according to vob ID, cell ID
			// for each cell, find the duration (and start time) from all PGCs
			GetPGCCells(*m_handle->vts_pgcit, *m_handle->vts_c_adt, &CellsHash);

			// Transfer hash map to the list to be sorted
			m_CellsList.clear();
			
			for(CellsHashType::iterator it = CellsHash.begin(); it != CellsHash.end(); ++it )
			{
				m_CellsList.append(it.value());
			}

			m_CellsList.arrange();
		}
	}
}
// ----------------------------------------------------------------------------
void IFOContent::GetPGCCells(const pgcit_t & pgcit, const c_adt_t & adt, CellsHashType * CellsHash)
{
	CellListElem* cle;

	for(int j=0; j < pgcit.nr_of_pgci_srp; j++)
	{
		pgci_srp_t& srp = pgcit.pgci_srp[j];
		for(int k=0; k < srp.pgc->nr_of_cells; k++)
		{
			uint16_t vob_id = srp.pgc->cell_position[k].vob_id_nr;
			uint8_t cell_id = srp.pgc->cell_position[k].cell_nr;

			if (CellsHash->find(MAKE_CELLS_KEY(vob_id, cell_id)) == CellsHash->end())
			{
				size_t cell_nr = adt.last_byte;
				cell_nr -= 7;
				cell_nr /= sizeof(cell_adr_t);
				for (size_t l=0; l < cell_nr; l++)
				{
					if (adt.cell_adr_table[l].vob_id == vob_id && adt.cell_adr_table[l].cell_id == cell_id)
					{
						cle = new CellListElem();
						(*CellsHash)[MAKE_CELLS_KEY(vob_id, cell_id)] = cle;

						cle->vobid = vob_id;
						cle->cellid = cell_id;

						cle->start_sector = adt.cell_adr_table[l].start_sector;
						cle->last_sector = adt.cell_adr_table[l].last_sector;
						cle->selected = true; // all cells selected for the moment
						cle->found = false;
						/* debug	* /			cle->found = true;*/

						cle->nb_frames = dvdtime2frame(&srp.pgc->cell_playback[k].playback_time, cle->frame_dur);
						cle->isStill = (srp.pgc->cell_playback[k].still_time > 0);
						if (cle->isStill)
						{
							if (srp.pgc->cell_playback[k].still_time == 0xFF) {
							  qWarning(qPrintable(QString("Still cell (%1.%2) detected with infinite duration !").arg(vob_id).arg(cell_id)));
							} else {
							  qWarning(qPrintable(QString("Still cell (%1.%2) detected. Assuming there is just one frame.").arg(vob_id).arg(cell_id)));
								cle->nb_frames = 1; // maybe not true ?
								cle->frame_dur = srp.pgc->cell_playback[k].still_time * 1000.0 / cle->nb_frames;
							}
						}
						break;
					}
				}
			}
		}
	}
}
// ----------------------------------------------------------------------------
uint8_t IFOContent::FindAudioStream(uint8_t streamID, bool menu) const
{
	assert(m_handle != NULL);
	if (menu)
	{
		if (m_handle->pgci_ut)
		{
			for (int i=0; i<m_handle->pgci_ut->nr_of_lus; i++)
			{
				pgci_lu_t &_lu = m_handle->pgci_ut->lu[i];
				for (int j=0; j < _lu.pgcit->nr_of_pgci_srp; j++)
				{
					pgc_t *_pgc = _lu.pgcit->pgci_srp[j].pgc;
					if(_pgc->audio_control[streamID] & 0x8000) // if present
						return (_pgc->audio_control[streamID] >> 8) & 0x7F;
				}
			}
		}
	}
	else
	{
		if (m_handle->vts_pgcit)
		{
			for (int j=0; j < m_handle->vts_pgcit->nr_of_pgci_srp; j++)
			{
				pgc_t *_pgc = m_handle->vts_pgcit->pgci_srp[j].pgc;
				if(_pgc->audio_control[streamID] & 0x8000) // if present
					return (_pgc->audio_control[streamID] >> 8) & 0x7F;
			}
		}
	}
	return streamID;
}
// ----------------------------------------------------------------------------
IdArray IFOContent::FindSubStream(uint8_t streamID, bool menu) const
{
	IdArray result;

	assert(m_handle != NULL);
	if (menu)
	{
		if (m_handle->pgci_ut)
		{
			for (int i=0; i<m_handle->pgci_ut->nr_of_lus; i++)
			{
				pgci_lu_t &_lu = m_handle->pgci_ut->lu[i];
				for (int j=0; j < _lu.pgcit->nr_of_pgci_srp; j++)
				{
					pgc_t *_pgc = _lu.pgcit->pgci_srp[j].pgc;
					if(_pgc->subp_control[streamID] & 0x80000000) // if present
					{
						result.push_back( _pgc->subp_control[streamID] >> 24 & 0x7F);
						if (_pgc->subp_control[streamID] & 0x007F0000)
							result.push_back( _pgc->subp_control[streamID] >> 16 & 0x7F);
						if (_pgc->subp_control[streamID] & 0x00007F00)
							result.push_back( _pgc->subp_control[streamID] >>  8 & 0x7F);
						if (_pgc->subp_control[streamID] & 0x0000007F)
							result.push_back( _pgc->subp_control[streamID] >>  0 & 0x7F);
					}
				}
			}
		}
	}
	else
	{
		if (m_handle->vts_pgcit)
		{
			for (int j=0; j < m_handle->vts_pgcit->nr_of_pgci_srp; j++)
			{
				pgc_t *_pgc = m_handle->vts_pgcit->pgci_srp[j].pgc;
				if(_pgc->subp_control[streamID] & 0x80000000) // if present
				{
					result.push_back( _pgc->subp_control[streamID] >> 24 & 0x7F);
					if (_pgc->subp_control[streamID] & 0x007F0000)
						result.push_back( _pgc->subp_control[streamID] >> 16 & 0x7F);
					if (_pgc->subp_control[streamID] & 0x00007F00)
						result.push_back( _pgc->subp_control[streamID] >>  8 & 0x7F);
					if (_pgc->subp_control[streamID] & 0x0000007F)
						result.push_back( _pgc->subp_control[streamID] >>  0 & 0x7F);
				}
			}
		}
	}

	return result;
}
// ----------------------------------------------------------------------------
