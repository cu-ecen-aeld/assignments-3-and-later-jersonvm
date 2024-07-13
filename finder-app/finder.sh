#!/bin/sh

filesdir=$1
searchstr=$2

echo "DEBUG finder.sh: filesdir = $filesdir"
echo "DEBUG finder.sh: searchstr = $searchstr"
	
if [ $# -ne 2 ]
then
	echo "ERROR finder.sh: Incorrect number of arguments"
	exit 1
fi

if 	[ -d ${filesdir} ]
then
	echo "MESSAGE finder.sh: Directory exists" 
	echo "MESSAGE finder.sh: Directory = ${filesdir}"
else
	echo "ERROR finder.sh: ${filesdir} is an invalid directory"
	exit 1
fi

cd ${filesdir}
X=$(find . -type f | wc -l)
Y=$(grep -r $searchstr * | wc -l)

echo "MESSAGE finder.sh:The number of files are ${X} and the number of matching lines are ${Y}"
exit 0
