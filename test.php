<?php
$config = array(
		'options' => array(
				'create_if_missing' => true,    // if the specified database didn't exist will create a new one
				'error_if_exists' => false,   // if the opened database exsits will throw exception
				'paranoid_checks' => false,
		),
		/* default readoptions */
		'readoptions' => array(
				'verify_check_sum' => false,
				'fill_cache' => true,
		),
		/* default write options */
		'writeoptions' => array(
				'sync' => false
		),
);
$ttl = 3;
$time = 5;
$db_name = "/data/rocksdb/test7";
$r = new RocksDB($db_name,$config['options'],$config['readoptions'],$config['writeoptions']);

$key = "kkk";
$val = "1111";
$res = $r->put($key,$val);
echo "put {$key} res:".var_export($res,1)."\n";
// $res = $r->get($key);
// echo "get {$key} res:".var_export($res,1)."\n";

// sleep($time);
// // echo "sleep {$time}s \n";
$res = $r->get($key);
echo "get {$key} again res:".var_export($res,1)."\n";
// $res = $r->delete($key);
// echo "delete {$key} res:".var_export($res,1)."\n";
// $res = $r->get($key);
// echo "after delete get {$key} res:".var_export($res,1)."\n";
// var_dump(RocksDB::$version);
