#
# $Id: Makefile,v 1.3 2010/07/21 16:29:33 john_f Exp $
#
# Makefile for building MXFDump utility from the AAFSDK
#
# Copyright (C) 2006  BBC Research, Philip de Nier <philipn@users.sourceforge.net>
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

CPP_CC = g++ -g -Wall -W -Werror -O2

CPP_LD = g++
ifeq ($(OSNAME),Darwin)
CPP_LD = g++ -framework CoreServices
endif

MISSING_AAFSDK_FILES:=$(shell for f in MXFLabels.h MXFMetaDictionary.h MXFDump.cpp AAFMetaDictionary.h; do if test ! -e $$f; then echo $$f; fi done)


.PHONY: all
all: MXFDump


ifeq ($(MISSING_AAFSDK_FILES),)

MXFDump: MXFDump.o
	$(CPP_LD) MXFDump.o -o $@

MXFDump.o: MXFDump.cpp MXFLabels.h MXFMetaDictionary.h
	$(CPP_CC) -c MXFDump.cpp

else
ifneq ($(AAFSDK),)
.PHONY: MXFDump.cpp
MXFDump.cpp:
	cp $(AAFSDK)/DevUtils/MXFDump/MXFLabels.h .
	cp $(AAFSDK)/DevUtils/MXFDump/MXFMetaDictionary.h .
	cp $(AAFSDK)/DevUtils/MXFDump/MXFDump.cpp .
	cp $(AAFSDK)/ref-impl/include/AAFMetaDictionary.h .

MXFDump.o: MXFDump.cpp MXFLabels.h MXFMetaDictionary.h
	$(CPP_CC) -c MXFDump.cpp

MXFDump: MXFDump.o
	$(CPP_LD) MXFDump.o -o $@

else
ifneq ($(MAKECMDGOALS),clean)
.PHONY: MXFDump.cpp
MXFDump.cpp:
	@echo Warning: cannot build MXFDump. Missing AAF SDK files: $(MISSING_AAFSDK_FILES). Set the AAFSDK environment variable to reference the AAF SDK source tree or copy the files manually
	
MXFDump: MXFDump.cpp
	true

endif
endif
endif

.PHONY: clean
clean:
	@rm -f *~ *.o *.so *.a MXFDump

.PHONY: srcclean
srcclean:
	@rm -f *~ *.o *.so *.a
