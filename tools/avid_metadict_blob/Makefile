CC = gcc -g -O2
INCLUDES = -I.


.PHONY: all
all: extract_blob gen_blob_source

gen_blob_source: gen_blob_source.o
	$(CC) gen_blob_source.o -o gen_blob_source

gen_blob_source.o: gen_blob_source.c
	$(CC) -c $(INCLUDES) gen_blob_source.c
	
extract_blob: extract_blob.o
	$(CC) extract_blob.o -o extract_blob

extract_blob.o: extract_blob.c
	$(CC) -c $(INCLUDES) extract_blob.c
	

clean:
	@rm -f *~ *.o gen_blob_source extract_blob


