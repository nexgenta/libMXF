#
# $Id: Makefile,v 1.3 2008/05/07 15:22:11 philipn Exp $
#
# Makefile for building transfertop2
#
# Copyright (C) 2007  BBC Research, Philip de Nier <philipn@users.sourceforge.net>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
# 02110-1301, USA.
#
TOPLEVEL = ../..
include $(TOPLEVEL)/vars.mk

CPP_CC = g++ -g -Wall -Wundef -Wno-non-virtual-dtor -Werror -D_DEBUG -DOM_DEBUG -DOM_STACK_TRACE_ON_ASSERT -fsigned-char -DOM_USE_SCHEMASOFT_SS -I/usr/local/lib/include -DUSE_LIBDV -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -D_XOPEN_SOURCE=500 -DLITTLEENDIAN=1
CPP_INCLUDES = -I. -I$(LIBMXF_DIR)/include -I$(AAFSDKINSTALL)/include
CPP_OBJECTS = avidp2transfer.o

C_OBJECTS = xml_writer.o avid_mxf_to_p2.o

ifeq "$(OS)" "Windows_NT"
  CPP_LD = g++
  EXTRA_LIBS=-lole32
else
  CPP_LD = g++ -ldl -rdynamic
  EXTRA_LIBS=$(UUIDLIB)
endif

.PHONY: all
ifndef AAFSDKINSTALL
# The test_avidp2transfer application depends on the AAFSDK, so skip if AAFSDKINSTALL path not set
# AAFSDKINSTALL is path to installed AAF SDK includes and libs (i.e. after an SDK make install)
# e.g. lib/debug/libaaflib.a lib/debug/libaafiid.a include/AAF.h etc
all: test_avid_mxf_to_p2
	@echo "*** AAFSDKINSTALL not set, test_avidp2transfer skipped ***"
else
all: test_avid_mxf_to_p2 test_avidp2transfer
endif

$(LIBMXF_DIR)/libMXF.a:
	$(MAKE) -C $(LIBMXF_DIR)


test_avid_mxf_to_p2: $(LIBMXF_DIR)/libMXF.a test_avid_mxf_to_p2.o $(C_OBJECTS)
	$(CC) test_avid_mxf_to_p2.o $(C_OBJECTS) -L$(LIBMXF_DIR) -lMXF $(EXTRA_LIBS) -o $@

test_avid_mxf_to_p2.o: test_avid_mxf_to_p2.c avid_mxf_to_p2.h
	$(CC) $(CFLAGS) -c test_avid_mxf_to_p2.c

avid_mxf_to_p2.o: avid_mxf_to_p2.c icon_avid_to_p2.h $(LIBMXF_DIR)/include/mxf/mxf.h
	$(CC) $(CFLAGS) -c avid_mxf_to_p2.c

xml_writer.o: xml_writer.c xml_writer.h
	$(CC) $(CFLAGS) -c xml_writer.c


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
	$(CPP_CC) -c $(CPP_INCLUDES) avidp2transfer.cpp


.PHONY: clean
clean:
	@rm -f *~ *.o *.so *.a test_avid_mxf_to_p2 test_avidp2transfer

.PHONY: libclean
libclean: clean
	$(MAKE) -C $(LIBMXF_DIR) clean
