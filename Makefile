#
# $Id: Makefile,v 1.3 2010/07/21 16:29:33 john_f Exp $
#
# Makefile for building libMXf library, tools and examples
#
# Copyright (C) 2008  BBC Research, Stuart Cunningham <stuart_hc@users.sourceforge.net>
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
	$(MAKE) -C lib
	$(MAKE) -C test
	$(MAKE) -C examples
	$(MAKE) -C tools

# The only installable files are in lib/ and examples/
.PHONY: install
install: all
	$(MAKE) -C lib $@
	$(MAKE) -C examples $@

.PHONY: uninstall
uninstall:
	$(MAKE) -C lib $@
	$(MAKE) -C examples $@

.PHONY: clean
clean:
	$(MAKE) -C lib $@
	$(MAKE) -C test $@
	$(MAKE) -C examples $@
	$(MAKE) -C tools $@

.PHONY: check
check: all
	$(MAKE) -C test $@
	$(MAKE) -C examples $@

.PHONY: valgrind-check
valgrind-check: all
	$(MAKE) -C examples $@

