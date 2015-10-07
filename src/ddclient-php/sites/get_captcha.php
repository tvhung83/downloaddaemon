<?php
// Connect to Daemon
$socket = socket_create(AF_INET, SOCK_STREAM, 0);
$connect = connect_to_daemon($socket, $_COOKIE['ddclient_host'], $_COOKIE['ddclient_port'], $_COOKIE['ddclient_passwd'], $_COOKIE['ddclient_enc'], 5);
$err_message = '';

if(isset($_GET['id'])) {
	$buf = "";
	if(isset($_GET['result'])) {
		send_all($socket, "DDP CAPTCHA SOLVE " . $_GET['id'] . " " . $_GET['result']);
		recv_all($socket, $buf);
		header("Location: " . $tpl_vars['T_SITE_URL'] . "index.php?site=manage");
	}

	$cap = "";
	send_all($socket, "DDP CAPTCHA REQUEST ". $_GET['id']);
	recv_all($socket, $cap);
	$parts = explode("|", $cap, 3);
	if(count($parts) != 3)
		die();

	if(isset($_GET['what'])) {
		if($_GET['what'] == "qestion") {
			die($parts[1]);
		}
		if($_GET['what'] == "image") {
			header("Content-Type: image/" . $parts[0]);
			die($parts[2]);
		}
	} else {
		$tpl_vars['T_Question'] = $parts[1];
		$tpl_vars['T_Image'] = "index.php?site=get_captcha&raw=1&what=image&id=" . $_GET['id'];
		$tpl_vars['T_ResultUrl'] = "index.php";
		$tpl_vars['T_HiddenFields'] = "<input type=\"hidden\" name=\"site\" value=\"get_captcha\"><input type=\"hidden\" name=\"id\" value=\"" . $_GET['id'] . "\">";
	}
}
?>
