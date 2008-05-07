#!/bin/sh

# ./check_mxf.sh <mxf file> <baseline dump file> <ignore lines file>

$LIBMXF_TEST_PATH/MXFDump/MXFDump --diff-friendly --show-dark "$1" > dump || exit 1

diff_options=""
if [ "$OS" = "Windows_NT" ] ; then
    # Prevent spurious differences due to CRLF line-endings on Windows
    diff_options="-w"
fi
diff $diff_options -w "$2" dump > diff_dump
python $LIBMXF_TEST_PATH/check_diff.py diff_dump "$3" || exit 1
rm dump
rm diff_dump
