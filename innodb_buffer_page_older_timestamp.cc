#include <stdlib.h>
#include <ctype.h>
#include "sql/sql_class.h"
#include "sql/table.h"
#include "sql/field.h"
#include "mysql/plugin.h"
#include "mysql/innodb_priv.h"

static struct st_mysql_information_schema ibd_buf_page_older_timestamp_info =
	{ MYSQL_INFORMATION_SCHEMA_INTERFACE_VERSION };

static ST_FIELD_INFO ibd_buf_page_older_timestamp_fields[] = 
	{
		{"NAME", 10, MYSQL_TYPE_STRING, 0, 0, 0, 0},
		{"VALUE", 6, MYSQL_TYPE_LONG, 0, MY_I_S_UNSIGNED, 0, 0},
		{0, 0, MYSQL_TYPE_NULL, 0, 0, 0, 0} // end of field definition
	};

static int ibd_buf_page_older_timestamp_fill_table(THD *thd, TABLE_LIST *tables, Item *cond)
{
	TABLE *table = tables->table;

	table->field[0]->store("Name 1", 6, system_charset_info);
	table->field[1]->store(1);
	if (schema_table_store_record(thd, table))
		return 1;

	table->field[0]->store("Name 2", 6, system_charset_info);
	table->field[1]->store(2);
	if (schema_table_store_record(thd, table))
		return 1;

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
	NULL
	NULL,
	0x0100,
	NULL,
	NULL,
	NULL,
	0
}
mysql_declare_plugin_end;