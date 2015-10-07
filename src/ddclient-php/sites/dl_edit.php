<?php
// Connect to Daemon
$socket = socket_create(AF_INET, SOCK_STREAM, 0);
$connect = connect_to_daemon($socket, $_COOKIE['ddclient_host'], $_COOKIE['ddclient_port'], $_COOKIE['ddclient_passwd'], $_COOKIE['ddclient_enc'], 5);

// Site Vars
$err_message = '';


$dlist = '';

send_all($socket, "DDP DL LIST");
recv_all($socket, $dlist);

$dlist = explode("\n", $dlist);

for($i = 0; $i < count($dlist); $i++) {
	$curr_line = explode_escaped('|', $dlist[$i]);
	if($curr_line[0] != "PACKAGE" && $curr_line[0] == $_GET['id']) {
		$tpl_vars['OLD_DL_TITLE'] = $curr_line[2];
		$tpl_vars['OLD_DL_URL'] = $curr_line[3];

	}

}

$tpl_vars['PKG_ID'] = $_GET["pkg_id"];
$tpl_vars['DL_ID'] = $_GET["id"];


?>
