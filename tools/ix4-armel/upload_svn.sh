#!/bin/sh

WDTVLIVE_FOLDER=/opt/storcenter
ORIGINAL_FOLDER=$WDTVLIVE_FOLDER/dd-wdtv-live
RAR_FOLDER=$WDTVLIVE_FOLDER/Public 
OUTPUT_FOLDER=$WDTVLIVE_FOLDER/output
OPT_FOLDER=$OUTPUT_FOLDER/opt/
SVN_FOLDER=$WDTVLIVE_FOLDER/svn/trunk

echo "uploading to SVN"
sudo chown -R boudcallens:boudcallens $SVN_FOLDER
cd $SVN_FOLDER
svn add --force ./*
svn commit --username boudcallens --password "" -m "Compiled version $1"
