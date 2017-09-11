<?php

function Action()
{
	if($_SERVER['REQUEST_METHOD'] != 'POST' || $_GET['action'] != 'stats' || !isset($_POST['data']))
		return false;

	$data = $_POST['data'];
	$json = json_decode($data);
	if($json == null)
		return false;
	
	return true;
}

$result = Action();
http_response_code($action ? 200 : 400);
die();

?>
