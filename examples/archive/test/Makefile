#
# $Id: Makefile,v 1.3 2010/07/21 16:29:33 john_f Exp $
#
# Makefile for building archive test applications
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
all: compare_d3_mxf double_clapperboard


compare_d3_mxf: compare_d3_mxf.o
	$(CC) compare_d3_mxf.o -o $@

compare_d3_mxf.o: compare_d3_mxf.c
	$(CC) $(CFLAGS) -c compare_d3_mxf.c


double_clapperboard: avsync_eval.o double_clapperboard.o
	$(CC) double_clapperboard.o avsync_eval.o -lm -o $@

double_clapperboard.o: double_clapperboard.c avsync_eval.h
	$(CC) $(CFLAGS) -c double_clapperboard.c

avsync_eval.o: avsync_eval.c avsync_eval.h
	$(CC) $(CFLAGS) -c avsync_eval.c

.PHONY: clean
clean:
	@rm -f *~ *.o compare_d3_mxf double_clapperboard
