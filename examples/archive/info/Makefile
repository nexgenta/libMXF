MXF_INSTALL_PREFIX ?= /usr/local

LIBMXF_DIR = ../../../lib

CC = gcc -Wall -W -Wno-unused-parameter -Werror -g -O2 -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE
INCLUDES = -I. -I.. -I$(LIBMXF_DIR)/include
AR = ar cr


	
.PHONY: all
all: d3_mxf_info libd3mxfinfo.a


.PHONY: libMXF.a
libMXF.a:
	@cd $(LIBMXF_DIR) && $(MAKE) $@

	
d3_mxf_info: libMXF.a d3_mxf_info.o
	$(CC) d3_mxf_info.o -L$(LIBMXF_DIR) -lMXF -luuid -o $@

d3_mxf_info.o: d3_mxf_info.c ../archive_types.h
	$(CC) -c $(INCLUDES) d3_mxf_info.c
	
	
libd3mxfinfo.a: d3_mxf_info_lib.o
	$(AR) libd3mxfinfo.a d3_mxf_info_lib.o

d3_mxf_info_lib.o: d3_mxf_info_lib.c ../archive_types.h
	$(CC) -c $(INCLUDES) d3_mxf_info_lib.c
	
	
.PHONY: clean
clean:
	@rm -f *~ *.o d3_mxf_info libd3mxfinfo.a
	
	
.PHONY: install
install: libd3mxfinfo.a
	cp libd3mxfinfo.a $(MXF_INSTALL_PREFIX)/lib
	mkdir -p $(MXF_INSTALL_PREFIX)/include
	cp d3_mxf_info_lib.h $(MXF_INSTALL_PREFIX)/include/
	cp ../archive_types.h $(MXF_INSTALL_PREFIX)/include/

	


