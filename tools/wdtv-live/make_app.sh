#!/bin/bash

rev=$1
WDTVLIVE_FOLDER=/opt/wdtv-live
APP_FOLDER=$WDTVLIVE_FOLDER/app_folder
MOUNT_FOLDER=$WDTVLIVE_FOLDER/mount_folder
OUTPUT_FOLDER=$WDTVLIVE_FOLDER/output
ORIGINAL_FOLDER=$WDTVLIVE_FOLDER/dd-wdtv-live
RAR_FOLDER=$WDTVLIVE_FOLDER/Public
SVN_FOLDER=$WDTVLIVE_FOLDER/svn/DownloadDaemon

echo "creating app_folder"
[ ! -f $APP_FOLDER ] && sudo rm -r $APP_FOLDER/
mkdir $APP_FOLDER
cd $APP_FOLDER
echo "creating app.bin"
dd if=/dev/zero of=./downloadd.app.bin bs=1M count=20
mkfs.ext3 -F downloadd.app.bin>1.log
tune2fs -c 0 -i 0 downloadd.app.bin>2.log
mkdir -p $MOUNT_FOLDER
echo "mounting"
sudo mount -o loop downloadd.app.bin $MOUNT_FOLDER/
echo "copying content"
cp -r $OUTPUT_FOLDER/* $MOUNT_FOLDER/
cd $MOUNT_FOLDER/apps/downloadd/
cp -r etc ../../
cd ../../
rm -r apps/
cd etc/init.d/
rm downloadd
cd $ORIGINAL_FOLDER
mkdir -p downloadd/
sudo mount -o loop downloadd.app.bin downloadd/
cd downloadd/
sudo cp -r bin $MOUNT_FOLDER/
sudo cp -r lib $MOUNT_FOLDER/
sudo cp -r var $MOUNT_FOLDER/
cd etc/init.d
sudo cp -r S97downloadd $MOUNT_FOLDER/etc/init.d/
cd ..
#sudo echo $rev>$MOUNT_FOLDER/etc/version.txt
#sudo echo $rev>$MOUNT_FOLDER/etc/version.detail
#sudo cp version.* $MOUNT_FOLDER/etc/
cd downloaddaemon
sudo cp dlist $MOUNT_FOLDER/etc/downloaddaemon/
cd $ORIGINAL_FOLDER/
sudo cp version* application.xml README $MOUNT_FOLDER
sudo cp version* change.txt LICENCE $APP_FOLDER/
cd $WDTVLIVE_FOLDER/downloaddaemon/trunk/src
sudo cp -r ddclient-php $MOUNT_FOLDER/var/www/
echo "copying to svn folder"
cd $MOUNT_FOLDER
#sudo rm -rf $SVN_FOLDER
#mkdir -p $SVN_FOLDER
sudo chown -R boudcallens\: $MOUNT_FOLDER
sudo cp -rf * $SVN_FOLDER
echo "cleaning up"
cd $ORIGINAL_FOLDER/
sudo umount downloadd/
rm -r downloadd/
cd $APP_FOLDER
sudo umount $MOUNT_FOLDER/
sudo rm -r $MOUNT_FOLDER/
rm 1.log
rm 2.log
echo "copying to location"
sudo cp downloadd.app.bin $RAR_FOLDER
rar a wdtv.downloadd.$rev.rar downloadd.app.bin version* change.txt LICENCE
sudo cp wdtv.downloadd.$rev.rar $RAR_FOLDER
$WDTVLIVE_FOLDER/upload_ftp.sh $rev
$WDTVLIVE_FOLDER/upload_svn.sh $rev
