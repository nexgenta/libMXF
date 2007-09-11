LIBMXF_DIR = ../../lib

CC = gcc -g -Wall -W -Werror -O2 -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE
INCLUDES = -I. -I$(LIBMXF_DIR)/include


.PHONY: all
all: libMXF.a writeaviddv50

.PHONY: libMXF.a
libMXF.a:
	@cd $(LIBMXF_DIR) && $(MAKE) $@

writeaviddv50: libMXF.a writeaviddv50.o
	$(CC) writeaviddv50.o -L$(LIBMXF_DIR) -lMXF -luuid -o writeaviddv50

writeaviddv50.o: writeaviddv50.c libMXF.a
	$(CC) -c $(INCLUDES) writeaviddv50.c
	
	
.PHONY: test
test:
	@cd test && $(MAKE) $@
	
	
.PHONY: clean
clean:
	@rm -f *~ *.o writeaviddv50
	@cd test && $(MAKE) $@
	

