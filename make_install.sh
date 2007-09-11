#!/bin/sh
# $Id: make_install.sh,v 1.1 2007/09/11 13:24:45 stuart_hc Exp $
#
# Temporary script to do a "make" or "make clean" for all
# libraries and utility programs

if [ "$1" = "clean" ] ; then
	cd lib && make clean
	cd ../examples && make clean
	cd reader && make clean
	exit 0
fi

cd lib && make && make install || exit 1

cd ../examples && make || exit 1
cd reader && make && make install || exit 1
cd ../archive && make || exit 1
cd write && make && make install || exit 1
cd ../info && make && make install || exit 1
echo "###############"
echo "# Utilities"
set -x
cp d3_mxf_info /usr/local/bin
