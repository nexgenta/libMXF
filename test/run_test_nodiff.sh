#!/bin/sh

# ./run_test.sh <name> <command>
# the MXF file output from command must be ${1}.mxf

echo "Testing ${1}..."
result=1

if ${2} 1> ${1}_results_stdout.txt 2> ${1}_results_stderr.txt
then
	rm -f ${1}_results_stdout.txt ${1}_results_stderr.txt
	echo "done"
	result=0
else
	echo "*** ERROR - Failed to execute test - see ${1}_results_stderr.txt and ${1}_results_stdout.txt" 
fi
rm -f ${1}.mxf

exit $result
