<?php

function recv_all(&$socket, &$recv_buf) {
	
	$recv_buf = "";
	$recv_number = "";
	while(true) {
		if(!@socket_recv($socket, $recv_buf, 1,  0)) {
			return false;
		}
		if($recv_buf == ":") break;
		$recv_number = $recv_number . $recv_buf;
	}
	$recv_buf = "";
	$received_bytes = 0;
	$temp_recv = "";

	// receive all
	while(($received_bytes += socket_recv($socket, $temp_recv, (int)$recv_number - $received_bytes, 0)) < $recv_number) {
		$recv_buf .= $temp_recv;
		if($received_bytes < 0) {
			return false;
		}
	}
	// append the rest
	$recv_buf .= $temp_recv;
	return true;
	
}

function send_all(&$socket, $snd) {
	$to_send = strlen($snd) . ":" . $snd;
	if(@socket_send($socket, $to_send, strlen($to_send), 0)) {
		return true;
	}
	return false;
	
}


function connect_to_daemon(&$socket, $host, $port, $pwd, $enc, $timeout = 0) {
	set_time_limit($timeout);

	if(!@socket_connect($socket, $host, $port)) {
		return "ERR_CONNECT";
	}
	
	@recv_all($socket, $recv_buf);

	if(substr($recv_buf, 0, 3) == "100") {
		return "SUCCESS";
	}

	@send_all($socket, "ENCRYPT");
	$rnd = "";
	@recv_all($socket, $rnd);
	$result = "";
	if(substr($rnd, 0, 3) != "102") {
		$rnd .= $pwd;
		$enc_passwd = md5($rnd, true);
		send_all($socket, $enc_passwd);
		recv_all($socket, $result);
	} else {
		// use plain-text if allowed
		if($enc == "0") {
			socket_close($socket);
			$socket = socket_create(AF_INET, SOCK_STREAM, 0);
			if(!@socket_connect($socket, $host, $port)) {
				return "ERR_CONNECT";
			}
			recv_all($socket, $recv_buf);
			send_all($socket, $pw);
			recv_all($socket, $result);
		}
	}
	
	if(substr($result, 0, 3) != "100") {
		return "ERR_PASSWD";
	} else {
		return "SUCCESS";
	}
}

function msg_generate($msg, $type) {
	if($msg != '') {
		return '<div id="'.$type.'">'.$msg.'</div>';
	}
}


function explode_escaped( $delimiter, $string ) {
	$string = str_replace( '\\' . $delimiter, urlencode( $delimiter ), $string );
	return array_map( 'urldecode', explode( $delimiter, $string ) );
}

?>
