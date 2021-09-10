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


struct tagStructField;
/* symbol */
typedef struct tagCCSymbol
{
	const char *_name;
	FLocation _loc;

	int32_t	_scope;
	int32_t _sclass;

	struct tagCCType* _type;
	
	/* flags */
	uint16_t _addressed : 1;
	uint16_t _computed : 1;
	uint16_t _temporary : 1;
	uint16_t _generated : 1;
	uint16_t _defined : 1;
	uint16_t _isparameter : 1;

	union {
		struct tagCCSymbol** _enumids; /* for enum identifiers, end with NULL */
		struct
		{
			struct tagStructField* _fields;
			uint8_t	_cfields : 1; /* has const field? */
			uint8_t	_vfields : 1; /* has volatile field? */
		} _s; /* for struct(union) */

		struct {
			FValue _min, _max;
		} _limits; /* for built-in type */

		FValue	_value; /* for constant or enum ids */
		struct tagVarInitializer* _initializer; /* for variable initialization */

		struct tagCCSymbol* _alias;
	} _u;

	struct tagCCSymbol* _up; /* link to previous symbol in symbol table */
} FCCSymbol;


extern struct tagCCSymbolTable* gConstants;
extern struct tagCCSymbolTable* gExternals;
extern struct tagCCSymbolTable* gGlobals;
extern struct tagCCSymbolTable* gIdentifiers;
extern struct tagCCSymbolTable* gLabels;
extern struct tagCCSymbolTable* gTypes;
extern int gCurrentLevel;

void cc_symbol_init();
void cc_symbol_enterscope();
void cc_symbol_exitscope();

/* generate n-labels, return the start number */
int cc_symbol_genlabel(int cnt);
BOOL cc_symbol_isgenlabel(const char* name);

FCCSymbol* cc_symbol_install(const char* name, struct tagCCSymbolTable** tpp, int level, enum EMMArea where);
FCCSymbol* cc_symbol_relocate(const char* name, struct tagCCSymbolTable* src, struct tagCCSymbolTable* dst);
FCCSymbol* cc_symbol_lookup(const char* name, struct tagCCSymbolTable* tp);

/* new a constant symbol */
FCCSymbol* cc_symbol_constant(struct tagCCType* ty, FValue val);

#endif /* __CC_SYMBOLS_H__ */
