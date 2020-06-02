# innodb_buffer_page_underuse

INFORMATION_SCHEMA plugin to aggregate unreferenced page information in the InnoDB LRU Cache

# How To Build
Must be built with the target mysql-server.

1. Download MySQL-Server source code
```bash
$ wget https://dev.mysql.com/get/Downloads/MySQL-8.0/mysql-boost-8.0.19.tar.gz
$ tar xzvf mysql-boost-8.0.19.tar.gz
$ cd mysql-8.0.19
```
2. Clone this repo clone under the plugin directory
```bash
$ cd plugin/
$ git clone git@github.com:lrf141/innodb_buffer_page_underuse.git
```
3. Build MySQL-Server
```bash
// move to mysql-server root dir
$ cd ../../
$ cmake . -DWITH_BOOST=./boost -DFORCE_INSOURCE_BUILD=1
$ make
```
