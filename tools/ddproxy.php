<?php
/*
* Welcome to ddproxy.php. If you want to use DownloadDaemon's proxy-alternation feature without actually having access to a real proxy server
* and without installing one to your personal root server (if you even have one), you have come to the right place.
* ddproxy.php is a simple php-script that can be used on any PHP-enabled webserver with the PHP-Curl-extension. There are no known drawbacks from using this script instead
* of a real proxy. When DownloadDaemon starts a download through ddproxy.php, the script will be a thin layer between the target-host and DownloadDaemon to hide DownloadDaemon's IP.
* So how to set this up?
*     It's pretty simple: Just put this script on a PHP/cURL enabled web-server. If you want to test if the script works properly, open this address in your webbrowser
*     http://<yourhost.tld>/<path to>/ddproxy.php?ddproxy_pass=secret&ddproxy_url=http%3A%2F%2Fdownloaddaemon.sourceforge.net%2Fimg%2Fteufelchen_web.png
*     If this will bring up the DownloadDaemon banner, you are ready to go. But first of all, set a proxy-password so nobody else can use your script for evil things.
*     To do so, replace the word secret in $proxy_password = "secret"; right below this comment. (If you set the password before opening the above test-link, you have to
*     replace the word "secret" in the test-link, too).
*     If all this is done, set up a proxy-line in ddclient-gui with this format: http://<proxy password>@<yourhost.tld>/<path to>/ddproxy.php
*     If you don't use ddclient-gui, you can also stop the DownloadDaemon service and manually put that line into the /etc/downloaddaemon/downloaddaemon.conf file
*     (the configuration variable is called proxy_list).
*/

// Replace this with the proxy-password you want to use so nobody can abuse your proxy-script
$proxy_password = "secret";

$url;
$headers  = getHeaders();;
$postdata = $_POST;
$getdata  = $_GET;

if(isset($_GET['ddproxy_url']))
	$url = $_GET['ddproxy_url'];
else
	die("PROXY_ERROR");


if(strpos($url, "//127.") !== false || strpos($url, "//192.168.") !== false || strpos($url, "//10.") !== false || strpos($url, "//172.16") !== false) { // block local ip-ranges for security reason
	die("PROXY_ERROR");
}


// set the content-disposition header to make it possible for the client to detect the filename:
$filename = $url;
$filename = substr($filename, strrpos($filename, "/") + 1);
$pos = strrpos($filename, "?");
if($pos !== false)
	$filename = substr($filename, 0, $pos);
	

	

if(!isset($_GET['ddproxy_pass']) || $_GET['ddproxy_pass'] != $proxy_password || $proxy_password == "") {
	die("PROXY_ERROR");
}
unset($_GET['ddproxy_pass']);

function getHeaders() {
	$headers = array();
	foreach ($_SERVER as $k => $v) {
		if (substr($k, 0, 5) == "HTTP_") {
			$k = str_replace('_', ' ', substr($k, 5));
			$k = str_replace(' ', '-', ucwords(strtolower($k)));
			if ($k == "Host") continue;
			$headers[] = $k . ": " . $v;
		}
	}
	return $headers;
}

function writefkt($res, $data) {
	echo ($data);
	return strlen($data);
}



function headerfkt($res, $data) {
	$retsize = strlen($data);
	global $proxy_password;
	global $url;
	global $filename;
	static $have_content_disposition = false;
	$data = trim($data);
	
	if(strpos($data, "Transfer-Encoding:") === 0) return $retsize;
	
	$retc = curl_getinfo($res, CURLINFO_HTTP_CODE); // return the http-status back to the client
	if($retc != "" && $retc != false && $retc != 200)
		header("x", true, $retc);
	
	$pos = strpos($data, "Location: ");
	if($pos !== false) {	// rewrite the Location: to use the proxy-url
		$loc = trim(substr($data, $pos + 10));
		if(strpos($loc, "http") !== 0) {
			// location-header is recursive... we have to make it absolute.
			if($loc[0] != "/") { // recursive to current dir
				$loc = substr($url, 0, strrpos($url, "/")) . "/" . $loc;
			} else { // recursive to server-root
				$ploc = parse_url($url);
				$userdata = "";
				$portstr = "";
				if(isset($ploc['user'])) {
					$userdata = $ploc['user'] . ":" . $ploc['pass'] . "@";
				}
				if(isset($ploc['port'])) {
					$portstr = ":" . $ploc['port'];
				}
				$loc = $ploc['scheme'] . "://" . $userdata . $ploc['host'] . $portstr . $loc;
				
			}

		}
		$data = "Location: http://" . $_SERVER['HTTP_HOST']. $_SERVER['PHP_SELF'] . "?ddproxy_url=" . urlencode($loc) . "&ddproxy_pass=" . $proxy_password;
	} elseif(strpos($data, "Set-Cookie: ") === 0) {
			// we have to rewrite the cookie-domain to represet the proxy-host so DD's libcurl supplies the cookies again
			$urldata = parse_url($url);
			$host = $urldata['host'];
			$data = str_replace($host, $_SERVER['HTTP_HOST'], $data);
			if(strpos($host, "www.") !== false)
				$data = str_replace(substr($host, 4), $_SERVER['HTTP_HOST'], $data);
	} elseif(strpos($data, "Content-Disposition:") === 0) {
		$have_content_disposition = true;
		if(strpos($data, "filename=") === false)
			$data .= "; filename=\"" . $filename . "\"";
	} elseif(strlen($data) <= 2 && !$have_content_disposition && strlen($filename) > 0) { // last header line, add content-disposition if neccessary 
		header("Content-Disposition: attachment; filename=\"" . $filename . "\"");
		$data = "";
	}
	
	

	if(strlen($data) > 0)
		header($data);
	
	
	return $retsize;
}


$url = str_replace(" ", "%20", $url);


if(isset($headers['Host']))
	unset($headers['Host']);

unset($getdata['ddproxy_url']);
unset($getdata['ddproxy_pass']);


$session = curl_init($url);
curl_setopt($session, CURLOPT_HTTPHEADER, $headers);


if (count($postdata) > 0) {

	curl_setopt($session, CURLOPT_POST, TRUE);
	$poststr = "";
	foreach($postdata as $k => $v) {
		$poststr .= str_replace("_", ".", $k) . "=" . $v . "&";
	}
	$poststr = substr($poststr, 0, -1);
	curl_setopt($session, CURLOPT_POSTFIELDS, $poststr);

}
curl_setopt($session, CURLOPT_HEADERFUNCTION, "headerfkt");
curl_setopt($session, CURLOPT_FOLLOWLOCATION, false);
curl_setopt($session, CURLOPT_WRITEFUNCTION , "writefkt");
//print_r($postdata);


curl_exec($session);
$httpcode = curl_getinfo($session, CURLINFO_HTTP_CODE);



curl_close($session);



die();

?>

