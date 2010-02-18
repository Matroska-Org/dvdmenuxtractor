# 
# There really shouldn't be any need to use @DL_LIBS@ and @CSS_LIBS@
# here but old versions of libtool failed to do the correct thing

INCLUDES = -I$(top_srcdir)

noinst_PROGRAMS = ifo_dump play_title title_info disc_id filestat

ifo_dump_SOURCES = ifo_dump.c
ifo_dump_LDADD = $(top_builddir)/dvdread/libdvdread.la @DL_LIBS@ @CSS_LIBS@

play_title_SOURCES = play_title.c
play_title_LDADD = $(top_builddir)/dvdread/libdvdread.la @DL_LIBS@ @CSS_LIBS@

title_info_SOURCES = title_info.c
title_info_LDADD = $(top_builddir)/dvdread/libdvdread.la @DL_LIBS@ @CSS_LIBS@

disc_id_SOURCES = disc_id.c
disc_id_LDADD = $(top_builddir)/dvdread/libdvdread.la @DL_LIBS@ @CSS_LIBS@

filestat_SOURCES = filestat.c
filestat_LDADD = $(top_builddir)/dvdread/libdvdread.la @DL_LIBS@ @CSS_LIBS@

