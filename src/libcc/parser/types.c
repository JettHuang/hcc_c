/* \brief
 *		types APIs.
 */

#include "cc.h"
#include "types.h"
#include "symbols.h"
#include "logger.h"

#include <float.h>


FCCBuiltinTypes gBuiltinTypes = { NULL };

static struct tagCCSymbol* spointersym = NULL;
static struct tagTypeEntry {
	struct tagCCType _type;
	struct tagTypeEntry* _link;
} *stypetable[128];


static FCCType* cc_type_new(int16_t op, FCCType* ty, int32_t size, int16_t align, FCCSymbol* sym)
{
	struct tagTypeEntry* tn;
	int h = op ^ ((int)ty >> 3);

	if (op != Type_Function && (op != Type_Array || size > 0))
	{
		for (tn = stypetable[h]; tn; tn = tn->_link)
		{
			if (tn->_type._op = op && tn->_type._type == ty
				&& tn->_type._size == size && tn->_type._align == align
				&& tn->_type._u._symbol == sym)
			{
				return &tn->_type;
			}
		} /* end for tn */
	}

	tn = (struct tagTypeEntry*)mm_alloc_area(sizeof(struct tagTypeEntry), CC_MM_PERMPOOL);
	if (!tn) {
		logger_output_s("out of memory at %s:%d\n", __FILE__, __LINE__);
		return NULL;
	}

	tn->_type._op = op;
	tn->_type._type = ty;
	tn->_type._size = size;
	tn->_type._align = align;
	tn->_type._u._symbol = sym;
	tn->_link = stypetable[h];
	stypetable[h] = tn;
	return &tn->_type;
}

void cc_type_init(const FCCTypeMetrics* m)
{
	FCCSymbol* p;

	{
		p = cc_symbol_install("char", &gTypes, SCOPE_GLOBAL, CC_MM_PERMPOOL);
		gBuiltinTypes._chartype = cc_type_new(Type_Char, NULL, m->_charmetric._size, m->_charmetric._align, p);
		p->_type = gBuiltinTypes._chartype;
		p->_u._limits._max._llong = ones(8 * p->_type->_size) >> 1;
		p->_u._limits._min._llong = -p->_u._limits._max._llong - 1;
	}
	{
		p = cc_symbol_install("wide char", &gTypes, SCOPE_GLOBAL, CC_MM_PERMPOOL);
		gBuiltinTypes._wchartype = cc_type_new(Type_Char, NULL, m->_wcharmetric._size, m->_wcharmetric._align, p);
		p->_type = gBuiltinTypes._wchartype;
		p->_u._limits._max._llong = ones(8 * p->_type->_size) >> 1;
		p->_u._limits._min._llong = -p->_u._limits._max._llong - 1;
	}
	{
		p = cc_symbol_install("signed char", &gTypes, SCOPE_GLOBAL, CC_MM_PERMPOOL);
		gBuiltinTypes._schartype = cc_type_new(Type_Char, NULL, m->_charmetric._size, m->_charmetric._align, p);
		p->_type = gBuiltinTypes._schartype;
		p->_u._limits._max._llong = ones(8 * p->_type->_size) >> 1;
		p->_u._limits._min._llong = -p->_u._limits._max._llong - 1;
	}
	{
		p = cc_symbol_install("unsigned char", &gTypes, SCOPE_GLOBAL, CC_MM_PERMPOOL);
		gBuiltinTypes._uchartype = cc_type_new(Type_Char, NULL, m->_charmetric._size, m->_charmetric._align, p);
		p->_type = gBuiltinTypes._uchartype;
		p->_u._limits._max._llong = ones(8 * p->_type->_size);
		p->_u._limits._min._llong = 0;
	}
	{
		p = cc_symbol_install("signed short", &gTypes, SCOPE_GLOBAL, CC_MM_PERMPOOL);
		gBuiltinTypes._sshorttype = cc_type_new(Type_SInteger, NULL, m->_shortmetric._size, m->_shortmetric._align, p);
		p->_type = gBuiltinTypes._sshorttype;
		p->_u._limits._max._llong = ones(8 * p->_type->_size) >> 1;
		p->_u._limits._min._llong = -p->_u._limits._max._llong - 1;
	}
	{
		p = cc_symbol_install("unsigned short", &gTypes, SCOPE_GLOBAL, CC_MM_PERMPOOL);
		gBuiltinTypes._ushorttype = cc_type_new(Type_UInteger, NULL, m->_shortmetric._size, m->_shortmetric._align, p);
		p->_type = gBuiltinTypes._ushorttype;
		p->_u._limits._max._llong = ones(8 * p->_type->_size);
		p->_u._limits._min._llong = 0;
	}
	{
		p = cc_symbol_install("signed int", &gTypes, SCOPE_GLOBAL, CC_MM_PERMPOOL);
		gBuiltinTypes._sinttype = cc_type_new(Type_SInteger, NULL, m->_intmetric._size, m->_intmetric._align, p);
		p->_type = gBuiltinTypes._sinttype;
		p->_u._limits._max._llong = ones(8 * p->_type->_size) >> 1;
		p->_u._limits._min._llong = -p->_u._limits._max._llong - 1;
	}
	{
		p = cc_symbol_install("unsigned int", &gTypes, SCOPE_GLOBAL, CC_MM_PERMPOOL);
		gBuiltinTypes._uinttype = cc_type_new(Type_UInteger, NULL, m->_intmetric._size, m->_intmetric._align, p);
		p->_type = gBuiltinTypes._uinttype;
		p->_u._limits._max._llong = ones(8 * p->_type->_size);
		p->_u._limits._min._llong = 0;
	}
	{
		p = cc_symbol_install("signed long", &gTypes, SCOPE_GLOBAL, CC_MM_PERMPOOL);
		gBuiltinTypes._slongtype = cc_type_new(Type_SInteger, NULL, m->_longmetric._size, m->_longmetric._align, p);
		p->_type = gBuiltinTypes._slongtype;
		p->_u._limits._max._llong = ones(8 * p->_type->_size) >> 1;
		p->_u._limits._min._llong = -p->_u._limits._max._llong - 1;
	}
	{
		p = cc_symbol_install("unsigned long", &gTypes, SCOPE_GLOBAL, CC_MM_PERMPOOL);
		gBuiltinTypes._ulongtype = cc_type_new(Type_UInteger, NULL, m->_longmetric._size, m->_longmetric._align, p);
		p->_type = gBuiltinTypes._ulongtype;
		p->_u._limits._max._llong = ones(8 * p->_type->_size);
		p->_u._limits._min._llong = 0;
	}
	{
		p = cc_symbol_install("signed long long", &gTypes, SCOPE_GLOBAL, CC_MM_PERMPOOL);
		gBuiltinTypes._sllongtype = cc_type_new(Type_SInteger, NULL, m->_longlongmetric._size, m->_longlongmetric._align, p);
		p->_type = gBuiltinTypes._sllongtype;
		p->_u._limits._max._llong = ones(8 * p->_type->_size) >> 1;
		p->_u._limits._min._llong = -p->_u._limits._max._llong - 1;
	}
	{
		p = cc_symbol_install("unsigned long long", &gTypes, SCOPE_GLOBAL, CC_MM_PERMPOOL);
		gBuiltinTypes._ullongtype = cc_type_new(Type_UInteger, NULL, m->_longlongmetric._size, m->_longlongmetric._align, p);
		p->_type = gBuiltinTypes._ullongtype;
		p->_u._limits._max._llong = ones(8 * p->_type->_size);
		p->_u._limits._min._llong = 0;
	}
	{
		p = cc_symbol_install("float", &gTypes, SCOPE_GLOBAL, CC_MM_PERMPOOL);
		gBuiltinTypes._floattype = cc_type_new(Type_Float, NULL, m->_floatmetric._size, m->_floatmetric._align, p);
		p->_type = gBuiltinTypes._floattype;
		p->_u._limits._max._ldouble = FLT_MAX;
		p->_u._limits._min._ldouble = FLT_MIN;
	}
	{
		p = cc_symbol_install("double", &gTypes, SCOPE_GLOBAL, CC_MM_PERMPOOL);
		gBuiltinTypes._doubletype = cc_type_new(Type_Double, NULL, m->_doublemetric._size, m->_doublemetric._align, p);
		p->_type = gBuiltinTypes._doubletype;
		p->_u._limits._max._ldouble = DBL_MAX;
		p->_u._limits._min._ldouble = DBL_MIN;
	}
	{
		p = cc_symbol_install("long double", &gTypes, SCOPE_GLOBAL, CC_MM_PERMPOOL);
		gBuiltinTypes._ldoubletype = cc_type_new(Type_Double, NULL, m->_longdoublemetric._size, m->_longdoublemetric._align, p);
		p->_type = gBuiltinTypes._ldoubletype;
		p->_u._limits._max._ldouble = LDBL_MAX;
		p->_u._limits._min._ldouble = LDBL_MIN;
	}
	{
		p = cc_symbol_install("void", &gTypes, SCOPE_GLOBAL, CC_MM_PERMPOOL);
		gBuiltinTypes._voidtype = cc_type_new(Type_Void, NULL, 0, 0, p);
	}
	{
		p = cc_symbol_install("...", &gTypes, SCOPE_GLOBAL, CC_MM_PERMPOOL);
		gBuiltinTypes._ellipsistype = cc_type_new(Type_Ellipsis, NULL, 0, 0, p);
	}

	spointersym = cc_symbol_install("T*", &gTypes, SCOPE_GLOBAL, CC_MM_PERMPOOL);
}

FCCType* cc_type_qual(FCCType* ty, int16_t	op)
{
	if (IsArray(ty)) {
		ty = cc_type_new(Type_Array, cc_type_qual(ty->_type, op), ty->_size, ty->_align, NULL);
	}
	else if (IsFunction(ty)) {
		logger_output_s("warning: qualified function type ignored.\n");
	}
	else if ((IsConst(ty) && (op & Type_Const)) || (IsVolatile(ty) && (op & Type_Volatile))) {
		logger_output_s("warning: illegal type qualifiers.\n");
	}
	else {
		if (IsQual(ty)) {
			op |= ty->_op;
			ty = ty->_type;
		}

		ty = cc_type_new(op, ty, ty->_size, ty->_align, NULL);
	}

	return ty;
}

FCCType* cc_type_ptr(FCCType* ty)
{
	// TODO:
	return NULL;
}

FCCType* cc_type_deref(FCCType* ty)
{
	// TODO:
	return NULL;
}

FCCType* cc_type_newarray(FCCType* ty, int elecnt, int align)
{
	// TODO:
	return NULL;
}

FCCType* cc_type_arraytoptr(FCCType* ty)
{
	// TODO:
	return NULL;
}

FCCType* cc_type_newstruct(int op, const char* name, int level)
{
	// TODO:
	return NULL;
}

FCCField* cc_type_newfield(const char* name, FCCType* sty, FCCType* fty)
{
	// TODO:
	return NULL;
}

FCCField* cc_type_findfield(const char* name, FCCType* sty)
{
	// TODO:
	return NULL;
}

FCCField* cc_type_fields(FCCType* sty)
{
	// TODO:
	return NULL;
}

FCCType* cc_type_func(FCCType* ret, FCCType** proto)
{
	// TODO:
	return NULL;
}

FCCType* cc_type_rettype(FCCType* fn)
{
	// TODO:
	return NULL;
}

BOOL cc_type_isvariance(FCCType* fn)
{
	// TODO:
	return FALSE;
}

BOOL cc_type_isequal(FCCType* ty1, FCCType* ty2)
{
	// TODO:
	return FALSE;
}

FCCType* cc_type_promote(FCCType* ty)
{
	// TODO:
	return NULL;
}

FCCType* cc_type_remove(int level)
{
	// TODO:
	return NULL;
}
