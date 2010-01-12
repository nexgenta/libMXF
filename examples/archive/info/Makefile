#
# $Id: Makefile,v 1.4 2010/01/12 17:41:39 john_f Exp $
#
# Makefile for building archive examples
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
TOPLEVEL = ../../..
include $(TOPLEVEL)/vars.mk


.PHONY: all
all: archive_mxf_info libarchivemxfinfo.a

CFLAGS += -I..

$(LIBMXF_DIR)/libMXF.a:
	$(MAKE) -C $(LIBMXF_DIR)

archive_mxf_info: $(LIBMXF_DIR)/libMXF.a archive_mxf_info.o
	$(CC) archive_mxf_info.o -L$(LIBMXF_DIR) -lMXF $(UUIDLIB) -o $@

archive_mxf_info.o: archive_mxf_info.c ../archive_types.h
	$(CC) $(CFLAGS) -c archive_mxf_info.c

libarchivemxfinfo.a: archive_mxf_info_lib.o
	$(AR) libarchivemxfinfo.a archive_mxf_info_lib.o

archive_mxf_info_lib.o: archive_mxf_info_lib.c ../archive_types.h
	$(CC) $(CFLAGS) -c archive_mxf_info_lib.c


.PHONY: clean
clean:
	@rm -f *~ *.o archive_mxf_info libarchivemxfinfo.a

.PHONY: install
install: libarchivemxfinfo.a
	mkdir -p $(MXF_INSTALL_PREFIX)/lib
	cp libarchivemxfinfo.a $(MXF_INSTALL_PREFIX)/lib
	mkdir -p $(MXF_INSTALL_PREFIX)/include
	cp archive_mxf_info_lib.h $(MXF_INSTALL_PREFIX)/include/
	cp ../archive_types.h $(MXF_INSTALL_PREFIX)/include/
	mkdir -p $(MXF_INSTALL_PREFIX)/bin
	cp archive_mxf_info $(MXF_INSTALL_PREFIX)/bin/
