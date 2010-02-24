#
# $Id: Makefile,v 1.3 2008/10/22 09:32:19 john_f Exp $
#
# Makefile for building and running tests
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
	$(MAKE) -C MXFDump
	$(MAKE) -C test_lib
	$(MAKE) -C test_utils

.PHONY: check
check:
	$(MAKE) -C MXFDump
	$(MAKE) -C test_lib $@
	$(MAKE) -C test_utils $@

.PHONY: clean
clean:
	@rm -f *~
	$(MAKE) -C MXFDump $@
	$(MAKE) -C test_lib $@
	$(MAKE) -C test_utils $@

