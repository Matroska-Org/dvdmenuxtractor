/* -*- c-basic-offset: 2; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2006 Björn Englund <d4bjorn@dtek.chalmers.se>
 * 
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

/*
 * This program is a demo of the DVDFileStat() call.
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif
#include <errno.h>
#include <string.h>
#if defined(HAVE_INTTYPES_H)
#include <inttypes.h>
#elif defined(HAVE_STDINT_H)
#include <stdint.h>
#endif

#include <dvdread/dvd_reader.h>
#include <dvdread/ifo_read.h>

static char *program_name;

void usage(void)
{
  fprintf(stderr, "Usage: %s <dvd path>\n", program_name);
}


void print_stat(dvd_stat_t *buf)
{
  
  printf("size: %lld", buf->size);
  if(buf->nr_parts > 1) {
    int n;
    for(n = 0; n < buf->nr_parts; n++) {
      printf(", [%lld]", buf->parts_size[n]);
    }
  }
}

void print_size(dvd_reader_t *dvd, int titlenum, 
                dvd_read_domain_t dom) 
{
  dvd_stat_t buf;
  char filename[256];
  
  if(titlenum == 0) {
    switch(dom) {
    case DVD_READ_INFO_FILE:
      sprintf(filename, "VIDEO_TS.IFO");
      break;
    case DVD_READ_INFO_BACKUP_FILE:
      sprintf(filename, "VIDEO_TS.BUP");
      break;
    case DVD_READ_MENU_VOBS:
      sprintf(filename, "VIDEO_TS.VOB");
      break;
    default:
      sprintf(filename, "illegal");
    }
  } else {
    switch(dom) {
    case DVD_READ_INFO_FILE:
      sprintf(filename, "VTS_%02d_0.IFO", titlenum);
      break;
    case DVD_READ_INFO_BACKUP_FILE:
      sprintf(filename, "VTS_%02d_0.BUP", titlenum);
      break;
    case DVD_READ_MENU_VOBS:
      sprintf(filename, "VTS_%02d_0.VOB", titlenum);
      break;
    case DVD_READ_TITLE_VOBS:
      sprintf(filename, "VTS_%02d_[1-9].VOB", titlenum);
      break;
    default:
      sprintf(filename, "illegal");
    }    
  }

  if(!DVDFileStat(dvd, titlenum, dom, &buf)) {
    printf("\n%s: ", filename);
    print_stat(&buf);
  } else {
    fprintf(stderr, "stat(%s): %s\n", filename, strerror(errno));
  }
}


int main(int argc, char *argv[])
{
  //int i;
  int err = 0;

  dvd_reader_t *dvd;

  program_name = argv[0];
  
  if(argc != 2) {
    usage();
    return 1;
  }

  dvd = DVDOpen( argv[ 1 ] );

  if( dvd ) {
    ifo_handle_t *vmg_ifo;
    int title_sets;
    //dvd_stat_t buf;
    int n;


    vmg_ifo = ifoOpen( dvd, 0);
    if(!vmg_ifo) {
      return 1;
    }

    title_sets = vmg_ifo->vmgi_mat->vmg_nr_of_title_sets;
    
    ifoClose(vmg_ifo);

    // VIDEO_TS.IFO
    print_size(dvd, 0, DVD_READ_INFO_FILE);
    // VIDEO_TS.VOB
    print_size(dvd, 0, DVD_READ_MENU_VOBS);
    // VIDEO_TS.BUP
    print_size(dvd, 0, DVD_READ_INFO_BACKUP_FILE);

    for(n = 0; n < title_sets; n++) {
      // foreach titleset
      //  VTS_??_0.IFO
      print_size(dvd, n+1, DVD_READ_INFO_FILE);
      //  VTS_??_0.VOB
      print_size(dvd, n+1, DVD_READ_MENU_VOBS);
      //  VTS_??_[1-9].VOB
      print_size(dvd, n+1, DVD_READ_TITLE_VOBS);
      //  VTS_??_0.BUP
      print_size(dvd, n+1, DVD_READ_INFO_BACKUP_FILE);
    }
    printf("\n");
    DVDClose(dvd);
  } else {
    fprintf( stderr, "Can't open disc '%s': %s\n",
             argv[ 1 ], strerror(errno) );
    err = -1;
  }

  DVDFinish();

  return err;  
}

