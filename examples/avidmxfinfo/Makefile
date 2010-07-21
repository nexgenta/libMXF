#
# $Id: Makefile,v 1.5 2010/07/21 16:29:33 john_f Exp $
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
all: libavidmxfinfo.a avidmxfinfo


$(LIBMXF_DIR)/libMXF.a:
	$(MAKE) -C $(LIBMXF_DIR)

	
main.o: main.c avid_mxf_info.h
	$(CC) $(CFLAGS) -c main.c

avid_mxf_info.o: avid_mxf_info.c avid_mxf_info.h
	$(CC) $(CFLAGS) -c avid_mxf_info.c

libavidmxfinfo.a: avid_mxf_info.o
	$(AR) libavidmxfinfo.a avid_mxf_info.o

avidmxfinfo: main.o $(LIBMXF_DIR)/libMXF.a libavidmxfinfo.a
	$(CC) main.o -L. -lavidmxfinfo -L$(LIBMXF_DIR) -lMXF $(UUIDLIB) -o $@


.PHONY: install
install: all
	mkdir -p $(MXF_INSTALL_PREFIX)/bin
	cp avidmxfinfo $(MXF_INSTALL_PREFIX)/bin/
	mkdir -p $(MXF_INSTALL_PREFIX)/lib
	cp libavidmxfinfo.a $(MXF_INSTALL_PREFIX)/lib
	mkdir -p $(MXF_INSTALL_PREFIX)/include/mxf/examples/avidmxfinfo
	cp avid_mxf_info.h $(MXF_INSTALL_PREFIX)/include/mxf/examples/avidmxfinfo/

.PHONY: uninstall
uninstall:
	rm -f $(MXF_INSTALL_PREFIX)/bin/avidmxfinfo
	rm -f $(MXF_INSTALL_PREFIX)/lib/libavidmxfinfo.a
	rm -Rf $(MXF_INSTALL_PREFIX)/include/mxf/examples/avidmxfinfo

.PHONY: clean
clean:
	@rm -f *~ *.a *.o avidmxfinfo

.PHONY: check
check: avidmxfinfo
	./test_avidmxfinfo.sh

.PHONY: valgrind-check
valgrind-check: avidmxfinfo
	./test_avidmxfinfo.sh --valgrind
