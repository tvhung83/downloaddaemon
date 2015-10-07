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
		send_all($socket, 'DDP PREMIUM SET ' . $_POST['host'] . ' ' . $_POST['user'] . ';' . $_POST['pass']);
		recv_all($socket, $buf);
		if($buf == '100 SUCCESS') {
			$err_message .= msg_generate($LANG['L_Premium_SUCCESS'], 'success');
		} else {
			$err_message .= msg_generate($LANG['L_Premium_FAIL'], 'error');
		}
	}

$buf = '';
send_all($socket, 'DDP PREMIUM LIST');
recv_all($socket, $buf);
$host_list = explode("\n", $buf);
$user_list = '';

$content .= 'Host: ';
$content .= '<select name="host">';
$content .= '<option value=""></option>';
for($i = 0; $i != count($host_list); $i++) {
	$content .= '<option value="' . $host_list[$i] . '"';
	$content .= '>' . $host_list[$i] . '</option>';
}
$content .= '</select>';
$content .= '<br /><br />';
$content .= 'Username: <input type="text" name="user" /><br />';
$content .= 'Password: <input type="password" name="pass" /><br />';
$content .= '<input type="submit" name="apply" value="Apply" id="apply" class="submit" />';	
}

$tpl_vars['content'] = $content;
$tpl_vars['err_message'] = $err_message;
$tpl_vars['L_Conf_general'] = $LANG['Conf_general'];
$tpl_vars['L_Conf_reconnect'] = $LANG['Conf_reconnect'];
$tpl_vars['L_Conf_premium'] = $LANG['Conf_premium'];


?>
