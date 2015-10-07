#!/bin/sh
HOST='ftp.hotfile.com'
USER=''
PASSWD=''
FILE="wdtv.downloadd.$1.rar"
NAME="test.txt"
WDTVLIVE_FOLDER=/opt/wdtv-live
APP_FOLDER=$WDTVLIVE_FOLDER/app_folder
MOUNT_FOLDER=$WDTVLIVE_FOLDER/mount_folder
OUTPUT_FOLDER=$WDTVLIVE_FOLDER/output
ORIGINAL_FOLDER=$WDTVLIVE_FOLDER/dd-wdtv-live
RAR_FOLDER=$WDTVLIVE_FOLDER/Public
SVN_FOLDER=$WDTVLIVE_FOLDER/svn/DownloadDaemon

cd $RAR_FOLDER

echo "uploading $FILE to hotfile"
ftp -n $HOST <<END_SCRIPT
quote USER $USER
quote PASS $PASSWD
cd downloaddaemon/
put $FILE
quit
END_SCRIPT
#cd /opt/wdtv-live/app_folder/
#sleep 30
#wget -O "$NAME" "api.hotfile.com/?action=getdownloadlinksfrompublicdirectory&username=$USER&password=$PASSWD&folder=1750529&hash=55e5a76"
#line=`cat "$NAME" | grep "$FILE|"`
#REV=${line%|*}
#LINK=${line#*|}
#echo $REV
#echo "Hotfile Link=$LINK"
#google-chrome "http://forum.wdlxtv.com/posting.php?mode=edit&f=40&p=17355" &
exit 0
