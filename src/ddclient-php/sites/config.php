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
	if(isset($_POST['change_pw'])) {
	send_all($socket, 'DDP VAR SET mgmt_password=' . $_POST['old_pw'] . ';' . $_POST['new_pw']);
	$buf = '';
	recv_all($socket, $buf);
	if(substr($buf, 0, 3) != '100') {
		$err_message .= msg_generate('The Password could not be changed.', 'error');
	}		
}

if(isset($_POST['downloading_active'])) {
	if($_POST['downloading_active'] == 'Activate Downloading') {
		send_all($socket, 'DDP VAR SET downloading_active=1');
	} else {
		send_all($socket, 'DDP VAR SET downloading_active=0');
	}
	$buf = '';
	recv_all($socket, $buf);
	if(substr($buf, 0, 3) != '100') {
		$err_message .= msg_generate('Failed to set DownloadDaemons download activity status.', 'error');
	}
}

if(isset($_POST['apply'])) {
	$buf = '';
	send_all($socket, 'DDP VAR SET download_timing_start = ' . $_POST['download_timing_start']);
	recv_all($socket, $buf);
	send_all($socket, 'DDP VAR SET download_timing_end = ' . $_POST['download_timing_end']);
	recv_all($socket, $buf);
	send_all($socket, 'DDP VAR SET download_folder = ' . $_POST['download_folder']);
	recv_all($socket, $buf);
	send_all($socket, 'DDP VAR SET simultaneous_downloads = ' . $_POST['simultaneous_downloads']);
	recv_all($socket, $buf);
	send_all($socket, 'DDP VAR SET log_level = ' . $_POST['log_level']);
	recv_all($socket, $buf);
	send_all($socket, 'DDP VAR SET max_dl_speed = ' . $_POST['max_dl_speed']);
	recv_all($socket, $buf);
}

	$buf = '';
	send_all($socket, 'DDP VAR GET downloading_active');
	recv_all($socket, $buf);
	if($buf == '0') {
		$enable_button = '<input type="submit" name="downloading_active" value="Activate Downloading" class="submit" />';
	} else {
		$enable_button = '<input type="submit" name="downloading_active" value="Deactivate Downloading" class="submit" />';
	}

	$dl_start = '';
	$dl_end = '';
	send_all($socket, 'DDP VAR GET download_timing_start');
	recv_all($socket, $dl_start);
	send_all($socket, 'DDP VAR GET download_timing_end');
	recv_all($socket, $dl_end);

	$dl_folder = '';
	send_all($socket, 'DDP VAR GET download_folder');
	recv_all($socket, $dl_folder);


	$sim_dls = '';
	send_all($socket, 'DDP VAR GET simultaneous_downloads');
	recv_all($socket, $sim_dls);

	$log_lvl = '';	
	send_all($socket, 'DDP VAR GET log_level');
	recv_all($socket, $log_lvl);
	$debug = '<select name="log_level" id="log_level"><option value="DEBUG" ';
	if($log_lvl == 'DEBUG') {
		$debug .= 'selected="selected" ';
	}
	$debug .= '>Debug</option>';
	$debug .= '<option value="WARNING" ';
	if($log_lvl == 'WARNING') {
		$debug .= 'selected="selected" ';
	}
	$debug .= '>Warning</option>';
	$debug .= '<option value="SEVERE" ';
	if($log_lvl == 'SEVERE') {
		$debug .= 'selected="selected" ';
	}
	$debug .= '>Severe</option>';
	$debug .= '<option value="OFF" ';
	if($log_lvl == 'OFF') {
		$debug .= 'selected="selected" ';
	}
	$debug .= '>Off</option></select>';

	$log_file = '';
	send_all($socket, 'DDP VAR GET log_file');
	recv_all($socket, $log_file);

	$max_dl_speed = '';
	send_all($socket, 'DDP VAR GET max_dl_speed');
	recv_all($socket, $max_dl_speed);
}
	$tpl_vars['T_Enable_Button'] = $enable_button;
	$tpl_vars['L_Conf_general'] = $LANG['Conf_general'];
	$tpl_vars['L_Conf_reconnect'] = $LANG['Conf_reconnect'];
	$tpl_vars['L_Conf_premium'] = $LANG['Conf_premium'];
	$tpl_vars['L_Change_PWD'] = $LANG['Change_PWD'];
	$tpl_vars['L_Add_multi_DL_Desc'] = $LANG['Add_multi_DL_Desc'];
	$tpl_vars['L_Old_PWD'] = $LANG['Old_PWD'];
	$tpl_vars['L_New_PWD'] = $LANG['New_PWD'];
	$tpl_vars['L_RT_PWD'] = $LANG['RT_PWD'];
	$tpl_vars['L_en_di'] = $LANG['en_di'];
	$tpl_vars['L_gen_conf'] = $LANG['gen_conf'];
	$tpl_vars['L_start_end_desc'] = $LANG['start_end_desc'];
	$tpl_vars['L_st_time'] = $LANG['st_time'];
	$tpl_vars['L_end_time'] = $LANG['end_time'];
	$tpl_vars['L_dl_dir_desc'] = $LANG['dl_dir_desc'];
	$tpl_vars['L_dl_dir'] = $LANG['dl_dir'];
	$tpl_vars['L_simul_dl_desc'] = $LANG['simul_dl_desc'];
	$tpl_vars['L_simul_dl'] = $LANG['simul_dl'];
	$tpl_vars['L_log_level_desc'] = $LANG['log_level_desc'];
	$tpl_vars['L_log_level'] = $LANG['log_level'];
	$tpl_vars['L_max_speed_desc'] = $LANG['max_speed_desc'];
	$tpl_vars['L_adv_config'] = $LANG['Adv_config'];
	$tpl_vars['Content'] = $dl_list;
	$tpl_vars['Debug'] = $debug;
	$tpl_vars['DL_Start'] = $dl_start;
	$tpl_vars['DL_End'] = $dl_end;
	$tpl_vars['DL_Folder'] = $dl_folder;
	$tpl_vars['DL_sim'] = $sim_dls;
	$tpl_vars['Log_File'] = $log_file;
	$tpl_vars['Max_DL_Speed'] = $max_dl_speed;
	$tpl_vars['err_message'] = $err_message;
	
?>
