#!/bin/bash
echo "build client start"

BIN_FOLDER=bin
if [ ! -d $BIN_FOLDER ]; then
    mkdir $BIN_FOLDER
#else
#    rm -rf $BIN_FOLDER/*
fi

BUILD_FOLDER=build
if [ ! -d $BUILD_FOLDER ]; then
    mkdir $BUILD_FOLDER
#else
#    rm -rf $BUILD_FOLDER/*
fi

COMPILE_OPTION="-DCMAKE_BUILD_TYPE:STRING=Release"

cd $BUILD_FOLDER

cmake $COMPILE_OPTION ..

make -j8

make install

cd ..

cp -r conf/*  bin/
cp -r lib/*.so bin/

echo "build client end"
