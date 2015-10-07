<?php

if (getenv("HTTP_CLIENT_IP"))
	echo getenv("HTTP_CLIENT_IP");
else if(getenv("HTTP_X_FORWARDED_FOR"))
	echo getenv("HTTP_X_FORWARDED_FOR");
else if(getenv("REMOTE_ADDR"))
	echo getenv("REMOTE_ADDR"); 

?>
