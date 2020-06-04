#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <mysql/plugin.h>
#include "sql/sql_class.h"
#include "sql/table.h"
#include "sql/field.h"
#include "mysql/innodb_priv.h"
#include "buf0buf.h"
#include "buf0lru.h"

constexpr auto PAGE_TYPE_BITS = 6;
constexpr auto PAGE_TYPE_UNKNOWN = FIL_PAGE_TYPE_UNKNOWN;

static struct st_mysql_information_schema ibd_buf_page_underuse =
	{ MYSQL_INFORMATION_SCHEMA_INTERFACE_VERSION };

static ST_FIELD_INFO ibd_buf_page_underuse_fields[] = 
	{
		{"POOL ID", 6, MYSQL_TYPE_LONG, 0, MY_I_S_UNSIGNED, 0, 0},
		{"SPACE ID", 6, MYSQL_TYPE_LONG, 0, MY_I_S_UNSIGNED, 0, 0},
		{"PAGE TYPE", 6, MYSQL_TYPE_LONG, 0, MY_I_S_UNSIGNED, 0, 0},
		{"TIMESTAMP", 6, MYSQL_TYPE_LONG, 0, MY_I_S_UNSIGNED, 0, 0},
		{0, 0, MYSQL_TYPE_NULL, 0, 0, 0, 0} // end of field definition
	};

struct buf_page_info_t {
	unsigned int pool_id : 32;
	unsigned int space_id : 32;
	unsigned int page_type : PAGE_TYPE_BITS;
	unsigned int num_recs : UNIV_PAGE_SIZE_SHIFT_MAX;
	unsigned int access_time;
};

static void ibd_buffer_page_get_underuse_info(
		const buf_page_t *bpage, 
		ulint pool_id, 
		buf_page_info_t *page_info)
{
	BPageMutex *mutex = buf_page_get_mutex(bpage);
	page_info->access_time = bpage->access_time;	
	page_info->pool_id = pool_id;

	mutex_enter(mutex);

	unsigned int page_state = buf_page_get_state(bpage);

	if (buf_page_in_file(bpage)) {
		const byte *frame;
		ulint page_type;

		page_info->space_id = bpage->id.space();
		page_info->access_time = bpage->access_time;

		switch(buf_page_get_io_fix(bpage)) {
			case BUF_IO_NONE:
			case BUF_IO_WRITE:
			case BUF_IO_PIN:
				break;
			case BUF_IO_READ:
				page_info->page_type = PAGE_TYPE_UNKNOWN;
			        mutex_exit(mutex);
				return;	
		}

		if (page_state == BUF_BLOCK_FILE_PAGE) {
			const buf_block_t *block;
			block = reinterpret_cast<const buf_block_t *>(bpage);
			frame = block->frame;
		} else {
			ut_ad(bpage->zip.ssize);
			frame = bpage->zip.data;
		}

		page_type = fil_page_get_type(frame);
		page_info->page_type = page_type;
	} else {
		page_info->page_type = PAGE_TYPE_UNKNOWN;
	}
	mutex_exit(mutex);
}

static int set_i_s_tables(THD *thd, TABLE *table, buf_page_info_t *info_page)
{
	table->field[0]->store(info_page->pool_id);
	table->field[1]->store(info_page->space_id);
	table->field[2]->store(info_page->page_type);
	table->field[3]->store(info_page->access_time);
	
	if (schema_table_store_record(thd, table))
		return 1;

	return 0;
}

static int set_ibd_buf_page_info(
		THD *thd, TABLE_LIST *tables, 
		buf_pool_t *buf_pool, 
		const ulint pool_id)
{
	int status = 0;
	buf_page_info_t info_buffer;
	buf_page_t *bpage;
	ulint lru_len;

	mutex_enter(&buf_pool->LRU_list_mutex);
	
	lru_len = UT_LIST_GET_LEN(buf_pool->LRU);
	
	bpage = UT_LIST_GET_LAST(buf_pool->LRU);
	while(bpage != NULL || bpage->old != 0) {
		
		if (bpage->oldest_modification != 0) {
			bpage = UT_LIST_GET_PREV(LRU, bpage);
			continue;
		}

		if (bpage->old == 0)
			break;

		memset(&info_buffer, 0, sizeof(info_buffer));
		ibd_buffer_page_get_underuse_info(bpage, pool_id, &info_buffer);	
		bpage = UT_LIST_GET_PREV(LRU, bpage);
		if (set_i_s_tables(thd, tables->table, &info_buffer))
			continue;
	}

	if (schema_table_store_record(thd, tables->table))
		status = 1;

	mutex_exit(&buf_pool->LRU_list_mutex);

	return status;
}

static int ibd_buf_page_underuse_fill_table(
		THD *thd, 
		TABLE_LIST *tables, 
		Item *cond)
{
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

static int ibd_buf_page_underuse_table_init(void *ptr)
{
	ST_SCHEMA_TABLE *schema_table = (ST_SCHEMA_TABLE *)ptr;
	schema_table->fields_info = ibd_buf_page_underuse_fields;
	schema_table->fill_table = ibd_buf_page_underuse_fill_table;
	return 0;
}

mysql_declare_plugin()
{
	MYSQL_INFORMATION_SCHEMA_PLUGIN,
	&ibd_buf_page_underuse,
	"INNODB_BUFFER_PAGE_UNDERUSE",
	"lrf141",
	"Table with detailed information for the buffer page in old sublist in buffer pool",
	PLUGIN_LICENSE_GPL,
	ibd_buf_page_underuse_table_init,
	NULL,
	NULL,
	0x0100,
	NULL,
	NULL,
	NULL,
	0
}
mysql_declare_plugin_end;
