#!/bin/sh

WDTVLIVE_FOLDER=/opt/wdtv-live
APP_FOLDER=$WDTVLIVE_FOLDER/app_folder
MOUNT_FOLDER=$WDTVLIVE_FOLDER/mount_folder
OUTPUT_FOLDER=$WDTVLIVE_FOLDER/output
ORIGINAL_FOLDER=$WDTVLIVE_FOLDER/dd-wdtv-live
RAR_FOLDER=$WDTVLIVE_FOLDER/Public
SVN_FOLDER=$WDTVLIVE_FOLDER/svn/DownloadDaemon

echo "uploading to SVN"
#sudo chown -R boudcallens\: $SVN_FOLDER
cd $SVN_FOLDER
svn add --force ./*
svn commit --username boudcaliens --password "" -m "Compiled version $1"
