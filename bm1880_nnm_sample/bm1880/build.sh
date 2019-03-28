#!/bin/bash


if [ ! -d "$TOOLCHAIN_PATH" ] ; then
    read -p "input TOOLCHAIN_PATH:" toolchain_path
    if [ ! -d $toolchain_path ] ; then
        echo "invalid toolchain path"
        exit 1
    fi
    export TOOLCHAIN_PATH=$toolchain_path
fi


if [ ! -d "$BMIVA_PATH" ] ; then
    read -p "input BMIVA_PATH:" bmiva_path
    if [ ! -d $bmiva_path ] ; then
        echo "invalid bmiva path"
        exit 1
    fi
    export BMIVA_PATH=$bmiva_path
fi

if [ -d "./build" ] ; then
    rm -rf "./build"
fi

mkdir -p "./build"
pushd "./build"
cmake -DTOOLCHAIN_PATH=$TOOLCHAIN_PATH -DBMIVA_PATH=$BMIVA_PATH ..
make
popd
