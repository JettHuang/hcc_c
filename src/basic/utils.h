/* \brief
		utility functions
 */

#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdint.h>
#include "mm.h"


unsigned int util_str_hash(const char* str, unsigned int len);
char util_char_lower(char c);
char util_char_upper(char c);

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
	const char* _astr;
	union
	{
		const char* _str;
		uint32_t	_chcnt;
	} _wstr;
} FValue;

#endif /* __UTILS_H__ */
