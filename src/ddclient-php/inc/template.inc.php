<?php

function template_get($file){
	if(file_exists('templates/'.TEMPLATE.'/'.$file.'.htm')){
		$temp = file_get_contents('templates/'.TEMPLATE.'/'.$file.'.htm');
		return $temp;
	}else{
		return false;
	}
}

function template_parse($file, $array){
	$temp = template_get($file);
	global $LANG;
	if($temp){
		foreach ($array as $key => $value){
			$temp = str_replace('{'.$key.'}', $value, $temp);
		}
		foreach ($LANG as $key => $value){
			$temp = str_replace('{'.$key.'}', $value, $temp);
		}
	}else{
		echo '\"templates/'.TEMPLATE.'/'.$file.'.htm\" Does Not Exist.';
	}
	return $temp;
}

function template_parse_site($file, $array){
	$temp = template_get('header');
	$temp .= template_get($file);
	$temp .= template_get('footer');
	global $LANG;
	if($temp){
		foreach ($array as $key => $value){
			$temp = str_replace('{'.$key.'}', $value, $temp);
		}
		foreach ($LANG as $key => $value){
			$temp = str_replace('{'.$key.'}', $value, $temp);
		}
	}else{
		echo '\"templates/'.TEMPLATE.'/'.$file.'.htm\" Does Not Exist.';
	}
	return $temp;
}
?>
