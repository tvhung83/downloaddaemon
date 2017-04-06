#!/bin/bash
WDTVLIVE_FOLDER=/opt/wdtv-live
ORIGINAL_FOLDER=$WDTVLIVE_FOLDER/dd-wdtv-live
sudo rm $ORIGINAL_FOLDER/application.xml
echo -e "<?xml version='1.0'?>\n \
	<application>\n \
	\t<name>DownloadDaemon</name>\n \
	\t<desc>DownloadDaemon is a comfortable download-manager with many features like one-click-hoster support, etc. It can be remote-controled in several ways (web/gui/console clients), which makes it perfect for file- and root-servers, as well as for local use.</desc>\n \
    	\t<author>boudcallens</author>\n \
    	\t<date>$(date +%Y-%m-%d)</date>\n \
    	\t<version>$1</version>\n \
    	\t<category>Download</category>\n \
    	\t<url>http://forum.wdlxtv.com/viewtopic.php?f=40&amp;t=2331</url>\n \
    	\t<size>20971520</size>\n \
    	\t<format>ext3</format>\n \
    	\t<provides>\n \
        	\t\t<binary>true</binary>\n \
        	\t\t<daemon>true</daemon>\n \
        	\t\t<kernelmodule>false</kernelmodule>\n \
        	\t\t<webend>true</webend>\n \
   	\t</provides>\n \
    	\t<dependencies>\n \
        	\t\t<model>LIVE</model>\n \
		\t\t<model>PLUS</model>\n \
        	\t\t<basefirmware>\n \
            		\t\t\t<min>1.01.00</min>\n \
        	\t\t</basefirmware>\n \
        	\t\t<firmware>\n \
            		\t\t\t<min>0.4.2.0</min>\n \
            		\t\t\t<max></max>\n \
        	\t\t</firmware>\n \
        	\t\t<wdtvext>false</wdtvext>\n \
        	\t\t<network>true</network>\n \
        	\t\t<config>APACHE=ON</config>\n \
        	\t\t<app></app>\n \
    	\t</dependencies>\n \
    	\t<id>downloadd</id>\n \
    	\t<download>downloadd.app.bin</download>\n \
</application>">>$ORIGINAL_FOLDER/application.xml
