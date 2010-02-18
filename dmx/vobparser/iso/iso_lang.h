/*****************************************************************************
 * iso_lang.h: function to decode language code (in dvd or a52 for instance).
 *****************************************************************************
 * Copyright (C) 1998-2001 VideoLAN
 * $Id: iso_lang.h,v 1.5 2002/05/20 22:36:42 sam Exp $
 *
 * Author: St�phane Borel <stef@via.ecp.fr>
 *         Arnaud de Bossoreille de Ribou <bozo@via.ecp.fr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111, USA.
 *****************************************************************************/

#ifndef _ISO_LANG_H_
#define _ISO_LANG_H_

struct iso639_lang_s
{
    char * psz_eng_name;        /* Description in English */
    char * psz_native_name;     /* Description in native language */
    char * psz_iso639_1;        /* ISO-639-1 (2 characters) code */
    char * psz_iso639_2T;       /* ISO-639-2/T (3 characters) English code */
    char * psz_iso639_2B;       /* ISO-639-2/B (3 characters) native code */
};

typedef struct iso639_lang_s iso639_lang_t;

#ifdef __cplusplus
extern "C" {
#endif

const iso639_lang_t * GetLang_1( const char * psz_iso639_1 );
const iso639_lang_t * GetLang_2T( const char * psz_iso639_2T );
const iso639_lang_t * GetLang_2B( const char * psz_iso639_2B );
const char * DecodeLanguage( uint16_t );

#ifdef __cplusplus
};
#endif
#endif // _ISO_LANG_H_
