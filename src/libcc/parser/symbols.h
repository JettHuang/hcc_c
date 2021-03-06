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

#define SC_LOWMASK		0xFF
#define SC_INLINE		0x100

/* function specifier */
#define FS_Unknown		0
#define FS_INLINE		1

/* scopes */
#define SCOPE_None		0
#define SCOPE_CONST		1
#define SCOPE_LABEL		2
#define SCOPE_GLOBAL	3
#define SCOPE_PARAM		4
#define SCOPE_LOCAL		5


struct tagStructField;

typedef union tagCCConstVal {
	int64_t		_sint;
	uint64_t	_uint;
	long double	_float;
	uint32_t	_pointer;
	const void* _payload;
} FCCConstVal;

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
	uint16_t _temporary : 1;
	uint16_t _generated : 1;
	uint16_t _defined : 1;
	uint16_t _isparameter : 1;
	uint16_t _isinline : 1;
	uint16_t _notoutput : 1;

	union {
		struct tagCCSymbol** _enumids; /* for enum identifiers, end with NULL */
		struct
		{
			struct tagStructField* _fields;
			uint8_t	_cfields : 1; /* has const field? */
			uint8_t	_vfields : 1; /* has volatile field? */
		} _s; /* for struct(union) */

		struct {
			FCCConstVal _min, _max;
		} _limits; /* for built-in type */

		FCCConstVal	_cnstval; /* for constant or enum ids */
		struct tagVarInitializer* _initializer; /* for variable initialization */

		struct tagCCSymbol* _alias;
	} _u;

	struct tagCCSymbol* _up; /* link to previous symbol in symbol table */

	/* for back-end */
	struct tagXSymbol {
		const char* _name;
		union {
			struct tagCCSymbol* _redirect;
			struct tagCCIRBasicBlock* _basicblock;
		} _u;
		int _refcnt;
		int _frame_offset; /* for argument or local variable's frame offset*/
	} _x;
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
int cc_symbol_gentemp(int cnt);
BOOL cc_symbol_isgenerated(const char* name);

FCCSymbol* cc_symbol_install(const char* name, struct tagCCSymbolTable** tpp, int level, enum EMMArea where);
FCCSymbol* cc_symbol_relocate(const char* name, struct tagCCSymbolTable* src, struct tagCCSymbolTable* dst);
FCCSymbol* cc_symbol_lookup(const char* name, struct tagCCSymbolTable* tp);

void cc_symbol_foreach(struct tagCCContext* ctx, struct tagCCSymbolTable* tp, int level, void (*callbck)(struct tagCCContext* ctx, FCCSymbol* p));

/* new a constant symbol */
FCCSymbol* cc_symbol_constant(struct tagCCType* ty, FCCConstVal val);

/* new a temporary symbol */
FCCSymbol* cc_symbol_temporary(struct tagCCType* ty, int sclass);

/* new a label symbol */
FCCSymbol* cc_symbol_label(const char* id, const FLocation *loc, enum EMMArea where);

void cc_symbol_reset(struct tagCCSymbolTable* tp);

/* duplicate a symbol */
FCCSymbol* cc_symbol_dup(FCCSymbol* p, enum EMMArea where);

#endif /* __CC_SYMBOLS_H__ */
