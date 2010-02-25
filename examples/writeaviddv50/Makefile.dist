#
# $Id: Makefile,v 1.2 2008/05/07 15:22:13 philipn Exp $
#
# Makefile for building writeaviddv50 example
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
all: writeaviddv50

$(LIBMXF_DIR)/libMXF.a:
	$(MAKE) -C $(LIBMXF_DIR)

writeaviddv50: $(LIBMXF_DIR)/libMXF.a writeaviddv50.o
	$(CC) writeaviddv50.o -L$(LIBMXF_DIR) -lMXF $(UUIDLIB) -o writeaviddv50

writeaviddv50.o: writeaviddv50.c
	$(CC) $(CFLAGS) -c $(INCLUDES) writeaviddv50.c


.PHONY: clean
clean:
	@rm -f *~ *.o writeaviddv50
	$(MAKE) -C test $@

.PHONY: check
check: writeaviddv50
	$(MAKE) -C test $@

.PHONY: valgrind-check
valgrind-check: writeaviddv50
	$(MAKE) -C test $@
