# innodb_buffer_page_underuse

INFORMATION_SCHEMA plugin to aggregate unreferenced page information in the InnoDB Old Sublist.

# Details
INFORMATION_SCHEMA.INNODB_BUFFER_PAGE_LRU to It is possible to use a buffer pool to obtain detailed information about a page in the buffer pool.  
However, SELECTing the table would only free up 64 bytes of memory to Required. Therefore, it is not recommended to run it even in the official documentation.  

Difficult to hit the table to see how long a never-used file in the buffer pool, especially in Old Sublist, has existed.  
So I developed the INFORMATION_SCHEMA Plugin to get only records with IS_OLD = 'YES' and OLDEST_MODIFICATION = 0, with minimal information.  
The memory usage is roughly less than 16 bytes * 3/8 * ActivePage.  

# INNODB_BUFFER_PAGE_UNDERUSE column desc

```
mysql> desc information_schema.innodb_buffer_page_underuse;
+-----------+--------------+------+-----+---------+-------+
| Field     | Type         | Null | Key | Default | Extra |
+-----------+--------------+------+-----+---------+-------+
| POOL_ID   | int unsigned | NO   |     |         |       |
| SPACE_ID  | int unsigned | NO   |     |         |       |
| PAGE_TYPE | varchar(28)  | NO   |     |         |       |
| TIMESTAMP | int unsigned | NO   |     |         |       |
+-----------+--------------+------+-----+---------+-------+
4 rows in set (0.01 sec)

```
(TIMESTAMP: unix time (sec))

# Example
```
mysql> select pool_id, space_id, page_type, FROM_UNIXTIME(timestamp) from information_schema.innodb_buffer_page_underuse where timestamp != 0 limit 10;
+---------+------------+------------+--------------------------+
| pool_id | space_id   | page_type  | FROM_UNIXTIME(timestamp) |
+---------+------------+------------+--------------------------+
|       0 |          0 | SYSTEM     | 2020-06-05 09:21:32      |
|       0 |          0 | INODE      | 2020-06-05 09:21:32      |
|       0 |          0 | INODE      | 2020-06-05 09:21:32      |
|       0 |          0 | SYSTEM     | 2020-06-05 09:21:32      |
|       0 | 4294967279 | RSEG_ARRAY | 2020-06-05 09:21:32      |
|       0 | 4294967279 | UNDO_LOG   | 2020-06-05 09:21:32      |
|       0 | 4294967279 | UNDO_LOG   | 2020-06-05 09:21:32      |
|       0 | 4294967279 | UNDO_LOG   | 2020-06-05 09:21:32      |
|       0 | 4294967279 | UNDO_LOG   | 2020-06-05 09:21:32      |
|       0 | 4294967279 | UNDO_LOG   | 2020-06-05 09:21:32      |
+---------+------------+------------+--------------------------+
10 rows in set (0.00 sec)
```

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

# How To Install

1. Check MySQL Plugin Dir

```
mysql> show variables like 'plugin_dir';
+---------------+----------------------------------------------+
| Variable_name | Value                                        |
+---------------+----------------------------------------------+
| plugin_dir    | /path/to/plugin/dir/                         |
+---------------+----------------------------------------------+
1 row in set (0.01 sec)
```

2. Move Plugin Shared-Object File

```
$ cp path/to/mysql-server/plugin_output_directory/innodb_buffer_page_underuse.so /path/to/plugin/dir/innodb_buffer_page_underuse.so
```

3. Install Plugin
```
mysql> install plugin innodb_buffer_page_underuse soname 'innodb_buffer_page_underuse.so';
```

4. Check INFORMATION_SCHEMA table
```
mysql> use INFORMATION_SCHEMA;
Reading table information for completion of table and column names
You can turn off this feature to get a quicker startup with -A

Database changed
mysql> show tables like 'innodb_buffer_page_underuse';
+------------------------------------------------------------+
| Tables_in_information_schema (INNODB_BUFFER_PAGE_UNDERUSE) |
+------------------------------------------------------------+
| INNODB_BUFFER_PAGE_UNDERUSE                                |
+------------------------------------------------------------+
1 row in set (0.00 sec)
```
