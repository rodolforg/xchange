/*
 * chartable.c
 *
 *  Created on: 25/01/2011
 *      Author: rodolfo
 */

#include "chartable.h"
#include "file.h"
#include <stdlib.h>
#include <iconv.h>
#include <string.h>
#include <errno.h>


#include <assert.h>

// FIXME : Remover stdio.h quando não for mais preciso
#include <stdio.h>


#define DEFAULT_UNKNOWN_CHARACTER "\xEF\xBF\xBD"
#define DEFAULT_LINEEND_MARK "\xE2\x8F\x8E"
#define DEFAULT_PARAGRAPH_MARK "\xC2\xB6"

enum EntryType
{
	ENTRY_NORMAL,
	ENTRY_LINEEND,
	ENTRY_PARAGRAPH
};

typedef struct Entry
{
	uint8_t *key;
	size_t nkey;
	uint8_t* value;
	size_t nvalue;
	enum EntryType type;
} Entry;

static Entry *create_table_entry(uint8_t *key, size_t nkey, uint8_t *value, size_t nvalue, enum EntryType type);
static void destroy_table_entry(Entry *e);

typedef struct EntryItem
{
	Entry *entry;
	struct EntryItem *next;
} EntryItem;

typedef struct List
{
	struct EntryItem *first;
	struct EntryItem *last;
	int size;
} EntryList;

static EntryList *entry_list_create();
static void entry_list_destroy(EntryList *list, int destroy_content);
static EntryList * entry_list_clone(EntryList *list);
static int entry_list_insert(EntryList *list, Entry *e);

static Entry** entry_list_to_array(const EntryList *list);

struct XChangeTable
{
	char *encoding;
	TableType type;
	int nentries;
	Entry **entries_key;
	Entry **entries_value;
//	Entry *entries;
	iconv_t icd;
	// Special entries
	unsigned char *unknown;
	int unknown_length;
	uint8_t *lineend_key;
	int lineend_key_length;
	uint8_t *paragraph_key;
	int paragraph_key_length;
};

static int convert_encoding(iconv_t icd, char**inbuf, size_t *inleft, char **outbuf, size_t *outleft);
static EntryList * load_table(const uint8_t *contents, size_t size, int *nentries);
static void sort_table_entries(XChangeTable* xt);

static XChangeTable *xchange_table_new()
{
	XChangeTable *xt = malloc(sizeof(XChangeTable));
	if (xt == NULL)
	{
		return NULL;
	}
	xt->icd = NULL;
	xt->encoding = NULL;
	xt->type = UNKNOWN_TABLE;
	xt->entries_key = NULL;
	xt->entries_value = NULL;
	xt->nentries = 0;

	xt->unknown = strdup(DEFAULT_UNKNOWN_CHARACTER);
	xt->lineend_key = NULL;
	xt->paragraph_key = NULL;
	xt->unknown_length = strlen(DEFAULT_UNKNOWN_CHARACTER);
	xt->lineend_key_length = 0;
	xt->paragraph_key_length = 0;

	return xt;
}

XChangeTable * xchange_table_open(const char *path, TableType type, const char *_encoding)
{
	XChangeFile *xf = xchange_open(path, "rb");
	if (xf == NULL)
		return NULL;

	size_t filesize = xchange_get_size(xf);
	if (filesize == (size_t)-1)
	{
		xchange_close(xf);
		return NULL;
	}

	uint8_t * contents = malloc(filesize);
	if (contents == NULL)
	{
		xchange_close(xf);
		return NULL;
	}

	if (xchange_get_bytes(xf, 0, contents, filesize) != filesize)
	{
		xchange_close(xf);
		free(contents);
		return NULL;
	}
	xchange_close(xf);


	size_t outsize = 4*filesize;
	char * buffer = malloc(outsize);
	if (buffer == NULL)
	{
		free(contents);
		return NULL;
	}

	iconv_t icd;

#define ENCODING_TRIES 7
	char *preferred_encoding[ENCODING_TRIES] = {"UTF-8", "ASCII", "MS-ANSI", "Shift-Jis", "EUC-JP", "ISO-8859-15", "ISO-8859-1"};
	int encoding_try = 0;
	int wrong_encoding;


	char *encoding;
	do
	{
		if (_encoding == NULL || strlen(_encoding) == 0) // FIXME: strlen() works fine with UTF-16?
		{
			if (encoding_try < ENCODING_TRIES)
				encoding = strdup(preferred_encoding[encoding_try++]);
		}
		else
		{
			encoding= strdup(_encoding);
		}
		if (encoding == NULL)
		{
			free(contents);
			free(buffer);
			return NULL;
		}

		icd = iconv_open("UTF-8", encoding);
		if (icd == (iconv_t)-1)
		{
			free(encoding);
			free(contents);
			free(buffer);
			return NULL;
		}

		char* inbuf = (char*)contents;
		size_t inleft = filesize;

		char *outbuf = buffer;
		size_t outleft = outsize;
		if (iconv(icd, &inbuf, &inleft, &outbuf, &outleft) == (size_t)-1)
		{
			free(encoding);
			iconv_close(icd);
			if (errno == EILSEQ || errno == EINVAL)
			{
				wrong_encoding = 1;
			}
			else
			{
				free(contents);
				free(buffer);
				return NULL;
			}
		}
		else
		{
			wrong_encoding = 0;
			outsize -= outleft;
		}
	} while (wrong_encoding);

	free(contents);
	char *tmp = realloc(buffer, outsize);
	if (tmp != NULL)
	{
		buffer = tmp;
	}

	int nentries;
	EntryList * entries = load_table((uint8_t*)buffer, outsize, &nentries);
	free(buffer);
	if (entries == NULL || entries->size <= 0)
	{
		free(encoding);
		entry_list_destroy(entries, 1);
		iconv_close(icd);
		return NULL;
	}

	Entry ** entries_array_1 = entry_list_to_array(entries);
	if (entries_array_1 == NULL)
	{
		free(encoding);
		iconv_close(icd);
		entry_list_destroy(entries, 1);
		return NULL;
	}
	Entry ** entries_array_2 = entry_list_to_array(entries);
	if (entries_array_2 == NULL)
	{
		free(encoding);
		iconv_close(icd);
		entry_list_destroy(entries, 1);
		free(entries_array_1);
		return NULL;
	}

	entry_list_destroy(entries, 0);


	XChangeTable *xt = xchange_table_new();
	if (xt == NULL)
	{
		free(encoding);
		iconv_close(icd);
		free(entries_array_1);
		free(entries_array_2);
		return NULL;
	}
	xt->icd = icd;
	xt->encoding = encoding;
	xt->type = THINGY_TABLE;
	xt->entries_key = entries_array_1;
	xt->entries_value = entries_array_2;
	xt->nentries = nentries;
	sort_table_entries(xt);

	int n;
	for (n = 0; n < nentries; n++)
		if (xt->entries_key[n]->type == ENTRY_LINEEND)
		{
			xt->lineend_key = xt->entries_key[n]->key;
			xt->lineend_key_length = xt->entries_key[n]->nkey;
		}
		else
		if (xt->entries_key[n]->type == ENTRY_PARAGRAPH)
		{
			xt->paragraph_key = xt->entries_key[n]->key;
			xt->paragraph_key_length = xt->entries_key[n]->nkey;
		}

	return xt;
}

XChangeTable * xchange_table_create_from_encoding(const char *encoding)
{
	if (encoding == NULL)
		return NULL;

	iconv_t icd = iconv_open("UTF-8", encoding);
	if (icd == (iconv_t)-1)
	{
		return NULL;
	}

	XChangeTable *xt = xchange_table_new();
	if (xt == NULL)
	{
		iconv_close(icd);
		return NULL;
	}

	xt->icd = icd;
	xt->encoding = strdup(encoding);
	xt->type = CHARSET_TABLE;
	xt->entries_key = NULL;
	xt->entries_value = NULL;
	xt->nentries = 0;
	return xt;
}

void xchange_table_close(XChangeTable *table)
{
	if (table == NULL)
		return;

	free(table->encoding);

	iconv_close(table->icd);

	int n;
	for (n = 0; n < table->nentries; n++)
		destroy_table_entry(table->entries_key[n]);
	free(table->entries_key);
	free(table->entries_value);

	free(table->unknown);
	// table->paragraph_key and table->lineend_key are freed when destroying their table->entries_key
	free(table);
}

const char * xchange_table_get_encoding(const XChangeTable *table)
{
	if (table == NULL)
		return NULL;
	return table->encoding;
}

TableType xchange_table_get_type(const XChangeTable *table)
{
	if (table == NULL)
		return XCHANGE_TABLE_UNKNOWN_ERROR;
	return table->type;
}

int xchange_table_get_size(const XChangeTable *table)
{
	if (table == NULL)
		return XCHANGE_TABLE_UNKNOWN_ERROR;
	return table->nentries;
}

static int get_largest_entry_id(const XChangeTable *table, int by_key)
{
	int id = -1;
	Entry *e;
	if (table->type == CHARSET_TABLE)
		return XCHANGE_TABLE_UNKNOWN_SIZE;

	if (by_key)
	{
		int n;
		int max = 0;
		for (n = 0; n < table->nentries; n++)
		{
			e = table->entries_key[n];
			if (e->nkey > max)
			{
				max = e->nkey;
				id = n;
				break;
			}
		}
	}
	else
	{
		int n;
		int max = 0;
		for (n = 0; n < table->nentries; n++)
		{
			e = table->entries_key[n];
			if (e->nvalue > max)
			{
				max = e->nvalue;
				id = n;
				break;
			}
		}
	}
	return id;
}

int xchange_table_get_largest_entry_length(const XChangeTable *table, int by_key)
{
	if (table == NULL)
		return XCHANGE_TABLE_UNKNOWN_ERROR;

	int id = get_largest_entry_id(table, by_key);
	if (id < 0)
		return id;
	return by_key ? table->entries_key[id]->nkey : table->entries_key[id]->nvalue;
}

static Entry *create_table_entry(uint8_t *key, size_t nkey, uint8_t *value, size_t nvalue, enum EntryType type)
{
	assert(key != NULL);
	assert(value != NULL);

	Entry *e = malloc(sizeof(Entry));
	if (e == NULL)
		return NULL;

	e->key = key;
	e->value = value;
	e->nkey = nkey;
	e->nvalue = nvalue;
	e->type = type;
	//	e->next = NULL;
	return e;
}

static void destroy_table_entry(Entry *e)
{
	assert(e);
	free(e->key);
	free(e->value);
	free(e);
}

static EntryList *entry_list_create()
{
	EntryList *list = malloc(sizeof(EntryList));
	if (list == NULL)
		return NULL;
	list->size = 0;
	list->first = NULL;
	list->last = NULL;
	return list;
}

static EntryItem *entry_list_item_create(Entry * e)
{
	EntryItem *item = malloc(sizeof(EntryItem));
	if (item == NULL)
		return NULL;
	item->entry = e;
	item->next = NULL;
	return item;
}

static void entry_list_destroy(EntryList *list, int destroy_content)
{
	assert(list);
	//1424
	//063103
	EntryItem *item = list->first;
	while (item != NULL)
	{
		EntryItem *next = item->next;
		if (destroy_content)
			destroy_table_entry(item->entry);
		free(item);
		item = next;
	}
	free(list);
}

static EntryList *entry_list_clone(EntryList *list)
{
	assert(list!=NULL);
	EntryList *clone = entry_list_create();
	if (clone == NULL)
		return NULL;

	EntryItem *item = list->first;

	while (item != NULL)
	{
		entry_list_insert(clone, item->entry);
		item = item->next;
	}
	return clone;
}

static int entry_list_insert(EntryList *list, Entry *e)
{
	assert(list != NULL);
	assert(e != NULL);

	EntryItem * new = entry_list_item_create(e);
	if (new == NULL)
		return 0;
	if (list->first == NULL)
	{
		list->first = new;
	}
	else
	{
		list->last->next = new;
	}
	list->last = new;
	list->size++;
	return 1;
}
/*
static int entry_list_insert_ordered(EntryList *list, Entry *e, int (*compare(Entry *, Entry*)))
{
	assert(list != NULL);
	assert(e != NULL);
	assert(compare != NULL);
	EntryList *next;
	EntryList * previous = NULL;
	EntryList * cur = list;
	EntryList * new = malloc(sizeof(EntryList));
	if (new == NULL)
		return 0;
	new->entry = e;
	new->next = NULL;
	while (cur != NULL)
	{
		next = cur->next;
		int comparison = compare(e, cur->entry);
		if (comparison == 0)
		{
			// add here
			new->next = cur->next;
			cur->next = new;
			break;
		}
		else if (comparison < 0)
		{
			previous->next = new;
			new->next = cur;
			break;
		}
		else
		{

		}
		previous = cur;
		cur = next;
	}
	return 1;
}
*/
// FIXME: ordenar lista não funciona?
static void entry_list_sort(EntryList *list, int size)
{
	assert(list!=NULL);
	EntryItem *p1, *p2, *p3;
	EntryItem *head = list->first;
	int i, j;
	for(i = 1; i < size; i++)
	{
		p1 = head;
		p2 = head->next;
		p3 = p2->next;

		for(j = 1; j <= (size - i) && p3!=NULL; j++)
		{
			if(p2->entry->nkey > p3->entry->nkey)
			{
				p2->next = p3->next;
				p3->next = p2;
				p1->next = p3;
				p1		= p3;
				p3		= p2->next;
			}
			else
			{
				p1 = p2;
				p2 = p3;
				p3 = p3->next;
			}
		}
	}

}

static Entry** entry_list_to_array(const EntryList *list)
{
	assert(list!=NULL);
	Entry ** entries = calloc(list->size, sizeof(Entry*));
	if (entries == NULL)
		return NULL;

	int n = 0;
	EntryItem *item = list->first;
	while (item != NULL)
	{
		entries[n] = item->entry;
		n++;
		item = item->next;
	}
	return entries;
}

static int convert_encoding(iconv_t icd, char**inbuf, size_t *inleft, char **outbuf, size_t *outleft)
{
	char *buffer = *outbuf;
	size_t outsize = *outleft;
	if (iconv(icd, inbuf, inleft, outbuf, outleft) == (size_t)-1)
	{
		return 0;
	}

	outsize -= *outleft;
	char *tmp = realloc(buffer, outsize);
	if (tmp != NULL)
	{
		buffer = tmp;
	}
	return 1;
}

static size_t get_line(const uint8_t *buffer, size_t buffer_size, uint8_t *line, size_t line_size)
{
	assert(buffer != NULL);
	assert(line != NULL);

	size_t n;
	int min = buffer_size > line_size ? line_size : buffer_size;
	int line_end_found = 0;
	for (n = 0; n < min; n++)
	{
		line[n] = buffer[n];
		if (buffer[n] == '\n' || buffer[n]=='\r')
		{
			line_end_found = 1;
			break;
		}
	}

	if (!line_end_found)
	{
		if (n < line_size)
			line[n] = 0;
		else
		{
			n = line_size -1;
			line[n] = 0;
		}
		return n;
	}

	line[n] = 0;
	if (buffer[n] == '\r')
	{
		if (n < buffer_size-1)
		{
			if (buffer[n+1]=='\n')
			{
				n++;
			}
		}
	}
	return n+1;
}

static size_t search_byte(const uint8_t *buffer, size_t size, uint8_t byte)
{
	assert(buffer != NULL);

	size_t n;
	int found = 0;

	for (n = 0; n < size; n++)
		if (buffer[n] == byte)
		{
			found = 1;
			break;
		}

	if (found)
		return n;
	return (size_t)-1;
}

#define UTF8_0 0x30
#define UTF8_9 0x39
#define UTF8_A 0x41
#define UTF8_F 0x46
#define UTF8_a 0x61
#define UTF8_f 0x66

#define UTF8_EQUAL_SIGN 0x3D

static int utf8_is_digit(uint8_t byte)
{
	return byte >= UTF8_0 && byte <= UTF8_9;
}

static int utf8_is_hexdigit(uint8_t byte)
{
	return (byte >= UTF8_0 && byte <= UTF8_9) || (byte >= UTF8_A && byte <= UTF8_F) || (byte >= UTF8_a && byte <= UTF8_f);
}

static int utf8_is_whitespace(uint8_t byte)
{
	return byte == 0x20 || byte == 0x09 || byte == 0x0A || byte == 0x0D || byte == 0x0B;
}

static uint8_t utf8_from_hex(char character)
{
	if (character <= UTF8_9)
		return character - UTF8_0;
	if (character <= UTF8_F)
		return character - UTF8_A + 10;
	if (character <= UTF8_f)
		return character - UTF8_a + 10;
	return (uint8_t)-1;
}

static uint8_t *utf8_get_hex(const uint8_t* line, size_t size)
{
	uint8_t *seq = calloc(size/2, 1);
	if (seq == NULL)
		return NULL;

	int n;
	for (n = 0; n < size; n++)
	{
		if (!utf8_is_hexdigit(line[n]))
		{
			free(seq);
			return NULL;
		}
		if (n % 2 == 0)
		{
			seq[n/2] = utf8_from_hex(line[n]);
			seq[n/2] <<= 4;
		}
		else
			seq[n/2] |= utf8_from_hex(line[n]);
	}
	return seq;
}

static EntryList * load_table(const uint8_t *contents, size_t size, int *nentries)
{
	assert(contents != NULL);

	uint8_t line[1024];

	EntryList *list = entry_list_create();
	if (list == NULL)
		return NULL;


	int qtd = 0;
	int line_number = 0;

	size_t n = 0;

	while (n < size)
	{
		int bytes_read = get_line(contents+n, size-n, line, sizeof(line));
		line_number++;
		n += bytes_read;

		int line_length = strlen((char*)line);
		if (line_length == 0)
			continue;

		if (line[0] == '#')
			continue; // Comment line
		if (line_length > 1 && line[0] == '/' && line[1] == '/')
			continue; // Comment line
		if (line[0] == '*')
		{
			int key_text_length = line_length-1;
			uint8_t *key = NULL;
			if (key_text_length > 0)
			{
				if (line[1] == UTF8_EQUAL_SIGN)
				{
					key_text_length--;
					if (key_text_length > 0)
						key = utf8_get_hex(&line[2], key_text_length);
				}
				else
					key = utf8_get_hex(&line[1], key_text_length);
			}
			if (key == NULL)
			{
				fprintf(stderr, "table line malformed (line #%i)\n", line_number); // FIXME
				continue;
			}
			else
			{
				size_t value_length = strlen(DEFAULT_PARAGRAPH_MARK);
				uint8_t *value = malloc(value_length);
				if (value == NULL)
					perror("allocating table entry value"); // FIXME
				else
				{
					memcpy(value,DEFAULT_PARAGRAPH_MARK,value_length);

					Entry *e = create_table_entry(key, key_text_length/2, value, value_length, ENTRY_PARAGRAPH);
					if (e == NULL)
						perror("creating table entry"); // FIXME
					else
					{
						if (!entry_list_insert(list, e))
							perror("inserting entry into table"); // FIXME
						else
							qtd++;
					}
				}
			}
			continue;
		}

		if (line[0] == '/')
		{
			int key_text_length = line_length-1;
			uint8_t *key = NULL;
			if (key_text_length > 0)
			{
				if (line[1] == UTF8_EQUAL_SIGN)
				{
					key_text_length--;
					if (key_text_length > 0)
						key = utf8_get_hex(&line[2], key_text_length);
				}
				else
					key = utf8_get_hex(&line[1], key_text_length);
			}
			if (key == NULL)
			{
				fprintf(stderr, "table line malformed (line #%i)\n", line_number); // FIXME
				continue;
			}
			else
			{
				size_t value_length = strlen(DEFAULT_LINEEND_MARK);
				uint8_t *value = malloc(value_length);
				if (value == NULL)
					perror("allocating table entry value"); // FIXME
				else
				{
					memcpy(value,DEFAULT_LINEEND_MARK,value_length);

					Entry *e = create_table_entry(key, key_text_length/2, value, value_length, ENTRY_LINEEND);
					if (e == NULL)
						perror("creating table entry"); // FIXME
					else
					{
						if (!entry_list_insert(list, e))
							perror("inserting entry into table"); // FIXME
						else
							qtd++;
					}
				}
			}
			continue;
		}

		// Normal entry
		size_t pos = search_byte(line, line_length, UTF8_EQUAL_SIGN);

		if (pos > 0 && pos != (size_t)-1)
		{
			size_t pos_value = pos+1;
			size_t value_length = line_length-pos_value;
			if (value_length <= 0)
			{
				continue;
			}
			// Trimming
			while (utf8_is_whitespace(line[pos-1]))
				pos--;
			// Get Hexcode
			if (pos % 2 == 0)
			{
				uint8_t *key = utf8_get_hex(line, pos);
				if (key == NULL)
				{
					fprintf(stderr, "Error while reading hexadecimal key from table (line #%i)\n", line_number); // FIXME
					continue;
				}
				uint8_t *value = malloc(value_length);
				if (value == NULL)
				{
					free(key);
					fprintf(stderr, "Error while storing value from table (line #%i)\n", line_number); // FIXME
					perror("Reason");
					continue;
				}
				memcpy(value, &line[pos_value], value_length);
				Entry *e = create_table_entry(key, pos/2, value, value_length, ENTRY_NORMAL);
				if (e == NULL)
				{
					free(key);
					free(value);
					fprintf(stderr, "Error creating entry for table (line #%i)\n", line_number); // FIXME
					continue;
				}
				if (!entry_list_insert(list, e))
				{
					destroy_table_entry(e);
					fprintf(stderr, "Error while inserting entry from table (line #%i)\n", line_number); // FIXME
					continue;
				}

				qtd++;
			}
			else
			{
				// TODO: Odd number of characters before equal sign
			}
		}
	}
	// TODO: Aceitar convenções de comentários do Toon
	//	# - comentário de Shell Script (meu preferido).
	//	// - comentário de uma linha só, C99 e C++.
	//	/* abre e fecha, comentário de C.

	// TODO: Aceitar marcadores
	// TODO: Entradas especiais

	if (nentries != NULL)
		*nentries = qtd;
	return list;
}


/*
    for i := 1 to length[A]-1 do
    begin
        value := A[i];
        j := i - 1;
        done := false;
        repeat
            { To sort in descending order simply reverse
              the operator i.e. A[j] < value }
            if A[j] > value then
            begin
                A[j + 1] := A[j];
                j := j - 1;
                if j < 0 then
                    done := true;
            end
            else
                done := true;
        until done;
        A[j + 1] := value;
    end;
 */
/*

for(i = 1; i < n; i++)
{
    p1 = head;
    p2 = head->next;
    p3 = p2->next;
 
    for(j = 1; j <= (n - i); j++)
    {
       if(p2->value < p3->value) 
       {
           p2->next = p3->next;
           p3->next = p2;
           p1->next = p3;
           p1       = p3;
           p3       = p2->next;
       }
       else
       {
           p1 = p2;
           p2 = p3;
           p3 = p3->next;
       }
    }
}

 */

static void swapbubble( void * v[], int i)
{
	void * aux;

	aux=v[i];
	v[i] = v[i+1];
	v[i+1] = aux;
}

static void bubble(void * v[], int qtd, int (*compare)(void*,void*))
{
	int i;
	int trocou;

	do
	{
		qtd--;
		trocou = 0;

		for(i = 0; i < qtd; i++)
			if(compare(v[i], v[i + 1]) > 0)
			{
				swapbubble(v, i);
				trocou = 1;

			}
	}while(trocou);
}

static int inverse_compare_entry_key_size(void *a, void *b)
{
	Entry *e = a, *f =b;
	return f->nkey - e->nkey;
}

static int inverse_compare_entry_value_size(void *a, void *b)
{
	Entry *e = a, *f =b;
	return f->nvalue - e->nvalue;
}

static void sort_table_entries(XChangeTable* xt)
{
	assert(xt != NULL);
	if (xt->entries_key == NULL)
		return;

	int n = xt->nentries;
	bubble(xt->entries_key, n, inverse_compare_entry_key_size);
	bubble(xt->entries_value, n, inverse_compare_entry_value_size);
	return;
}

static Entry* search_entry_by_key(Entry * const *list, int list_size, const unsigned char * key, int key_length)
{
	if (list == NULL || key == NULL)
		return NULL;

	if (key_length <= 0)
		return NULL;

	Entry *e = NULL;
	int n;
	for (n = 0; n < list_size; n++)
	{
		e = list[n];
		if (e->nkey != key_length)
			continue;
		if (memcmp(key, e->key, key_length) == 0)
		{
			break;
		}
	}
	return e;
}

int xchange_table_print_stringUTF8(const XChangeTable *table,
		const uint8_t *bytes, size_t size, char *text)
{
	if (table == NULL || bytes == NULL)
		return -1;

	off_t inpos = 0;
	off_t outpos = 0;
	if (table->type == CHARSET_TABLE)
	{
		/*
		char *tmpbuffer = NULL;
		char *inbuf = bytes;
		size_t inleft = size;
		char *outbuf = text;
		size_t outleft = 4*size;
		// Sadly iconv doesn't compute their thing without a valid outbuffer...
		if (text == NULL)
		{
			tmpbuffer = malloc(outleft);
			if (tmpbuffer == NULL)
				return -1;
			outbuf = tmpbuffer;
		}
		if (iconv(table->icd, &inbuf, &inleft, &outbuf, &outleft) == (size_t)-1)
		{
			if (tmpbuffer != NULL)
			{
				free(tmpbuffer);
			}
			return -1;
		}

		if (tmpbuffer != NULL)
		{
			free(tmpbuffer);
		}

		return text != NULL? outbuf - text : outbuf - tmpbuffer;
		*/
		return xchange_table_print_best_stringUTF8(table, bytes, size, text, size, NULL);
	}
	// Table files
	Entry *e;
	int n;
	int unknown_character_length = table->unknown_length;
	while (inpos < size)
	{
		// Search best match entry
		e = NULL;
		for (n = 0; n < table->nentries; n++)
		{
			e = table->entries_key[n];
			if (inpos + e->nkey > size)
				continue;
			if (memcmp(bytes + inpos, e->key, e->nkey) == 0)
			{
				break;
			}
		}
		// Copy entry text
		if (n != table->nentries)
		{
			if (text != NULL)
				memcpy(text+outpos, e->value, e->nvalue);
			outpos += e->nvalue;
			inpos += e->nkey;
		}
		else
		{
			if (text != NULL)
			{
				memcpy(text+outpos, table->unknown, unknown_character_length);
			}
			outpos+=unknown_character_length;
			inpos++;
		}
	}
	return outpos;
}

int xchange_table_print_best_stringUTF8(const XChangeTable *table, const uint8_t *bytes, size_t size, char *text, size_t min_read, size_t *read)
{
	if (table == NULL || bytes == NULL || size == 0 || min_read == 0)
		return -1;

	if (min_read > size)
		return -1;

	off_t inpos = 0;
	off_t outpos = 0;

	if (table->type == CHARSET_TABLE)
	{
		char *tmpbuffer = NULL;
		char *inbuf = bytes;
		size_t inleft = min_read;
		char *outbuf = text;
		size_t outsize = 4*size;
		size_t outleft = outsize;

		// Sadly iconv doesn't compute their thing without a valid outbuffer...
		if (text == NULL)
		{
			tmpbuffer = malloc(outleft);
			if (tmpbuffer == NULL)
				return -1;
			outbuf = tmpbuffer;
		}


		int unknown_character_length = table->unknown_length;
		int error = 0;
		while (inleft > 0)
		{
			if (iconv(table->icd, &inbuf, &inleft, &outbuf, &outleft) == (size_t)-1)
			{
				switch (errno)
				{
				case EILSEQ:
				{
					inbuf++;
					inleft--;
					memcpy(outbuf, table->unknown, unknown_character_length);
					outbuf += unknown_character_length;
					outleft -= unknown_character_length;
					break;
				}
				case EINVAL:
				{
					if (inleft <= 3)
					{
						// Tenta ler o que falta...
						size_t fake_inleft;
						int qtd_extra = 1;

						while (!error || qtd_extra < 5)
						{
							if (min_read < size - qtd_extra)
								fake_inleft = inleft + qtd_extra;
							else
							{
								error = 1;
								break;
							}
							if (iconv(table->icd, &inbuf, &fake_inleft, &outbuf, &outleft) == (size_t)-1)
							{
								if (errno == EINVAL)
								{
									qtd_extra++;
									continue;
								}
								else
								{
									if (errno == EILSEQ)
									{
										// Tudo errado: sequência inválida
										//  copia byte de caractere desconhecido e fim
										if (outleft >= unknown_character_length)
										{
											inbuf++;
											inleft--;
											memcpy(outbuf, table->unknown, unknown_character_length);
											outbuf += unknown_character_length;
											outleft -= unknown_character_length;
										}
										else
											error = 1;
										break;
									}
									perror("Iconv error 1");
									error = 1;
								}
								break;
							}
							else
							{
								inleft = -qtd_extra;
								error = 2;
								break;
							}
						}
					}
					else
					{
						perror("Iconv error 2");
						error = 1;
					}
					break;
				}
				default:
					error = 1;
					perror("Iconv error 3");
					break;
				}
			}
			if (error)
				break;
		}
		if (read != NULL)
			*read = min_read - inleft;
		free(tmpbuffer);
		return outbuf - text;
	}

	Entry *e;
	int n;
	while (inpos < min_read)
	{
		// Search best match entry
		for (n = 0; n < table->nentries; n++)
		{
			e = table->entries_key[n];
			if (inpos + e->nkey > size)
				continue;
			if (memcmp(bytes + inpos, e->key, e->nkey) == 0)
			{
				break;
			}
		}
		// Copy entry text
		if (n != table->nentries)
		{
			if (text != NULL)
				memcpy(text+outpos, e->value, e->nvalue);
			outpos += e->nvalue;
			inpos += e->nkey;
		}
		else
		{
			if (text != NULL)
			{
				memcpy(text+outpos, table->unknown, table->unknown_length);
				outpos+=table->unknown_length;
			}
			else
				outpos+=1;
			inpos++;
		}
	}
	if (read != NULL)
		*read = inpos;
	return outpos;
}

int xchange_table_scan_stringUTF8(const XChangeTable *table, const char *text, size_t size, uint8_t *bytes)
{
	if (table == NULL || text == NULL)
		return -1;

	if (table->type == CHARSET_TABLE)
	{
		char *inbuf = text;
		size_t inleft = size;
		size_t outleft = 4*size;
		char *tmp, *tmp2;
		if (bytes == NULL)
		{
			tmp2 = malloc(outleft);
			if (tmp2 == NULL)
				return -1;
			tmp = tmp2;
		}
		else
			tmp = bytes;
		char *outbuf = tmp;
		iconv_t icd = iconv_open(table->encoding, "utf-8");
		if (icd == (iconv_t)-1)
		{
			return -2;
		}
		if (iconv(icd, &inbuf, &inleft, &outbuf, &outleft) == (size_t)-1)
		{
			if (bytes == NULL)
				free(tmp2);
			return -3;
		}
		iconv_close(icd);
		if (bytes == NULL)
			free(tmp2);

		if (bytes == NULL)
			return outbuf - tmp2;
		else
			return outbuf - (char*)bytes;
	}

	off_t inpos = 0;
	off_t outpos = 0;
	Entry *e;
	int n;
	while (inpos < size)
	{
		for (n = 0; n < table->nentries; n++)
		{
			e = table->entries_value[n];
			if (memcmp(text + inpos, e->value, e->nvalue) == 0)
			{
				break;
			}
		}
		if (n != table->nentries)
		{
			if (bytes != NULL)
				memcpy(bytes+outpos, e->key, e->nkey);
			outpos += e->nkey;
			inpos += e->nvalue;
		}
		else
		{
			return -2;
		}
	}
	return outpos;
}

void xchange_table_dump_entries(XChangeTable *xt)
{
	if (xt == NULL)
		return;

	Entry *e;
	int n;
	int c;
	for (c = 0; c < xt->nentries; c++)
	{
		e = xt->entries_key[c];
		for (n=0; n < e->nkey; n++)
			printf("%0hhx", e->key[n]);
		printf(" - ");
		for (n=0; n < e->nvalue; n++)
			printf("%c", e->value[n]);
		puts("");
	}
}


int xchange_table_set_unknown_charUTF8(XChangeTable *table, const char* character, int length)
{
	if (table == NULL)
		return 0;
	if (character == NULL)
		return 0;

	if (length <= 0)
	{
		length = strlen(character);
		if (length == 0)
			return 0;
	}

	unsigned char *unknown = malloc(length+1);
	if (unknown == 0)
		return 0;

	memcpy(unknown, character, length);
	unknown[length] = 0;

	free(table->unknown);
	table->unknown = unknown;
	table->unknown_length = length;

	return 1;
}

int xchange_table_set_lineend_markUTF8(XChangeTable *table, const char* character, int length)
{
	if (table == NULL)
		return 0;
	if (character == NULL)
		return 0;

	if (length <= 0)
	{
		length = strlen(character);
		if (length == 0)
			return 0;
	}

	if (table->lineend_key == NULL || table->lineend_key_length <= 0)
		return 0;

	unsigned char *new_lineend = malloc(length+1);
	if (new_lineend == 0)
		return 0;

	memcpy(new_lineend, character, length);
	new_lineend[length] = 0;

	Entry *e = search_entry_by_key(table->entries_key, table->nentries, table->lineend_key, table->lineend_key_length);
	if (e == NULL)
	{
		free (new_lineend);
		return 0;
	}
	free(e->value);
	e->value = new_lineend;
	e->nvalue = length;

	bubble(table->entries_value, table->nentries, inverse_compare_entry_value_size);

	return 1;
}

int xchange_table_set_paragraph_markUTF8(XChangeTable *table, const char* character, int length)
{
	if (table == NULL)
		return 0;
	if (character == NULL)
		return 0;

	if (length <= 0)
	{
		length = strlen(character);
		if (length == 0)
			return 0;
	}

	if (table->paragraph_key == NULL || table->paragraph_key_length <= 0)
		return 0;

	unsigned char *new_paragraph = malloc(length+1);
	if (new_paragraph == 0)
		return 0;

	memcpy(new_paragraph, character, length);
	new_paragraph[length] = 0;

	Entry *e = search_entry_by_key(table->entries_key, table->nentries, table->paragraph_key, table->paragraph_key_length);
	if (e == NULL)
	{
		free (new_paragraph);
		return 0;
	}
	free(e->value);
	e->value = new_paragraph;
	e->nvalue = length;

	bubble(table->entries_value, table->nentries, inverse_compare_entry_value_size);

	return 1;
}

const unsigned char* xchange_table_get_unknown_charUTF8(XChangeTable *table, int *length)
{
	if (table == NULL)
		return NULL;
	if (length != NULL)
		*length = table->unknown_length;
	return table->unknown;
}

const uint8_t* xchange_table_get_lineend_markUTF8(XChangeTable *table, int *length)
{
	if (table == NULL)
		return NULL;

	Entry *e = search_entry_by_key(table->entries_key, table->nentries, table->lineend_key, table->lineend_key_length);
	if (e == NULL)
		return NULL;

	if (length != NULL)
		*length = e->nvalue;
	return e->value;
}

const uint8_t* xchange_table_get_paragraph_markUTF8(XChangeTable *table, int *length)
{
	if (table == NULL)
		return NULL;

	Entry *e = search_entry_by_key(table->entries_key, table->nentries, table->paragraph_key, table->paragraph_key_length);
	if (e == NULL)
		return NULL;

	if (length != NULL)
		*length = e->nvalue;
	return e->value;
}
