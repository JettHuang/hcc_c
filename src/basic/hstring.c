/* \brief
		hash string implementation
 */

#include "hstring.h"
#include "mm.h"
#include "utils.h"
#include <string.h>


#define HASH_POOL		MMA_Area_0
#define TABLE_SIZE		1024 

typedef struct tagHashEntry
{
	struct tagHashEntry* _next;
	unsigned int _count;
	char _ch[1];
} FHashEntry;

#define ENTRY_BYTES(len)	(sizeof(FHashEntry) - sizeof(char) + len)
static FHashEntry* g_htable[TABLE_SIZE] = { NULL };


const char* hs_hashstr(const char* str)
{
	size_t len = strlen(str);
	return hs_hashnstr(str, len + 1);
}

const char* hs_hashnstr(const char* str, unsigned int len)
{
	unsigned int hash = util_str_hash(str, len);
	unsigned int index = hash & (TABLE_SIZE - 1);
	FHashEntry* ptrentry = g_htable[index];

	while (ptrentry)
	{
		if (ptrentry->_count == len && !memcmp(ptrentry->_ch, str, len))
		{
			return ptrentry->_ch;
		}

		ptrentry = ptrentry->_next;
	} /* end while */

	ptrentry = (FHashEntry*)mm_alloc_area(ENTRY_BYTES(len), HASH_POOL);
	if (!ptrentry) { return NULL; }

	memcpy(ptrentry->_ch, str, len);
	ptrentry->_count = len;
	ptrentry->_next = g_htable[index];
	g_htable[index] = ptrentry;

	return ptrentry->_ch;
}

const char* hs_hashnstr2(const char* str, unsigned int len)
{
	unsigned int hash = util_str_hash2(str, len);
	unsigned int index = hash & (TABLE_SIZE - 1);
	FHashEntry* ptrentry = g_htable[index];

	len++;
	while (ptrentry)
	{
		if (ptrentry->_count == len && !memcmp(ptrentry->_ch, str, len))
		{
			return ptrentry->_ch;
		}

		ptrentry = ptrentry->_next;
	} /* end while */

	ptrentry = (FHashEntry*)mm_alloc_area(ENTRY_BYTES(len), HASH_POOL);
	if (!ptrentry) { return NULL; }

	memcpy(ptrentry->_ch, str, len-1);
	ptrentry->_ch[len-1] = '\0';
	ptrentry->_count = len;
	ptrentry->_next = g_htable[index];
	g_htable[index] = ptrentry;

	return ptrentry->_ch;
}
