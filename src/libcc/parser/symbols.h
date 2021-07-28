/* \brief
 *	Symbols
 */

#ifndef __CC_SYMBOLS_H__
#define __CC_SYMBOLS_H__

#include "utils.h"
#include "hstring.h"
#include "mm.h"


/* storage classes */
#define SC_Unknown		0
#define SC_Auto			1
#define SC_Register		2
#define SC_Static		3
#define SC_External		4
#define SC_Typedef		5
#define SC_Enum			6

/* function specifier */
#define FS_Unknown		0
#define FS_Normal		1
#define FS_INLINE		2

/* scopes */
#define SCOPE_None		0
#define SCOPE_CONST		1
#define SCOPE_LABEL		2
#define SCOPE_GLOBAL	3
#define SCOPE_PARAM		4
#define SCOPE_LOCAL		5


/* symbol */
typedef struct tagCCSymbol
{
	const char *_name;
	FLocation _loc;

	int32_t	_scope;
	int32_t _sclass;

	struct tagType *_type;
	FValue	_value;

	/* flags */
	uint16_t _addressed : 1;
	uint16_t _computed : 1;
	uint16_t _temporary : 1;
	uint16_t _generated : 1;
	uint16_t _defined : 1;
	uint16_t _isparameter : 1;
} FCCSymbol;

typedef struct tagSymbolListNode
{
	FCCSymbol _symbol;
	struct tagSymbolListNode *_next;
} FSymbolListNode;



#endif /* __CC_SYMBOLS_H__ */
