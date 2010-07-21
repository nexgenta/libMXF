#
# $Id: Makefile,v 1.4 2010/07/21 16:29:33 john_f Exp $
#
# Makefile for building examples directory
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

.PHONY: all
all:
	$(MAKE) -C writeavidmxf $@
	$(MAKE) -C writeaviddv50 $@
	$(MAKE) -C transfertop2 $@
	$(MAKE) -C archive $@
	$(MAKE) -C reader $@
	$(MAKE) -C avidmxfinfo $@

.PHONY: install
install: all
	$(MAKE) -C writeavidmxf $@
	$(MAKE) -C archive $@
	$(MAKE) -C reader $@
	$(MAKE) -C avidmxfinfo $@

.PHONY: uninstall
uninstall:
	$(MAKE) -C writeavidmxf $@
	$(MAKE) -C archive $@
	$(MAKE) -C reader $@
	$(MAKE) -C avidmxfinfo $@

.PHONY: clean
clean:
	@rm -f *~
	$(MAKE) -C writeavidmxf $@
	$(MAKE) -C writeaviddv50 $@
	$(MAKE) -C transfertop2 $@
	$(MAKE) -C archive $@
	$(MAKE) -C reader $@
	$(MAKE) -C avidmxfinfo $@

.PHONY: check
check: all
	$(MAKE) -C writeavidmxf $@
	$(MAKE) -C writeaviddv50 $@
	$(MAKE) -C archive $@
	$(MAKE) -C reader $@
	$(MAKE) -C avidmxfinfo $@

.PHONY: valgrind-check
valgrind-check: all
	$(MAKE) -C writeavidmxf $@
	$(MAKE) -C writeaviddv50 $@
	$(MAKE) -C archive $@
	$(MAKE) -C reader $@
	$(MAKE) -C avidmxfinfo $@
