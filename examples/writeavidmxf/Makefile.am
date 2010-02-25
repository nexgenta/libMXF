bin_PROGRAMS = writeavidmxf

noinst_LTLIBRARIES = libwriteavidmxf.la

writeavidmxf_SOURCES = main.c

writeavidmxf_LDADD = libwriteavidmxf.la

libwriteavidmxf_la_SOURCES = write_avid_mxf.c write_avid_mxf.h \
	package_definitions.c package_definitions.h

libwriteavidmxf_la_LIBADD = ../../lib/libMXF.la
