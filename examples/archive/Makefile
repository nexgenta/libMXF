LIBMXF_DIR = ../../lib

CC = gcc -Wall -W -Wno-unused-parameter -Werror -g -O2 -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE
INCLUDES = -I. -I$(LIBMXF_DIR)/include


.PHONY: all
all: test_timecode_index write info


.PHONY: write
write:
	@cd write && $(MAKE) all
	
	
.PHONY: info
info:
	@cd info && $(MAKE) all
	

.PHONY: libMXF.a
libMXF.a:
	@cd $(LIBMXF_DIR) && $(MAKE) $@

	
test_timecode_index: libMXF.a test_timecode_index.o timecode_index.o
	$(CC) test_timecode_index.o timecode_index.o -L$(LIBMXF_DIR) -lMXF -luuid -o $@

test_timecode_index.o: test_timecode_index.c archive_types.h
	$(CC) -c $(INCLUDES) test_timecode_index.c
	
timecode_index.o: timecode_index.c timecode_index.h archive_types.h
	$(CC) -c $(INCLUDES) timecode_index.c
	
	
	
.PHONY: clean
clean:
	@rm -f *.o *~ test_timecode_index
	@cd write && $(MAKE) $@
	@cd info && $(MAKE) $@
	
	
	