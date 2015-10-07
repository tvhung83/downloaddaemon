@ECHO off
rem call it with arguments: add_download.bat 127.0.0.1 url_list.txt my_password
rem If you call this from flashgot, use the following options: Program-path: C:\Windows\system32\cmd.exe
rem Parameters: /C <path to script> <path to ddconsole.exe> <host> [UFILE] <password>
rem password is optional. This only works on Windows and with DownloadDaemon 0.9 and above. To use it on Linux/OS X, use add_download.sh


set DDCONSOLE=%1
set HOST=%2
set PASSWORD=%4


set pkg_id=""



IF /I ["%PASSWORD%"]==[""] (
	%DDCONSOLE% --host "%HOST%" --command "pkg add" > c:\tmp_flashgot
) ELSE (
	%DDCONSOLE% --host "%HOST%" --password "%PASSWORD%" --command "pkg add" > c:\tmp_flashgot
)

for /f "tokens=*" %%l in ('type c:\tmp_flashgot') do (
	set pkg_id=%%l%
)
	
del c:\tmp_flashgot


for /f "tokens=*" %%l in ('type %3%') do (
	IF /I ["%PASSWORD%"]==[""] (
		%DDCONSOLE% --host "%HOST%" --command "dl add %pkg_id% %%l%"
	) ELSE (
		%DDCONSOLE% --host "%HOST%" --password "%PASSWORD%" --command "dl add %pkg_id% %%l%"
	)

)


