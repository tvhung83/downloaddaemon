#!/bin/bash
# call it with arguments: ./add_downloads.sh 127.0.0.1 url_list.txt my_password
# If you call this from flashgot, use the following options: Program-path: /bin/bash
# Parameters: <path to script> <host> [UFILE] <password>
# password is optional. This only works on linux and with DownloadDaemon 0.9 and above

HOST="${1}"
PASSWORD="${3}"


pkg_id="0"
if [ "$PASSWORD" == "" ]; then
	pkg_id="`ddconsole --host \"${HOST}\" --command \"pkg add\"`"
else
	pkg_id="`ddconsole --host \"${HOST}\" --password \"${PASSWORD}\" --command \"pkg add\"`"
fi

IFS='
'

for line in $pkg_id; do
	pkg_id=$line
done

while read line; do
	if [ "$PASSWORD" == "" ]; then
		ddconsole --host "${HOST}" --command "dl add ${pkg_id} ${line}"
	else
		ddconsole --host "${HOST}" --password "${PASSWORD}" --command "dl add ${pkg_id} ${line}"
	fi
done < ${2}
