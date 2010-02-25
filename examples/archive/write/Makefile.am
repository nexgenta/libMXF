lib_LTLIBRARIES = libwritearchivemxf.la

include_HEADERS = write_archive_mxf.h

bin_PROGRAMS = update_archive_mxf

noinst_PROGRAMS = test_write_archive_mxf

libwritearchivemxf_la_SOURCES = write_archive_mxf.c write_archive_mxf.h \
	../archive_types.h ../timecode_index.c ../timecode_index.h

libwritearchivemxf_la_LIBADD = ../../../lib/libMXF.la

libwritearchivemxf_la_LDFLAGS = -avoid-version

update_archive_mxf_SOURCES = update_archive_mxf.c

update_archive_mxf_LDADD = libwritearchivemxf.la

test_write_archive_mxf_SOURCES = test_write_archive_mxf.c

test_write_archive_mxf_LDADD = libwritearchivemxf.la

INCLUDES = @INCLUDES@ -I${srcdir}/..
