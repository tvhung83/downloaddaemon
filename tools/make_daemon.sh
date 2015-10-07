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
BUILDDEP_DD="debhelper (>= 7), cmake, libcurl4-gnutls-dev"

# dependencies (leave empty for auto-detection)
DEP_DD=""

# synopsis (up to 60 characters)
SYN_DD="A remote controlable download manager"

# description (if you want a new line, you need to put a space right behind it)
DESC_DD="DownloadDaemon is a download manager with many features
 including parsing of one click hoster, automatic
 reconnection, alternating proxies etc.
 In order to control the daemon you need a suitable client, 
 which can be optained from
 http://downloaddaemon.sourceforge.net/. The ubuntu packages are:
 ddconsole for a Command line interface
 ddclient-gui for a GUI-client.
 There is also a PHP client on the website."

# specify all files/directorys (array) and the path's where they should go to (basically a cp -r FILES_XXX[i] PATHS_XXX[i] is done)
# the .svn folders are removed automatically. Folders are created automatically before copying
FILES_DD=("../src/daemon" "../src/CheckCxx11.cmake" "../src/include/netpptk" "../src/include/crypt" "../src/include/reg_ex" "../src/include/cfgfile" "../src/include/ddcurl.h" "../etc/downloaddaemon" "../etc/init.d/downloadd" "../share/downloaddaemon/reconnect" "../share/downloaddaemon/plugins/captchadb/*.tar.bz2"
"../AUTHORS" "../CHANGES" "../TODO" "../LICENCE" "../INSTALLING")
PATHS_DD=("src/" "src/" "src/include" "src/include" "src/include" "src/include" "src/include" "etc/" "etc/init.d" "share/downloaddaemon" "share/downloaddaemon/plugins/captchadb/")

script_dir=`pwd`

#set -x 

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
pkg_name="downloaddaemon"
if [ "$4" != "binary" ]
then
	BINARY=false
fi
if [ "$4" = "nightly" ]
then
	pkg_name="downloaddaemon-nightly"
fi
	

VERSION=$1
EMAIL=$2


mkdir -p ../version/${VERSION}


for path in $(seq 0 $((${#FILES_DD[@]} - 1))); do
	mkdir -p ../version/${VERSION}/${pkg_name}-${VERSION}/${PATHS_DD[${path}]}
	cp -r ${FILES_DD[${path}]} ../version/${VERSION}/${pkg_name}-${VERSION}/${PATHS_DD[${path}]}
done
	


cd ../version/${VERSION}

echo "cmake_minimum_required (VERSION 2.6)
project(DownloadDaemon)
SET(VERSION "${VERSION}")
add_subdirectory(src/daemon)" > ${pkg_name}-${VERSION}/CMakeLists.txt

echo "removing unneeded files..."
find -name .svn | xargs rm -rf
find -name "*~" | xargs rm -f
find -name "CMakeLists*.user*" | xargs rm -f


echo "BUILDING SOURCE ARCHIVES..."
tar -cz ${pkg_name}-${VERSION} > ${pkg_name}-${1}.tar.gz

echo "DONE"
echo "PREPARING DEBIAN ARCHIVES..."

for dist in ${DEB_DISTS[@]}; do
	mkdir -p "debs_${VERSION}/$dist"
	# only if it's the first ubuntu rev, we copy the .orig files. otherwise we just update.
	if [ "$UBUNTU_REV" == "1" ]; then
		cp "${pkg_name}-${VERSION}.tar.gz" "debs_${VERSION}/$dist/${pkg_name}_${VERSION}.orig.tar.gz"
	fi

	cd "debs_${VERSION}/$dist"
	rm -rf ${pkg_name}-${VERSION}
	cp -rf ../../${pkg_name}-${VERSION} .

	cd ${pkg_name}-${VERSION}
	echo "Settings for DownloadDaemon:"
	echo "\n" | dh_make -m -c gpl -e ${EMAIL} --packagename ${pkg_name}
	cd debian
	cd ../..


	if [ "$DEP_DD" == "" ]; then
		DEP_DD='${shlibs:Depends}, ${misc:Depends}'
	fi

	################################################################################ DOWNLOADDAEMON PREPARATION
	echo "Preparing debian/* files for DownloadDaemon"
	cd ${pkg_name}-${VERSION}/debian
	replace="$(<changelog)"
	replace="${replace/${VERSION}-1/${VERSION}-0ubuntu${UBUNTU_REV}+agib~${dist}1}"
	replace="${replace/unstable/$dist}"
	echo "$replace" > changelog

	replace=$(<control)
	replace="${replace/'Section: unknown'/Section: net}"
	replace="${replace/'Homepage: <insert the upstream URL, if relevant>'/Homepage: http://downloaddaemon.sourceforge.net/}"
	replace="${replace/'Description: <insert up to 60 chars description>'/Description: $SYN_DD}"
	replace="${replace/'Build-Depends: debhelper (>= 7), cmake'/Build-Depends: $BUILDDEP_DD}"
	# DO NOT INDENT THESE LINES!
	if [ "${pkg_name}" = "downloaddaemon" ]; then
		replace="${replace/'Depends: ${shlibs:Depends}, ${misc:Depends}'/Depends: $DEP_DD
Recommends: gocr, tar, unzip, unrar
Suggests: ddclient-gui, ddconsole
Conflicts: downloaddaemon-nightly}"
	else
		replace="${replace/'Depends: ${shlibs:Depends}, ${misc:Depends}'/Depends: $DEP_DD
Recommends: gocr, tar, unzip, unrar
Suggests: ddclient-gui, ddconsole
Conflicts: downloaddaemon}"
	fi

	replace="${replace/'<insert long description, indented with spaces>'/$DESC_DD}"
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
	#/etc/downloaddaemon
	#/usr/share"
	#echo "$replace" > dirs

	mv postinst.ex postinst
	mv postrm.ex postrm
	replace="$(<postinst)"
	replace="${replace/'    configure)'/    configure)
		if ! getent group downloadd >/dev/null; then
			addgroup --system downloadd
		fi

		if ! getent passwd downloadd >/dev/null; then
			adduser --system --ingroup downloadd --home /etc/downloaddaemon downloadd
			usermod -c DownloadDaemon downloadd
		fi

		if [ -d /etc/downloaddaemon ]; then
	  		chown -R downloadd:downloadd /etc/downloaddaemon
		fi
		if [ -d /var/downloads ]; then
	  		chown -R downloadd:downloadd /var/downloads
		fi
		if [ -x /etc/init.d/downloadd ]; then
			update-rc.d downloadd defaults >/dev/null
			/etc/init.d/downloadd restart
		fi
	}"
	echo "$replace" > postinst

	replace="$(<postrm)"
	replace="${replace/'    purge|remove|upgrade|failed-upgrade|abort-install|abort-upgrade|disappear)'/    purge|remove|upgrade|failed-upgrade|abort-install|abort-upgrade|disappear)
		if [ \"\$1\" = \"purge\" ] ; then
			update-rc.d downloadd remove >/dev/null || exit \$?
		fi
	}"

	echo "$replace" > postrm

	cp $script_dir/rules_tpl rules
	if [ "$4" == "nightly" ]; then
		replace="$(<rules)"
		replace="${replace//downloaddaemon/downloaddaemon-nightly}"
		echo "$replace" > rules
	fi

	rm docs cron.d.ex ${pkg_name}.default.ex ${pkg_name}.doc-base.EX emacsen-install.ex emacsen-remove.ex emacsen-startup.ex init.d.ex init.d.lsb.ex manpage.* menu.ex README.Debian watch.ex  preinst.ex  prerm.ex
	cd ..

	echo "


	"
	if [ "$4" != "nightly" ]; then
	read -p "Basic debian package preparation for the clients is done. Please move to <svn root>/version/debs_${1}/$dist/downloaddaemon-${1}/debian and modify the files as you need them. 
Then press [enter] to continue package building. expecially the changelog file should be changed."
	fi
	echo "building downloaddaemon packages..."
	echo "currently in `pwd`"
	if [ $BINARY == true ]; then
		debuild -d -sa
	else
		debuild -S -sa
	fi

	cd ../../..
done

