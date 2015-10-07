#!/bin/bash
# takes 2 arguments: the passphrase for your private key to sign the files and the sf-password for svn up
if [ "$1" == "" ] || [ "$2" == "" ]; then
	echo "Usage: $0 <passphrase> <sourceforge-pw>"
	exit -1
fi

# set this variable to the svn-trunk directory to manage
trunk_root="/home/adrian/downloaddaemon/trunk"

# set this to the e-mail adress to use for signing the packages
sign_email="adrian@batzill.com" # "example@example.com"

# ubuntu ppa to upload to
ppa="ppa:agib/ppa"

# sf password
sf_password="$2"

function log () {
	echo "[`date`]: $1" >> $trunk_root/tools/autobuild.log
}

dd_up=false
gui_up=false
con_up=false
php_up=false

svn_up_cmd="$trunk_root/tools/svn_with_pw.expect"
svn_ci_cmd="$trunk_root/tools/commit_with_pw.expect"

cd "$trunk_root/src/daemon"

if [ `$svn_up_cmd up $sf_password  | wc -l` -gt 3 ]; then
	dd_up=true
	log "New DownloadDaemon version available"
fi

cd "$trunk_root/src/ddclient-gui"
if [ `$svn_up_cmd up $sf_password | wc -l` -gt 3 ]; then
	gui_up=true
	log "New ddclient-gui version available"
fi

cd "$trunk_root/src/ddconsole"
if [ `$svn_up_cmd up $sf_password | wc -l` -gt 3 ]; then
	con_up=true
	log "New ddconsole version available"
fi

cd "$trunk_root/src/ddclient-php"
if [ `$svn_up_cmd up $sf_password | wc -l` -gt 3 ]; then
	php_up=true
	log "New ddclient-php version available"
fi

if [ $dd_up == false -a $gui_up == false -a $con_up == false -a $php_up == false ]; then
	log "No updates available tonight"
	exit 0
fi

cd "${trunk_root}/.."
$svn_up_cmd up $sf_password
cd "${trunk_root}/tools"

version="`svn info | grep Revision | cut -f2 -d ' ' /dev/stdin`"


if [ $dd_up == true ]; then 
	cd "${trunk_root}/tools"
	expect -c "
set timeout 120
match_max 100000
spawn ./make_daemon.sh $version $sign_email 1 nightly
expect {
	\"*assphrase:\" {
		send \"$1\r\"
		exp_continue
	}
	eof {
		exit
	}	
}
"
	cd ../version/${version}/debs_${version}


	for f in $( ls ); do
		dput $ppa ${f}/downloaddaemon-nightly_${version}-0ubuntu1+agib~${f}1_source.changes
	done

	cd ..
	cp downloaddaemon-nightly-${version}.tar.gz ${trunk_root}/../tags
	svn add ${trunk_root}/../tags/downloaddaemon-nightly-${version}.tar.gz
	cd ${trunk_root}/version
	rm -r ${version}
fi

if [ $gui_up == true -o $php_up == true -o $con_up == true ]; then
	cd "${trunk_root}/tools"
        expect -c "
set timeout 120
match_max 100000
spawn ./make_clients.sh $version $sign_email 1 nightly
expect {
        \"*assphrase:\" {
                send \"$1\r\"
                exp_continue
        }
        eof {
                exit
        }
}
"
	cd ../version/${version}/debs_${version}
	for f in $( ls ); do
		if [ $gui_up == true ]; then
			dput $ppa ${f}/ddclient-gui-nightly_${version}-0ubuntu1+agib~${f}1_source.changes
		fi
		if [ $con_up == true ]; then
			dput $ppa ${f}/ddconsole-nightly_${version}-0ubuntu1+agib~${f}1_source.changes
		fi
	done
	cd ..
	if [ $gui_up == true ]; then
		cp ddclient-gui-nightly-${version}.tar.gz ${trunk_root}/../tags
		svn add ${trunk_root}/../tags/ddclient-gui-nightly-${version}.tar.gz
	fi
	if [ $con_up == true ]; then
		cp ddconsole-nightly-${version}.tar.gz ${trunk_root}/../tags
		svn add ${trunk_root}/../tags/ddconsole-nightly-${version}.tar.gz
	fi
	if [ $php_up == true ]; then
		cp ddclient-php-nightly-${version}.tar.gz ${trunk_root}/../tags
		svn add ${trunk_root}/../tags/ddclient-php-nightly-${version}.tar.gz
	fi
	cd ${trunk_root}/version
	rm -r ${version}
fi	

cd $trunk_root/../tags



$svn_ci_cmd "release of DownloadDaemon nightly revision ${version}" $sf_password
