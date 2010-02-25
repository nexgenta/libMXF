lib_LTLIBRARIES = libarchivemxfinfo.la

include_HEADERS = archive_mxf_info_lib.h

bin_PROGRAMS = archive_mxf_info

libarchivemxfinfo_la_SOURCES = archive_mxf_info_lib.c archive_mxf_info_lib.h

libarchivemxfinfo_la_LIBADD = ../../../lib/libMXF.la

libarchivemxfinfo_la_LDFLAGS = -avoid-version

archive_mxf_info_SOURCES = archive_mxf_info.c

archive_mxf_info_LDADD = libarchivemxfinfo.la

INCLUDES = @INCLUDES@ -I${srcdir}/..
