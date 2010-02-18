/* -*- c-basic-offset: 2; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2001 Billy Biggs <vektor@dumbterm.net>,
 *                    Håkan Hjort <d95hjort@dtek.chalmers.se>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "config.h"

#include <stdio.h>

#if defined(HAVE_INTTYPES_H)
#include <inttypes.h>
#elif defined(HAVE_STDINT_H)
#include <stdint.h>
#endif

#include <dvdread/dvd_reader.h>
#include <dvdread/ifo_types.h>
#include <dvdread/ifo_read.h>

int main( int argc, char **argv )
{
    dvd_reader_t *dvd;
    ifo_handle_t *ifo_file;
    tt_srpt_t *tt_srpt;
    int i, j;

    /**
     * Usage.
     */
    if( argc != 2 ) {
        fprintf( stderr, "Usage: %s <dvd path>\n", argv[ 0 ] );
        return -1;
    }

    dvd = DVDOpen( argv[ 1 ] );
    if( !dvd ) {
        fprintf( stderr, "Couldn't open DVD: %s\n", argv[ 1 ] );
        return -1;
    }

    ifo_file = ifoOpen( dvd, 0 );
    if( !ifo_file ) {
        fprintf( stderr, "Can't open VMG info.\n" );
        DVDClose( dvd );
        return -1;
    }
    tt_srpt = ifo_file->tt_srpt;

    printf( "There are %d titles.\n", tt_srpt->nr_of_srpts );
    for( i = 0; i < tt_srpt->nr_of_srpts; ++i ) {
        ifo_handle_t *vts_file;
        vts_ptt_srpt_t *vts_ptt_srpt;
        pgc_t *cur_pgc;
        int vtsnum, ttnnum, pgcnum, chapts;
	
        vtsnum = tt_srpt->title[ i ].title_set_nr;
        ttnnum = tt_srpt->title[ i ].vts_ttn;
        chapts = tt_srpt->title[ i ].nr_of_ptts;
	
        printf( "\nTitle %d:\n", i + 1 );
        printf( "\tIn VTS: %d [TTN %d]\n", vtsnum, ttnnum );
        printf( "\n" );
        printf( "\tTitle has %d chapters and %d angles\n", chapts,
                tt_srpt->title[ i ].nr_of_angles );

        vts_file = ifoOpen( dvd, vtsnum );
        if( !vts_file ) {
            fprintf( stderr, "Can't open info file for title %d.\n", vtsnum );
            DVDClose( dvd );
            return -1;
        }

        vts_ptt_srpt = vts_file->vts_ptt_srpt;
        for( j = 0; j < chapts; ++j ) {
            int start_cell;
            int pgn;

            pgcnum = vts_ptt_srpt->title[ ttnnum - 1 ].ptt[ j ].pgcn;
            pgn = vts_ptt_srpt->title[ ttnnum - 1 ].ptt[ j ].pgn;
            cur_pgc = vts_file->vts_pgcit->pgci_srp[ pgcnum - 1 ].pgc;
            start_cell = cur_pgc->program_map[ pgn - 1 ] - 1;
	
            printf( "\tChapter %3d [PGC %2d, PG %2d] starts at Cell %2d [sector %x-%x]\n",
                    j, pgcnum, pgn, start_cell,
                    cur_pgc->cell_playback[ start_cell ].first_sector,
                    cur_pgc->cell_playback[ start_cell ].last_sector );
        }

        ifoClose( vts_file );
    }

    ifoClose( ifo_file );
    DVDClose( dvd );
    DVDFinish();
    return 0;
}

