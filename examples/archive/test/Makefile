CC = gcc -Wall -W -Wno-unused-parameter -Werror -g -O3 -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE
INCLUDES = -I.


.PHONY: all
all: compare_d3_mxf double_clapperboard


compare_d3_mxf: compare_d3_mxf.o
	$(CC) compare_d3_mxf.o -o $@

compare_d3_mxf.o: compare_d3_mxf.c
	$(CC) -c $(INCLUDES) compare_d3_mxf.c
	

double_clapperboard: avsync_eval.o double_clapperboard.o
	$(CC) double_clapperboard.o avsync_eval.o -lm -o $@

double_clapperboard.o: double_clapperboard.c avsync_eval.h
	$(CC) -c $(INCLUDES) double_clapperboard.c
	
	
avsync_eval.o: avsync_eval.c avsync_eval.h
	$(CC) -c $(INCLUDES) avsync_eval.c
	
	
.PHONY: clean
clean:
	@rm -f *~ *.o compare_d3_mxf double_clapperboard
	
	


