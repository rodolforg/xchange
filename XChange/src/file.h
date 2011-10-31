/***************************************************************************
 *            file.h
 *
 *  Qui dezembro 23 17:47:54 2010
 *  Copyright  2010  Rodolfo Ribeiro Gomes
 *  <rodolforg@gmail.com>
 ****************************************************************************/

#ifndef HEXCHANGE_FILE_H
#define HEXCHANGE_FILE_H

#include <stdint.h>
#include <sys/types.h>

typedef struct XChangeFile XChangeFile;

void xchange_set_max_memory_size(size_t memsize);

XChangeFile * xchange_open(const char *path, const char *mode);
int xchange_save(const XChangeFile * xfile);
int xchange_save_as(XChangeFile * xfile, const char *path);
int xchange_save_copy_as(const XChangeFile * xfile, const char *path);
void xchange_close(XChangeFile * xfile);

// File information/data
size_t xchange_get_bytes(const XChangeFile * xfile, off_t offset, uint8_t *bytes, size_t size);
size_t xchange_get_size(const XChangeFile * xfile);
off_t xchange_find(const XChangeFile * xfile, off_t start, off_t end, const uint8_t *key, size_t key_length);

// File navigation
typedef enum
{
	XC_SEEK_SET,
	XC_SEEK_CUR,
	XC_SEEK_END
} SeekBase;
int xchange_seek(XChangeFile * xfile, off_t offset, SeekBase base);
off_t xchange_position(const XChangeFile * xfile);

// Read data using native byte order
int8_t xchange_readByte(XChangeFile * xfile);
int16_t xchange_readShort(XChangeFile * xfile);
int32_t xchange_readInt(XChangeFile * xfile);
int64_t xchange_readLong(XChangeFile * xfile);
uint8_t xchange_readUByte(XChangeFile * xfile);
uint16_t xchange_readUShort(XChangeFile * xfile);
uint32_t xchange_readUInt(XChangeFile * xfile);
uint64_t xchange_readULong(XChangeFile * xfile);

// Read data using little-endian byte order
int16_t xchange_readLEShort(XChangeFile * xfile);
int32_t xchange_readLEInt(XChangeFile * xfile);
int64_t xchange_readLELong(XChangeFile * xfile);
uint16_t xchange_readLEUShort(XChangeFile * xfile);
uint32_t xchange_readLEUInt(XChangeFile * xfile);
uint64_t xchange_readLEULong(XChangeFile * xfile);

// Read data using big-endian byte order
int16_t xchange_readBEShort(XChangeFile * xfile);
int32_t xchange_readBEInt(XChangeFile * xfile);
int64_t xchange_readBELong(XChangeFile * xfile);
uint16_t xchange_readBEUShort(XChangeFile * xfile);
uint32_t xchange_readBEUInt(XChangeFile * xfile);
uint64_t xchange_readBEULong(XChangeFile * xfile);

// File modifications
int xchange_replace_bytes(XChangeFile * xfile, off_t offset, const uint8_t *bytes, size_t size);
int xchange_insert_bytes(XChangeFile * xfile, off_t offset, const uint8_t *bytes, size_t size);
int xchange_delete_bytes(XChangeFile * xfile, off_t offset, size_t size);

// Modification history
int xchange_undo(XChangeFile * xfile);
int xchange_redo(XChangeFile * xfile);
int xchange_has_undo(const XChangeFile * xfile);
int xchange_has_redo(const XChangeFile * xfile);
int xchange_get_undo_list_size(const XChangeFile * xfile);
int xchange_has_redo_list_size(const XChangeFile * xfile);
void xchange_clear_history(XChangeFile * xfile);
void xchange_enable_history(XChangeFile * xfile, int enable);

int xchange_check_sanity(const XChangeFile * xfile);

#endif //HEXCHANGE_FILE_H
