#!/bin/bash

cd `dirname $0`
SOURCE_PATH=`pwd`

# source this script to put your new compiler in the PATH.
export DD_CHECKOUT_PATH="$SOURCE_PATH"

[ ! -d ./output ] && mkdir ./output
rm -rf ./output/*

cd src/daemon
rm -rf CMakeCache.txt CMakeFiles cmake_install.cmake Makefile
cd ../../src/ddconsole
rm -rf CMakeCache.txt CMakeFiles cmake_install.cmake Makefile
cd ../../
rm -rf CMakeCache.txt CMakeFiles cmake_install.cmake Makefile

cmake -DCMAKE_CXX_FLAGS="-g -Wall" ./src/daemon -DCMAKE_TOOLCHAIN_FILE=./pc.cmake 
DESTDIR=./output make install

rm -rf CMakeCache.txt CMakeFiles cmake_install.cmake Makefile

cmake ./src/ddconsole -DCMAKE_TOOLCHAIN_FILE=./pc.cmake
DESTDIR=./output make install
