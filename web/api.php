<?php

require("forum/Settings.php");

/*
CREATE TABLE ca_stats
(
	id int AUTO_INCREMENT PRIMARY KEY,
	uid varchar(50) NOT NULL,
	version int NOT NULL,
	winver int NOT NULL,
	ram float NOT NULL,
	cpu_flags int NOT NULL,
	vs_ver int NOT NULL,
	ps_ver int NOT NULL,
	insertdate datetime NOT NULL,
	ip varchar(50) NOT NULL,
	sse bit NOT NULL,
	sse2 bit NOT NULL,
	x64 bit NOT NULL
)
*/

function Action()
{
	if($_SERVER['REQUEST_METHOD'] != 'POST' || $_GET['action'] != 'stats' || !isset($_POST['data']) || !isset($_POST['c']))
		return 0;
	
	$data = $_POST['data'];
	$c = $_POST['c'];
	
	if(crc32($data) != $c)
		return 0;
	
	$json = json_decode(base64_decode(strtr($data, '._-', '+/=')), true);
	if($json == null)
		return 0;
	
	$uid = $json['uid'];
	$version = (int)$json['version'];
	$winver = (int)$json['winver'];
	$ram = (float)$json['ram'];
	$cpu_flags = (int)$json['cpu_flags'];
	$vs_ver = (int)$json['vs_ver'];
	$ps_ver = (int)$json['ps_ver'];
	
	if(!isset($uid) || !isset($version) || !isset($winver) || !isset($ram) || !isset($cpu_flags) || !isset($vs_ver) || !isset($ps_ver))
		return 0;
	
	$ip = $_SERVER['REMOTE_ADDR'];
	$x64 = $cpu_flags & (1 << 0);
	$sse = $cpu_flags & (1 << 2);
	$sse2 = $cpu_flags & (1 << 3);
	
	global $db_server;
	global $db_user;
	global $db_passwd;
	global $db_name;
	
	$conn = new mysqli($db_server, $db_user, $db_passwd, $db_name);
	if($conn->connect_errno)
		return 1;
	
	$query = $conn->prepare('INSERT INTO ca_stats(uid,version,winver,ram,cpu_flags,vs_ver,ps_ver,insertdate,ip,sse,sse2,x64) VALUES(?,?,?,?,?,?,?,now(),?,?,?,?)');
	$ok = $query->bind_param("siidiiisiii", $uid, $version, $winver, $ram, $cpu_flags, $vs_ver, $ps_ver, $ip, $sse, $sse2, $x64);
	if($ok && $query->execute())
		$ok = true;
	
	$conn->close();
	
	return $ok ? 2 : 1;
}

$result = Action();
if($result == 0)
	$result = 400;
else if($result == 1)
	$result = 401;
else
	$result = 200;
http_response_code($result);
die();

?>
