/**
 * @file chartable.h
 * @brief TODO
 */
/*
 * chartable.h
 *
 *  Created on: 25/01/2011
 *      Author: rodolfo
 */

#ifndef HEXCHANGE_CHARTABLE_H
#define HEXCHANGE_CHARTABLE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

struct XChangeTable;
/**
 * The obfuscated character table handler.
 */
typedef struct XChangeTable XChangeTable;

/**
 * Character table types
 */
typedef enum
{
	UNKNOWN_TABLE = 0, ///< Unknown table format
	CHARSET_TABLE = 1, ///< A known and standard character set, eg. ISO-88591, UTF-8, ASCII, SHIFT-JIS, etc
	THINGY_TABLE = 2,  ///< Custom table, based on a file using Thingy table syntax
} TableType;

#define XCHANGE_TABLE_UNKNOWN_ERROR	-1
#define XCHANGE_TABLE_UNKNOWN_TABLE	-2 ///< Table type is unknown.
#define XCHANGE_TABLE_UNKNOWN_SIZE	-3

/**
 * Create a table handler from a character table file.
 * The file encoding should be passed. If NULL or empty, it tries to detect the encoding, but not too hard.
 *
 * @param[in] path The table file path
 * @param type The table type
 * @param[in] encoding The encoding used by table file. It's should be ASCII encoded. Can be NULL for autodetection.
 * @return The table handler. It should be freed by xchange_table_close().
 */
XChangeTable * xchange_table_open(const char *path, TableType type, const char *encoding);
/**
 * Create a table handler based on a standard character set, like UTF-8 and EUC-JP.
 * It supports every charset the iconv library does.
 * @param[in] encoding The character encoding name. It's should be ASCII encoded.
 * @return The table handler. It should be freed by xchange_table_close().
 */
XChangeTable * xchange_table_create_from_encoding(const char *encoding);
/**
 * Free a table handler properly.
 * @param[in] table The table handler.
 */
void xchange_table_close(XChangeTable *table);

/**
 * If the table was created from a table file, returns the used (or detected) file encoding.
 * If it is a table based on a standard charset (its type is CHARSET_TABLE), return its name.
 * @param[in] table The table handler.
 * @return The used character encoding name.
 */
const char * xchange_table_get_encoding(const XChangeTable *table);
/**
 * Get the table type set on handler creation.
 * @param[in] table The table handler.
 * @return The table type. If it's a NULL handler, return XCHANGE_TABLE_UNKNOWN_ERROR.
 */
TableType xchange_table_get_type(const XChangeTable *table);
/**
 * Get the number of entries of the table. it cannot be used with charset tables.
 * @param[in] table The table handler.
 * @return Number of entries in the table. Return XCHANGE_TABLE_UNKNOWN_SIZE if it is a CHARSET_TABLE type.
 * 			If it's a NULL handler, return XCHANGE_TABLE_UNKNOWN_ERROR.
 */
int xchange_table_get_size(const XChangeTable *table);
int xchange_table_get_largest_entry_length(const XChangeTable *table, int by_key);

/**
 * Set how a not-found table entry is represented.
 * There are two ways:
 * - cloak every unknown byte: they'll be replaced by string set by xchange_table_set_unknown_charUTF8().
 * - use a byte escape: the unknown byte has its value shown using hexadecimal notation, e.g. <$0E>.
 * You can change the escape pattern by using xchange_table_set_byte_escape_patternUTF8().
 *
 * @param[in] table The table handler.
 * @param use_byte_escape 0 if it must use byte value escapes. Any other value, otherwise.
 * @return 1 on success, 0 on failure. The only way to fail is using an invalid handler.
 */
int xchange_table_use_unknown_byte_as_byte_escape(XChangeTable *table, int use_byte_escape);

/**
 * Get the current setting of how to represent a not-found entry.
 * See xchange_table_use_unknown_byte_as_byte_escape().
 * @param[in] table The table handler.
 * @return 0 if it doesn't use the byte value escape method. Any other value, otherwise.
 */
int xchange_table_is_using_byte_escape(XChangeTable *table);
/**
 * Set a not-found table entry to be represented as a byte escape sequence, e.g <$9A>.
 * @param[in] table The table handler.
 * @param[in] start_string The UTF-8 encoded prefix for the hexadecimal value, e.g. "<$". Cannot be NULL.
 * @param start_length The UTF-8 prefix byte length. -1 if start_string is NULL terminated.
 * @param[in] end_string The UTF-8 encoded suffix for the hexadecimal value, e.g. ">". Can be NULL.
 * @param end_length The UTF-8 suffix byte length. -1 if start_string is NULL terminated.
 * @return 1 on success, 0 otherwise.
 */
int xchange_table_set_byte_escape_patternUTF8(XChangeTable *table, const char* start_string, int start_length, const char* end_string, int end_length);
/**
 * Set how a not-found table entry is represented.
 *
 * If there is no conversion possible to a byte string (whose length can be from xchange_table_get_largest_entry_length() to 1),
 * the first byte of string will be represented by this "character" string.
 *
 * @param[in] table The table handler.
 * @param[in] character A UTF-8 encoded string to be used as a representation for a not-found entry.
 * @param length Length of "character" string argument.
 * @return 1 if successful, 0 otherwise.
 */
int xchange_table_set_unknown_charUTF8(XChangeTable *table, const char* character, int length);
/**
 * Set the character string to represent the line-end entry.
 *
 * The table must have a line-end entry.
 * @param[in] table The table handler.
 * @param[in] character A UTF-8 encoded string to be used as a representation for a line-end entry.
 * @param length Length of "character" string argument.
 * @return 1 if successful, 0 otherwise.
 */
int xchange_table_set_lineend_markUTF8(XChangeTable *table, const char* character, int length);
/**
 * Set the character string to represent the paragraph-end entry.
 *
 * The table must have a paragraph-end entry.
 * @param[in] table The table handler.
 * @param[in] character A UTF-8 encoded string to be used as a representation for a paragraph-end entry.
 * @param length Length of "character" string argument.
 * @return 1 if successful, 0 otherwise.
 */
int xchange_table_set_paragraph_markUTF8(XChangeTable *table, const char* character, int length);
const unsigned char* xchange_table_get_unknown_charUTF8(XChangeTable *table, int *length);
const uint8_t* xchange_table_get_lineend_markUTF8(XChangeTable *table, int *length);
const uint8_t* xchange_table_get_paragraph_markUTF8(XChangeTable *table, int *length);

/**
 * Convert a binary data into a UTF-8 encoded character string.
 * The string will be written in "text" argument and it is NOT 0-terminated.
 *
 * If "text" argument is NULL, the function just compute the needed space for converted text.
 *
 * @param[in] table The table handler.
 * @param[in] bytes The binary data array to be converted.
 * @param size The data array size to be converted.
 * @param[out] text A pre-allocated array to put the generated UTF-8 string, or NULL for just length computation.
 * @return The output length (in bytes). -1 on error.
 */
int xchange_table_print_stringUTF8(const XChangeTable *table, const uint8_t *bytes, size_t size, char *text);
/**
 * Convert a binary data into a character string.
 * The output encode used is that one specified/detected on the loaded table file, or the specified one if it is a standard charset table type .
 * The string will be written in "text" argument and it is NOT 0-terminated.
 *
 * If "text" argument is NULL, the function just compute the needed space for converted text.
 *
 * @param[in] table The table handler.
 * @param[in] bytes The binary data array to be converted.
 * @param size The data array size to be converted.
 * @param[out] text A pre-allocated array to put the generated string, or NULL for just length computation.
 * @return The output length (in bytes). -1 on error.
 */
int xchange_table_print_string(const XChangeTable *table, const uint8_t *bytes, size_t size, char *text);
/**
 * Convert a UTF-8 encoded character string into a binary string.
 * The character string may not be NULL-terminated.
 *
 * If the bytes array pointer is NULL, the function will just computed the needed space on it.
 *
 * @param[in] table The table handler.
 * @param[in] text The UTF-8 encoded string to be converted.
 * @param size The text byte-size to be converted.
 * @param[out] bytes A pre-allocated array to put the generated binary data, or NULL for just length computation.
 * @return The output length (in bytes). On error, return a number less than 0.
 */
int xchange_table_scan_stringUTF8(const XChangeTable *table, const char *text, size_t size, uint8_t *bytes);
/**
 * Convert a character string into a binary string.
 * The character string may not be NULL-terminated and its encoding must be the same as the table.
 *
 * If the bytes array pointer is NULL, the function will just computed the needed space on it.
 *
 * @param[in] table The table handler.
 * @param[in] text The string to be converted.
 * @param size The text byte-size to be converted.
 * @param[out] bytes A pre-allocated array to put the generated binary data, or NULL for just length computation.
 * @return The output length (in bytes). On error, return a number less than 0.
 */
int xchange_table_scan_string(const XChangeTable *table, const char *text, size_t size, uint8_t *bytes);

int xchange_table_print_best_stringUTF8(const XChangeTable *table, const uint8_t *bytes, size_t size, char *text, size_t min_read, size_t *read);

#ifdef __cplusplus
}
#endif

#endif /* HEXCHANGE_CHARTABLE_H */
