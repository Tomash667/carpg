<?php

include("../forum/config.php");

/*
CREATE TABLE ca_stats
(
	id int AUTO_INCREMENT PRIMARY KEY,
	uid varchar(50) NOT NULL,
	version int NOT NULL,
	winver int NOT NULL,
	ram float NOT NULL,
	cpu_flags int NOT NULL,
	cpu_flags2 int NOT NULL,
	vs_ver int NOT NULL,
	ps_ver int NOT NULL,
	insertdate datetime NOT NULL DEFAULT GETDATE(),
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
	
	$json = json_decode(base64_decode($data), true);
	if($json == null)
		return 0;
	
	$uid = $json['uid'];
	$version = (int)$json['version'];
	$winver = (int)$json['winver'];
	$ram = (float)$json['ram'];
	$cpu_flags = (int)$json['cpu_flags'];
	$cpu_flags2 = (int)$json['cpu_flags2'];
	$vs_ver = (int)$json['vs_ver'];
	$ps_ver = (int)$json['ps_ver'];
	
	if(!isset($uid) || !isset($version) || !isset($winver) || !isset($ram) || !isset($cpu_flags) || !isset($cpu_flags2) || !isset($vs_ver) || !isset($ps_ver))
		return 0;
	
	$sse = $cpu_flags2 & (1 << 25);
	$sse2 = $cpu_flags2 & (1 << 26);
	$x64 = $cpu_flags & (1 << 29);
	
	$conn = mysql_connect($dbhost, $dbuser, $dbpass);
	if(!$conn)
		return 1;
	mysql_select_db($dbname);
	
	$ok = mysql_query("INSERT INTO ca_stats (uid,version,winver,ram,cpu_flags,cpu_flags2,vs_ver,ps_ver,sse,sse2,x64) values ('".$uid."',".$version.",".$winver.",".$ram.",".$cpu_flags.",".$cpu_flags2.",".$vs_ver.",".$ps_ver.",".$sse.",".$sse2.",".$x64.")", $conn);
	if(!$ok)
		return 1;
	
	mysql_close($conn);
	
	return 2;
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
