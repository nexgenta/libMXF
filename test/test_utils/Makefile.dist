#
# $Id: Makefile,v 1.1 2008/10/27 18:00:39 john_f Exp $
#
# Makefile for building and running tests
#
# Copyright (C) 2008  BBC Research, Philip de Nier <philipn@users.sourceforge.net>
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
all: test_mxf_page_file


test_mxf_page_file: $(LIBMXF_DIR)/libMXF.a test_mxf_page_file.o
	$(CC) test_mxf_page_file.o -L$(LIBMXF_DIR) -lMXF $(UUIDLIB) -o test_mxf_page_file


.PHONY: clean
clean:
	@rm -f *.o *~ test_mxf_page_file


.PHONY: check
check: all
	./test_mxf_page_file

.PHONY: valgrind-check
valgrind-check: all
	valgrind ./test_mxf_page_file
