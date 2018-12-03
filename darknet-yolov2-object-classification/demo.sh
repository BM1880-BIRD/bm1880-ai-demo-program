#!/bin/bash
#if [ `id -u` -ne 0 ];then
#    echo "ERROR: must be run as root"
#    exit 1
#fi

if [ -z $1 ]; then
    echo "param: file directory or file [cpu]"
    echo "file can be photo directory or mp4 , if file type webcam will open webcam."
    echo "cpu for run as cpu"
    echo "ex:"
    echo "./demo.sh images"
    echo "./demo.sh images cpu"
    echo "./demo.sh demo1.mp4"
    echo "./demo.sh webcam"
    echo "./demo.sh demo1.mp4 cpu"
    exit 1
fi

DEMO_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

pushd ${DEMO_DIR}/darknet
if [ "$2" == "cpu" ]; then
    if [[ $1 == *"mp4"* ]]; then
        ./darknet detector demo cfg/coco.data cfg/yolov2.cfg models/darknet/yolov2.weights ${DEMO_DIR}/$1 || exit $?
    elif [ "$1" == "webcam" ]; then
        ./darknet detector demo cfg/coco.data cfg/yolov2.cfg models/darknet/yolov2.weights || exit $?
    else
        ./darknet detect cfg/yolov2.cfg models/darknet/yolov2.weights ${DEMO_DIR}/$1 || exit $?
    fi
else
    if [[ $1 == *"mp4"* ]]; then
        ./darknet detector demo cfg/coco.data cfg/bmnet_yolov2.cfg models/darknet/yolov2.weights ${DEMO_DIR}/$1 || exit $?
    elif [ "$1" == "webcam" ]; then
         ./darknet detector demo cfg/coco.data cfg/bmnet_yolov2.cfg models/darknet/yolov2.weights || exit $?
    else
#       ./darknet detect cfg/bmnet_yolov2.cfg models/darknet/yolov2.weights ${DEMO_DIR}/$1 || exit $?
	./darknet detect cfg/bmnet_yolov2.cfg models/darknet/yolov2.weights ${DEMO_DIR}/$1 || exit $?
    fi
fi
popd
