#!/bin/sh
#
# $Id: test_avidmxfinfo.sh,v 1.1 2008/10/08 09:38:51 philipn Exp $
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

for input in ../writeavidmxf/*.mxf
do
	command="$VALGRIND_CMD ./avidmxfinfo $input"
	echo $command
	$command > /dev/null || exit 1
done
