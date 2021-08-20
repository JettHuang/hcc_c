/* \brief
 *		types APIs.
 */

#include "cc.h"
#include "types.h"
#include "symbols.h"
#include "logger.h"

#include <float.h>
#include <string.h>


FCCBuiltinTypes gBuiltinTypes = { NULL };
static FCCTypeMetrics sTypeMetric;
static struct tagCCSymbol* sPointersym = NULL;
static struct tagTypeEntry {
	struct tagCCType _type;
	struct tagTypeEntry* _link;
} *stypetable[128];
static int sMaxlevel = 0; /* max level in the type table */


static FCCType* cc_type_new(int16_t op, FCCType* ty, int32_t size, int16_t align, FCCSymbol* sym)
{
	struct tagTypeEntry* tn;
	int h = op ^ ((int)ty >> 3);

	if (op != Type_Function && (op != Type_Array || size > 0))
	{
		for (tn = stypetable[h]; tn; tn = tn->_link)
		{
			if (tn->_type._op == op && tn->_type._type == ty
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

	sMaxlevel = 0;
	memset(stypetable, 0, sizeof(stypetable));
	sTypeMetric = *m;

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

	sPointersym = cc_symbol_install("T*", &gTypes, SCOPE_GLOBAL, CC_MM_PERMPOOL);
}

FCCType* cc_type_tmp(int16_t op, FCCType* ty)
{
	FCCType* tmp;

	tmp = (FCCType*)mm_alloc_area(sizeof(FCCType), CC_MM_TEMPPOOL);
	if (!tmp) {
		logger_output_s("out of memory at %s:%d\n", __FILE__, __LINE__);
		return NULL;
	}

	tmp->_op = op;
	tmp->_type = ty;
	return tmp;
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
	return cc_type_new(Type_Pointer, ty, sTypeMetric._ptrmetric._size, sTypeMetric._ptrmetric._align, sPointersym);
}

FCCType* cc_type_deref(FCCType* ty)
{
	if (IsPtr(ty)) {
		return UnQual(ty)->_type;
	} else {
		logger_output_s("error: pointer expected! but is %t\n", ty);
	}

	return IsEnum(ty) ? ty->_type : ty;
}

FCCType* cc_type_newarray(FCCType* ty, int elecnt, int align)
{
	assert(ty);
	if (IsFunction(ty)) {
		logger_output_s("illegal type 'array of function'.\n");
		return cc_type_newarray(gBuiltinTypes._sinttype, elecnt, align);
	}
	if (IsArray(ty) && ty->_size == 0) {
		logger_output_s("error: missing array size.\n");
	}
	if (ty->_size == 0) {
		if (UnQual(ty) == gBuiltinTypes._voidtype) {
			logger_output_s("error: illegal type 'array of void'.\n");
		}
	}
	else if (elecnt > INT_MAX / ty->_size) {
		logger_output_s("warning: size of array is exceeds %d bytes.\n", INT_MAX);
		elecnt = 1;
	}

	return cc_type_new(Type_Array, ty, elecnt * ty->_size, align ? align : ty->_align, NULL);
}

FCCType* cc_type_arraytoptr(FCCType* ty)
{
	if (IsArray(ty)) {
		return cc_type_ptr(ty->_type);
	}

	logger_output_s("error: array expected. %t\n", ty);
	return cc_type_ptr(ty);
}

FCCType* cc_type_newstruct(int op, const char* name, const FLocation* loc, int level)
{
	FCCSymbol* p;
	
	assert(name);
	if (!name || *name == '\0') {
		name = hs_hashstr(util_itoa(cc_symbol_genlabel(1)));
	}
	else {
		if ((p = cc_symbol_lookup(name, gTypes)) != NULL &&
			(p->_scope == level || (p->_scope == SCOPE_PARAM && level == SCOPE_LOCAL)))
		{
			if (p->_type->_op == op && !p->_defined) {
				return p->_type;
			}
		
			logger_output_s("error: redefinition of type '%s', at %w, previous is at %w \n", name, loc, &p->_loc);
			return NULL;
		}
	}

	p = cc_symbol_install(name, &gTypes, level, CC_MM_PERMPOOL);
	p->_type = cc_type_new(op, NULL, 0, 0, p);
	if (p->_scope > sMaxlevel) {
		sMaxlevel = p->_scope;
	}
	p->_loc = *loc;
	return p->_type;
}

FCCField* cc_type_newfield(const char* name, const FLocation* loc, FCCType* sty, FCCType* fty)
{
	FCCField*p, **q;

	q = &(sty->_u._symbol->_u._s._fields);
	if (!name || *name == '\0')
	{
		name = hs_hashstr(util_itoa(cc_symbol_genlabel(1)));
	}
	for (p = *q; p; q = &p->_next, p = *q)
	{
		if (p->_name == name) {
			logger_output_s("error: duplicated structure field. %s, at %w\n", name, loc);
			return NULL;
		}
	} /* end for */

	p = (FCCField*)mm_alloc_area(sizeof(FCCField), CC_MM_PERMPOOL);
	if (!p) {
		logger_output_s("out of memory. %s:%d\n", __FILE__, __LINE__);
		return NULL;
	}

	*q = p;
	p->_name = name;
	p->_next = NULL;
	p->_type = fty;
	p->_loc = *loc;
	
	return p;
}

FCCField* cc_type_findfield(const char* name, FCCType* sty)
{
	FCCField* p = sty->_u._symbol->_u._s._fields;

	for (; p; p=p->_next)
	{
		if (p->_name == name) {
			return p;
		}
	} /* end for */

	return NULL;
}

FCCField* cc_type_fields(FCCType* sty)
{
	return sty->_u._symbol->_u._s._fields;
}

FCCType* cc_type_func(FCCType* ret, FCCType** proto)
{
	FCCType* ty = NULL;

	if (ret && (IsArray(ret) || IsFunction(ret)))
	{
		logger_output_s("illegal return type: %t \n", ret);
	}

	ty = cc_type_new(Type_Function, ret, 0, 0, NULL);
	ty->_u._f._protos = proto;

	return ty;
}

FCCType* cc_type_rettype(FCCType* fn)
{
	if (IsFunction(fn)) {
		return fn->_type;
	}

	logger_output_s("error: function expected. but current is %t\n", fn);
	return NULL;
}

BOOL cc_type_isvariance(FCCType* fn)
{
	if (IsFunction(fn) && fn->_u._f._protos) {
		int cnt;
		for (cnt = 0; fn->_u._f._protos[cnt]; ++cnt)
		{ /* do nothing */ }

		return cnt > 1 && fn->_u._f._protos[cnt - 1] == gBuiltinTypes._ellipsistype;
	}
	return FALSE;
}

BOOL cc_type_isequal(FCCType* ty1, FCCType* ty2, BOOL option)
{
	if (ty1 == ty2) { return TRUE; }
	if (ty1->_op != ty2->_op) { return FALSE; }
	
	if (IsQual(ty1)) {
		return cc_type_isequal(ty1->_type, ty2->_type, TRUE);
	}

	switch (ty1->_op)
	{
	case Type_Char:
	case Type_SInteger:
	case Type_UInteger:
	case Type_Float:
	case Type_Double:
	case Type_Enum:
	case Type_Union:
	case Type_Struct:
		return FALSE;
	case Type_Pointer:
		return cc_type_isequal(ty1->_type, ty2->_type, TRUE);
	case Type_Array:
		if (cc_type_isequal(ty1->_type, ty2->_type, TRUE)) {
			if (ty1->_size == ty2->_size) {
				return TRUE;
			}
			if (ty1->_size == 0 || ty2->_size == 0) {
				return option;
			}
		}
		return FALSE;
	case Type_Function:
		if (cc_type_isequal(ty1->_type, ty2->_type, TRUE)) {
			FCCType** p1 = ty1->_u._f._protos, **p2 = ty2->_u._f._protos;

			assert(p1 && p2);
			if (p1 == p2) { return TRUE; }
			if (p1 && p2) {
				for (; *p1 && *p2; p1++, p2++)
				{
					if (cc_type_isequal(UnQual(*p1), UnQual(*p2), TRUE) == FALSE)
						return FALSE;
				} /* end for */
			}

			if (*p1 == NULL && *p2 == NULL) {
				return TRUE;
			}
		}
		return FALSE;
	default:
		break;
	}

	assert(0);
	return FALSE;
}

FCCType* cc_type_promote(FCCType* ty)
{
	ty = UnQual(ty);
	switch (ty->_op)
	{
	case Type_Enum:
		return gBuiltinTypes._sinttype;
	case Type_SInteger:
		if (ty->_size < gBuiltinTypes._sinttype->_size) {
			return gBuiltinTypes._sinttype;
		}
		break;
	case Type_UInteger:
		if (ty->_size < gBuiltinTypes._sinttype->_size) {
			return gBuiltinTypes._sinttype;
		}
		if (ty->_size < gBuiltinTypes._uinttype->_size) {
			return gBuiltinTypes._uinttype;
		}
		break;
	case Type_Float:
		if (ty->_size < gBuiltinTypes._doubletype->_size) {
			return gBuiltinTypes._doubletype;
		}
	default:
		break;
	}

	return ty;
}

void cc_type_remove(int level)
{
	int i;

	if (sMaxlevel < level) {
		return; /* not contains types of level */
	}

	sMaxlevel = 0;
	for (i=0; i<(sizeof(stypetable)/sizeof(stypetable[0])); i++)
	{
		struct tagTypeEntry* p, ** q;
		for (q = &stypetable[i]; *q; )
		{
			p = *q;
			if (IsFunction(&p->_type)) {
				q = &p->_link; /* omit, move next */
			}
			else if (p->_type._u._symbol && p->_type._u._symbol->_scope >= level)
			{
				*q = p->_link; /* remove it */
			}
			else
			{
				if (p->_type._u._symbol && p->_type._u._symbol->_scope > sMaxlevel)
				{
					sMaxlevel = p->_type._u._symbol->_scope;
				}
				q = &p->_link;
			}
		}
	} /* end for */
}
