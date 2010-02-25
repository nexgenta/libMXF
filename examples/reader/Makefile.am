lib_LTLIBRARIES = libMXFReader.la

include_HEADERS = mxf_essence_helper.h mxf_index_helper.h mxf_op1a_reader.h \
	mxf_opatom_reader.h mxf_reader.h mxf_reader_int.h

noinst_PROGRAMS = test_mxf_reader

libMXFReader_la_SOURCES = mxf_reader.c mxf_essence_helper.c \
	mxf_index_helper.c mxf_opatom_reader.c mxf_op1a_reader.c

libMXFReader_la_LIBADD = ../../lib/libMXF.la

libMXFReader_la_LDFLAGS = -avoid-version

test_mxf_reader_SOURCES = test_mxf_reader.c

test_mxf_reader_LDADD = libMXFReader.la
