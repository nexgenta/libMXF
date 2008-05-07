#!/bin/sh

# ./run_test.sh <name> <command 1> <command 2>
# the MXF file output from command 1 must be ${1}.mxf
# the MXF file output from command 2 must be ${1}.mxf_2


# run command 1 and command 2
echo "Creating ${1} test set..."
${2}
sleep 2s
${3}

# create dumps
$LIBMXF_TEST_PATH/MXFDump/MXFDump --diff-friendly --show-dark "${1}.mxf" > "${1}.dump"
$LIBMXF_TEST_PATH/MXFDump/MXFDump --diff-friendly --show-dark "${1}.mxf_2" > "${1}.dump_2"

# diff dumps and extract lines that can be ignored
diff "${1}.dump" "${1}.dump_2" > diff_dump
python $LIBMXF_TEST_PATH/extract_ignore_lines.py diff_dump > "${1}.ignore"

# cleanup
rm -f "${1}.dump_2" diff_dump
rm -f ${1}.mxf ${1}.mxf_2

