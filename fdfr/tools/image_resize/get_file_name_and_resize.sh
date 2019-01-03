#!/bin/bash

#echo $0 $1

#echo $#

if [ $# != 2 ]
then
	echo "wrong input"
	exit -1
fi

file_folder=`readlink -f ${1}`
resize_folder=`readlink -f ${2}`

TOP=`pwd`
mkdir -p $TOP/temp

function getdir(){
	#for element in `ls -Q $1`
	mkdir -p $TOP/temp$1
	touch $TOP/temp$1/files.txt
	ls $1 > $TOP/temp$1/files.txt

	cat $TOP/temp$1/files.txt | while read element
	do
		#echo $element
		#echo ${element// /\\ }
		#element_1=`echo ${element// /\\ }`
		#echo element_1 : ${element_1}
		dir_or_file=$1"/""${element// /\\ }"
		#echo ${dir_or_file}
		if [ -d "$dir_or_file" ]
		then
			getdir $dir_or_file
		else
			echo $dir_or_file
			./img_resize  $dir_or_file  $resize_folder
		fi
	done
}



getdir $file_folder
rm $TOP/temp -fr 
