/**
 * @file file.h
 * @brief Handle files easier than via standard FILE
 *
 * XChangeFile can be used for an easy file edition.
 * \code
 * uint8_t *data = ...;
 * int data_len = ...;
 * uint8_t *header = ...;
 * int header_len = ...;
 *
 * XChangeFile *xf = xchange_file_open("path/myfile", "rb+");
 * if (!xf)
 * {
 * 	fprintf(stderr, "Couldn't open file!\n");
 * 	return 0;
 * }
 * if (!xchange_file_insert_bytes(xf, 0, header, header_len))
 * {
 * 	fprintf(stderr, "Couldn't insert data!\n");
 * 	return 0;
 * }
 * if (!xchange_file_replace_bytes(xf, 0x1500, data, data_len))
 * {
 * 	fprintf(stderr, "Couldn't replace data!\n");
 * 	return 0;
 * }
 * if(!xchange_file_save(xf))
 * {
 * 	fprintf(stderr, "Couldn't save file!!\n");
 * 	return 0;
 * }
 * xchange_file_close(xf);
 * return 1;
 * \endcode
 */
/***************************************************************************
 *            file.h
 *
 *  Qui dezembro 23 17:47:54 2010
 *  Copyright  2010  Rodolfo Ribeiro Gomes
 *  <rodolforg@gmail.com>
 ****************************************************************************/

#ifndef HEXCHANGE_FILE_H
#define HEXCHANGE_FILE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <sys/types.h>

struct XChangeFile;
/**
 * The obfuscated file handler.
 */
typedef struct XChangeFile XChangeFile;

/**
 * Change globally how much memory XChange can use initially for file loading.
 * If the file size is less than that, it is entirely loaded into memory.
 * Otherwise, it is not loaded to; just changes will be on memory, without these limits.
 * @param memsize Upper limit for file size in order to be loaded into memory.
 */
void xchange_file_set_max_memory_size(size_t memsize);


/**
 * Open a file for reading, creating, change it...
 * @param[in] path The file path to be opened.
 * @param[in] mode The opening mode, same options as those from standard C library fopen.
 * @return a newly allocated handler. Should be freed by xchange_file_close(). Returns NULL on failure.
 */
XChangeFile * xchange_file_open(const char *path, const char *mode);

/**
 * Save file previously opened by xchange_file_open() with its changes.
 * @param[in] xfile The XChangeFile handler.
 * @return 1 on success, 0 otherwise.
 */
int xchange_file_save(const XChangeFile * xfile);

/**
 * Save opened file with another file path, overwriting if it already exists.
 * The handler will be changed to this new file path.
 * @param[in] xfile The XChangeFile handler.
 * @param[in] path Where the changed file will be saved to.
 * @return 1 on success, 0 otherwise.
 */
int xchange_file_save_as(XChangeFile * xfile, const char *path);

/**
 * Save opened file with another file path, overwriting if it already exists.
 * The handler will remain with the previous file path.
 * @param[in] xfile The XChangeFile handler.
 * @param[in] path Where the changed file will be saved to.
 * @return 1 on success, 0 otherwise.
 */
int xchange_file_save_copy_as(const XChangeFile * xfile, const char *path);

/**
 * Close file and destroy the handler.
 * @param[in] xfile File and handler to be released.
 */
void xchange_file_close(XChangeFile * xfile);


// File information/data
/**
 * Get data from file and put it into a array.
 * If file has been changed by XChange, they'll be used even if they are not saved yet.
 * @param[in] xfile File handler.
 * @param offset From where start to copy data bytes.
 * @param[out] bytes Preallocated array which will be filled.
 * @param size Array size.
 * @return The number of bytes successfully read from file. Zero on failure.
 */
size_t xchange_file_get_bytes(const XChangeFile * xfile, off_t offset, uint8_t *bytes, size_t size);

/**
 * Retrieve the current file size.
 * If file has been changed by XChange, they'll be used even if they are not saved yet.
 * @param[in] xfile File handler.
 * @return The file size. On failure, it returns (size_t)-1.
 */
size_t xchange_file_get_size(const XChangeFile * xfile);

/**
 * Find the file position of the first byte sequence match inside a file slice.
 * @param[in] xfile The file handler.
 * @param start The start point of search.
 * @param end The end point of search. If 0, search until the end  of file.
 * @param[in] key Byte array to be searched.
 * @param key_length the key byte array length.
 * @return The offset of the first match. Return (off_t)-1 on failure or if not found.
 */
off_t xchange_file_find(const XChangeFile * xfile, off_t start, off_t end, const uint8_t *key, size_t key_length);
off_t xchange_file_find_backwards(const XChangeFile * xfile, off_t start, off_t end, const uint8_t *key, size_t key_length);

// File navigation
typedef enum
{
	XC_SEEK_SET, ///< Seek from file beginning
	XC_SEEK_CUR, ///< Seek from current file position
	XC_SEEK_END, ///< Seek from file end
} SeekBase;

/**
 * Go to a file offset.
 *
 * The offset can be absolute (by using XC_SEEK_SET) or relative (by using XC_SEEK_CUR).
 * If it is a relative displacement, offset argument can be negative.
 * If it is a seek from end of file, offset must be negative.
 *
 * This function is useful for file data readings with xchange_file_read_*() functions.
 * @param[in] xfile The file handler.
 * @param offset How much to displace the cursor in file.
 * @param base Where to seek from.
 * @return 1 on success, 0 otherwise. The function fails if offset gives an invalid value or xfile is invalid.
 */
int xchange_file_seek(XChangeFile * xfile, off_t offset, SeekBase base);

/**
 * Get the current file position used for xchange_file_read_*() functions.
 *
 * The file position is changed by xchange_file_seek() or xchange_file_read_*() calls.
 *
 * @param[in] xfile The file handler.
 * @return The current file position or -1 on failure (xfile is invalid).
 */
off_t xchange_file_position(const XChangeFile * xfile);

// Read data using native byte order
/**
 * Get a byte from file at current file position.
 *
 * The current position can be known by using xchange_file_position().
 * It can be changed by xchange_file_seek() or successive xchange_file_read_*()'ings.
 * @param xfile The file handler.
 * @return The read byte.
 */
int8_t xchange_file_read_byte(XChangeFile * xfile);
/**
 * Get a 16-bit integer from file at current file position. It uses native byte ordering.
 *
 * The current position can be known by using xchange_file_position().
 * It can be changed by xchange_file_seek() or successive xchange_file_read_*()'ings.
 * @param xfile The file handler.
 * @return The read 16-bit integer.
 */
int16_t xchange_file_read_short(XChangeFile * xfile);
/**
 * Get a 32-bit integer from file at current file position. It uses native byte ordering.
 *
 * The current position can be known by using xchange_file_position().
 * It can be changed by xchange_file_seek() or successive xchange_file_read_*()'ings.
 * @param xfile The file handler.
 * @return The read 32-bit integer.
 */
int32_t xchange_file_read_int(XChangeFile * xfile);
/**
 * Get a 64-bit integer from file at current file position. It uses native byte ordering.
 *
 * The current position can be known by using xchange_file_position().
 * It can be changed by xchange_file_seek() or successive xchange_file_read_*()'ings.
 * @param xfile The file handler.
 * @return The read 64-bit integer.
 */
int64_t xchange_file_read_long(XChangeFile * xfile);
/**
 * Get an unsigned byte from file at current file position.
 *
 * The current position can be known by using xchange_file_position().
 * It can be changed by xchange_file_seek() or successive xchange_file_read_*()'ings.
 * @param xfile The file handler.
 * @return The read unsigned byte.
 */
uint8_t xchange_file_read_ubyte(XChangeFile * xfile);
/**
 * Get a 16-bit unsigned integer from file at current file position. It uses native byte ordering.
 *
 * The current position can be known by using xchange_file_position().
 * It can be changed by xchange_file_seek() or successive xchange_file_read_*()'ings.
 * @param xfile The file handler.
 * @return The read unsigned 16-bit integer.
 */
uint16_t xchange_file_read_ushort(XChangeFile * xfile);
/**
 * Get a 32-bit unsigned integer from file at current file position. It uses native byte ordering.
 *
 * The current position can be known by using xchange_file_position().
 * It can be changed by xchange_file_seek() or successive xchange_file_read_*()'ings.
 * @param xfile The file handler.
 * @return The read unsigned 32-bit integer.
 */
uint32_t xchange_file_read_uint(XChangeFile * xfile);
/**
 * Get a 64-bit unsigned integer from file at current file position. It uses native byte ordering.
 *
 * The current position can be known by using xchange_file_position().
 * It can be changed by xchange_file_seek() or successive xchange_file_read_*()'ings.
 * @param xfile The file handler.
 * @return The read unsigned 64-bit integer.
 */
uint64_t xchange_file_read_ulong(XChangeFile * xfile);

// Read data using little-endian byte order
/**
 * Get a 16-bit integer from file at current file position. It uses little-endian as byte ordering.
 *
 * The current position can be known by using xchange_file_position().
 * It can be changed by xchange_file_seek() or successive xchange_file_read_*()'ings.
 * @param xfile The file handler.
 * @return The read 16-bit integer.
 */
int16_t xchange_file_read_le_short(XChangeFile * xfile);
/**
 * Get a 32-bit integer from file at current file position. It uses little-endian as byte ordering.
 *
 * The current position can be known by using xchange_file_position().
 * It can be changed by xchange_file_seek() or successive xchange_file_read_*()'ings.
 * @param xfile The file handler.
 * @return The read 32-bit integer.
 */
int32_t xchange_file_read_le_int(XChangeFile * xfile);
/**
 * Get a 64-bit integer from file at current file position. It uses little-endian as byte ordering.
 *
 * The current position can be known by using xchange_file_position().
 * It can be changed by xchange_file_seek() or successive xchange_file_read_*()'ings.
 * @param xfile The file handler.
 * @return The read 64-bit integer.
 */
int64_t xchange_file_read_le_long(XChangeFile * xfile);
/**
 * Get a 16-bit unsigned integer from file at current file position. It uses little-endian as byte ordering.
 *
 * The current position can be known by using xchange_file_position().
 * It can be changed by xchange_file_seek() or successive xchange_file_read_*()'ings.
 * @param xfile The file handler.
 * @return The read 16-bit unsigned integer.
 */
uint16_t xchange_file_read_le_ushort(XChangeFile * xfile);
/**
 * Get a 32-bit unsigned integer from file at current file position. It uses little-endian as byte ordering.
 *
 * The current position can be known by using xchange_file_position().
 * It can be changed by xchange_file_seek() or successive xchange_file_read_*()'ings.
 * @param xfile The file handler.
 * @return The read 32-bit unsigned integer.
 */
uint32_t xchange_file_read_le_uint(XChangeFile * xfile);
/**
 * Get a 64-bit unsigned integer from file at current file position. It uses little-endian as byte ordering.
 *
 * The current position can be known by using xchange_file_position().
 * It can be changed by xchange_file_seek() or successive xchange_file_read_*()'ings.
 * @param xfile The file handler.
 * @return The read 64-bit unsigned integer.
 */
uint64_t xchange_file_read_le_ulong(XChangeFile * xfile);

// Read data using big-endian byte order
/**
 * Get a 16-bit integer from file at current file position. It uses big-endian as byte ordering.
 *
 * The current position can be known by using xchange_file_position().
 * It can be changed by xchange_file_seek() or successive xchange_file_read_*()'ings.
 * @param xfile The file handler.
 * @return The read 16-bit integer.
 */
int16_t xchange_file_read_be_short(XChangeFile * xfile);
/**
 * Get a 32-bit integer from file at current file position. It uses big-endian as byte ordering.
 *
 * The current position can be known by using xchange_file_position().
 * It can be changed by xchange_file_seek() or successive xchange_file_read_*()'ings.
 * @param xfile The file handler.
 * @return The read 32-bit integer.
 */
int32_t xchange_file_read_be_int(XChangeFile * xfile);
/**
 * Get a 64-bit integer from file at current file position. It uses big-endian as byte ordering.
 *
 * The current position can be known by using xchange_file_position().
 * It can be changed by xchange_file_seek() or successive xchange_file_read_*()'ings.
 * @param xfile The file handler.
 * @return The read 64-bit integer.
 */
int64_t xchange_file_read_be_long(XChangeFile * xfile);
/**
 * Get a 16-bit unsigned integer from file at current file position. It uses big-endian as byte ordering.
 *
 * The current position can be known by using xchange_file_position().
 * It can be changed by xchange_file_seek() or successive xchange_file_read_*()'ings.
 * @param xfile The file handler.
 * @return The read 16-bit unsigned integer.
 */
uint16_t xchange_file_read_be_ushort(XChangeFile * xfile);
/**
 * Get a 32-bit unsigned integer from file at current file position. It uses big-endian as byte ordering.
 *
 * The current position can be known by using xchange_file_position().
 * It can be changed by xchange_file_seek() or successive xchange_file_read_*()'ings.
 * @param xfile The file handler.
 * @return The read 32-bit unsigned integer.
 */
uint32_t xchange_file_read_be_uint(XChangeFile * xfile);
/**
 * Get a 64-bit unsigned integer from file at current file position. It uses big-endian as byte ordering.
 *
 * The current position can be known by using xchange_file_position().
 * It can be changed by xchange_file_seek() or successive xchange_file_read_*()'ings.
 * @param xfile The file handler.
 * @return The read 64-bit unsigned integer.
 */
uint64_t xchange_file_read_be_ulong(XChangeFile * xfile);

// File modifications

/**
 * Replace data since offset with bytes.
 *
 * If the replacement would further beyond current file end, nothing will be done
 * and function will return failure.
 * Changes will take place on file system only after file saving.
 * @param[in] xfile The file handler.
 * @param offset Where in file start the operation.
 * @param[in] bytes The data array to be used as replacement.
 * @param size The length of bytes.
 * @return 1 if successful, 0 otherwise.
 */
int xchange_file_replace_bytes(XChangeFile * xfile, off_t offset, const uint8_t *bytes, size_t size);

/**
 * Insert size bytes in offset.
 *
 * If offset is the end of file, the data will be appended.
 * If it is beyond of it, however, a failure occurs.
 *
 * After this operation, the file size will increase in size.
 * However, these changes will remain on memory until you save the file with e.g xchange_file_save().
 *
 * @param[in] xfile The file handler.
 * @param offset File offset where the data will be inserted into.
 * @param[in] bytes Data to be inserted.
 * @param size Length of data to be inserted.
 * @return 1 if successful, 0 otherwise.
 */
int xchange_file_insert_bytes(XChangeFile * xfile, off_t offset, const uint8_t *bytes, size_t size);

/**
 * Delete some bytes from file.
 *
 * If it tries to delete data from an valid position, but it tries to delete beyond end of file,
 * the data from start offset to end of file are successfully deleted and the function returns
 * successfully.
 *
 * After this operation, the file size will be reduced.
 *
 * @param[in] xfile The file handler.
 * @param offset Where it should start deleting bytes from.
 * @param size How many bytes will be deleted.
 * @return 1 if successful, 0 otherwise.
 */
int xchange_file_delete_bytes(XChangeFile * xfile, off_t offset, size_t size);

// Modification history
/**
 * Undo the last file modification (replacement, insertion, deletion).
 * This undone modification can be redone by xchange_file_redo() if no other modification is done meanwhile.
 *
 * In order to any action can be undone, history must be enabled by xchange_file_enable_history().
 * It fails if there is no action to be undone.
 *
 * @param[in] xfile The file handler.
 * @return 1 if successful, 0 otherwise.
 */
int xchange_file_undo(XChangeFile * xfile);
/**
 * Redo the last undone file modification by xchange_file_undo().
 * It should be clear this does not repeat the last file modification.
 *
 * In order to any action can be redone, history must be enabled by xchange_file_enable_history().
 * It fails if there is no action to be redone.
 *
 * @param[in] xfile The file handler.
 * @return 1 if successful, 0 otherwise.
 */
int xchange_file_redo(XChangeFile * xfile);
/**
 * Check if there is any action to be undone by using xchange_file_undo().
 *
 * @param[in] xfile The file handler.
 * @return 1 if there is something to be undone. 0 if not.
 */
int xchange_file_has_undo(const XChangeFile * xfile);
/**
 * Check if there is any action to be redone by using xchange_file_redo().
 *
 * @param[in] xfile The file handler.
 * @return 1 if there is something to be redone. 0 if not.
 */
int xchange_file_has_redo(const XChangeFile * xfile);
/**
 * Get the number of actions that can be undone by using xchange_file_undo().
 *
 * @param[in] xfile The file handler.
 * @return the number of undo"able" actions.
 */
int xchange_file_get_undo_list_size(const XChangeFile * xfile);
/**
 * Get the number of actions that can be redone by using xchange_file_redo().
 *
 * @param[in] xfile The file handler.
 * @return the number of redo"able" actions.
 */
int xchange_file_get_redo_list_size(const XChangeFile * xfile);
/**
 * Clear the file modification history.
 * Right after calling this, there will be no action to be undone or redone.
 * Later modifications will be recorded to be undone by xchange_file_undo() if
 * history is still enabled by xchange_file_enable_history().
 *
 * This function does not disable file modification history. This should be
 * done via xchange_file_enable_history().
 *
 * @param[in] xfile The file handler.
 */
void xchange_file_clear_history(XChangeFile * xfile);
/**
 * Enable or disable file modification history recording.
 *
 * File modifications are byte replacement, insertion or deletion.
 * Every time you modify a xfile, the modifier action is stacked up.
 * You can undo it by using xchange_file_undo().
 *
 * Disabling history clear it. See xchange_file_clear_history().
 *
 * Note that history uses memory and it is unlimited.
 *
 * History is not enabled by default.
 *
 * @param[in] xfile The file handler.
 * @param enable 0 if history should be disabled. Anything else enable history.
 */
void xchange_file_enable_history(XChangeFile * xfile, int enable);
/**
 * Check if the file modification history is enabled.
 * @param[in] xfile The file handler
 * @return 1 if it is enabled, 0 if not.
 */
int xchange_file_is_history_enabled(const XChangeFile * xfile);

/**
 * Check if the file modifications does not corrupt the file itself.
 *
 * If it fails, xchange_file_save() cannot be performed because it is unsafe.
 * However, xchange_file_save_as() and xchange_file_save_copy_as() can be executed,
 * but the saved file could be wrong.
 *
 * @param[in] xfile  The file handler.
 * @return 1 if xfile is sane. 0 otherwise.
 */
int xchange_file_check_sanity(const XChangeFile * xfile);

#ifdef __cplusplus
};
#endif

#endif //HEXCHANGE_FILE_H
