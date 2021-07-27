/* \brief
 *		token for compiler
 */

#ifndef __CC_TOKEN_H__
#define __CC_TOKEN_H__

#include "stdint.h"
#include "utils.h"


enum ECCToken
{
#define TOKEN(k, s, f)		k,
	#include "../basic/tokens.inl"
#undef TOKEN
};

#define TK_MAX_COUNT  (TK_EOF + 1)

typedef struct tagCCTokenDesc
{
	enum ECCToken _type;
	uint32_t	_flags;
	const char* _text;
} FCCTokenDesc;

extern FCCTokenDesc gCCTokenMetas[TK_MAX_COUNT];

typedef struct tagCCToken
{
	enum ECCToken _type;
	FLocation	_loc;
	FValue		_val;
} FCCToken;

#endif /* __CC_LEXER_H__ */

