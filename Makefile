PHP_INCLUDE = `php-config --includes`
PHP_LIBS = `php-config --libs`
PHP_LDFLAGS = `php-config --ldflags`
PHP_INCLUDE_DIR = `php-config --include-dir`
PHP_EXTENSION_DIR = `php-config --extension-dir`
PHPX_INCLUDE_DIR = "/home/shiguangqi/workspace/php-x/include"
ROCKSDB_DIR = "/usr/local/include/rocksdb"

php-rocksdb.so: rocksdb.cpp
	clang++ -lrocksdb -DHAVE_CONFIG_H -g -o php-rocksdb.so -O0 -fPIC -shared rocksdb.cpp -std=c++11 ${PHP_INCLUDE} -I${PHP_INCLUDE_DIR} -I${PHPX_INCLUDE_DIR} -I${ROCKSDB_DIR} ${PHP_LDFLAGS}
embed: embed.cpp
	clang++ -lrocksdb -DHAVE_CONFIG_H -g -o embed -O0 embed.cpp -std=c++11 ${PHP_INCLUDE} -I${PHP_INCLUDE_DIR} -I${PHPX_INCLUDE_DIR} -I${ROCKSDB_DIR} ${PHP_LDFLAGS} -lphp7 ${PHP_LIBS}
install: php-rocksdb.so
	cp php-rocksdb.so ${PHP_EXTENSION_DIR}/
clean:
	rm *.so