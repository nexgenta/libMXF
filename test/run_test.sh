#!/bin/sh

# ./run_test.sh <name> <command>

echo "Testing ${1}..."
if ${2} 1> ${1}_results_stdout.txt 2> ${1}_results_stderr.txt
then
	env LIBMXF_TEST_PATH=${3} ${3}/check_mxf.sh ${1}.mxf ${1}.dump ${1}.ignore >> ${1}_results_stdout.txt
	echo "done"
	if [[ `wc -l ${1}_results_stdout.txt | awk '{print $1}'` != 0 ]] 
	then 
		echo "*** ERROR - There are differences - see ${1}_results_stdout.txt"
	else
		rm -f ${1}_results_stdout.txt ${1}_results_stderr.txt
	fi
else
	echo "*** ERROR - Failed to execute test - see ${1}_results_stderr.txt and ${1}_results_stdout.txt" 
fi
rm -f ${1}.mxf

