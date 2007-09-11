MXF_INSTALL_PREFIX ?= /usr/local

LIBMXF_DIR = ../../../lib

CC = gcc -Wall -W -Werror -g -O2 -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE
INCLUDES = -I. -I.. -I$(LIBMXF_DIR)/include
AR = ar cr
	


	
.PHONY: all
all: libMXF.a libwritearchivemxf.a test_write_archive_mxf


.PHONY: libMXF.a
libMXF.a:
	@cd $(LIBMXF_DIR) && $(MAKE) $@

	
libwritearchivemxf.a: write_archive_mxf.o ../timecode_index.o
	$(AR) libwritearchivemxf.a write_archive_mxf.o ../timecode_index.o

write_archive_mxf.o: write_archive_mxf.c write_archive_mxf.h ../archive_types.h ../timecode_index.h libMXF.a
	$(CC) -c $(INCLUDES) write_archive_mxf.c
	
../timecode_index.o: ../timecode_index.c ../timecode_index.h ../archive_types.h libMXF.a
	$(CC) -c $(INCLUDES) ../timecode_index.c -o ../timecode_index.o
	
	
	
	
test_write_archive_mxf: libMXF.a libwritearchivemxf.a test_write_archive_mxf.o
	$(CC) test_write_archive_mxf.o -L$(LIBMXF_DIR) -L. -lwritearchivemxf -lMXF -luuid -o $@

test_write_archive_mxf.o: test_write_archive_mxf.c write_archive_mxf.h ../archive_types.h
	$(CC) -c $(INCLUDES) test_write_archive_mxf.c

.PHONY: install
install: libwritearchivemxf.a
	cp libwritearchivemxf.a $(MXF_INSTALL_PREFIX)/lib
	cp write_archive_mxf.h $(MXF_INSTALL_PREFIX)/include
	
.PHONY: clean
clean:
	@rm -f *~ *.o *.a test_write_archive_mxf
