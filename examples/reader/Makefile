LIBMXF_DIR ?= ../../lib
MXF_INSTALL_PREFIX ?= /usr/local

CC = gcc -Wall -W -Werror -Wno-unused-parameter -g -O2 -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE
INCLUDES = -I. -I.. -I$(LIBMXF_DIR)/include
AR = ar cr
	


	
.PHONY: all
all: libMXF.a libMXFReader.a test_mxf_reader


.PHONY: libMXF.a
libMXF.a:
	@cd $(LIBMXF_DIR) && $(MAKE) $@

libMXFReader.a:mxf_reader.o mxf_essence_helper.o mxf_index_helper.o mxf_opatom_reader.o mxf_op1a_reader.o
	$(AR) libMXFReader.a mxf_reader.o mxf_essence_helper.o mxf_index_helper.o mxf_opatom_reader.o mxf_op1a_reader.o

	
mxf_reader.o: mxf_reader.c mxf_reader.h mxf_reader_int.h libMXF.a
	$(CC) -c $(INCLUDES) mxf_reader.c
	
mxf_essence_helper.o: mxf_essence_helper.c mxf_essence_helper.h mxf_reader.h mxf_reader_int.h libMXF.a
	$(CC) -c $(INCLUDES) mxf_essence_helper.c
	
mxf_index_helper.o: mxf_index_helper.c mxf_index_helper.h mxf_reader.h mxf_reader_int.h libMXF.a
	$(CC) -c $(INCLUDES) mxf_index_helper.c
	
mxf_opatom_reader.o: mxf_opatom_reader.c mxf_opatom_reader.h mxf_reader.h mxf_reader_int.h libMXF.a
	$(CC) -c $(INCLUDES) mxf_opatom_reader.c
	
mxf_op1a_reader.o: mxf_op1a_reader.c mxf_op1a_reader.h mxf_essence_helper.h mxf_index_helper.h mxf_reader.h mxf_reader_int.h libMXF.a
	$(CC) -c $(INCLUDES) mxf_op1a_reader.c
	
	
test_mxf_reader: libMXF.a libMXFReader.a test_mxf_reader.o
	$(CC) test_mxf_reader.o -L$(LIBMXF_DIR) -L. -lMXFReader -lMXF -luuid -o $@

test_mxf_reader.o: test_mxf_reader.c mxf_reader.h
	$(CC) -c $(INCLUDES) test_mxf_reader.c

	

.PHONY: install
install: libMXFReader.a
	cp libMXFReader.a $(MXF_INSTALL_PREFIX)/lib
	mkdir -p $(MXF_INSTALL_PREFIX)/include
	cp *.h $(MXF_INSTALL_PREFIX)/include/

	
.PHONY: clean
clean:
	@rm -f *~ *.o *.a test_mxf_reader
	


