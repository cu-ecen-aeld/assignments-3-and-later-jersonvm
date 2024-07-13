#!/bin/sh

writefile=$1
writestr=$2

if [ $# -ne 2 ]
then
	echo "Incorrect number of arguments"
	exit 1
fi

filename=${writefile##*/}
directory=${writefile%%/$filename}

echo "DEBUG writer.sh: writefile = $writefile"
echo "DEBUG writer.sh: writestr = $writestr"
echo "DEBUG writer.sh: filename = $filename"
echo "DEBUG writer.sh: directory = $directory"


if [ -d ${directory} ]
then
	echo "MESSAGE writer.sh: Location $directory already exists" 
else
	mkdir -p $directory
	
	if [ -d ${directory} ]
	then
		echo "MESSAGE writer.sh: Location $directory was successfully created"
	else
		echo "ERROR writer.sh: Location $directory could not be created"
		echo "ERROR writer.sh: File could not be created"
		exit 1
	fi
fi

cd $directory
touch $filename

if 	[ -e ${writefile} ]
then
	echo "MESSAGE writer.sh: $filename was successfully created inside location $directory"
	echo "$writestr" > "$writefile"
else
	echo "ERROR writer.sh: File could not be created"
	exit 1
fi

exit 0

