#!/bin/bash
#if [ `id -u` -ne 0 ];then
#    echo "ERROR: must be run as root"
#    exit 1
#fi

if [ -z $1 ]; then
   echo "input a picture"
   return 1
fi

DEMO_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

pushd ${DEMO_DIR}/../darknet
rm predictions.png
./darknet detect cfg/bmnet_yolov2.cfg models/darknet/yolov2.weights ${DEMO_DIR}/$1 || exit $?
display predictions.png
popd
