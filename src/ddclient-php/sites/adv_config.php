<?php
// Connect to Daemon
$socket = socket_create(AF_INET, SOCK_STREAM, 0);
$connect = connect_to_daemon($socket, $_COOKIE['ddclient_host'], $_COOKIE['ddclient_port'], $_COOKIE['ddclient_passwd'], $_COOKIE['ddclient_enc'], 5);


if(isset($_POST['submit_adv_form'])) {
	foreach($_POST as $var => $val) {
		if($var != "submit_adv_form") {
			$buf = "";
			send_all($socket, "DDP VAR SET " . $var . " = " . $val);
			recv_all($socket, $buf);
		}
	}
	header("Location: index.php?site=config");
}

$res = "";
send_all($socket, "DDP VAR LIST");
recv_all($socket, $res);
$Variables = explode("\n", $res);

$tpl_vars['L_Conf_general'] = $LANG['Conf_general'];
$tpl_vars['L_Conf_reconnect'] = $LANG['Conf_reconnect'];
$tpl_vars['L_Conf_premium'] = $LANG['Conf_premium'];




?>
