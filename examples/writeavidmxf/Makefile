#
# $Id: Makefile,v 1.5 2010/07/14 13:06:36 john_f Exp $
#
# Makefile for building writeavidmxf example
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
all: writeavidmxf


$(LIBMXF_DIR)/libMXF.a:
	$(MAKE) -C $(LIBMXF_DIR)

main.o: main.c write_avid_mxf.h
	$(CC) $(CFLAGS) -c main.c

write_avid_mxf.o: write_avid_mxf.c write_avid_mxf.h
	$(CC) $(CFLAGS) -c write_avid_mxf.c

package_definitions.o: package_definitions.c package_definitions.h
	$(CC) $(CFLAGS) -c package_definitions.c

libwriteavidmxf.a: write_avid_mxf.o package_definitions.o
	$(AR) libwriteavidmxf.a write_avid_mxf.o package_definitions.o

writeavidmxf: main.o $(LIBMXF_DIR)/libMXF.a libwriteavidmxf.a
	$(CC) main.o -L$(LIBMXF_DIR) -L. -lwriteavidmxf -lMXF $(UUIDLIB) -o $@


.PHONY: install
install: all
	mkdir -p $(MXF_INSTALL_PREFIX)/bin
	cp writeavidmxf $(MXF_INSTALL_PREFIX)/bin/
	mkdir -p $(MXF_INSTALL_PREFIX)/lib
	cp libwriteavidmxf.a $(MXF_INSTALL_PREFIX)/lib
	mkdir -p $(MXF_INSTALL_PREFIX)/include/mxf/examples/writeavidmxf
	cp *.h $(MXF_INSTALL_PREFIX)/include/mxf/examples/writeavidmxf/

.PHONY: clean
clean:
	@rm -f *~ *.o *.a writeavidmxf test_*.mxf

.PHONY: check
check: writeavidmxf
	./test_writeavidmxf.sh

.PHONY: valgrind-check
valgrind-check: writeavidmxf
	./test_writeavidmxf.sh --valgrind
