#!/usr/bin/python

import sys

# file position in the ignore lines file
ignoreLineFilePos = 0


# go through the ignore lines, starting from ignoreLineFilePos, until we have
# a match or eof
# if there is a match, then ignore ignoreNum lines in the diff file
def check_diff_line(diffFile, ignoreLinesFile, diffLine):
	global ignoreLineFilePos
	
	ignoreLinesFile.seek(ignoreLineFilePos)

	while True:	
		line = ignoreLinesFile.readline().strip()
		if not line:
			break
			
		ignoreLine = line.split(' ')[0]
		ignoreNum = int(line.split(' ')[1])
		ignoreLineStr = "%sc%s" % (ignoreLine, ignoreLine)
		
		if diffLine.strip() == ignoreLineStr:
			# update ignore lines file pos
			ignoreLineFilePos = ignoreLinesFile.tell()
			
			# ignore ignoreNum lines in the diff file
			while ignoreNum > 0:
				line = diffFile.readline()
				if not line:
					break
				ignoreNum -= 1
				
			return True
		
	return False
	

if __name__ == '__main__':
	if len(sys.argv) != 3:
		sys.exit("Usage:" + sys.argv[0] + " <diff file> <ignore lines file>")

	diffFile = open(sys.argv[1], "rb")
	if not diffFile:
		raise "Could not open diff file"

	ignoreLinesFile = open(sys.argv[2], "rb")
	if not ignoreLinesFile:
		raise "Could not open ignore lines file"

		
	while True:
		diffLine = diffFile.readline()
		if not diffLine:
			break
		
		if not (diffLine.startswith('<') or diffLine.startswith('>') or diffLine.startswith('-')):
			# check if diff can be ignored, and continue if yes
			if check_diff_line(diffFile, ignoreLinesFile, diffLine):
				continue
		print diffLine,
		
	ignoreLinesFile.close()
	diffFile.close()
		

