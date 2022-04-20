/* \brief
		utility functions
 */

#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdint.h>
#include <assert.h>
#include "mm.h"


#define TRUE		1
#define FALSE		0
typedef int			BOOL;

#ifndef MIN
#define MIN(x, y)	((x < y) ? x : y)
#endif

#ifndef MAX
#define MAX(x, y)	((x > y) ? x : y)
#endif

typedef struct tagLocation {
	const char* _filename;
	unsigned int _line;
	unsigned int _col;
} FLocation;

typedef union tagValue {
	char		_ch;
	int16_t		_wch;
	int32_t		_int;
	uint32_t	_uint;
	int32_t		_long;
	uint32_t	_ulong;
	int64_t		_llong;
	uint64_t	_ullong;
	float		_float;
	double		_double;
	long double	_ldouble;
	struct
	{
		const char* _str;
		uint32_t	_chcnt; /* include tail '00' */
	} _astr, _wstr;
} FValue;

#define ones(n) ((n)>=8*sizeof (unsigned long) ? ~0UL : ~((~0UL)<<(n))) /* n bits */
#define ELEMENTSCNT(a)	((int)(sizeof(a) / sizeof((a)[0])))

unsigned int util_str_hash(const char* str, unsigned int len);
unsigned int util_str_hash2(const char* str, unsigned int len);
char util_char_lower(char c);
char util_char_upper(char c);

const char* util_convert_abs_pathname(const char* pathname);
BOOL util_is_relative_pathname(const char* pathname);

/* convert xxx\bbb\cc\ to xxx/bbb/cc/ */
const char* util_normalize_pathname(const char* pathname);
const char* util_normalize_pathdir(const char* pathname);
const char* util_process_bin_dir();
const char* util_process_working_dir();
const char* util_getpath_from_pathname(const char* pathname);
const char* util_make_pathname(const char* path, const char* filename);

const char* util_stringf(const char* format, ...);
/* integer to string */
const char* util_itoa(int i);
/* round up (align = 2^n) */
int util_roundup(int val, int align);

void* util_memset(void *s, int ch, unsigned int n);

typedef struct tagArray
{
	void* _data;
	int	  _elecount;
	int   _capacity;
	int   _elesize;
	enum EMMArea _marea;
} FArray;

void array_init(FArray* array, int cap, int elesize, enum EMMArea where);
void array_make_cap_enough(FArray* array, int count);
void array_append(FArray* array, const void* ptritem);
void array_append_zeroed(FArray* array);
void array_popback(FArray* array);
void array_clear(FArray* array);
void array_copy(FArray* dst, FArray* src);
/* return pointer to item */
void* array_element(FArray* array, int index);

#endif /* __UTILS_H__ */
