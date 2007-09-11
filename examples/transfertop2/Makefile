LIBMXF_DIR = ../../lib


CPP_CC = g++ -g -Wall -Wundef -Wno-non-virtual-dtor -Werror -D_DEBUG -DOM_DEBUG -DOM_STACK_TRACE_ON_ASSERT -fsigned-char -DOM_USE_SCHEMASOFT_SS -I/usr/local/lib/include -DUSE_LIBDV  -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -D_XOPEN_SOURCE=500 -DLITTLEENDIAN=1
CPP_INCLUDES = -I. -I$(LIBMXF_DIR) -I$(AAFSDKINSTALL)/include
CPP_OBJECTS = avidp2transfer.o

C_CC = gcc -Wall -W -Werror -g -O2 -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE
C_INCLUDES = -I. -I$(LIBMXF_DIR)/include
C_OBJECTS = xml_writer.o avid_mxf_to_p2.o

ifeq "$(OS)" "Windows_NT"
  CPP_LD = g++
  EXTRA_LIBS=-lole32
else
  CPP_LD = g++ -ldl -rdynamic
  EXTRA_LIBS=-luuid
endif

.PHONY: all
all: libMXF test_avid_mxf_to_p2 test_avidp2transfer

.PHONY: libMXF
libMXF:
	@cd $(LIBMXF_DIR) && $(MAKE)


test_avid_mxf_to_p2: $(LIBMXF_DIR)/libMXF.a test_avid_mxf_to_p2.o $(C_OBJECTS)
	$(C_CC) test_avid_mxf_to_p2.o $(C_OBJECTS) -L$(LIBMXF_DIR) -lMXF $(EXTRA_LIBS) -o $@

test_avid_mxf_to_p2.o: test_avid_mxf_to_p2.c avid_mxf_to_p2.h
	$(C_CC) -c $(C_INCLUDES) test_avid_mxf_to_p2.c

avid_mxf_to_p2.o: avid_mxf_to_p2.c icon_avid_to_p2.h $(LIBMXF_DIR)/include/mxf/mxf.h
	$(C_CC) -c $(C_INCLUDES) avid_mxf_to_p2.c

xml_writer.o: xml_writer.c xml_writer.h
	$(C_CC) -c $(C_INCLUDES) xml_writer.c


test_avidp2transfer: $(LIBMXF_DIR)/libMXF.a test_avidp2transfer.o $(CPP_OBJECTS) $(C_OBJECTS)
ifeq "$(OS)" "Windows_NT"
	# FIXME: Remove once AAF SDK static libs are ported to mingw
	$(CPP_LD) test_avidp2transfer.o $(CPP_OBJECTS) $(C_OBJECTS) /home/dp/AAF-src-1.1.1/AAFi686LinuxSDK/g++/aafiid/debug/aafiid.o -L$(LIBMXF_DIR) -L$(AAFSDKINSTALL)/bin -lMXF -laafcoapi $(EXTRA_LIBS) -o $@
else
	$(CPP_LD) test_avidp2transfer.o $(CPP_OBJECTS) $(C_OBJECTS) -Xlinker -rpath -Xlinker . -L$(LIBMXF_DIR) -L$(AAFSDKINSTALL)/lib/debug -lMXF -laaflib -laafiid $(EXTRA_LIBS) -o $@
endif

test_avidp2transfer.o: test_avidp2transfer.cpp avidp2transfer.h
	$(CPP_CC) -c $(CPP_INCLUDES) test_avidp2transfer.cpp

avidp2transfer.o: avidp2transfer.cpp avidp2transfer.h
ifndef AAFSDKINSTALL
	# AAFSDKINSTALL is path to installed AAF SDK includes and libs (i.e. after an SDK make install)
	$(error Please set the AAFSDKINSTALL environment variable to the location of your installed AAFSDK files i.e. path to lib/debug/libaaflib.a lib/debug/libaafiid.a include/AAF.h etc)
endif
	$(CPP_CC) -c $(CPP_INCLUDES) avidp2transfer.cpp



.PHONY: clean
clean:
	@rm -f *~ *.o *.so *.a test_avid_mxf_to_p2 test_avidp2transfer

.PHONY: libclean
libclean: clean
	@cd $(LIBMXF_DIR) && $(MAKE) clean

