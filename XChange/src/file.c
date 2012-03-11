/***************************************************************************
 *            file.c
 *
 *  Qui dezembro 23 17:57:11 2010
 *  Copyright  2010  Rodolfo Ribeiro Gomes
 *  <rodolforg@gmail.com>
 ****************************************************************************/

#include "file.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Decidir limite de memória
size_t MAXFULLFILEBUFFERSIZE=30*1024*1024;

void xchange_file_set_max_memory_size(size_t memsize)
{
	MAXFULLFILEBUFFERSIZE = memsize;
}

////////////////
// File sections
////////////////

enum FileSectionType
{
	XCF_SECTION_MEMORY,
	XCF_SECTION_FILE
};

typedef struct FileSection
{
	enum FileSectionType type;
	size_t size;
	off_t offset;
	union
	{
		unsigned char *data;
		off_t f_offset;
	} data;
	struct FileSection *next;
} FileSection;

static FileSection * xchange_new_section(enum FileSectionType type, size_t size, off_t offset)
{
	FileSection * section = malloc(sizeof(FileSection));
	if (section == NULL)
		return NULL;

	section->type = type;
	section->size = size;
	section->offset = offset;

	if (type == XCF_SECTION_MEMORY)
	{
		section->data.data = size != 0? malloc(size) : NULL;
		if (section->data.data == NULL && size != 0)
		{
			free(section);
			return NULL;
		}
	}
	else
		section->data.f_offset = 0;

	section->next = NULL;

	return section;
}

static FileSection * xchange_new_memory_section(const uint8_t *data, size_t size, off_t offset)
{
	FileSection * section = xchange_new_section(XCF_SECTION_MEMORY, size, offset);
	if (section == NULL)
		return NULL;

	memcpy(section->data.data, data, size);

	return section;
}

static void xchange_destroy_section(FileSection * xfs)
{
	if (xfs == NULL)
		return;

	if (xfs->type == XCF_SECTION_MEMORY)
		free(xfs->data.data);

	free(xfs);
}

///////////////////////
// File history
///////////////////////

enum ActionType
{
	XFILE_INSERT,
	XFILE_DELETE,
	XFILE_REPLACE,
};

typedef struct FileAction
{
	enum ActionType type;
	off_t offset;
	size_t size;
	uint8_t *data;
	struct FileAction *next;
} FileAction;

static int xchange_insert_undo(XChangeFile * xfile, FileAction *a);

static void destroyFileAction(FileAction *a)
{
	if (a == NULL)
		return;

	free(a->data);
	free(a);
}

static void destroyFileActionChain(FileAction *a)
{
	if (a != NULL)
	{
		FileAction *b = a;
		FileAction *c;
		do
		{
			c = b->next;
			destroyFileAction(b);
			b = c;
		} while (b != NULL);
	}
}

///////////////////////
// File open/save/close
///////////////////////

struct XChangeFile
{
	char *filename;
	FILE *f;

	size_t size;

	FileSection *sections;

	off_t next_read;

	int use_history;
	FileAction * history_undo;
	FileAction * history_redo;
};

XChangeFile * xchange_file_open(const char *path, const char *mode)
{
	FILE *f = NULL;
	if (path != NULL)
	{
		f = fopen(path, mode);
		if (f == NULL)
			return NULL;
	}

	// Try to create hexchange handler
	XChangeFile *xf = malloc(sizeof(struct XChangeFile));
	if (xf == NULL)
	{
		fclose(f);
		return NULL;
	}

	// File history initialization
	xf->use_history = 0;
	xf->history_redo = NULL;
	xf->history_undo = NULL;

	xf->f = f;

	if (f != NULL)
	{
		// Fill size field
		if (fseek(f, 0, SEEK_END) != 0)
		{
			fclose(f);
			free(xf);
			return NULL;
		}

		xf->size = ftell(f);
		if ((long) xf->size == -1)
		{
			fclose(f);
			free(xf);
			return NULL;
		}
		/* Por quê?
		if (xf->size > 0)
			xf->size --;
		*/
	}
	else
	{
		xf->size = 0;
	}

	// Keep the filepath and file handler
	if (path != NULL)
	{
		xf->filename = strdup(path);
		if (xf->filename == NULL)
		{
			fclose(f);
			free(xf);
			return NULL;
		}
	}
	else
	{
		xf->filename = NULL;
	}

	// Create initial section...
	if (xf->size == 0)
	{
		xf->sections = NULL;
	}
	else if (xf->size <= MAXFULLFILEBUFFERSIZE)
	{
		// File fits into memory

		if (fseek(f, 0, SEEK_SET) != 0)
		{
			fclose(f);
			free(xf->filename);
			free(xf);
			return NULL;
		}
		xf->sections = xchange_new_section(XCF_SECTION_MEMORY, xf->size, 0);
		if (xf->sections == NULL)
		{
			fclose(f);
			free(xf->filename);
			free(xf);
			return NULL;
		}
		if (fread(xf->sections->data.data, 1, xf->size, f) < xf->size)
		{
			if (ferror(f))
			{
				fclose(f);
				free(xf->sections);
				free(xf->filename);
				free(xf);
				return NULL;
			}
		}
	}
	else
	{
		// File too large to be loaded to memory
		xf->sections = xchange_new_section(XCF_SECTION_FILE, xf->size, 0);
		if (xf->sections == NULL)
		{
			fclose(f);
			free(xf->filename);
			free(xf);
			return NULL;
		}
	}

	xf->next_read = 0;

	return xf;
}

void xchange_file_close(XChangeFile * xfile)
{
	if (xfile == NULL)
		return;

	if (xfile->filename != NULL)
		free(xfile->filename);

	if (xfile->f != NULL)
		fclose(xfile->f);

	if (xfile->sections != NULL)
	{
		FileSection *section = xfile->sections, *section2;
		do
		{
			section2 = section->next;
			if (section->type == XCF_SECTION_MEMORY)
				free(section->data.data);
			free(section);
			section = section2;
		} while(section != NULL);
	}

	destroyFileActionChain(xfile->history_undo);
	destroyFileActionChain(xfile->history_redo);

	free(xfile);
}

static int check_save_overwrite_sanity(const XChangeFile * xfile)
{
	FileSection * section;
	off_t output_pos;

	if (xfile == NULL)
		return 0;

	section = xfile->sections;
	output_pos = 0;

	if (section == NULL)
		return 1;

	for (; section != NULL; section = section->next)
	{
		// Do copy
		switch (section->type)
		{
			case XCF_SECTION_MEMORY:
			{
				output_pos += section->size;
				break;
			}
			case XCF_SECTION_FILE:
			{
				if (section->data.f_offset < output_pos)
					return 0;
				output_pos += section->size;
				break;
			}
			default:
			{
				fprintf(stderr, "Can't identity section type (%i).\n", section->type);
				return 0;
			}
		}
	}

	return 1;
}

static int xchange_save_bytes(const XChangeFile * xfile, FILE *f)
{
	FileSection * section;
	off_t output_pos;

	if (xfile == NULL || f == NULL)
		return 0;

	section = xfile->sections;
	output_pos = 0;

	if (section == NULL)
	{
		return 1;
	}

	for (; section != NULL; section = section->next)
	{
		// Do copy
		switch (section->type)
		{
			case XCF_SECTION_MEMORY:
			{
				size_t wrote = fwrite(section->data.data, 1, section->size, f);
				if (wrote != section->size)
					return 0;
				break;
			}
			case XCF_SECTION_FILE:
			{
				uint8_t *bytes = malloc(section->size);
				if (bytes == NULL)
					return 0;
				fseek(xfile->f, section->data.f_offset, SEEK_SET);
				size_t read = fread(bytes, 1, section->size, xfile->f);

				if (read != section->size)
				{
					fprintf(stderr, "Can't read from file (read %zu instead of %zu bytes).\n", read, section->size);
					perror("Reason");
					free(bytes);
					return 0;
				}

				if (xfile->f == f)
					fseek(xfile->f, output_pos, SEEK_SET);
				size_t wrote = fwrite(bytes, 1, section->size, f);
				free(bytes);

				if (wrote != section->size)
					return 0;

				break;
			}
			default:
			{
				fprintf(stderr, "Can't identity section type (%i).\n", section->type);
				return 0;
			}
		}
		output_pos += section->size;
	}

	fflush(f);
	return 1;
}


int xchange_file_save(const XChangeFile * xfile)
{
	if (xfile == NULL)
		return 0;
	if (!check_save_overwrite_sanity(xfile))
	{
		// TODO: Fazer a versão que copia parte a parte permitindo sobrescrita mesmo com inserção em arquivos grandes...
		fprintf(stderr, "For the moment, it's unsafe save it by overwriting. Use save as or save copy instead.\n");
		return 0;
	}
	if (!xchange_file_check_sanity(xfile))
	{
		return 0;
	}
	fseek(xfile->f, 0 , SEEK_SET);
	return xchange_save_bytes(xfile, xfile->f);
}

int xchange_file_save_copy_as(const XChangeFile * xfile, const char *path)
{
	if (xfile == NULL || path == NULL)
		return 0;

	FILE *f = fopen(path, "wb");
	if (f == NULL)
		return 0;

	int resp = xchange_save_bytes(xfile, f);
	fclose(f);
	return resp;
}

int xchange_file_save_as(XChangeFile * xfile, const char *path)
{
	if (xfile == NULL || path == NULL)
		return 0;

	char *new_name = strdup(path);
	if (new_name == NULL)
	{
		return 0;
	}

	FILE *f = fopen(path, "wb");
	if (f == NULL)
	{
		free(new_name);
		return 0;
	}

	int sucesso = xchange_save_bytes(xfile, f);

	if (!sucesso)
	{
		free(new_name);
		fclose(f);
		return 0;
	}

	if (xfile->f != NULL)
		fclose(xfile->f);

	xfile->f = f;
	free(xfile->filename);
	xfile->filename = new_name;
	return sucesso;
}

// TODO: Se from e until estiverem na mesma seção e ela for na memória, não precisa duplicar na memória...
off_t xchange_file_find(const XChangeFile * xfile, off_t from, off_t until, const uint8_t *key, size_t key_length)
{
	const size_t BUFFER_SIZE = 1024 * 1024;
	off_t offset;
	off_t achou = (off_t) -1;
	uint8_t *buffer;

	if (xfile == NULL || key == NULL || key_length == 0)
		return achou;
	if (until && until < from)
		return achou;

	buffer = malloc(BUFFER_SIZE);
	if (buffer == NULL)
		return (off_t) -1;

	if (until == 0 || until >= xchange_file_get_size(xfile)-1)
		until = xchange_file_get_size(xfile)-1;

	for (offset = from; offset + key_length -1 <= until; offset += BUFFER_SIZE - key_length + 1)
	{

		int p;
		size_t got;
		got = xchange_file_get_bytes(xfile, offset, buffer, offset + BUFFER_SIZE  - 1 > until ? until - offset + 1 : BUFFER_SIZE);
		if (got < key_length)
			break;
		size_t internal_limit = got-key_length;

		for (p = 0; p <= internal_limit; p++)
		{
			if (memcmp(&buffer[p], key, key_length) == 0)
			{
				achou = offset + p;
				break;
			}
		}
		if (achou != (off_t) -1)
			break;
	}

	free(buffer);

	return achou;
}

off_t xchange_file_find_backwards(const XChangeFile * xfile, off_t from, off_t until, const uint8_t *key, size_t key_length)
{
	const size_t BUFFER_SIZE = 1024 * 1024;
	off_t offset;
	off_t achou = (off_t) -1;
	uint8_t *buffer;

	if (xfile == NULL || key == NULL || key_length == 0)
		return achou;
	if (until && until < from)
		return achou;

	buffer = malloc(BUFFER_SIZE);
	if (buffer == NULL)
		return (off_t) -1;

	if (until == 0 || until >= xchange_file_get_size(xfile))
		until = xchange_file_get_size(xfile) -1;

	offset = until - from < BUFFER_SIZE ? from : until - BUFFER_SIZE;

	while (1)
	{
		int p;
		size_t got;
		got = xchange_file_get_bytes(xfile, offset, buffer, offset + BUFFER_SIZE  - 1 > until ? until - offset + 1 : BUFFER_SIZE);
		if (got < key_length)
			break;
		size_t internal_limit = got-key_length;

		for (p = internal_limit; p >= 0; p--)
		{
			if (memcmp(&buffer[p], key, key_length) == 0)
			{
				achou = offset + p;
				break;
			}
		}
		if (achou != (off_t) -1)
			break;

		if (offset <= from)
			break;

		if (offset < BUFFER_SIZE - key_length + 1)
			offset = 0;
		else
			offset -= BUFFER_SIZE - key_length + 1;
	}

	free(buffer);

	return achou;
}

static FileSection * search_section(const XChangeFile * xfile, off_t offset)
{
	if (xfile == NULL)
		return NULL;

	if (xfile->size == 0 && offset == 0)
		return xfile->sections;

	if (offset >= xfile->size)
		return NULL;

	FileSection * section;
	for (section = xfile->sections; section != NULL; section = section->next)
	{
		if (offset >= section->offset && offset < section->offset + section->size)
		{
			// Found section with starting offset
			break;
		}
	}
	return section;
}

size_t xchange_file_get_size(const XChangeFile * xfile)
{
	if (xfile == NULL)
		return (size_t)-1;
	return xfile->size;
}

size_t xchange_file_get_bytes(const XChangeFile * xfile, off_t offset, uint8_t *bytes, size_t size)
{
	FileSection * section;

	if (xfile == NULL || bytes == NULL || size < 0)
		return 0;

	if (size == 0)
		return 1;

	// Check if the number of bytes available to read is less than requested
	if (offset + size > xfile->size)
		size = xfile->size - offset;

	section = search_section(xfile, offset);

	if (section == NULL)
		return 0;

	// Copy and search for end section
	off_t out_pos = 0;
	size_t bytes_read = 0;
	size_t bytes_to_read;
	for (; section != NULL && size > 0; section = section->next)
	{
		// Calculate how many bytes to copy from this section
		bytes_to_read = section->size - (offset - section->offset);
		if (section->offset + section->size > offset + size)
			bytes_to_read -= (section->offset + section->size) - (offset + size);

		// Do copy
		switch (section->type)
		{
			case XCF_SECTION_MEMORY:
			{
				memcpy(bytes+out_pos, section->data.data+(offset-section->offset), bytes_to_read);
				offset += bytes_to_read;
				size -= bytes_to_read;
				bytes_read += bytes_to_read;
				out_pos += bytes_to_read;
				break;
			}
			case XCF_SECTION_FILE:
			{
				size_t read;
				fseek(xfile->f, section->data.f_offset+(offset-section->offset), SEEK_SET);
				read = fread(bytes+out_pos, 1, bytes_to_read, xfile->f);
				offset += read;
				size -= read;
				bytes_read += read;
				out_pos += read;
				if (read != bytes_to_read)
				{
					fprintf(stderr, "Can't read from file (read %zu instead of %zu bytes).\n", read, bytes_to_read);
					perror("Reason");
					return bytes_read;
				}
				break;
			}
			default:
			{
				fprintf(stderr, "Can't identity section type (%i).\n", section->type);
				return bytes_read;
			}
		}
	}

	if (size < 0)
		fprintf(stderr, "Got more bytes than requested!\n");// Very very strange!

	return bytes_read;
}


/////////////////////
// File modifications
/////////////////////

static void revmemcpy(uint8_t *dst, uint8_t *src, size_t size)
{
	int n;
	for (n = size - 1; n >= 0; n--)
	{
		dst[n] = src[n];
	}
}

static void dirmemcpy(uint8_t *dst, uint8_t *src, size_t size)
{
	int n;
	for (n = 0; n < size; n++)
	{
		dst[n] = src[n];
	}
}


static int reduceFileFileSectionSize(FileSection *s, size_t new_size)
{
	if (s == NULL)
		return 0;
	if (s->type != XCF_SECTION_FILE)
		return 0;
	if (new_size > s->size)
		return 0;

	s->size = new_size;
	return 1;
}

static int reduceMemoryFileSectionSize(FileSection *s, size_t new_size)
{
	if (s == NULL)
		return 0;
	if (s->type != XCF_SECTION_MEMORY)
		return 0;

	if (new_size > s->size)
		return 0;

	uint8_t *new_data = realloc(s->data.data, new_size);
	if (new_data == NULL && new_size!=0)
	{
		return 0;
	}
	else
	{
		s->data.data = new_data;
	}
	s->size = new_size;
	return 1;
}

static int reduceFileSectionSize(FileSection *s, size_t new_size)
{
	if (s == NULL)
		return 0;
	if (s->type == XCF_SECTION_MEMORY)
		return reduceMemoryFileSectionSize(s, new_size);
	if (s->type == XCF_SECTION_FILE)
		return reduceFileFileSectionSize(s, new_size);
	return 0;
}


static int removeFileFileSectionBeginning(FileSection *s, off_t new_start)
{
	if (s == NULL)
		return 0;
	if (s->type != XCF_SECTION_FILE)
		return 0;

	if (new_start <= s->offset)
		return 0;

	size_t removed_size = new_start - s->offset;
	s->data.f_offset += removed_size;
	s->size -= removed_size;
	s->offset = new_start;
	return 1;
}

static int removeMemoryFileSectionBeginning(FileSection *s, off_t new_start)
{
	unsigned char *tmp;

	if (s == NULL)
		return 0;
	if (s->type != XCF_SECTION_MEMORY)
		return 0;

	if (new_start <= s->offset)
		return 1;
	if (new_start >= s->offset + s->size)
		return 0;

	size_t removed_size = new_start - s->offset;
	dirmemcpy(s->data.data, s->data.data + removed_size, s->size - removed_size);
	tmp = realloc(s->data.data, s->size-removed_size);
	if (tmp == NULL)
		return 0;//TODO: care to return original values if realloc fails
	tmp = s->data.data;
	s->size -= removed_size;
	s->offset = new_start;
	return 1;
}

static int removeFileSectionBeginning(FileSection *s, off_t new_start)
{
	if (s == NULL)
		return 0;
	if (s->type == XCF_SECTION_MEMORY)
		return removeMemoryFileSectionBeginning(s, new_start);
	if (s->type == XCF_SECTION_FILE)
		return removeFileFileSectionBeginning(s, new_start);
	return 0;
}

static int splitFileFileSection(FileSection *s, off_t offset)
{
	if (s == NULL)
		return 0;

	if (s->type != XCF_SECTION_FILE)
		return 0;

	if (offset <= s->offset || offset >= s->offset + s->size)
		return 0;

	size_t s_new_size = offset - s->offset;
	FileSection *ns = xchange_new_section(XCF_SECTION_FILE, s->size - s_new_size, offset);
	if (ns == NULL)
		return 0;

	ns->data.f_offset = s->data.f_offset + s_new_size;
	s->size = s_new_size;

	ns->next = s->next;
	s->next = ns;
	return 1;
}

static int insertFileSectionAfter(FileSection *new_section, FileSection *section_reference)
{
	if (new_section == NULL || section_reference == NULL)
		return 0;

	new_section->next = section_reference->next;
	section_reference->next = new_section;
	return 1;
}

static int removeFileSection(FileSection *sectionList, FileSection *section)
{
	if (section == NULL || sectionList == NULL)
		return 0;

	FileSection *s = sectionList;
	while (s->next != NULL && s->next != section)
		s = s->next;
	if (s->next == NULL)
		return 0;

	s->next = s->next->next;
	xchange_destroy_section(section);
	return 1;
}

static int destroyFileSections(FileSection *section_start, FileSection *section_end)
{
	FileSection *s = section_start;
	FileSection *s_next = section_start;

	if (section_start == NULL)
		return 0;
	while (s != NULL && s != section_end)
	{
		s_next = s->next;
		xchange_destroy_section(s);
		s = s_next;
	}
	if (s == NULL && section_end != NULL)
		fprintf(stderr, "Internal error. Destroying file sections.\n");
	return 1;
}

static void fixSectionOffset(FileSection *section_start, long shift)
{
	while (section_start != NULL)
	{
		section_start->offset += shift;
		section_start = section_start->next;
	}
}


static int xchange_do_insert_bytes(XChangeFile * xfile, off_t offset, const uint8_t *bytes, const size_t size, const int add_history)
{
	if (xfile == NULL || bytes == NULL || size == 0)
		return 0;

	FileSection * section, *section_to_fix = NULL;

	section = search_section(xfile, offset);

	if (section == NULL)
	{
		if (xfile->size > 0 && offset != xfile->size )
			return 0; // Offset not valid

		if (xfile->size == 0)
		{
			//if (xfile->sections != NULL)
			//	return 0; // Weird!!!

			if (offset != 0)
				return 0; // empty file and requested insertion at non-0 offset

			// Insert at beginning: empty file
			FileSection * ns = xchange_new_memory_section(bytes, size, offset);
			if (ns == NULL)
				return 0;

			xfile->sections = ns;
		}
		else
		{
			// Add it to the end of file:
			//   Search for last section and possibly merge with it...
			section = search_section(xfile, xfile->size-1);
		}
	}

	if (section != NULL && section->type == XCF_SECTION_FILE)
	{
		if (offset == section->offset || offset == section->offset + section->size)
		{
			// At the section edges... Check if nearest section is a memory one... so make it merge...
			FileSection * neighbor_section;
			if (offset == section->offset)
				neighbor_section = search_section(xfile, section->offset-1); // Previous
			else
				neighbor_section = section->next; // Next
			// Check if nearest section exist and it is a memory one
			if (neighbor_section != NULL && neighbor_section->type == XCF_SECTION_MEMORY)
			{
				// Repass for insert at neighbor section
				section = neighbor_section;
			}
			else
			{
				// Add a new memory section before/after this section!
				FileSection * ns = xchange_new_memory_section(bytes, size, offset);
				if (ns == NULL)
					return 0;
				// Relink section list
				if (offset == section->offset)
				{
					if ( neighbor_section != NULL)
						neighbor_section->next = ns;
					ns->next = section;
					if (offset == 0)
						xfile->sections = ns;
				}
				else
				{
					section->next = ns;
					ns->next = neighbor_section;
				}

				section_to_fix = ns->next;
			}
		}
		else
		{
			// Splice section in twice and create a memory one in the middle
			FileSection * last_section = xchange_new_section(XCF_SECTION_FILE,
			                                                 section->size - (offset - section->offset),
			                                                 offset);
			if (last_section == NULL)
				return 0;
			FileSection * middle_section = xchange_new_memory_section(bytes, size, offset);
			if (middle_section == NULL)
			{
				xchange_destroy_section (last_section);
				return 0;
			}
			last_section->data.f_offset = section->data.f_offset + (offset - section->offset);
			last_section->next = section->next;
			middle_section->next = last_section;
			section->next = middle_section;
			section->size = offset - section->offset;

			section_to_fix = last_section;
		}
	}

	if (section != NULL && section->type == XCF_SECTION_MEMORY)
	{
		// Resize section data
		uint8_t * new_data = realloc(section->data.data, section->size + size);
		if (new_data == NULL)
			return 0;
		section->data.data = new_data;
		// Shift left the data
		// memcpy not safe: dst and src overlaps each other
		revmemcpy(section->data.data + (offset - section->offset) + size, section->data.data + (offset - section->offset), section->size - (offset - section->offset));
		// Insert data
		memcpy(section->data.data + (offset - section->offset), bytes, size);
		section->size += size;
		// Since what section it should fix offsets
		section_to_fix = section->next;
	}

	// Fix sections offset
	while (section_to_fix != NULL)
	{
		section_to_fix->offset += size;
		section_to_fix = section_to_fix->next;
	}

	// Update file size
	xfile->size += size;

	// Add action to undo list
	if (add_history)
	{
		FileAction * a = malloc(sizeof(FileAction));
		if (a == NULL)
		{
			fprintf(stderr, "Action could not be \"undo\"ne\n");
			; // TODO!
		}
		a->data = NULL;
		a->offset = offset;
		a->size = size;
		a->type = XFILE_INSERT;
		xchange_insert_undo(xfile, a); // TODO: se não der certo?
	}

	return 1;
}

int xchange_file_insert_bytes(XChangeFile * xfile, off_t offset, const uint8_t *bytes, size_t size)
{
	if (xfile == NULL)
		return 0;

	return xchange_do_insert_bytes(xfile, offset, bytes, size, xfile->use_history);
}

static int xchange_do_delete_bytes(XChangeFile * xfile, const off_t offset, size_t size)
{
	if (xfile == NULL || size == 0)
		return 0;

	if (offset >= xfile->size)
		return 0;

	if (offset + size > xfile->size)
		size = xfile->size - offset;

	int end_offset = offset+size-1;
	FileSection *s1, *s2, *section_to_fix = NULL;
	s1 = search_section(xfile, offset);
	s2 = search_section(xfile, end_offset);

	if (s1 == NULL || s2 == NULL)
		return 0; // Weird

	if (s1 == s2)
	{
		// Delete inside one section only
		switch (s1->type)
		{
			case XCF_SECTION_MEMORY:
			{
				dirmemcpy(s1->data.data + offset - s1->offset,
				       s1->data.data + offset - s1->offset + size,
				       s1->size - (offset - s1->offset + size));
				s1->size -= size;
				s1->data.data = realloc(s1->data.data, s1->size);
				section_to_fix = s1->next;
				if (s1->size == 0)
				{
					FileSection *s;
					if (xfile->sections == s1)
						xfile->sections = s1->next;
					else
					{
						for (s = xfile->sections; s != NULL; s = s->next)
						{
							if (s->next == s1)
							{
								s->next = s1->next;
								break;
							}
						}
					}
					xchange_destroy_section (s1);
				}
				break;
			}
			case XCF_SECTION_FILE:
			{
				s2 = xchange_new_section(XCF_SECTION_FILE, s1->size - size - (offset - s1->offset), offset);
				s2->data.f_offset = s1->data.f_offset+(offset - s1->offset)+size;
				s1->size = offset - s1->offset;
				s2->next = s1->next;
				s1->next = s2;
				section_to_fix = s2->next;
				if (s2->size == 0)
				{
					s1->next = s2->next;
					xchange_destroy_section(s2);
				}
				if (s1->size == 0)
				{
					if (xfile->sections == s1)
						xfile->sections = s1->next;
					else
					{
						FileSection *s;
						for (s = xfile->sections; s != NULL; s = s->next)
						{
							if (s->next == s1)
							{
								s->next = s1->next;
								break;
							}
						}
					}
					xchange_destroy_section (s1);
				}
				break;
			}
			default:
				return 0;
		}
	}
	else
	{
		if (!reduceFileSectionSize(s1, offset - s1->offset))
			return 0;
		removeFileSectionBeginning(s2, offset+size);
		destroyFileSections(s1->next, s2);
		if (s2->size != 0)
		{
			s1->next = s2;
			section_to_fix = s2;
		}
		else
		{
			s1->next = s2->next;
			section_to_fix = s2->next;
			xchange_destroy_section(s2);
		}
		if (s1->size == 0)
		{
			FileSection *p = xfile->sections;
			if (s1 == xfile->sections)
			{
				xfile->sections = s1->next;
			}
			else
			{
				while (p->next != s1)
					p = p->next;
				p->next = s1->next;
			}
			xchange_destroy_section(s1);
		}
	}

	// Fix later sections offset
	fixSectionOffset(section_to_fix, -size);

	// Update file size
	xfile->size -= size;

	return 1;
}

int xchange_file_delete_bytes(XChangeFile * xfile, off_t offset, size_t size)
{
	if (xfile == NULL || size == 0)
		return 0;

	uint8_t * dados = NULL;
	size_t diff = xfile->size;
	FileAction *a = NULL;

	if (xfile->use_history)
	{
		dados = malloc(size);
		if (dados == NULL)
			return 0;
		xchange_file_get_bytes(xfile, offset, dados, size);
		a = malloc(sizeof(FileAction));
		if (a == NULL)
		{
			free(dados);
			return 0;
		}
	}
	int resp = xchange_do_delete_bytes(xfile, offset, size);
	if (resp == 0)
	{
		if (xfile->use_history)
		{
			free(dados);
			free(a);
		}
		return 0;
	}
	if (xfile->use_history)
	{
		diff -= xfile->size;
		a->data = dados;
		a->offset = offset;
		a->size = diff;
		a->type = XFILE_DELETE;
		xchange_insert_undo(xfile, a);
	}
	return 1;
}

static int xchange_do_replace_bytes(XChangeFile * xfile, const off_t offset, const uint8_t *bytes, size_t size)
{
	if (xfile == NULL || bytes == NULL || offset >= xfile->size || size == 0)
		return 0;

	// achar as 2 seções da ponta... se forem de tipos iguais, melhor
	if (offset+size > xfile->size)
		size = xfile->size - offset;
	off_t end_offset = offset+size-1;
	FileSection *s1, *s2;
	int merge_with_start_session = 0;
	int merge_with_end_section = 0;
	s1 = search_section(xfile, offset);
	s2 = search_section(xfile, end_offset);

	if (s1==NULL)
		return 0;

	if (s2 == NULL)
	{
		if (end_offset < xfile->size-1)
			return 0;
	}

	if (s1 == s2)
	{
		if (s1->type == XCF_SECTION_FILE)
		{// TODO: Defrag
			FileSection *s_new = xchange_new_memory_section(bytes, size, offset);
			if (s_new == NULL)
				return 0;

			if (s1->offset != offset)
			{
				if (!splitFileFileSection(s1, offset))
				{
					xchange_destroy_section(s_new);
					return 0;
				}
				if (s1->offset + s1->size != offset + size)
				{
					if (!splitFileFileSection(s1->next, offset + size))
					{
						xchange_destroy_section(s_new);
						return 0;
					}
					if (!removeFileSection(s1, s1->next))
					{
						xchange_destroy_section(s_new);
						return 0;
					}
					if (!insertFileSectionAfter(s_new, s1))
					{
						xchange_destroy_section(s_new);
						return 0;
					}
				}
				else
				{
					if (s1->next != NULL)
					{
						s_new->next = s1->next->next;
						xchange_destroy_section(s1->next);
					}
					s1->next = s_new;
				}
			}
			else
			{
				FileSection *s_prev = xfile->sections;
				while (s_prev != NULL && s_prev->next != s1)
					s_prev = s_prev->next;
				if (s_prev != NULL)
					s_prev->next = s_new;
				else
				{
					xfile->sections = s_new;
				}
				if (!removeFileFileSectionBeginning(s1, offset + size))
				{
					xchange_destroy_section(s_new);
					return 0;
				}
				if (s1->size == 0)
				{
					s_new->next = s1->next;
					xchange_destroy_section(s1);
				}
				else
					s_new->next = s1;
			}
		}
		else if (s1->type == XCF_SECTION_MEMORY)
		{
			memcpy(s1->data.data + offset - s1->offset, bytes, size);
		}
		else
		{
			fprintf(stderr, "Can't identity section type (%i).\n", s1->type);
			return 0;
		}

		return 1;
	}

	// Destrói até ANTES de s2!!
	destroyFileSections(s1->next, s2);

	if (s1->type == XCF_SECTION_FILE)
	{
		reduceFileFileSectionSize(s1, offset - s1->offset);
		merge_with_start_session = 0;
	}
	else if (s1->type == XCF_SECTION_MEMORY)
	{
		merge_with_start_session = 1;
	}
	else
	{
		fprintf(stderr, "Can't identity section type (%i).\n", s1->type);
		return 0;
	}

	if (s2 != NULL)
	{
		if (s2->type == XCF_SECTION_FILE)
		{
			if(!removeFileFileSectionBeginning(s2, end_offset+1))
				; // TODO! Alertar erro ou qq coisa
			merge_with_end_section = 0;
		}
		else if (s2->type == XCF_SECTION_MEMORY)
		{
			if (!removeMemoryFileSectionBeginning(s2, end_offset+1))
				; // TODO! Alertar erro ou qq coisa
			merge_with_end_section = 1;
		}
		else
		{
			fprintf(stderr, "Can't identity section type (%i).\n", s2->type);
			return 0;
		}
	}
	else
		merge_with_end_section = 0;

	size_t new_section_size;

	if (merge_with_start_session)
	{
		new_section_size = size + (offset - s1->offset);
		void * tmp = realloc(s1->data.data, new_section_size);
		if (tmp == NULL)
			return 0;
		s1->data.data = tmp;

		memcpy(s1->data.data + (offset - s1->offset), bytes, size);
		s1->size = new_section_size;

		if (merge_with_end_section)
		{
			s1->data.data = realloc(s1->data.data, s1->size + s2->size);
			if (s1->data.data == NULL)
				return 0;

			memcpy(s1->data.data + s1->size, s2->data.data, s2->size);
			s1->size += s2->offset + s2->size - (offset + size);

			s1->next = s2->next;
			xchange_destroy_section(s2);
		}
		else
		{
			s1->next = s2;
		}
	}
	else
	{
		// s1 is section type "File"
		if (merge_with_end_section)
		{
			FileSection *new_s = xchange_new_memory_section(bytes, size, offset);
			if (new_s == NULL)
				return 0;

			s1->next = new_s;
			new_s->next = s2;

			uint8_t *tmp = realloc(new_s->data.data, size + s2->size);
			if (tmp == NULL)
				return 1; // FIXME: Não desfragmentou

			memcpy(tmp + size, s2->data.data, s2->size);
			new_s->data.data = tmp;
			new_s->next = s2->next;
			new_s->size += s2->size;

			xchange_destroy_section(s2);

		}
		else
		{
			// Cria e insere
			FileSection *new_s = xchange_new_memory_section(bytes, size, offset);
			if (new_s == NULL)
				return 0;

			s1->next = new_s;
			new_s->next = s2;

		}
	}

	return 1;
}

int xchange_file_replace_bytes(XChangeFile * xfile, off_t offset, const uint8_t *bytes, size_t size)
{
	if (xfile == NULL)
		return 0;

	FileAction *a = NULL;
	uint8_t *dados = NULL;
	int tam = size;

	if (xfile->use_history)
	{
		dados = malloc(size);
		if (dados == NULL)
			return 0;
		tam = xchange_file_get_bytes(xfile, offset, dados, size);
		if (tam != size)
		{
			uint8_t *d = realloc(dados, tam);
			if (d != NULL)
				dados = d;
			else if (tam == 0)
			{
				// Já vai ter liberado dados no realloc
				dados = NULL;
			}
		}
		a = malloc(sizeof(FileAction));
		if (a == NULL)
		{
			free(dados);
			return 0;
		}
	}

	int resp = xchange_do_replace_bytes(xfile, offset, bytes, size);

	if (resp == 0)
	{
		if (xfile->use_history)
		{
			free(dados);
			free(a);
		}
		return 0;
	}

	if (xfile->use_history)
	{
		a->data = dados;
		a->offset = offset;
		a->size = tam; // FIXME: size? ou tam? Replace pode acrescentar bytes...
		a->type = XFILE_REPLACE;
		xchange_insert_undo(xfile, a);
	}
	return 1;
}

int xchange_file_check_sanity(const XChangeFile * xfile)
{
	if (xfile == NULL)
		return 0;

	size_t computedSize = 0;
	FileSection *s = xfile->sections;
	off_t previousOffset = 0;
	int ns = 0;
	int erro = 0;

	while (s != NULL)
	{
		if (s->offset != previousOffset)
			fprintf(stderr, "%i: Erro de offset na seção %i!\n", erro++, ns);
		previousOffset += s->size;

		computedSize += s->size;

		if (s->size == 0)
			fprintf(stderr, "%i: Seção %i com tamanho nulo!\n", erro++, ns);

		ns++;
		s = s->next;
	}

	if (xfile->size != computedSize)
		fprintf(stderr, "%i: Erro de tamanho de arquivo\n", erro++);

	return erro == 0;
}

// FIXME: Remove!
void xchange_dump_section(const XChangeFile *xf)
{
	if (xf == NULL)
		return;

	FileSection *s = xf->sections;
	uint8_t b[2048];
	while (s!= NULL)
	{
		int t = s->size +1 < sizeof(b) ? s->size+1 : sizeof(b);
		xchange_file_get_bytes(xf, s->offset,b, t);
		b[t-1] = 0;
		printf("Seção [%s] (%lli - %zu): \"%s\"\n", s->type == XCF_SECTION_MEMORY? "memo" : "file",(long long int) s->offset, s->size, (char*)b);
		s = s->next;
	}
}

////////////////////////
// File history
////////////////////////

static int xchange_insert_undo(XChangeFile * xfile, FileAction *a)
{
	if (xfile == NULL || a == NULL)
		return 0;

	destroyFileActionChain(xfile->history_redo);
	xfile->history_redo = NULL;

	a->next = xfile->history_undo;
	xfile->history_undo = a;

	return 1;
}

int xchange_file_undo(XChangeFile * xfile)
{
	if (xfile == NULL)
		return 0;

	FileAction *a = xfile->history_undo;
	if (a == NULL)
		return 0;

	switch (a->type)
	{
		case XFILE_DELETE:
		{
			if (!xchange_do_insert_bytes(xfile, a->offset, a->data, a->size, 0))
				return 0;
			break;
		}
		case XFILE_INSERT:
		{
			if (a->data == NULL)
			{
				uint8_t *data = malloc(a->size);
				if (data == NULL)
					return 0;
				xchange_file_get_bytes(xfile, a->offset, data, a->size);
				a->data = data;
			}
			if (!xchange_do_delete_bytes(xfile, a->offset, a->size))
				return 0;
			break;
		}
		case XFILE_REPLACE:
		{

			uint8_t *data = malloc(a->size);
			if (data == NULL)
				return 0;
			xchange_file_get_bytes(xfile, a->offset, data, a->size);

			if (!xchange_do_replace_bytes(xfile, a->offset, a->data, a->size))
			{
				free(data);
				return 0;
			}

			free(a->data);
			a->data = data;

			break;
		}

	}

	xfile->history_undo = a->next;
	a->next = xfile->history_redo;
	xfile->history_redo = a;

	return 1;
}

int xchange_file_redo(XChangeFile * xfile)
{
	if (xfile == NULL)
		return 0;

	FileAction *a = xfile->history_redo;
	if (a == NULL)
		return 0;

	switch (a->type)
	{
		case XFILE_INSERT:
		{
			if (!xchange_do_insert_bytes(xfile, a->offset, a->data, a->size, 0))
				return 0;
			break;
		}
		case XFILE_DELETE:
		{
			if (!xchange_do_delete_bytes(xfile, a->offset, a->size))
				return 0;
			break;
		}
		case XFILE_REPLACE:
		{
			uint8_t *data = malloc(a->size);
			if (data == NULL)
				return 0;
			xchange_file_get_bytes(xfile, a->offset, data, a->size);

			if (!xchange_do_replace_bytes(xfile, a->offset, a->data, a->size))
			{
				free(data);
				return 0;
			}

			free(a->data);
			a->data = data;

			break;
		}

	}

	xfile->history_redo = a->next;
	a->next = xfile->history_undo;
	xfile->history_undo = a;

	return 1;
}

int xchange_file_has_undo(const XChangeFile * xfile)
{
	if (xfile == NULL)
		return 0;
	return xfile->history_undo != NULL;
}

int xchange_file_has_redo(const XChangeFile * xfile)
{
	if (xfile == NULL)
		return 0;
	return xfile->history_redo != NULL;
}

int xchange_file_get_undo_list_size(const XChangeFile * xfile)
{
	if (xfile == NULL)
		return 0;

	FileAction *a = xfile->history_undo;
	int size = 0;
	while (a != NULL)
	{
		size++;
		a = a->next;
	}
	return size;
}

int xchange_file_get_redo_list_size(const XChangeFile * xfile)
{
	if (xfile == NULL)
		return 0;

	FileAction *a = xfile->history_redo;
	int size = 0;
	while (a != NULL)
	{
		size++;
		a = a->next;
	}
	return size;
}


void xchange_file_clear_history(XChangeFile * xfile)
{
	if (xfile == NULL)
		return;

	destroyFileActionChain(xfile->history_undo);
	xfile->history_undo = NULL;
	destroyFileActionChain(xfile->history_redo);
	xfile->history_redo = NULL;
}

void xchange_file_enable_history(XChangeFile * xfile, int enable)
{
	if (xfile == NULL)
		return;

	if (enable)
		xfile->use_history = 1;
	else
	{
		xchange_file_clear_history(xfile);
		xfile->use_history = 0;
	}
}





/////////////////////////
// File navigation
/////////////////////////

int xchange_file_seek(XChangeFile * xfile, off_t offset, SeekBase base)
{
	if (xfile == NULL)
		return 0;

	switch (base)
	{
		case SEEK_CUR:
		{
			off_t o = xfile->next_read + offset;
			if (o >= xfile->size)
				return 0;
			if (o < 0)
				return 0;
			xfile->next_read = offset;
			break;
		}
		case SEEK_SET:
		{
			if (offset >= xfile->size)
				return 0;
			if (offset < 0)
				return 0;
			xfile->next_read = offset;
			break;
		}
		case SEEK_END:
		{
			if (offset > 0)
				return 0;
			if (offset < -xfile->size)
				return 0;
			xfile->next_read = xfile->size + offset;
			break;
		}
	}
	return 1;
}

off_t xchange_file_position(const XChangeFile * xfile)
{
	if (xfile == NULL)
		return (off_t) -1;
	return xfile->next_read;
}

// Read data using native byte order
int8_t xchange_file_readByte(XChangeFile * xfile)
{
	return xchange_file_readUByte(xfile);
}

int16_t xchange_file_readShort(XChangeFile * xfile);
int32_t xchange_file_readInt(XChangeFile * xfile);
int64_t xchange_file_readLong(XChangeFile * xfile);
uint8_t xchange_file_readUByte(XChangeFile * xfile)
{
	if (xfile == NULL)
		return EOF;

	uint8_t b;
	if (!xchange_file_get_bytes(xfile, xfile->next_read, &b, 1))
		return EOF;

	xfile->next_read++;
	return b;
}
uint16_t xchange_file_readUShort(XChangeFile * xfile)
{
	if (xfile == NULL)
		return EOF;

	uint16_t s;
	if (!xchange_file_get_bytes(xfile, xfile->next_read, (uint8_t*)&s, 2))
		return EOF;

	xfile->next_read+=2;
	return s;
}
uint32_t xchange_file_readUInt(XChangeFile * xfile)
{
	if (xfile == NULL)
		return EOF;

	uint32_t i;
	if (!xchange_file_get_bytes(xfile, xfile->next_read, (uint8_t*)&i, 4))
		return EOF;

	xfile->next_read+=4;
	return i;
}
uint64_t xchange_file_readULong(XChangeFile * xfile)
{
	if (xfile == NULL)
		return EOF;

	uint64_t l;
	if (!xchange_file_get_bytes(xfile, xfile->next_read, (uint8_t*)&l, 8))
		return EOF;

	xfile->next_read+=8;
	return l;
}

// Read data using little-endian byte order
static int64_t read_ULEbytes(XChangeFile * xfile, int number)
{
	if (xfile == NULL)
		return EOF;

	uint8_t b[8];
	if (!xchange_file_get_bytes(xfile, xfile->next_read, b, number))
		return EOF;

	xfile->next_read+=number;

	uint64_t answer = 0;

	int n;
	for (n = number -1; n > 0; n--)
	{
		answer <<= 8;
		answer |= b[n];
	}

	return answer;
}

static int64_t read_UBEbytes(XChangeFile * xfile, int number)
{
	if (xfile == NULL)
		return EOF;

	uint8_t b[8];
	if (!xchange_file_get_bytes(xfile, xfile->next_read, b, number))
		return EOF;

	xfile->next_read+=number;

	uint64_t answer = 0;

	int n;
	for (n = 0; n < number; n++)
	{
		answer <<= 8;
		answer |= b[n];
	}

	return answer;
}

int16_t xchange_file_readLEShort(XChangeFile * xfile);
int32_t xchange_file_readLEInt(XChangeFile * xfile);
int64_t xchange_file_readLELong(XChangeFile * xfile);
uint16_t xchange_file_readLEUShort(XChangeFile * xfile)
{
	return read_ULEbytes(xfile, 2);
}

uint32_t xchange_file_readLEUInt(XChangeFile * xfile)
{
	return read_ULEbytes(xfile, 4);
}
uint64_t xchange_file_readLEULong(XChangeFile * xfile)
{
	return read_ULEbytes(xfile, 8);
}

// Read data using big-endian byte order
int16_t xchange_file_readBEShort(XChangeFile * xfile);
int32_t xchange_file_readBEInt(XChangeFile * xfile);
int64_t xchange_file_readBELong(XChangeFile * xfile);
uint16_t xchange_file_readBEUShort(XChangeFile * xfile)
{
	return read_UBEbytes(xfile, 2);
}

uint32_t xchange_file_readBEUInt(XChangeFile * xfile)
{
	return read_UBEbytes(xfile, 4);
}

uint64_t xchange_file_readBEULong(XChangeFile * xfile)
{
	return read_UBEbytes(xfile, 8);
}

