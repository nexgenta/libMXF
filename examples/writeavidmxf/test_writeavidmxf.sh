#!/bin/sh
#
# $Id: test_writeavidmxf.sh,v 1.5 2010/10/18 17:54:08 john_f Exp $
#
# Simple test script
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

VALGRIND_CMD=""
if [ "$1" = "--valgrind" ] ; then
	VALGRIND_CMD="valgrind --leak-check=full"
fi

# create sample essence as input file (size of largest input for unc1080i)
dd if=/dev/zero bs=4147200 count=1 of=essence.dat 2>/dev/null

# TODO: create DV test files
for format in IMX30 IMX40 IMX50 \
	DNxHD720p120 DNxHD720p185 \
	DNxHD1080i120 DNxHD1080i185 DNxHD1080i185X \
	DNxHD1080p36 DNxHD1080p120 DNxHD1080p185 DNxHD1080p185X \
	unc unc1080i unc720p
do
	command="$VALGRIND_CMD ./writeavidmxf --prefix test_$format --$format essence.dat --pcm essence.dat"
	echo $command
	$command || exit 1
done

for format in \
	DNxHD1080p36 DNxHD1080p115 DNxHD1080p175 DNxHD1080p175X
do
	command="$VALGRIND_CMD ./writeavidmxf --prefix test_${format}_23.976 --film23.976 --$format essence.dat --pcm essence.dat"
	echo $command
	$command || exit 1
done

rm -f essence.dat
