/* -*- c-basic-offset: 2; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2000 Björn Englund <d4bjorn@dtek.chalmers.se>,
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307
 * USA
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#if defined(HAVE_INTTYPES_H)
#include <inttypes.h>
#elif defined(HAVE_STDINT_H)
#include <stdint.h>
#endif

#include <dvdread/dvd_reader.h>
#include <dvdread/ifo_print.h>

static char *program_name;

void usage(void)
{
  fprintf(stderr, "Usage: %s <dvd path> <title number>\n", program_name);
}

int main(int argc, char *argv[])
{
  dvd_reader_t *dvd;
  program_name = argv[0];
  
  if(argc != 3) {
    usage();
    return 1;
  }

  dvd = DVDOpen( argv[ 1 ] );
  if( !dvd ) {
    fprintf( stderr, "Can't open disc %s!\n", argv[ 1 ] );
    return -1;
  }

  ifoPrint( dvd, atoi( argv[ 2 ] ) );
  DVDClose(dvd);

  DVDFinish(); //to keep memory checkers from complaining 
  return 0;  
}

