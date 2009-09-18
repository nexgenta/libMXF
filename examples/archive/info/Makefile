#
# $Id: Makefile,v 1.3 2009/09/18 14:35:20 philipn Exp $
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
all: d3_mxf_info libd3mxfinfo.a

CFLAGS += -I..

$(LIBMXF_DIR)/libMXF.a:
	$(MAKE) -C $(LIBMXF_DIR)

d3_mxf_info: $(LIBMXF_DIR)/libMXF.a d3_mxf_info.o
	$(CC) d3_mxf_info.o -L$(LIBMXF_DIR) -lMXF $(UUIDLIB) -o $@

d3_mxf_info.o: d3_mxf_info.c ../archive_types.h
	$(CC) $(CFLAGS) -c d3_mxf_info.c

libd3mxfinfo.a: d3_mxf_info_lib.o
	$(AR) libd3mxfinfo.a d3_mxf_info_lib.o

d3_mxf_info_lib.o: d3_mxf_info_lib.c ../archive_types.h
	$(CC) $(CFLAGS) -c d3_mxf_info_lib.c


.PHONY: clean
clean:
	@rm -f *~ *.o d3_mxf_info libd3mxfinfo.a

.PHONY: install
install: libd3mxfinfo.a
	mkdir -p $(MXF_INSTALL_PREFIX)/lib
	cp libd3mxfinfo.a $(MXF_INSTALL_PREFIX)/lib
	mkdir -p $(MXF_INSTALL_PREFIX)/include
	cp d3_mxf_info_lib.h $(MXF_INSTALL_PREFIX)/include/
	cp ../archive_types.h $(MXF_INSTALL_PREFIX)/include/
	mkdir -p $(MXF_INSTALL_PREFIX)/bin
	cp d3_mxf_info $(MXF_INSTALL_PREFIX)/bin/
