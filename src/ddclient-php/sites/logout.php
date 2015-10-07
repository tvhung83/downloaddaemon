<?php
$past = time() - 100;

setcookie("ddclient_host", "", $past);
setcookie("ddclient_port", "", $past);
setcookie("ddclient_passwd", "", $past);
setcookie("ddclient_enc", "", $past);
header("Location: index.php");
?>