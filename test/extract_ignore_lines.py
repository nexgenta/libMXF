#!/usr/bin/python

import sys


if __name__ == '__main__':
	if len(sys.argv) != 2:
		sys.exit("Usage:" + sys.argv[0] + " <diff file>")

	diffFile = open(sys.argv[1], "rb")
	if not diffFile:
		raise "Could not open diff file"

	ignoreLine = None
	ignoreNum = 0
	
	diffLine = diffFile.readline()
	while True:
		if not diffLine:
			break
			
		# extract line with differences (extract xxx[,yyy] from format xxx[,yyy]cxxx[,yyy])
		ignoreLine = diffLine.split('c')[0]
		
		# count the number of lines starting with '<', '>' or '-'
		ignoreCount = 0
		while True:
			diffLine = diffFile.readline()
			
			if not diffLine:
				break
				
			if not (diffLine.startswith('<') or diffLine.startswith('>') or diffLine.startswith('-')):
				break
				
			ignoreCount += 1
			
		print "%s %d" % (ignoreLine, ignoreCount)
		
	diffFile.close()
		
