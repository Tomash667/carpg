<?php

require("forum/Settings.php");

function Action()
{
	if($_SERVER['REQUEST_METHOD'] != 'POST' || $_GET['action'] != 'stats' || !isset($_POST['data']) || !isset($_POST['c']))
		return 400;
	
	$data = $_POST['data'];
	$c = $_POST['c'];
	
	if(crc32($data) != $c)
		return 400;
	
	$json = json_decode(base64_decode(strtr($data, '._-', '+/=')), true);
	if($json == null)
		return 400;
	
	$ouid = $json['ouid'];
	$uid = $json['uid'];
	$version = (int)$json['version'];
	$winver = (int)$json['winver'];
	$ram = (float)$json['ram'];
	$cpu_flags = (int)$json['cpu_flags'];
	$vs_ver = (int)$json['vs_ver'];
	$ps_ver = (int)$json['ps_ver'];
	
	if(!isset($uid) || !isset($version) || !isset($winver) || !isset($ram) || !isset($cpu_flags) || !isset($vs_ver) || !isset($ps_ver))
		return 400;
	
	if(!isset($ouid))
		$ouid = $uid;
	$ip = $_SERVER['REMOTE_ADDR'];
	
	global $db_server;
	global $db_user;
	global $db_passwd;
	global $db_name;
	
	$conn = new mysqli($db_server, $db_user, $db_passwd, $db_name);
	if($conn->connect_errno)
		return 401;
	
	$query = $conn->prepare('INSERT INTO ca_stats(ouid,uid,version,winver,ram,cpu_flags,vs_ver,ps_ver,insertdate,ip) VALUES(?,?,?,?,?,?,?,?,now(),?)');
	$ok = $query->bind_param("ssiidiiis", $ouid, $uid, $version, $winver, $ram, $cpu_flags, $vs_ver, $ps_ver, $ip);
	if($ok && $query->execute())
		$ok = true;
	
	$conn->close();
	
	return $ok ? 200 : 401;
}

$result = Action();
http_response_code($result);
die();

?>
