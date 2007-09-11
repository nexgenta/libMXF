#!/bin/sh

# ./check_mxf.sh <mxf file> <baseline dump file> <ignore lines file>

$LIBMXF_TEST_PATH/MXFDump/MXFDump --diff-friendly --show-dark "$1" > dump
diff "$2" dump > diff_dump
$LIBMXF_TEST_PATH/check_diff.py diff_dump "$3"
rm dump
rm diff_dump
