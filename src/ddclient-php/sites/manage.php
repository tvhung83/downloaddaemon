<?php
// Connect to Daemon
$socket = socket_create(AF_INET, SOCK_STREAM, 0);
$connect = connect_to_daemon($socket, $_COOKIE['ddclient_host'], $_COOKIE['ddclient_port'], $_COOKIE['ddclient_passwd'], $_COOKIE['ddclient_enc'], 5);

// Site Vars
$err_message = '';
$dl_list = '';
$any_download_running = false;


if($connect != 'SUCCESS') {
	$err_msg = msg_generate($LANG[$connect], 'error');
}else{
	if(isset($_GET['action'])){
		switch ($_GET['action']) {
    		case 'activate':
				if($_GET['id'] != "") {
					send_all($socket, "DDP DL ACTIVATE " . $_GET['id']);
				} else {
					send_all($socket, "DDP PKG ACTIVATE " . $_GET['pkg_id']);
				}
				$buf = "";
				recv_all($socket, $buf);
				if(substr($buf, 0, 3) != "100") {
					$err_message .= msg_generate($LANG['ERR_DL_ACTIVATE'], 'error');
				}
        		break;
    		case 'deactivate':
    			if($_GET['id'] != "") {
					send_all($socket, "DDP DL DEACTIVATE " . $_GET['id']);
				} else {
					send_all($socket, "DDP PKG DEACTIVATE " . $_GET['pkg_id']);
				}
				$buf = "";
				recv_all($socket, $buf);
				if(substr($buf, 0, 3) != "100") {
					$err_message .= msg_generate($LANG['ERR_DL_DEACTIVATE'], 'error');
				}
        		break;
    		case 'delete':
				$buf = "";
				if($_GET['pkg_id'] != "") {
					send_all($socket, "DDP PKG DEL " . $_GET['pkg_id']);
				} else {
					send_all($socket, "DDP DL DEL " . $_GET['id']);
				}
				recv_all($socket, $buf);
				if(substr($buf, 0, 3) != "100") {
					$err_message .= msg_generate($LANG['ERR_DL_DEL'], 'error');
				}
        		break;
    		case 'del_file':
				send_all($socket, "DDP FILE DEL " . $_GET['id']);
				$buf = "";
				recv_all($socket, $buf);
				if(substr($buf, 0, 3) != "100") {
					$err_message .= msg_generate($LANG['ERR_FILE_DEL'], 'error');
				}
    			break;
    		case 'move':
				$buf = "";
				if($_GET['pkg_id'] != "") {
					send_all($socket, "DDP PKG UP " . $_GET['pkg_id']);
				} else {
					send_all($socket, "DDP DL UP " . $_GET['id']);
				}
				recv_all($socket, $buf);
				if(substr($buf, 0, 3) != "100") {
					$err_message .= msg_generate($LANG['ERR_DL_UP'], 'error');
				}
    			break;
			case 'edit':
				$buf = "";
				if($_GET['pkg_id'] != "") {
					send_all($socket, "DDP PKG SET " . $_GET['pkg_id'] . " PKG_NAME = " . $_POST['pkg_edit_name']);
					recv_all($socket, $buf);
					if(substr($buf, 0, 3) != "100") {
						$err_message .= msg_generate($LANG['ERR_EDIT'], 'error');
						break;
					}
					send_all($socket, "DDP PKG SET " . $_GET['pkg_id'] . " PKG_PASSWORD = " . $_POST['pkg_edit_passwd']);
				} else {
					send_all($socket, "DDP DL SET " . $_GET['id'] . " DL_TITLE = " . $_POST['dl_edit_title']);
					recv_all($socket, $buf);
					if(substr($buf, 0, 3) != "100") {
						$err_message .= msg_generate($LANG['ERR_EDIT'], 'error');
						break;
					}
					send_all($socket, "DDP DL SET " . $_GET['id'] . " DL_URL = " . $_POST['dl_edit_url']);
				}
				recv_all($socket, $buf);
				if(substr($buf, 0, 3) != "100") {
					$err_message .= msg_generate($LANG['ERR_EDIT'], 'error');
				}					
			break;
    		default:
    			break;
		}
	}
	$list = "";
	send_all($socket, "DDP DL LIST");
	recv_all($socket, $list);

	$download_index[] = array();
	$download_index = explode("\n", $list);

	$exp_dls[] = array();
	for($i = 0; $i < count($download_index); $i++) {
		$exp_dls[$i] = explode_escaped ('|', $download_index[$i]);
	}

	for($i = 0; $i < count($exp_dls); $i++) {
		if($exp_dls[$i][0] == "") continue;
		if($exp_dls[$i][0] == "PACKAGE") {
			$tpl_manage_vars = array(
				'T_Activate_Button' => '',
				'T_DelFile_Button' => '',
				'T_TR_CLASS' => 'pkg',
				'T_PKG_ID' => $exp_dls[$i][1],
				'T_DL_ID' => '',
				'T_DL_Date' => '',
				'T_DL_Title' => $exp_dls[$i][2],
				'T_DL_Title_short' => $exp_dls[$i][2],
				'T_DL_URL' => '',
				'T_DL_URL_short' => '',
				'T_DL_Class' => '',
				'T_DL_Status' => '',
				'T_SITE_URL' => $tpl_vars['T_SITE_URL'],
				'L_Activate' => '" style="visibility:hidden',
				'L_Delete' => '',
				'L_Move' => $LANG['Move'],
				'L_Delete_File' => '',
				'T_EDIT_SITE' => 'pkg_edit',
			);

			if($exp_dls[$i][3] != "") {
				$tpl_manage_vars['T_DL_Title'] = "<img src=\"".$tpl_vars['T_SITE_URL']."/templates/default/css/images/key.png\" alt=\"".$LANG['L_PASS_SET']."\" 
								  title=\"".$LANG['L_PASS_SET']."\" />" . $exp_dls[$i][2];
			}

			$dl_list .= template_parse('manage_line', $tpl_manage_vars);
			continue;
		}
		
		//Alternating <tr> Colors
		if($i%2 == 0) {
			$tr_class = 'even'; 
		}else{
			$tr_class = 'odd';
		}
	
		$dl_status = '';
		$percent = $exp_dls[$i][6];
		if($percent != 0)
			$percent = number_format($exp_dls[$i][5] / $percent * 100, 1);

		switch($exp_dls[$i][4]){
			case 'DOWNLOAD_RUNNING':
				$any_download_running = true;
				if($exp_dls[$i][7] > 0 && $exp_dls[$i][8] == 'PLUGIN_SUCCESS') {
					$dl_status .= 'Download running. Waiting ' . $exp_dls[$i][7] . ' seconds.';
				} else if($exp_dls[$i][7] > 0 && $exp_dls[$i][8] != 'PLUGIN_SUCCESS') {
					$dl_status .= 'Error: ' . $exp_dls[$i][8] . ' Retrying in ' . $exp_dls[$i][7] . 's';
				} else {
					if($exp_dls[$i][6] == 0) {
						$dl_status .= 'Running, fetched ' . number_format($exp_dls[$i][5] / 1048576, 1) . 'MB';
					} else {
						$dl_status .= number_format($exp_dls[$i][9] / 1024) . 'kb/s - ' . number_format($exp_dls[$i][5] / 1048576, 1) . '/' . number_format($exp_dls[$i][6] / 1048576, 1) . 'MB - ' . $percent . '%';
					}
				}
				break;
			case 'DOWNLOAD_INACTIVE':
				if($exp_dls[$i][8] == 'PLUGIN_SUCCESS') {
					$dl_status .= 'Download Inactive';
				} else {
					$dl_status .= 'Inactive. Error: ' . $exp_dls[$i][8];
				}
				break;
			case 'DOWNLOAD_PENDING':
				if($exp_dls[$i][8] == 'PLUGIN_SUCCESS') {
					$dl_status .= 'Download Pending';
					$display_ddinactive_warn = true;
					if($exp_dls[$i][6] > 0) {
						$dl_status .= ' Size: ' . number_format($exp_dls[$i][6] / 1048576, 1) . 'MB';
					}
				} else {
					$dl_status .= 'Error: ' . $exp_dls[$i][8];
				}
				break;
			case 'DOWNLOAD_WAITING':
				$any_download_running = true;
				$dl_status .= 'Have to wait ' . $exp_dls[$i][7] . ' seconds';
				break;
			case 'DOWNLOAD_FINISHED':
				$dl_status .= 'Download Finished';
				break;
			case 'DOWNLOAD_RECONNECTING':
				$any_download_running = true;
				$dl_status .= 'Reconnecting...';
				break;
			default:
				$dl_status .= 'Status not detected';	
		}
	
	if($exp_dls[$i][4] == "DOWNLOAD_INACTIVE") {
		$activate_button = '';
	} else {
		$activate_button = 'de';
	}
	$buf = "";
	if(CHECK_FILE_STATUS) {
		send_all($socket, "DDP FILE GETPATH " . $exp_dls[$i][0]);
		recv_all($socket, $buf);
	}
	if($buf != "" || !CHECK_FILE_STATUS) {
		$del_file = '<a href="index.php?site=manage&amp;action=del_file&amp;id={T_DL_ID}" title="{L_Delete_File}"><img src="{T_SITE_URL}/templates/default/css/images/delete_file.png" alt="{L_Delete_File}" /></a>';
	}else{
		$del_file = '';
	}
	
	$tpl_manage_vars = array(
		'T_Activate_Button' => $activate_button,
		'T_DelFile_Button' => $del_file,
		'T_TR_CLASS' => $tr_class,
		'T_PKG_ID' => '',
		'T_DL_ID' => $exp_dls[$i][0],
		'T_DL_Date' => $exp_dls[$i][1],
		'T_DL_Title' => $exp_dls[$i][2],
		'T_DL_Title_short' => substr($exp_dls[$i][2], 0, 15),
		'T_DL_URL' => $exp_dls[$i][3],
		'T_DL_URL_short' => substr($exp_dls[$i][3], 0, 20) . "..." . substr($exp_dls[$i][3], -20, 20),
		'T_DL_Class' => strtolower($exp_dls[$i][4]),
		'T_DL_Status' => $dl_status,
		'T_DL_Percent' => $percent,
		'T_SITE_URL' => $tpl_vars['T_SITE_URL'],
		'L_Activate' => $LANG['Activate'],
		'L_Delete' => $LANG['Delete'],
		'L_Move' => $LANG['Move'],
		'L_Delete_File' => $LANG['Delete_File'],
		'T_EDIT_SITE' => 'dl_edit',
	);
	if(strlen($tpl_manage_vars['T_DL_URL']) <= 42)
		$tpl_manage_vars['T_DL_URL_short'] = $tpl_manage_vars['T_DL_URL'];
	if(strlen($tpl_manage_vars['T_DL_Title']) > 15)
		$tpl_manage_vars['T_DL_Title_short'] .= "...";
	
	$dl_list .= template_parse('manage_line', $tpl_manage_vars);
}
}

$tpl_vars['L_Title'] = $LANG['Title'];
$tpl_vars['L_URL'] = $LANG['URL'];
$tpl_vars['L_ID'] = $LANG['ID'];
$tpl_vars['L_Date'] = $LANG['Date'];
$tpl_vars['L_Status'] = $LANG['Status'];
$tpl_vars['T_List'] = $dl_list;
//if(AUTO_REFRESH && $any_download_running) {
//	$tpl_vars['T_META'] = '<meta http-equiv="refresh" content="5; URL=index.php?site=manage" />';
//}

?>
