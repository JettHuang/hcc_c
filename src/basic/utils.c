/* \brief
		utility functions
 */

#include "utils.h"
#include "hstring.h"

#include <string.h>
#include <ctype.h>
#include <assert.h>
#ifdef _WIN32
#include <windows.h>
#endif


unsigned int util_str_hash(const char* str, unsigned int len)
{
	unsigned int h = 0;
	while (len-- > 0)
	{
		h = h * 13131 + (unsigned int)*str++;
	}

	return h;
}

unsigned int util_str_hash2(const char* str, unsigned int len)
{
	unsigned int h = 0;
	while (len-- > 0)
	{
		h = h * 13131 + (unsigned int)*str++;
	}

	h = h * 13131 + (unsigned int)'\0';
	return h;
}

char util_char_lower(char c)
{
	return (unsigned short)c - ((((unsigned short)c - 'a') < 26u) << 5);
}

char util_char_upper(char c)
{
	return (unsigned short)c + ((((unsigned short)c - 'A') < 26u) << 5);
}


const char* util_convert_abs_pathname(const char* pathname)
{
	if (util_is_relative_pathname(pathname))
	{
		char szbuf[1024];
		const char* dir = util_process_working_dir();
		const char* normal = util_normalize_pathname(pathname);

		strcpy(szbuf, dir);
		strcat(szbuf, normal);

		return hs_hashstr(szbuf);
	}

	return hs_hashstr(pathname);
}

BOOL util_is_relative_pathname(const char* pathname)
{
	int len = strlen(pathname);
	int bIsRooted = len &&
		(pathname[0] == '/' || /* linux root */
			(len >= 2 &&
				((pathname[0] == '//' && pathname[1] == '//') || /* network */
					(pathname[1] == ':' && isalpha(pathname[0])) /* windows */
					)
				)
			);

	return !bIsRooted;
}

/* convert xxx\bbb\cc\ to xxx/bbb/cc/ */
const char* util_normalize_pathname(const char* pathname)
{
	char szbuf[1024], *ptrstr;

	strcpy(szbuf, pathname);
	for (ptrstr = szbuf; *ptrstr; ++ptrstr)
	{
		if (*ptrstr == '\\') {
			*ptrstr = '/';
		}
	}

	return hs_hashstr(szbuf);
}

const char* util_process_bin_dir()
{
	static const char* binstr = NULL;
	
	if (binstr) { return binstr; }

#ifdef _WIN32
	{
		char szPath[MAX_PATH];

		if (GetModuleFileNameA(NULL, szPath, MAX_PATH))
		{
			char* ptr = strrchr(szPath, '\\');
			if (ptr)
			{
				*(ptr+1) = '\0';
			}

			binstr = util_normalize_pathname(szPath);
		}
	}

#else
#error "not implementate"
#endif

	return binstr;
}

const char* util_process_working_dir()
{
	static const char* workstr = NULL;

	if (workstr) { return workstr; }

#ifdef _WIN32
	{
		char szPath[MAX_PATH];

		if (GetCurrentDirectoryA(MAX_PATH, szPath))
		{
			strcat(szPath, "/");
			workstr = util_normalize_pathname(szPath);
		}
	}

#else
#error "not implementate"
#endif

	return workstr;
}

const char* util_getpath_from_pathname(const char* pathname)
{
	char szbuf[1024], *ptr;

	strcpy(szbuf, pathname);
	ptr = strrchr(szbuf, '/');
	if (ptr) {
		*(ptr + 1) = '\0';
		
		return hs_hashstr(szbuf);
	}

	return NULL;
}

const char* util_make_pathname(const char* path, const char* filename)
{
	char szbuf[1024];
	
	szbuf[0] = '\0';
	if (path) {
		strcat(szbuf, path);
		strcat(szbuf, "/");
	}
	strcat(szbuf, filename);
	return hs_hashstr(szbuf);
}


void array_init(FArray* array, int cap, int elesize, enum EMMArea where)
{
	array->_capacity = cap;
	array->_data = NULL;
	array->_elecount = 0;
	array->_elesize = elesize;
	array->_marea = where;

	array->_data = mm_alloc_area(cap * elesize, where);
}

void array_make_cap_enough(FArray* array, int count)
{
	if (array->_capacity < count)
	{
		int newcap = count * 2;
		void* data = mm_alloc_area(newcap * array->_elesize, array->_marea);
		if (data)
		{
			memcpy(data, array->_data, array->_elecount * array->_elesize);
		}

		assert(data);
		array->_data = data;
		array->_capacity = newcap;
	}
}

void array_append(FArray* array, void* ptritem)
{
	void* data;

	array_make_cap_enough(array, array->_elecount + 1);
	data = (char*)(array->_data) + array->_elesize * array->_elecount;
	memcpy(data, ptritem, array->_elesize);
	array->_elecount++;
}

void array_append_zeroed(FArray* array)
{
	void* data;

	array_make_cap_enough(array, array->_elecount + 1);
	data = (char*)(array->_data) + array->_elesize * array->_elecount;
	memset(data, 0, array->_elesize);
	array->_elecount++;
}

void array_copy(FArray* dst, FArray* src)
{
	assert(dst->_capacity >= src->_elecount);
	assert(dst->_elesize == src->_elesize);

	memcpy(dst->_data, src->_data, src->_elecount * src->_elesize);
	dst->_elecount = src->_elecount;
}

void* array_element(FArray* array, int index)
{
	void* data;

	assert(array->_elecount > index && index >= 0);
	data = (char*)(array->_data) + array->_elesize * index;
	return data;
}