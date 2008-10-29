#
# $Id: Makefile,v 1.3 2008/10/29 17:54:26 john_f Exp $
#
# Makefile for building MXF reader library
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


.PHONY: all
all: libMXFReader.a test_mxf_reader


$(LIBMXF_DIR)/libMXF.a:
	$(MAKE) -C $(LIBMXF_DIR)

libMXFReader.a: mxf_reader.o mxf_essence_helper.o mxf_index_helper.o mxf_opatom_reader.o mxf_op1a_reader.o
	$(AR) libMXFReader.a mxf_reader.o mxf_essence_helper.o mxf_index_helper.o mxf_opatom_reader.o mxf_op1a_reader.o


mxf_reader.o: mxf_reader.c mxf_reader.h mxf_reader_int.h
	$(CC) $(CFLAGS) -c mxf_reader.c

mxf_essence_helper.o: mxf_essence_helper.c mxf_essence_helper.h mxf_reader.h mxf_reader_int.h
	$(CC) $(CFLAGS) -c mxf_essence_helper.c

mxf_index_helper.o: mxf_index_helper.c mxf_index_helper.h mxf_reader.h mxf_reader_int.h
	$(CC) $(CFLAGS) -c mxf_index_helper.c

mxf_opatom_reader.o: mxf_opatom_reader.c mxf_opatom_reader.h mxf_reader.h mxf_reader_int.h
	$(CC) $(CFLAGS) -c mxf_opatom_reader.c

mxf_op1a_reader.o: mxf_op1a_reader.c mxf_op1a_reader.h mxf_essence_helper.h mxf_index_helper.h mxf_reader.h mxf_reader_int.h
	$(CC) $(CFLAGS) -c mxf_op1a_reader.c


test_mxf_reader: $(LIBMXF_DIR)/libMXF.a libMXFReader.a test_mxf_reader.o
	$(CC) test_mxf_reader.o -L$(LIBMXF_DIR) -L. -lMXFReader -lMXF $(UUIDLIB) -o $@

test_mxf_reader.o: test_mxf_reader.c mxf_reader.h
	$(CC) $(CFLAGS) -Wno-unused-parameter -c test_mxf_reader.c


.PHONY: install
install: libMXFReader.a
	cp libMXFReader.a $(MXF_INSTALL_PREFIX)/lib
	mkdir -p $(MXF_INSTALL_PREFIX)/include
	cp *.h $(MXF_INSTALL_PREFIX)/include/

.PHONY: clean
clean:
	@rm -f *~ *.o *.a test_mxf_reader

.PHONY: check
check: all
	./test_mxf_reader ../writeavidmxf/test_unc_v1.mxf /dev/null

.PHONY: valgrind-check
valgrind-check: all
	valgrind ./test_mxf_reader ../writeavidmxf/test_unc_v1.mxf /dev/null
