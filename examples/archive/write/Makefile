#
# $Id: Makefile,v 1.2 2008/05/07 15:22:06 philipn Exp $
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
all: libwritearchivemxf.a test_write_archive_mxf

CFLAGS += -I..

$(LIBMXF_DIR)/libMXF.a:
	$(MAKE) -C $(LIBMXF_DIR)

libwritearchivemxf.a: write_archive_mxf.o ../timecode_index.o
	$(AR) libwritearchivemxf.a write_archive_mxf.o ../timecode_index.o

write_archive_mxf.o: write_archive_mxf.c write_archive_mxf.h ../archive_types.h ../timecode_index.h
	$(CC) $(CFLAGS) -c write_archive_mxf.c

../timecode_index.o: ../timecode_index.c ../timecode_index.h ../archive_types.h
	$(CC) $(CFLAGS) -c ../timecode_index.c -o ../timecode_index.o


test_write_archive_mxf: $(LIBMXF_DIR)/libMXF.a libwritearchivemxf.a test_write_archive_mxf.o
	$(CC) test_write_archive_mxf.o -L$(LIBMXF_DIR) -L. -lwritearchivemxf -lMXF $(UUIDLIB) -o $@

test_write_archive_mxf.o: test_write_archive_mxf.c write_archive_mxf.h ../archive_types.h
	$(CC) $(CFLAGS) -c test_write_archive_mxf.c

.PHONY: install
install: libwritearchivemxf.a
	cp libwritearchivemxf.a $(MXF_INSTALL_PREFIX)/lib
	cp write_archive_mxf.h $(MXF_INSTALL_PREFIX)/include

.PHONY: clean
clean:
	@rm -f *~ *.o *.a test_write_archive_mxf

.PHONY: check
check: test_write_archive_mxf
	dd if=/dev/zero bs=500000 count=1 of=input.mxf && ./test_write_archive_mxf 10 input.mxf

.PHONY: valgrind-check
valgrind-check: test_write_archive_mxf
	dd if=/dev/zero bs=500000 count=1 of=input.mxf && valgrind ./test_write_archive_mxf 10 input.mxf
