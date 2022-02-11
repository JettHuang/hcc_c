/* \brief
 *		symbols APIs
 */

#include "cc.h"
#include "symbols.h"
#include "types.h"
#include "logger.h"

#include <string.h>

extern void cc_gen_internalname(struct tagCCSymbol* sym);

#define SYM_HASHSIZE 256

typedef struct tagCCSymbolTable {
	int _level;
	struct tagCCSymbolTable* _previous;
	struct tagSymEntry {
		struct tagCCSymbol _sym;
		struct tagSymEntry* _link;
	} *_buckets[SYM_HASHSIZE];

	struct tagCCSymbol* _all; /* symbols in this level scope */
} FCCSymbolTable;

static struct tagCCSymbolTable sConstants;
static struct tagCCSymbolTable sExternals;
static struct tagCCSymbolTable sIdentifiers;
static struct tagCCSymbolTable sLabels;
static struct tagCCSymbolTable sTypes;

struct tagCCSymbolTable* gConstants = &sConstants;
struct tagCCSymbolTable* gExternals = &sExternals;
struct tagCCSymbolTable* gGlobals = &sIdentifiers;
struct tagCCSymbolTable* gIdentifiers = &sIdentifiers;
struct tagCCSymbolTable* gLabels = &sLabels;
struct tagCCSymbolTable* gTypes = &sTypes;

int gCurrentLevel = SCOPE_GLOBAL;
static int slabelId = 0;
static int stempId = 0;


static void cc_init_table(FCCSymbolTable* tp, int level)
{
	tp->_level = level;
	tp->_previous = NULL;
	tp->_all = NULL;
	memset(tp->_buckets, 0, sizeof(tp->_buckets));
}

/* create a symbol table */
static FCCSymbolTable* cc_new_table(FCCSymbolTable* up, int level)
{
	FCCSymbolTable* tp = (FCCSymbolTable*)mm_alloc_area(sizeof(FCCSymbolTable), CC_MM_TEMPPOOL);
	
	if (!tp) {
		logger_output_s("out of memory at %s:%d\n", __FILE__, __LINE__);
		return NULL;
	}

	cc_init_table(tp, level);
	tp->_previous = up;
	if (up) {
		tp->_all = up->_all;
	}

	return tp;
}

void cc_symbol_reset(struct tagCCSymbolTable* tp)
{
	tp->_previous = NULL;
	tp->_all = NULL;
	memset(tp->_buckets, 0, sizeof(tp->_buckets));
}

void cc_symbol_init()
{
	cc_init_table(&sConstants, SCOPE_CONST);
	cc_init_table(&sLabels, SCOPE_LABEL);
	cc_init_table(&sExternals, SCOPE_GLOBAL);
	cc_init_table(&sIdentifiers, SCOPE_GLOBAL);
	cc_init_table(&sTypes, SCOPE_GLOBAL);
	gCurrentLevel = SCOPE_GLOBAL;
	slabelId = 1;
}

int cc_symbol_genlabel(int cnt)
{
	slabelId += cnt;
	return slabelId - cnt;
}

int cc_symbol_gentemp(int cnt)
{
	stempId += cnt;
	return stempId - cnt;
}

BOOL cc_symbol_isgenerated(const char* name)
{
	return (*name >= '1' && *name <= '9');
}

void cc_symbol_enterscope()
{
	if (++gCurrentLevel == SCOPE_LOCAL)
	{
		stempId = 0;
	}
}

void cc_symbol_exitscope()
{
	cc_type_remove(gCurrentLevel);

	if (gTypes->_level == gCurrentLevel) {
		gTypes = gTypes->_previous;
	}
	if (gIdentifiers->_level == gCurrentLevel) {
		gIdentifiers = gIdentifiers->_previous;
	}

	assert(gCurrentLevel > SCOPE_GLOBAL);
	--gCurrentLevel;
}

FCCSymbol* cc_symbol_install(const char* name, struct tagCCSymbolTable** tpp, int level, enum EMMArea where)
{
	FCCSymbolTable* tp = *tpp;
	struct tagSymEntry* p;
	int h = (int)name & (SYM_HASHSIZE - 1);

	assert(level == SCOPE_GLOBAL || level >= tp->_level);
	if (tp->_level < level) {
		tp = *tpp = cc_new_table(tp, level);
	}

	assert(tp);
	p = (struct tagSymEntry*)mm_alloc_area(sizeof(struct tagSymEntry), where);
	if (!p) {
		logger_output_s("out of memory: %s:%d\n", __FILE__, __LINE__);
		return NULL;
	}

	memset(&p->_sym, 0, sizeof(p->_sym));
	p->_sym._type = NULL;
	p->_sym._name = name;
	p->_sym._scope = level;
	p->_sym._up = tp->_all;
	tp->_all = &p->_sym;
	p->_link = tp->_buckets[h];
	tp->_buckets[h] = p;
	return &p->_sym;
}

FCCSymbol* cc_symbol_relocate(const char* name, struct tagCCSymbolTable* src, struct tagCCSymbolTable* dst)
{
	struct tagSymEntry *p, **q;
	struct tagCCSymbol **r;
	int h = (int)name & (SYM_HASHSIZE - 1);
	
	for (q = &src->_buckets[h]; *q; q = &(*q)->_link)
	{
		if (name == (*q)->_sym._name)
			break;
	} /* end for q */
	assert(*q);

	/* remove the entry from src's hash chain and from its list of all symbols */
	p = *q;
	*q = (*q)->_link;
	for (r = &src->_all; *r && *r != &p->_sym; r = &(*r)->_up)
	{
		/* do nothing */
	}

	assert(*r == &p->_sym);
	*r = p->_sym._up;

	/* insert the entry into dst's hash chain and into its list of all symbols */
	p->_link = dst->_buckets[h];
	dst->_buckets[h] = p;
	p->_sym._up = dst->_all;
	dst->_all = &p->_sym;
	return &p->_sym;
}

FCCSymbol* cc_symbol_lookup(const char* name, struct tagCCSymbolTable* tp)
{
	struct tagSymEntry* p;
	int h = (int)name & (SYM_HASHSIZE - 1);

	assert(tp);
	do 
	{
		for (p = tp->_buckets[h]; p; p = p->_link)
		{
			if (name == p->_sym._name)
				return &p->_sym;
		}
	} while ((tp = tp->_previous) != NULL);

	return NULL;
}

void cc_symbol_foreach(struct tagCCContext* ctx, struct tagCCSymbolTable* tp, int level, void (*callbck)(struct tagCCContext* ctx, FCCSymbol* p))
{
	FCCSymbol* p;

	for (; tp && tp->_level != level; tp = tp->_previous) 
	{ /* do nothing */ }

	if (!tp || tp->_level != level) {
		return;
	}

	for (p = tp->_all; p; p = p->_up)
	{
		(*callbck)(ctx, p);
	}
}

FCCSymbol* cc_symbol_constant(struct tagCCType* ty, FCCConstVal val)
{
	struct tagSymEntry* p;
	int h = (int)val._sint & (SYM_HASHSIZE - 1);

	ty = UnQual(ty);
	for (p = gConstants->_buckets[h]; p; p = p->_link)
	{
		if (cc_type_isequal(ty, p->_sym._type, 1))
		{
			switch (ty->_op)
			{
			case Type_SInteger:
				if (p->_sym._u._cnstval._sint == val._sint) {
					return &p->_sym;
				}
				break;
			case Type_UInteger:
				if (p->_sym._u._cnstval._uint == val._uint) {
					return &p->_sym;
				}
				break;
			case Type_Float:
				if (p->_sym._u._cnstval._float == val._float) {
					return &p->_sym;
				}
				break;
			case Type_Pointer:
				if (p->_sym._u._cnstval._pointer == val._pointer) {
					return &p->_sym;
				}
				break;
			case Type_Array:
				if (p->_sym._u._cnstval._payload == val._payload) {
					return &p->_sym;
				}
				break;
			default:
				assert(0);
				break;
			}
		}
	} /* end for */

	p = (struct tagSymEntry*)mm_alloc_area(sizeof(struct tagSymEntry), CC_MM_PERMPOOL);
	if (!p) {
		logger_output_s("out of memory: %s:%d\n", __FILE__, __LINE__);
		return NULL;
	}

	memset(&p->_sym, 0, sizeof(p->_sym));
	p->_sym._name = hs_hashstr(util_itoa(cc_symbol_genlabel(1)));
	p->_sym._type = ty;
	p->_sym._scope = SCOPE_CONST;
	p->_sym._sclass = SC_Static;
	p->_sym._u._cnstval = val;
	p->_sym._defined = 1;
	p->_sym._generated = 1;
	p->_sym._up = gConstants->_all;
	gConstants->_all = &p->_sym;
	p->_link = gConstants->_buckets[h];
	gConstants->_buckets[h] = p;

	cc_gen_internalname(&p->_sym);
	return &p->_sym;
}

FCCSymbol* cc_symbol_temporary(struct tagCCType* ty, int sclass)
{
	FCCSymbol* p;

	p = mm_alloc_area(sizeof(struct tagCCSymbol), CC_MM_TEMPPOOL);
	if (!p) { return NULL; }

	memset(p, 0, sizeof(*p));
	p->_name = hs_hashstr(util_itoa(cc_symbol_gentemp(1)));
	p->_type = ty;
	p->_sclass = sclass;
	p->_scope = gCurrentLevel;
	p->_generated = 1;
	p->_temporary = 1;
	p->_defined = 1;

	return p;
}

FCCSymbol* cc_symbol_label(const char* id, const FLocation* loc, enum EMMArea where)
{
	FCCSymbol* p;
	BOOL isgenerated;

	isgenerated = !id;
	if (isgenerated) {
		id = hs_hashstr(util_itoa(cc_symbol_genlabel(1)));
	}
	p = cc_symbol_install(id, &gLabels, SCOPE_LABEL, where);
	p->_generated = isgenerated;
	p->_defined = isgenerated;
	if (loc) { p->_loc = *loc; }

	cc_gen_internalname(p);
	return p;
}

FCCSymbol* cc_symbol_dup(FCCSymbol* p, enum EMMArea where)
{
	FCCSymbol* q;

	q = mm_alloc_area(sizeof(struct tagCCSymbol), where);
	if (!q) { return NULL; }

	*q = *p;
	q->_up = NULL;

	return q;
}
