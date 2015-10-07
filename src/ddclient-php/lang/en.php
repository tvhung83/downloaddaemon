<?php
$LANG = array(
// Global
	'DD' => 'DownloadDaemon',
	'Add' => 'Add Download(s)',
	'List' => 'List Downloads',
	'Manage' => 'Manage Downloads',
	'Config' => 'Configure DownloadDaemon',
	'Conf_general' => 'General Configuration',
	'Conf_premium' => 'Premium account setup',
	'Conf_reconnect' => 'Reconnect setup',
	'Logout' => 'Log Out',
	'Login' => 'Login',
	'Manager' => 'Manager',
	'Dl_edit' => 'Download properties',
	'Pkg_edit' => 'Package properties',
	'Adv_config' => 'Advanced configuration',

// Login Site
	'Host' => 'Host',
	'Port' => 'Port',
	'Password' => 'Password',
	'Encrypt' => 'Force Encryption',
	'Stay_li' => 'Stay logged in',

// Add Downloads
	'Title' => 'Title',
	'URL' => 'URL',
	'Add_single_DL' => 'Add single Download',
	'Add_multi_DL' => 'Add multiple Downloads',
	'Add_multi_DL_Desc' => '<p style="margin-left: 110px;">If you want to specify a comment for a download, enter it as:<br />http://something.aa/bb::A Fancy Title<br /></p>',
	'SUCC_ADD_SINGLE' => 'Download added successfully!',
	'SUCC_ADD_MULTI' => 'All Downloads added successfully!',
	'package' => 'Package',
	'sel_or_type' => 'Select or type',
	'L_Add_DLC' => 'Add a DLC download container',
	'L_DLCFILE' => 'Choose file',

// Manage
	'ID' => 'ID',
	'Date' => 'Date',
	'Status' => 'Status',
	'Delete' => 'Delete from list',
	'Move' => 'Move up',
	'Activate' => 'Activate / Deactivate',
	'Delete_File' => 'Delete File',
	'L_Edit' => 'Edit',
	'L_PASS_SET' => 'Extractor-password set.',

// download and package edit pages
	'L_DL_TITLE' => 'Download Title',
	'L_DL_URL' => 'Download URL',
	'L_PKG_NAME' => 'Package name',
	'L_PKG_PASS' => 'Package password',

//Config
	'Change_PWD' => 'Change Password',
	'Old_PWD' => 'Old Password:',
	'New_PWD' => 'New Password:',
	'RT_PWD' => 'Retype Password:',
	'en_di' => 'Enable / Disable',
	'gen_conf' => 'General Configuration',
	'start_end_desc' => 'You can force DownloadDaemon to only download at specific times. Therefore you can specifie a start time and 
			     an end time in the format hours:minutes.<br /> Leave this fields empty if you want to allow DownloadDaemon to download permanently.',
	'st_time' => 'Start Time:',
	'end_time' => 'End Time:',
	'dl_dir_desc' => 'This option specifies where finished downloads should be saved on the server.',
	'dl_dir' => 'Download directory:',
	'simul_dl_desc' => 'Here you can specify how many downloads may run at the same time.',
	'simul_dl' => 'Simultaneous downloads:',
	'log_level_desc' => 'This option specifies DownloadDaemons logging activity.',
	'log_level' => 'Logging activity:',
	'max_speed_desc' => 'Max. download speed (will only be used for Downloads that are not yet running).',
	'L_variable' => 'Variable',
	'L_value' => 'Value',

// Errors
	'ERR_COOKIE' => 'Cookies missing. If you have cookies disabled on your machine, please enable them. Else, try to log on again by clicking <a href="index.php">here</a>',
	'ERR_CONNECT' => 'Could not connect to Daemon.',
	'ERR_SUCCESS' => 'läuft',
	'ERR_PASSWD' => 'You entered a wrong password or used an encryption method the server does not support.',
	'ERR_URL_INVALID' => 'The Download could not be added. The URL seems to be invalid!',
	'ERR_DL_ACTIVATE' => 'The Download could not be activated.',
	'ERR_DL_DEACTIVATE' => 'The Download could not be deactivated.',
	'ERR_DL_DEL' => 'Unable to delete this download.',
	'ERR_FILE_DEL' => 'Unable to delete this file.',
	'ERR_DL_UP' => 'The Download could not be moved.',
	'ERR_PKG_OR_DL_NAM' => 'Invalid package name or Download title.',
	'ERR_EDIT' => 'Failed to save changes.',
)
?>
