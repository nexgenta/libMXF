#
# $Id: Makefile,v 1.1 2008/05/07 15:21:48 philipn Exp $
#
# Makefile for building tools directory
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
	$(MAKE) -C avid_metadict_blob

.PHONY: clean
clean:
	$(MAKE) -C avid_metadict_blob $@

.PHONY: check
check: all
	$(MAKE) -C avid_metadict_blob $@

.PHONY: valgrind-check
valgrind-check: all
	$(MAKE) -C avid_metadict_blob $@
