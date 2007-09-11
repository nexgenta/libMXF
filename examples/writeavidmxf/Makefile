LIBMXF_DIR = ../../lib

CC = gcc -Wall -W -Werror -g -O2 -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE
INCLUDES = -I. -I$(LIBMXF_DIR)/include
AR = ar cr
	


	
.PHONY: all
all: libMXF.a libwriteavidmxf.a writeavidmxf


.PHONY: libMXF.a
libMXF.a:
	@cd $(LIBMXF_DIR) && $(MAKE) $@

	
writeavidmxf: main.o libMXF.a libwriteavidmxf.a
	$(CC) main.o -L$(LIBMXF_DIR) -L. -lwriteavidmxf -lMXF -luuid -o $@

main.o: main.c write_avid_mxf.h
	$(CC) -c $(INCLUDES) main.c
	

libwriteavidmxf.a: write_avid_mxf.o package_definitions.o
	$(AR) libwriteavidmxf.a write_avid_mxf.o package_definitions.o
	
	
write_avid_mxf.o: write_avid_mxf.c write_avid_mxf.h
	$(CC) -c $(INCLUDES) write_avid_mxf.c
	
package_definitions.o: package_definitions.c package_definitions.h
	$(CC) -c $(INCLUDES) package_definitions.c
	
	
	
.PHONY: clean
clean:
	@rm -f *~ *.o *.a writeavidmxf
	



