#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <mysql/plugin.h>
#include "sql/sql_class.h"
#include "sql/table.h"
#include "sql/field.h"
#include "mysql/innodb_priv.h"
//#include "../../storage/innobase/include/ut0mutex.h"
#include "../../storage/innobase/include/buf0lru.h"
#include "buf0buf.h"

static struct st_mysql_information_schema ibd_buf_page_older_timestamp_info =
	{ MYSQL_INFORMATION_SCHEMA_INTERFACE_VERSION };

static ST_FIELD_INFO ibd_buf_page_older_timestamp_fields[] = 
	{
		{"NAME", 10, MYSQL_TYPE_STRING, 0, 0, 0, 0},
		{"TIMESTAMP", 6, MYSQL_TYPE_LONG, 0, MY_I_S_UNSIGNED, 0, 0},
		{0, 0, MYSQL_TYPE_NULL, 0, 0, 0, 0} // end of field definition
	};

static int set_ibd_buf_page_info(THD *thd, TABLE_LIST *tables, buf_pool_t *buf_pool, const ulint pool_id)
{
	int status = 0;
	//buf_pool_info_t *info_buffer;
	buf_page_t *bpage;
	ulint lru_pos = 0;
	ulint lru_len;

	// need mutex lock
	// buf_pool_mutex_enter(buf_pool);
	lru_len = UT_LIST_GET_LEN(buf_pool->LRU);
	
	// need refactoring
	//info_buffer = (buf_pool_info_t *)my_malloc(PSI_INSTRUMENT_ME, lru_len * sizeof *info_buffer, MYF(MY_WME));
	//if (!info_buffer) {
	//	return 1;
	//}

	// need refactoring
	//memset(info_buffer, 0, lru_len * sizeof *info_buffer);
	bpage = UT_LIST_GET_LAST(buf_pool->LRU);
	int counter = 0;
	while(bpage != NULL) {
		bpage = UT_LIST_GET_PREV(LRU, bpage);
		lru_pos++;

		char str[126];
		sprintf(str, "Name %d", counter);
		counter++;
		if (counter > 10)
			break;
		tables->table->field[0]->store(str, strlen(str), system_charset_info);
		tables->table->field[1]->store(bpage->access_time);
		if (schema_table_store_record(thd, tables->table))
			continue;
	}

	tables->table->field[0]->store("total size", 6, system_charset_info);
	tables->table->field[1]->store(lru_len);
	if (schema_table_store_record(thd, tables->table))
		status = 1;

	//my_free(info_buffer);

	return status;
}

static int ibd_buf_page_older_timestamp_fill_table(THD *thd, TABLE_LIST *tables, Item *cond)
{
	TABLE *table = tables->table;
	
	int status = 0;

	for (ulint pool_id = 0; pool_id < srv_buf_pool_instances; pool_id++) {
		buf_pool_t* buf_pool;
		buf_pool = buf_pool_from_array(pool_id);
		status = set_ibd_buf_page_info(thd, tables, buf_pool, pool_id);
		if (status)
			continue;
	}

	return 0;
}

static int ibd_buf_page_older_timestamp_table_init(void *ptr)
{
	ST_SCHEMA_TABLE *schema_table = (ST_SCHEMA_TABLE *)ptr;
	schema_table->fields_info = ibd_buf_page_older_timestamp_fields;
	schema_table->fill_table = ibd_buf_page_older_timestamp_fill_table;
	return 0;
}

mysql_declare_plugin()
{
	MYSQL_INFORMATION_SCHEMA_PLUGIN,
	&ibd_buf_page_older_timestamp_info,
	"INNODB_BUFFER_PAGE_OLDER_TIMESTAMP",
	"lrf141",
	"Table with detailed timestamp information for the buffer pool",
	PLUGIN_LICENSE_GPL,
	ibd_buf_page_older_timestamp_table_init,
	NULL,
	NULL,
	0x0100,
	NULL,
	NULL,
	NULL,
	0
}
mysql_declare_plugin_end;
