#!/bin/sh


BASH_DIR=$(cd `dirname $0`; pwd) 
BUILD_DIR=$BASH_DIR/build

if [ $# -lt 1 ] ; then 
    if [ ! -d "$BUILD_DIR" ]; then
        mkdir $BUILD_DIR
        cmake -G "Ninja" -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -S . -B $BUILD_DIR
    fi
elif [ $1 -eq "-r" ] then
    rm -rf $BUILD_DIR/*
    cmake -G "Ninja" -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -S . -B $BUILD_DIR
fi

cd $BUILD_DIR
ninja
