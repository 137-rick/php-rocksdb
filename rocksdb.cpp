/*
 +----------------------------------------------------------------------+
 | Swoole                                                               |
 +----------------------------------------------------------------------+
 | This source file is subject to version 2.0 of the Apache license,    |
 | that is bundled with this package in the file LICENSE, and is        |
 | available through the world-wide-web at the following url:           |
 | http://www.apache.org/licenses/LICENSE-2.0.html                      |
 | If you did not receive a copy of the Apache2.0 license and are unable|
 | to obtain it through the world-wide-web, please send a note to       |
 | license@swoole.com so we can mail you a copy immediately.            |
 +----------------------------------------------------------------------+
 | Author: Tianfeng Han  <mikan.tenny@gmail.com>                        |
 +----------------------------------------------------------------------+
 */

#include "phpx.h"
#include "rocksdb/db.h"
#include "rocksdb/utilities/db_ttl.h"

#include <iostream>

using namespace php;
using namespace std;
using namespace rocksdb;

PHPX_METHOD(rocksDB, construct) {
	auto _path = args[0];
	auto _option = args[1];
	auto _readoption = args[2];
	auto _writeoption = args[3];
	auto _read_only = args[4]; //0 default 1 readonly
	auto _ttl = args[5]; //0 default x ttl time second

	int read_only = 0;
	if (_read_only.isInt()) {
		read_only = _read_only.toInt();
	}
	int32_t ttl = 0;
	if (_ttl.isInt() && _ttl.toInt() > 0) {
		ttl = (int32_t) _ttl.toInt();
	}
	string path = _path.toString();
	if (!_option.isArray()) {
		throwException("\\Exception",
				"RocksDB construct function parameter 2 open option should be array.");
		return;
	}
	//open option 处理
	Options options;
	options.IncreaseParallelism();
	options.OptimizeLevelStyleCompaction();

	Array option(_option);
	if (option.exists("create_if_missing")) {
		options.create_if_missing = true;
	}
	if (option.exists("error_if_exists")
			&& option["error_if_exists"].toBool()) {
		options.error_if_exists = true;
	}
	if (option.exists("paranoid_checks")
			&& option["paranoid_checks"].toBool()) {
		options.paranoid_checks = true;
	}
	if (option.exists("max_open_files") && option["max_open_files"].isInt()) {
		options.max_open_files = option["max_open_files"].toInt();
	}

	Status s;
	if (ttl > 0 && read_only == 0) {
		DBWithTTL* db;
		s = DBWithTTL::Open(options, path, &db, ttl);
		auto _db = newResource("dbResource", db);
		_this.set("rocksdb", _db);
	} else if (ttl > 0 && read_only == 1) {
		DBWithTTL* db;
		s = DBWithTTL::Open(options, path, &db, ttl, true);
		auto _db = newResource("dbResource", db);
		_this.set("rocksdb", _db);
	} else if (read_only == 0) {
		DB* db;
		s = DB::Open(options, path, &db);
		auto _db = newResource("dbResource", db);
		_this.set("rocksdb", _db);
	} else {
		DB* db;
		s = DB::OpenForReadOnly(options, path, &db);
		auto _db = newResource("dbResource", db);
		_this.set("rocksdb", _db);
	}

	//抛出异常
	if (!s.ok()) {
		throwException("\\Exception", "RocksDB open failed ");
		return;
	}
	assert(s.ok());

	//WriteOptions
	WriteOptions* write_options = new WriteOptions();
	Array writeoption(_writeoption);
	if (writeoption.exists("sync") && writeoption["sync"].toBool()) {
		write_options->sync = true;
	}
	if (writeoption.exists("disableWAL")
			&& writeoption["disableWAL"].toBool()) {
		write_options->disableWAL = true;
	}
	auto _write = newResource("writeOptionsResource", write_options);
	_this.set("write_options", _write);

	//ReadOptions
	ReadOptions* read_options = new ReadOptions();
	Array readoption(_readoption);
	if (readoption.exists("verify_checksums")
			&& readoption["verify_checksums"].toBool()) {
		read_options->verify_checksums = true;
	}
	if (readoption.exists("fill_cache") && readoption["fill_cache"].toBool()) {
		read_options->fill_cache = true;
	}

	auto _read = newResource("readOptionsResource", read_options);
	_this.set("read_options", _read);
}

PHPX_METHOD(rocksDB, put) {
	//write option get
	WriteOptions* wop = _this.get("write_options").toResource<WriteOptions>(
			"writeOptionsResource");
	auto _key = args[0];
	auto _value = args[1];
	string key = _key.toString();
	string value = _value.toString();
	DB* db = _this.get("rocksdb").toResource<DB>("dbResource");
	Status s = db->Put(*wop, key, value);
	if (!s.ok()) {
		retval = false;
		throwException("\\Exception", "RocksDB put with read only mode");
	} else {
		retval = true;
	}
}

PHPX_METHOD(rocksDB, get) {
	//read option get
	ReadOptions* rop = _this.get("read_options").toResource<ReadOptions>(
			"readOptionsResource");
	DB* db = _this.get("rocksdb").toResource<DB>("dbResource");

	auto _key = args[0];
	string key = _key.toString();
	string value;
	Status s = db->Get(*rop, key, &value);
	if (!s.ok()) {
		retval = false;
	} else {
		retval = value;
	}
}

PHPX_METHOD(rocksDB, delete) {
	//write option get
	WriteOptions* wop = _this.get("write_options").toResource<WriteOptions>(
			"writeOptionsResource");
	auto _key = args[0];
	string key = _key.toString();
	DB* db = _this.get("rocksdb").toResource<DB>("dbResource");
	Status s = db->Delete(*wop, key);
	if (!s.ok()) {
		retval = false;
	} else {
		retval = true;
	}
}

static void dbResource_destory(zend_resource *res) {
	DB* db = static_cast<DB *>(res->ptr);
	delete db;
}

static void readOptionsResource_destory(zend_resource *res) {
	ReadOptions* r = static_cast<ReadOptions *>(res->ptr);
	delete r;
}

static void writeOptionsResource_destory(zend_resource *res) {
	WriteOptions* w = static_cast<WriteOptions *>(res->ptr);
	delete w;
}

PHPX_EXTENSION()
{
	Extension *extension = new Extension("phpx-rocksdb", "0.0.1");

	extension->onStart =
			[extension]() noexcept
			{
//				cout << extension->name << "startup" << endl;
				extension->registerConstant("CPP_EXT_VERSION", 1002);
				Class *c = new Class("RocksDB");
				c->addMethod("__construct", rocksDB_construct);
				c->addMethod("put" , rocksDB_put);
				c->addMethod("get" , rocksDB_get);
				c->addMethod("delete" , rocksDB_delete);

				c->addProperty("version" , "0.0.1", STATIC);

				extension->registerResource("dbResource", dbResource_destory);
				extension->registerResource("writeOptionsResource", writeOptionsResource_destory);
				extension->registerResource("readOptionsResource", readOptionsResource_destory);
				extension->registerClass(c);
			};

//    extension->onShutdown = [extension]() noexcept
//    {
//        cout << extension->name << "shutdown" << endl;
//    };
//
//    extension->onBeforeRequest = [extension]() noexcept
//    {
//        cout << extension->name << "beforeRequest" << endl;
//    };
//
//    extension->onAfterRequest = [extension]() noexcept
//    {
//        cout << extension->name << "afterRequest" << endl;
//    };
	return extension;
}
