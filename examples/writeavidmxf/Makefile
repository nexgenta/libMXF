LIBMXF_DIR = ../../lib

CC = gcc -Wall -W -Werror -g -O2 -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE
INCLUDES = -I. -I$(LIBMXF_DIR)/include
AR = ar cr
	


	
.PHONY: all
all: libMXF.a libwriteavidmxf.a test_write_avid_mxf


.PHONY: libMXF.a
libMXF.a:
	@cd $(LIBMXF_DIR) && $(MAKE) $@

	
test_write_avid_mxf: test_write_avid_mxf.o libMXF.a libwriteavidmxf.a
	$(CC) test_write_avid_mxf.o -L$(LIBMXF_DIR) -L. -lwriteavidmxf -lMXF -luuid -o $@

test_write_avid_mxf.o: test_write_avid_mxf.c write_avid_mxf.h
	$(CC) -c $(INCLUDES) test_write_avid_mxf.c
	

libwriteavidmxf.a: write_avid_mxf.o package_definitions.o
	$(AR) libwriteavidmxf.a write_avid_mxf.o package_definitions.o
	
	
write_avid_mxf.o: write_avid_mxf.c write_avid_mxf.h
	$(CC) -c $(INCLUDES) write_avid_mxf.c
	
package_definitions.o: package_definitions.c package_definitions.h
	$(CC) -c $(INCLUDES) package_definitions.c
	
	
	
.PHONY: clean
clean:
	@rm -f *~ *.o *.a test_write_avid_mxf
	



