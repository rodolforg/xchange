/*
 * chartable.h
 *
 *  Created on: 25/01/2011
 *      Author: rodolfo
 */

#ifndef CHARTABLE_H_
#define CHARTABLE_H_

#include <stdint.h>
#include <stddef.h>

typedef struct XChangeTable XChangeTable;

typedef enum
{
	UNKNOWN_TABLE = 0,
	CHARSET_TABLE = 1,
	THINGY_TABLE,
} TableType;

#define XCHANGE_TABLE_UNKNOWN_ERROR	-1
#define XCHANGE_TABLE_UNKNOWN_TABLE	-2
#define XCHANGE_TABLE_UNKNOWN_SIZE	-3

XChangeTable * xchange_table_open(const char *path, TableType type, const char *encoding);
XChangeTable * xchange_table_create_from_encoding(const char *encoding);
void xchange_table_close(XChangeTable *table);

const char * xchange_table_get_encoding(const XChangeTable *table);
TableType xchange_table_get_type(const XChangeTable *table);
int xchange_table_get_size(const XChangeTable *table);
int xchange_table_get_largest_entry_length(const XChangeTable *table, int by_key);

int xchange_table_set_unknown_char(XChangeTable *table, const char* character, int length);
int xchange_table_set_lineend_mark(XChangeTable *table, const char* character, int length);
int xchange_table_set_paragraph_mark(XChangeTable *table, const char* character, int length);

int xchange_table_print_stringUTF8(const XChangeTable *table, const uint8_t *bytes, size_t size, char *text);
int xchange_table_scan_stringUTF8(const XChangeTable *table, const char *text, size_t size, uint8_t *bytes);

int xchange_table_print_best_stringUTF8(const XChangeTable *table, const uint8_t *bytes, size_t size, char *text, size_t min_read, size_t *read);

#endif /* CHARTABLE_H_ */
