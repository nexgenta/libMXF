CPP_CC = g++ -g -Wall -Wundef -Wno-non-virtual-dtor -Werror -D_DEBUG -DOM_DEBUG -DOM_STACK_TRACE_ON_ASSERT -fsigned-char -DOM_USE_SCHEMASOFT_SS -I/usr/local/lib/include -DUSE_LIBDV  -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -D_XOPEN_SOURCE=500 -DLITTLEENDIAN=1
CPP_INCLUDES = -I. 

ifeq "$(OS)" "Windows_NT"
  CPP_LD = g++
else
  CPP_LD = g++ -ldl -rdynamic
endif

.PHONY: all
all: MXFDump

MXFDump: MXFDump.o
	$(CPP_LD) MXFDump.o -Xlinker -rpath -Xlinker . -o $@

MXFDump.o: MXFDump.cpp MXFLabels.h MXFMetaDictionary.h
	$(CPP_CC) -c $(CPP_INCLUDES) MXFDump.cpp


.PHONY: clean
clean:
	@rm -f *~ *.o *.so *.a MXFDump

.PHONY: srcclean
srcclean:
	@rm -f *~ *.o *.so *.a


