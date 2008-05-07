#
# $Id: Makefile,v 1.2 2008/05/07 15:22:13 philipn Exp $
#
# Makefile for testing writeaviddv50 example
#
# Copyright (C) 2007  BBC Research, Philip de Nier <philipn@users.sourceforge.net>
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
WRITEAVIDDV50_PATH = ../
LIBMXF_TEST_PATH = ../../../test


.PHONY: check
check: $(LIBMXF_TEST_PATH)/MXFDump/MXFDump $(WRITEAVIDDV50_PATH)/writeaviddv50
	@$(LIBMXF_TEST_PATH)/run_test.sh writeaviddv50 \
		"$(WRITEAVIDDV50_PATH)/writeaviddv50 --test input.dv writeaviddv50.mxf" $(LIBMXF_TEST_PATH)

$(LIBMXF_TEST_PATH)/MXFDump/MXFDump:
	$(MAKE) -C $(LIBMXF_TEST_PATH)/MXFDump
	
.PHONY: valgrind-check
valgrind-check: $(WRITEAVIDDV50_PATH)/writeaviddv50
	@$(LIBMXF_TEST_PATH)/run_test.sh writeaviddv50 \
		"valgrind $(WRITEAVIDDV50_PATH)/writeaviddv50 --test input.dv writeaviddv50.mxf" $(LIBMXF_TEST_PATH)

.PHONY: create
create: $(WRITEAVIDDV50_PATH)/writeaviddv50
	@env LIBMXF_TEST_PATH=$(LIBMXF_TEST_PATH) $(LIBMXF_TEST_PATH)/create_test_set.sh writeaviddv50 \
		"$(WRITEAVIDDV50_PATH)/writeaviddv50 --test input.dv writeaviddv50.mxf" "$(WRITEAVIDDV50_PATH)/writeaviddv50 --test input.dv writeaviddv50.mxf_2"

$(WRITEAVIDDV50_PATH)/writeaviddv50:
	$(MAKE) -C $(WRITEAVIDDV50_PATH) writeaviddv50


.PHONY: clean
clean:
	@rm -f *~
	@rm -f writeaviddv50
	@rm -f *results_std*.txt
