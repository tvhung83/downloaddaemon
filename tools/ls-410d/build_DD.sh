#!/bin/bash

LS410D_FOLDER=/home/robert/ls-410d
ORIGINAL_FOLDER=$LS410D_FOLDER/dd-wdtv-live

export DSM52_TOOLCHAIN_PATH="$LS410D_FOLDER/tools/arm-marvell-linux-gnueabi"


cd `dirname $0`
SOURCE_PATH=`pwd`

# source this script to put your new compiler in the PATH.
# set your WDTV live toolchain path
export DSM52_TOOLCHAIN_PATH
export DD_CHECKOUT_PATH="$SOURCE_PATH"
export BINPATH="$DSM52_TOOLCHAIN_PATH/bin"
export PATH=$PATH:$BINPATH

export CC=$BINPATH/arm-linux-gnueabihf-gcc
export CXX=$BINPATH/arm-linux-gnueabihf-g++
export LD=$BINPATH/arm-linux-gnueabihf-ld
export RANLIB=$BINPATH/arm-linux-gnueabihf-ranlib
export COMPILKIND='glibc hardfloat'

echo "updating time"
sudo ntpdate be.pool.ntp.org

echo "DownloadDaemon info"
svn info https://downloaddaemon.svn.sourceforge.net/svnroot/downloaddaemon > $ORIGINAL_FOLDER/version.detail

cat $ORIGINAL_FOLDER/version.detail

# get DD revision number
DD_SVN_REV_NUMBER=`cat $ORIGINAL_FOLDER/version.detail \
	 | grep Revision | awk '{print $2}'`
DD_SVN_REV_DESC="Rev$DD_SVN_REV_NUMBER"

echo "Checking if local version is up to date"
if [ "`cat $ORIGINAL_FOLDER/version`" == "$DD_SVN_REV_NUMBER" ] && [ "$1" != "-f" ] ; then
echo "Local version and build are up to date"
exit 0
fi

echo $DD_SVN_REV_NUMBER > $ORIGINAL_FOLDER/version
echo "Creating Application Xml"
./make_xml.sh $DD_SVN_REV_NUMBER

echo "Checkout DownloadDaemon Version:$DD_SVN_REV_DESC rev:$DD_SVN_REV_NUMBER"
svn checkout https://downloaddaemon.svn.sourceforge.net/svnroot/downloaddaemon \
	downloaddaemon -r "$DD_SVN_REV_NUMBER"

echo "Create DownloadDaemon Log"
svn log downloaddaemon > $ORIGINAL_FOLDER/change.txt

if [ ! -f ./external/usr/lib/libboost_thread-mt.so.1.40.0 ];then
	cd ./external/usr/lib/
	cp libboost_thread.so.1.40.0 libboost_thread-mt.so.1.40.0
	ln -s libboost_thread-mt.so.1.40.0 libboost_thread-mt.so
	cd -
fi

[ ! -d ./output ] && mkdir ./output
rm -rf ./output/*

cd downloaddaemon/trunk/src/daemon
rm -rf CMakeCache.txt CMakeFiles cmake_install.cmake Makefile
cd ../../src/ddconsole
rm -rf CMakeCache.txt CMakeFiles cmake_install.cmake Makefile
cd ../../
rm -rf CMakeCache.txt CMakeFiles cmake_install.cmake Makefile

cmake -DCMAKE_CXX_FLAGS="-g -Wall" ./src/daemon -DCMAKE_TOOLCHAIN_FILE=../../ls-410d.cmake 
DESTDIR=../../output make install

rm -rf CMakeCache.txt CMakeFiles cmake_install.cmake Makefile

cmake ./src/ddconsole -DCMAKE_TOOLCHAIN_FILE=../../ls-410d.cmake
cmake ./src/ddconsole -DCMAKE_TOOLCHAIN_FILE=../../ls-410d.cmake
DESTDIR=../../output make install
cd ../../
./make_app.sh $DD_SVN_REV_DESC
