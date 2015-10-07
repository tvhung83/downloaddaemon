<?php
// Connect to Daemon
$socket = socket_create(AF_INET, SOCK_STREAM, 0);
$connect = connect_to_daemon($socket, $_COOKIE['ddclient_host'], $_COOKIE['ddclient_port'], $_COOKIE['ddclient_passwd'], $_COOKIE['ddclient_enc'], 5);

// Site Vars
$err_message = '';
$dl_list = '';

if($connect != 'SUCCESS') {
	$err_msg = msg_generate($LANG[$connect], 'error');
}else{
	$content = '';
	if(isset($_POST['apply'])) {
	$buf = '';
	send_all($socket, 'DDP ROUTER SET reconnect_policy = ' . $_POST['reconnect_policy']);
	recv_all($socket, $buf);
	send_all($socket, 'DDP ROUTER SETMODEL ' . $_POST['router_model']);
	recv_all($socket, $buf);
	send_all($socket, 'DDP ROUTER SET router_ip = ' . $_POST['router_ip']);
	recv_all($socket, $buf);
	send_all($socket, 'DDP ROUTER SET router_username = ' . $_POST['router_username']);
	recv_all($socket, $buf);
	if($_POST['router_password'] != '') {
		send_all($socket, 'DDP ROUTER SET router_password = ' . $_POST['router_password']);
		recv_all($socket, $buf);
	}	
}
if(isset($_POST['enable_reconnect'])) {
	$buf = '';
	send_all($socket, 'DDP VAR SET enable_reconnect = 1');
	recv_all($socket, $buf);
}
if(isset($_POST['disable_reconnect'])) {
	$buf = '';
	send_all($socket, 'DDP VAR SET enable_reconnect = 0');
	recv_all($socket, $buf);
}

$buf = '';
send_all($socket, 'DDP VAR GET enable_reconnect');
recv_all($socket, $buf);
if($buf == '1' || $buf == 'true') {
	$content .= '<input type="submit" name="disable_reconnect" value="Disable Reconnecting" id="disable_reconnect" class="submit" />';
} else {
	$content .= '<input type="submit" name="enable_reconnect" value="Enable Reconnecting" id="enable_reconnect" class="submit" />';
}
	$content .= '<br /><br />';
	send_all($socket, 'DDP ROUTER GET reconnect_policy');
	$buf = '';
	recv_all($socket, $buf);
	$content .= 'This option specifies DownloadDaemons reconnect activity: <br />';
	$content .= 'HARD cancels all downloads if a reconnect is needed<br />';
	$content .= 'CONTINUE only cancels downloads that can be continued after the reconnect<br />';
	$content .= 'SOFT will wait until all other downloads are finished<br />';
	$content .= 'PUSSY will only reconnect if there is no other choice (no other download can be started without a reconnect)<br />';
	$content .= 'Reconnect Policy: <select name="reconnect_policy"><option value="HARD" ';
	if($buf == 'HARD') {
		$content .= 'selected="selected" ';
	}
	$content .= '>HARD</option>';
	$content .= '<option value="CONTINUE" ';
	if($buf == 'CONTINUE') {
		$content .= 'selected="selected" ';
	}
	$content .= '>CONTINUE</option>';
	$content .= '<option value="SOFT" ';
	if($buf == 'SOFT') {
		$content .= 'selected="selected" ';
	}
	$content .= '>SOFT</option>';
	$content .= '<option value="PUSSY" ';
	if($buf == 'PUSSY') {
		$content .= 'selected="selected" ';
	}
	$content .= '>PUSSY</option></select>';
	$content .= '<br /><br />';
	$buf = '';
	$model = '';
	send_all($socket, 'DDP ROUTER GET router_model');
	recv_all($socket, $model);
	send_all($socket, 'DDP ROUTER LIST');
	recv_all($socket, $buf);
	$model_list = explode("\n", $buf);
	
	$content .= 'Router Model: ';
	$content .= '<select name="router_model">';
	$content .= '<option value=""></option>';
	for($i = 0; $i != count($model_list); $i++) {
		$content .= '<option value="' . $model_list[$i] . '"';
		if($model_list[$i] == $model) {
			$content .= ' SELECTED ';
		}
		$content .= '>' . $model_list[$i] . '</option>';
	}
	$content .= '</select>';
	$content .= '<br /><br />';
	$buf = '';
	send_all($socket, 'DDP ROUTER GET router_ip');
	recv_all($socket, $buf);
	$content .= 'Router IP: <input type="text" name="router_ip" value="' . $buf . '" /><br />';
	send_all($socket, 'DDP ROUTER GET router_username');
	recv_all($socket, $buf);
	$content .= 'Username: <input type="text" name="router_username" value="' . $buf . '" /><br />';
	$content .= 'Password: <input type="text" name="router_password" ';
	if(strlen($buf) > 0) {
		$content .= 'value="****" ';
	}
	$content .=  '/><br />';
	$content .= '<br /><input type="submit" name="apply" value="Apply" id="apply" class="submit" /><br />';
}

$tpl_vars['content'] = $content;
$tpl_vars['err_message'] = $err_message;
$tpl_vars['L_Conf_general'] = $LANG['Conf_general'];
$tpl_vars['L_Conf_reconnect'] = $LANG['Conf_reconnect'];
$tpl_vars['L_Conf_premium'] = $LANG['Conf_premium'];


?>
