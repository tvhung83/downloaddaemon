<?php
/*
 Sets the default host that will be seen on the login page
*/
define('DEFAULT_HOST', '127.0.0.1');

/*
 Sets the default port that will be seen on the login page
*/
define('DEFAULT_PORT', '56789');

/*
 Sets the template/theme that should be used for displaying. Right now there is only 'default'
*/
define('TEMPLATE', 'default');

/*
 Sets the language that should be used. Right now there is 'en' and 'de', but not all parts will be
 translated yet, if you set something else than 'en'. But feel free to try it out.
*/
define('LANG', 'en');

/*
 If this is set to true, ddclient-php will automatically refresh the displayed download list if
 there are running downloads. Careful: This might prevent the machine where DD runs on from
 spinning down its hard-disk if no download is running and you leave ddclient-php open in your browser
*/
define('AUTO_REFRESH', true);

/*
 If AUTO_REFRESH is set to true, this sets the reload-interval in seconds.
*/
define('REFRESH_TIME', 3);

/*
 If this is set to true, ddclient-php will check for the file-status of each file in the download
 list when updating. This can take a long time. Generally, you should only set this to true if your server
 runs on fast hardware or you usually only use small download lists (up to ~50 downloads at once).
 If you have larger lists, the server will be overloaded pretty fast.
 The advantage of setting this to true, however, is, that you will only get the "delete file" button
 on the manage-site if there really is a file to delete. If you set it to false, the "delete file" button
 will always show up, and file-deletion will simply fail if you press it and there is no file.
*/
define('CHECK_FILE_STATUS', false);

/*
 set this to true to skip the login page
*/
define('SKIP_LOGIN_PAGE', false);

/*
 the default password to use for logging in the SKIP_LOGIN_PAGE is set to true
*/
define('SKIP_LOGIN_PASSWORD', '');


?>
