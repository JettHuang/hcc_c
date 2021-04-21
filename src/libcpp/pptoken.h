/* \brief
 *		token for preprocessor
 */

#ifndef __PP_TOKEN_H__
#define __PP_TOKEN_H__

#include "stdint.h"
#include "utils.h"


enum EPPToken
{
#define TOKEN(k, s, f)		k,
	#include "../basic/tokens.inl"
#undef TOKEN
};

typedef struct tagPPToken
{
	enum EPPToken _type;
	const char* _str;
	uint32_t	_wscnt; /* number of white space chars  at the head of _Chars.  ie. '    ABC' */
	int32_t		_hidesetid;

	FLocation	_loc;
} FPPToken;

#endif /* __PP_TOKEN_H__ */
