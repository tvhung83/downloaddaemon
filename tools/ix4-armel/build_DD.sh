#!/bin/bash

#DD_SVN_REV_NUMBER="1108"
#DD_SVN_REV_DESC="Rev1108"
#DD_SVN_REV_NUMBER="head"
#DD_SVN_REV_DESC="1.0rc1+"

WDTVLIVE_FOLDER=/opt/storcenter
ORIGINAL_FOLDER=$WDTVLIVE_FOLDER/dd-wdtv-live

export SMP86XX_TOOLCHAIN_PATH=$WDTVLIVE_FOLDER/arm-2008q1


cd `dirname $0`
SOURCE_PATH=`pwd`

# source this script to put your new compiler in the PATH.
# set your WDTV live toolchain path
export SMP86XX_TOOLCHAIN_PATH
export DD_CHECKOUT_PATH="$SOURCE_PATH"
export BINPATH="$SMP86XX_TOOLCHAIN_PATH/bin"
export PATH=$PATH:$BINPATH

export CC=$BINPATH/arm-none-linux-gnueabi-gcc
export CXX=$BINPATH/arm-none-linux-gnueabi-g++
export LD=$BINPATH/arm-none-linux-gnueabi-ld
export RANLIB=$BINPATH/arm-none-linux-gnueabi-ranlib
export COMPILKIND='glibc hardfloat'

echo "DownloadDaemon info"
svn info https://downloaddaemon.svn.sourceforge.net/svnroot/downloaddaemon > $ORIGINAL_FOLDER/version.detail

cat $ORIGINAL_FOLDER/version.detail

# get DD revision number
DD_SVN_REV_NUMBER=`cat $ORIGINAL_FOLDER/version.detail \
	 | grep Revision | awk '{print $2}'`
DD_SVN_REV_DESC="Rev$DD_SVN_REV_NUMBER"
REV=$DD_SVN_REV_NUMBER
echo $DD_SVN_REV_NUMBER > $ORIGINAL_FOLDER/version

echo "Checkout DownloadDaemon Version:$DD_SVN_REV_DESC rev:$DD_SVN_REV_NUMBER"
svn checkout https://downloaddaemon.svn.sourceforge.net/svnroot/downloaddaemon \
	downloaddaemon -r "$DD_SVN_REV_NUMBER"

echo "Create DownloadDaemon log"
svn log downloaddaemon > $ORIGINAL_FOLDER/change.txt

#if [ ! -f ./external/lib/libboost_thread-mt.so.1.40.0 ];then
#	cd ./external/lib/
#	cp libboost_thread.so.1.40.0. libboost_thread-mt.so.1.40.0
#	ln -s libboost_thread-mt.so.1.40.0 libboost_thread-mt.so
#	cd -
#fi

[ ! -d ./output ] && mkdir ./output
sudo rm -rf ./output/*

cd downloaddaemon/trunk
rm -rf CMakeCache.txt CMakeFiles cmake_install.cmake Makefile

cmake -DCMAKE_CXX_FLAGS="-g -Wall" ./src/daemon -DCMAKE_TOOLCHAIN_FILE=../../wdtv-live.cmake 
cmake -DCMAKE_CXX_FLAGS="-g -Wall" ./src/daemon -DCMAKE_TOOLCHAIN_FILE=../../wdtv-live.cmake
DESTDIR=../../output make install

rm -rf CMakeCache.txt CMakeFiles cmake_install.cmake Makefile

cmake ./src/ddconsole -DCMAKE_TOOLCHAIN_FILE=../../wdtv-live.cmake
cmake ./src/ddconsole -DCMAKE_TOOLCHAIN_FILE=../../wdtv-live.cmake
DESTDIR=../../output make install
cd ../../
#./make_app.sh $DD_SVN_REV_DESC
RAR_FOLDER=$WDTVLIVE_FOLDER/Public 
OUTPUT_FOLDER=$WDTVLIVE_FOLDER/output
OPT_FOLDER=$OUTPUT_FOLDER/opt/
SVN_FOLDER=$WDTVLIVE_FOLDER/svn/trunk
cd libs 
cp -r bin $OPT_FOLDER
cp -r etc $OPT_FOLDER
cp -r lib $OPT_FOLDER
cd $OUTPUT_FOLDER 
rm -r etc/
cd $OPT_FOLDER/share
mkdir www
cd $WDTVLIVE_FOLDER/downloaddaemon/trunk/src
sudo cp -r ddclient-php $OPT_FOLDER/share/www/
echo "copying to svn folder"
cd $OPT_FOLDER
#sudo rm -rf $SVN_FOLDER
#mkdir -p $SVN_FOLDER
sudo cp -rf * $SVN_FOLDER
cd $OUTPUT_FOLDER/
rar a ix4.downloadd.$DD_SVN_REV_DESC.rar * 
sudo cp ix4.downloadd.$DD_SVN_REV_DESC.rar $RAR_FOLDER 
$WDTVLIVE_FOLDER/upload_ftp.sh $DD_SVN_REV_DESC
$WDTVLIVE_FOLDER/upload_svn.sh $DD_SVN_REV_DESC
