<?php
include 'inc/_config.inc.php';

//Language File
if(file_exists('lang/'.LANG.'.php')){
	include 'lang/'.LANG.'.php';	
}else{
	include 'lang/en.php';	
}

include 'inc/template.inc.php';
include 'inc/functions.inc.php';

//Logged in?
if(isset($_COOKIE['ddclient_host'])){
	$logged_in = true;	
}else{
	$logged_in = false;
}


if(isset($_GET['site']) && file_exists('sites/'.$_GET['site'].'.php') && $logged_in == true){
	$site = $_GET['site'];
}else{
	$site = 'login';
}

$site_url = "";
if(!isset($_SERVER['HTTPS']) || strtolower($_SERVER['HTTPS']) == "off") {
	$site_url = "http://";
} else {
	$site_url = "https://";
}

$err_message='';
if(isset($_SERVER['HTTP_X_FORWARDED_SERVER']))
	$host = $_SERVER['HTTP_X_FORWARDED_HOST'];
else
	$host = $_SERVER['HTTP_HOST'];

$tpl_vars = array(
	'T_SITE_URL' => $site_url . $host . substr($_SERVER['SCRIPT_NAME'], 0, strrpos($_SERVER['SCRIPT_NAME'], "/")+1),
	'T_DEFAULT_LANG' => LANG,
	'L_DD' => $LANG['DD'],
	'L_Manager' => $LANG['Manager'],
	'L_Add' => $LANG['Add'],
	'L_List' => $LANG['List'],
	'L_Manage' => $LANG['Manage'],
	'L_Config' => $LANG['Config'],
	'L_Logout' => $LANG['Logout'],
	'L_Site' => isset($LANG[ucfirst($site)]) ? $LANG[ucfirst($site)] : "",
	'T_META' => '',
);

include 'sites/'.$site.'.php';

$tpl_vars['err_message'] = $err_message;

if(isset($_GET["echo"])) {
	// stupid internet explorer caches ajax requests..
	header("Last-Modified: " . gmdate("D, d M Y H:i:s") . " GMT");
	header("Cache-Control: no-store, no-cache, must-revalidate");
	header("Cache-Control: post-check=0, pre-check=0", false);
	header("Pragma: no-cache");  
	eval("?>" . template_parse($site, $tpl_vars));
} elseif(isset($_GET["raw"])) {
} else {
	eval("?>" . template_parse_site($site, $tpl_vars));
}
?>
