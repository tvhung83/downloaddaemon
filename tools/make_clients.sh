#!/bin/bash

# debian target distributions
DEB_DISTS=("lucid" "precise" "quantal" "raring")

# upstream authors "Name LastName <mail@foo.foo>", seperated by newline and 4 spaces
UPSTREAM_AUTHORS="Adrian Batzill <downloaddaemon # batzill ! com>
    Susanne Eichel <susanne.eichel # web ! de>"

# copyright holders "Copyright (C) {Year(s)} by {Author(s)} {Email address(es)}", seperated by newline and 4 spaces
COPYRIGHT_HOLDERS="Copyright (C) 2009, 2010 by Adrian Batzill <adrian # batzill ! com>
    Copyright (C) 2009, 2010 by Susanne Eichel <susanne.eichel # web ! de>"

# build dependencies
BUILDDEP_DDCONSOLE="debhelper (>= 7), cmake"
BUILDDEP_DDCLIENTGUI="debhelper (>= 7), cmake, libqt4-dev, qt4-dev-tools"

# dependencies (leave empty for auto-detection)
DEP_DDCONSOLE=""
DEP_DDCLIENTGUI=""

# synopsis (up to 60 characters)
SYN_DDCONSOLE="A DownloadDaemon console client"
SYN_DDCLIENTGUI="A graphical DownloadDaemon client"

# description (if you want a new line, you need to put a space right behind it)
DESC_DDCONSOLE="with ddclient you can easily manage your
 DownloadDaemon server - even without X"
DESC_DDCLIENTGUI="With ddclient-gui you can manage your DownloadDaemon
 server in a comfortable, easy way."

# specify all files/directorys (array) and the path's where they should go to (basically a cp -r FILES_XXX[i] PATHS_XXX[i] is done)
# the .svn folders are removed automatically. Folders are created automatically before copying
FILES_DDCONSOLE=("../src/ddconsole" "../src/include/netpptk" "../src/include/crypt/md5.*" "../src/include/config.h" "../AUTHORS" "../CHANGES" "../TODO" "../LICENCE" "../INSTALLING")
PATHS_DDCONSOLE=("src/" "src/include/" "src/include/crypt/" "src/include/")

FILES_DDCLIENTPHP=("../src/ddclient-php" "../AUTHORS" "../CHANGES" "../TODO" "../LICENCE" "../INSTALLING")
PATHS_DDCLIENTPHP=("")

FILES_DDCLIENTGUI=("../src/ddclient-gui" "../src/CheckCxx11.cmake" "../src/include/cfgfile" "../src/include/netpptk" "../src/include/crypt/md5.*" "../src/include/language" "../src/include/downloadc" "../share/applications/ddclient-gui.desktop" "../share/ddclient-gui" "../share/pixmaps/ddclient-gui*" "../AUTHORS" "../CHANGES" "../TODO" "../LICENCE" "../INSTALLING")
PATHS_DDCLIENTGUI=("src/" "src/" "src/include/" "src/include" "src/include/crypt/" "src/include/" "src/include/" "share/applications" "share/" "share/pixmaps")



if [ "$1" == "" ]
then
	echo "No version number specified"
	echo "usage: $0 version email [ubuntu revision] [binary|source|nightly]"
	exit
fi

if [ "$2" == "" ]
then
	echo "No email address specified"
	echo "usage: $0 version email [ubuntu revision] [binary|source|nightly]"	
	exit
fi
UBUNTU_REV=$3
if [ "$3" == "" ]
then
	echo "No ubuntu revision specified, assuming 1"
	UBUNTU_REV="1"
fi
BINARY=true
if [ "$4" != "binary" ]
then
	BINARY=false
fi

VERSION=$1
EMAIL=$2

name_suffix=""
opposite_suffix="-nightly" # needed to create the "Conflicts:" field
if [ "$4" == "nightly" ]; then
	name_suffix="-nightly"
	opposite_suffix=""
fi

mkdir -p ../version/${VERSION}

for path in $(seq 0 $((${#FILES_DDCONSOLE[@]} - 1))); do
	mkdir -p ../version/${VERSION}/ddconsole${name_suffix}-${VERSION}/${PATHS_DDCONSOLE[${path}]}
	cp -r ${FILES_DDCONSOLE[${path}]} ../version/${VERSION}/ddconsole${name_suffix}-${VERSION}/${PATHS_DDCONSOLE[${path}]}
done

for path in $(seq 0 $((${#FILES_DDCLIENTPHP[@]} - 1))); do
	mkdir -p ../version/${VERSION}/ddclient-php${name_suffix}-${VERSION}/${PATHS_DDCLIENTPHP[${path}]}
	cp -r ${FILES_DDCLIENTPHP[${path}]} ../version/${VERSION}/ddclient-php${name_suffix}-${VERSION}/${PATHS_DDCLIENTPHP[${path}]}
done

for path in $(seq 0 $((${#FILES_DDCLIENTGUI[@]} - 1))); do
	mkdir -p ../version/${VERSION}/ddclient-gui${name_suffix}-${VERSION}/${PATHS_DDCLIENTGUI[${path}]}
	cp -r ${FILES_DDCLIENTGUI[${path}]} ../version/${VERSION}/ddclient-gui${name_suffix}-${VERSION}/${PATHS_DDCLIENTGUI[${path}]}
done

cd ../version/${VERSION}

echo "cmake_minimum_required (VERSION 2.6)
project(ddclient-gui)
SET(VERSION "${VERSION}")
add_subdirectory(src/ddclient-gui)" > ddclient-gui${name_suffix}-${VERSION}/CMakeLists.txt

echo "cmake_minimum_required (VERSION 2.6)
project(ddconsole)
SET(VERSION "${VERSION}")
add_subdirectory(src/ddconsole)" > ddconsole${name_suffix}-${VERSION}/CMakeLists.txt

echo "removing unneeded files..."
find -name .svn | xargs rm -rf
find -name "*~" | xargs rm -f
find -name "*.o" | xargs rm -f
find -name "CMakeLists*.user*" | xargs rm -f
cd ddclient-gui${name_suffix}-${VERSION}/src/ddclient-gui
rm Makefile *.user moc_*.cpp
cd ../../..


echo "BUILDING SOURCE ARCHIVES..."
tar -cz ddconsole${name_suffix}-${VERSION} > ddconsole${name_suffix}-${1}.tar.gz
tar -cz ddclient-php${name_suffix}-${VERSION} > ddclient-php${name_suffix}-${1}.tar.gz
tar -cz ddclient-gui${name_suffix}-${VERSION} > ddclient-gui${name_suffix}-${1}.tar.gz
echo "DONE"
echo "PREPARING DEBIAN ARCHIVES..."

for dist in ${DEB_DISTS[@]}; do
	mkdir -p debs_${VERSION}/$dist/

	# only if it's the first ubuntu rev, we copy the .orig files. otherwise we just update.
	if [ "$UBUNTU_REV" == "1" ]; then
		cp ddconsole${name_suffix}-${VERSION}.tar.gz debs_${VERSION}/$dist/ddconsole${name_suffix}_${VERSION}.orig.tar.gz
		cp ddclient-gui${name_suffix}-${VERSION}.tar.gz debs_${VERSION}/$dist/ddclient-gui${name_suffix}_${VERSION}.orig.tar.gz
	fi

	cd debs_${VERSION}/$dist
	rm -rf ddconsole${name_suffix}-${VERSION} ddclient-gui${name_suffix}-${VERSION}
	cp -rf ../../ddconsole${name_suffix}-${VERSION} ../../ddclient-gui${name_suffix}-${VERSION} .

	cd ddclient-gui${name_suffix}-${VERSION}
	echo "Settings for ddclient-gui:"
	echo "in `pwd`"
	echo "\n" | dh_make -s -c gpl -e ${EMAIL}
	cd debian

	#rm *.ex *.EX
	cd ../../ddconsole${name_suffix}-${VERSION}
	echo "Settings for ddconsole:"
	echo "\n" | dh_make -s -c gpl -e ${EMAIL}
	cd debian
	#rm *.ex *.EX
	cd ../..

	if [ "$DEP_DDCONSOLE" == "" ]; then
		DEP_DDCONSOLE='${shlibs:Depends}, ${misc:Depends}'
	fi
	if [ "$DEP_DDCLIENTGUI" == "" ]; then
		DEP_DDCLIENTGUI='${shlibs:Depends}, ${misc:Depends}'
	fi


	######################################################################## DDCLIENT-GUI PREPARATION
	echo "Preparing debian/* files for ddclient-gui"
	cd ddclient-gui${name_suffix}-${VERSION}/debian
	replace="$(<changelog)"
	replace="${replace/${VERSION}-1/${VERSION}-0ubuntu${UBUNTU_REV}+agib~${dist}1}"
	replace="${replace/unstable/$dist}"
	echo "$replace" > changelog

	replace=$(<control)
	replace="${replace/'Section: unknown'/Section: net}"
	replace="${replace/'Homepage: <insert the upstream URL, if relevant>'/Homepage: http://downloaddaemon.sourceforge.net/}"
	replace="${replace/'Description: <insert up to 60 chars description>'/Description: $SYN_DDCLIENTGUI}"
	replace="${replace/'Build-Depends: debhelper (>= 7), cmake'/Build-Depends: $BUILDDEP_DDCLIENTGUI}"
	replace="${replace/'Depends: ${shlibs:Depends}, ${misc:Depends}'/Depends: $DEP_DDCLIENTGUI
Suggests: downloaddaemon, ddconsole
Conflicts: ddclient-gui${opposite_suffix}}"
	replace="${replace/'<insert long description, indented with spaces>'/$DESC_DDCLIENTGUI}"
	echo "$replace" > control

	replace="$(<copyright)"
	replace="${replace/<url:\/\/example.com>/http://downloaddaemon.sourceforge.net/}"
	replace="${replace/<put author\'s name and email here>
	    <likewise for another author>/$UPSTREAM_AUTHORS}"
	replace="${replace/<Copyright (C) YYYY Firtname Lastname>
	    <likewise for another author>/$COPYRIGHT_HOLDERS}"
	replace="${replace/
	### SELECT: ###/}"
	replace="${replace/
	### OR ###
	   This package is free software; you can redistribute it and\/or modify
	   it under the terms of the GNU General Public License version 2 as
	   published by the Free Software Foundation.
	##########/}"
	replace="${replace/
	# Please also look if there are files or directories which have a
	# different copyright\/license attached and list them here./}"
	echo "$replace" > copyright

	#replace="$(<dirs)"
	#replace+="
	#/usr/share"
	#echo "$replace" > dirs

	rm docs cron.d.ex ddclient-gui.default.ex ddclient-gui.doc-base.EX emacsen-install.ex emacsen-remove.ex emacsen-startup.ex init.d.ex init.d.lsb.ex manpage.* menu.ex README.Debian watch.ex postinst.ex  postrm.ex  preinst.ex  prerm.ex
	cd ../..


	###################################################################### DDCONSOLE PREPARATION
	echo "Preparing debian/* files for ddconsole"
	cd ddconsole${name_suffix}-${VERSION}/debian
	replace="$(<changelog)"
	replace="${replace/${VERSION}-1/${VERSION}-0ubuntu${UBUNTU_REV}+agib~${dist}1}"
	replace="${replace/unstable/$dist}"
	echo "$replace" > changelog

	replace=$(<control)
	replace="${replace/'Section: unknown'/Section: net}"
	replace="${replace/'Homepage: <insert the upstream URL, if relevant>'/Homepage: http://downloaddaemon.sourceforge.net/}"
	replace="${replace/'Description: <insert up to 60 chars description>'/Description: $SYN_DDCONSOLE}"
	replace="${replace/'Build-Depends: debhelper (>= 7), cmake'/Build-Depends: $BUILDDEP_DDCONSOLE}"
	replace="${replace/'Depends: ${shlibs:Depends}, ${misc:Depends}'/Depends: $DEP_DDCONSOLE
Suggests: downloaddaemon, ddclient-gui
Conflicts: ddconsole${opposite_suffix}}"
	replace="${replace/'<insert long description, indented with spaces>'/$DESC_DDCONSOLE}"
	echo "$replace" > control

	replace="$(<copyright)"
	replace="${replace/<url:\/\/example.com>/http://downloaddaemon.sourceforge.net/}"
	replace="${replace/<put author\'s name and email here>
	    <likewise for another author>/$UPSTREAM_AUTHORS}"
	replace="${replace/<Copyright (C) YYYY Firtname Lastname>
	    <likewise for another author>/$COPYRIGHT_HOLDERS}"
	replace="${replace/
	### SELECT: ###/}"
	replace="${replace/
	### OR ###
	   This package is free software; you can redistribute it and\/or modify
	   it under the terms of the GNU General Public License version 2 as
	   published by the Free Software Foundation.
	##########/}"
	replace="${replace/
	# Please also look if there are files or directories which have a
	# different copyright\/license attached and list them here./}"
	echo "$replace" > copyright

	rm docs cron.d.ex ddconsole.default.ex ddconsole.doc-base.EX emacsen-install.ex emacsen-remove.ex emacsen-startup.ex init.d.ex init.d.lsb.ex manpage.* menu.ex README.Debian watch.ex postinst.ex  postrm.ex  preinst.ex  prerm.ex
	cd ../..


	echo "


	"
	if [ "$4" != "nightly" ]; then
	read -p "Basic debian package preparation for the clients is done. Please move to <svn root>/version/debs_${1}/*/debian and modify the files as you need them. 
	Then press [enter] to continue package building. expecially the changelog file should be changed."
	fi

	if [ $BINARY == true ]; then
		echo "building client packages..."
		cd ddconsole${name_suffix}-${VERSION}
		debuild -d -sa
		cd ..
		cd ddclient-gui${name_suffix}-${VERSION}
		debuild -d -sa
		cd ..
	else
		echo "building client packages..."
		cd ddconsole${name_suffix}-${VERSION}
		debuild -S -sa
		cd ..
		cd ddclient-gui${name_suffix}-${VERSION}
		debuild -S -sa
		cd ..
	fi
	cd ../..

done

