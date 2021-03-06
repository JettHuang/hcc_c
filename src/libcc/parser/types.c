/* \brief
 *		types APIs.
 */

#include "cc.h"
#include "types.h"
#include "symbols.h"
#include "logger.h"

#include <float.h>
#include <string.h>


FCCBuiltinTypes gbuiltintypes = { NULL };
int gdefcall = Type_Cdecl;
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
	int h = (op ^ ((int)ty >> 3)) & (ELEMENTSCNT(stypetable) - 1);

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
	util_memset(stypetable, 0, sizeof(stypetable));
	sTypeMetric = *m;

	{
		p = cc_symbol_install("char", &gTypes, SCOPE_GLOBAL, CC_MM_PERMPOOL);
		gbuiltintypes._chartype = cc_type_new(Type_UInteger, NULL, m->_charmetric._size, m->_charmetric._align, p);
		p->_type = gbuiltintypes._chartype;
		p->_u._limits._max._sint = ones(8 * p->_type->_size) >> 1;
		p->_u._limits._min._sint = -p->_u._limits._max._sint - 1;
	}
	{
		p = cc_symbol_install("wide char", &gTypes, SCOPE_GLOBAL, CC_MM_PERMPOOL);
		gbuiltintypes._wchartype = cc_type_new(Type_UInteger, NULL, m->_wcharmetric._size, m->_wcharmetric._align, p);
		p->_type = gbuiltintypes._wchartype;
		p->_u._limits._max._sint = ones(8 * p->_type->_size) >> 1;
		p->_u._limits._min._sint = -p->_u._limits._max._sint - 1;
	}
	{
		p = cc_symbol_install("signed char", &gTypes, SCOPE_GLOBAL, CC_MM_PERMPOOL);
		gbuiltintypes._schartype = cc_type_new(Type_SInteger, NULL, m->_charmetric._size, m->_charmetric._align, p);
		p->_type = gbuiltintypes._schartype;
		p->_u._limits._max._sint = ones(8 * p->_type->_size) >> 1;
		p->_u._limits._min._sint = -p->_u._limits._max._sint - 1;
	}
	{
		p = cc_symbol_install("unsigned char", &gTypes, SCOPE_GLOBAL, CC_MM_PERMPOOL);
		gbuiltintypes._uchartype = cc_type_new(Type_UInteger, NULL, m->_charmetric._size, m->_charmetric._align, p);
		p->_type = gbuiltintypes._uchartype;
		p->_u._limits._max._sint = ones(8 * p->_type->_size);
		p->_u._limits._min._sint = 0;
	}
	{
		p = cc_symbol_install("signed short", &gTypes, SCOPE_GLOBAL, CC_MM_PERMPOOL);
		gbuiltintypes._sshorttype = cc_type_new(Type_SInteger, NULL, m->_shortmetric._size, m->_shortmetric._align, p);
		p->_type = gbuiltintypes._sshorttype;
		p->_u._limits._max._sint = ones(8 * p->_type->_size) >> 1;
		p->_u._limits._min._sint = -p->_u._limits._max._sint - 1;
	}
	{
		p = cc_symbol_install("unsigned short", &gTypes, SCOPE_GLOBAL, CC_MM_PERMPOOL);
		gbuiltintypes._ushorttype = cc_type_new(Type_UInteger, NULL, m->_shortmetric._size, m->_shortmetric._align, p);
		p->_type = gbuiltintypes._ushorttype;
		p->_u._limits._max._sint = ones(8 * p->_type->_size);
		p->_u._limits._min._sint = 0;
	}
	{
		p = cc_symbol_install("signed int", &gTypes, SCOPE_GLOBAL, CC_MM_PERMPOOL);
		gbuiltintypes._sinttype = cc_type_new(Type_SInteger, NULL, m->_intmetric._size, m->_intmetric._align, p);
		p->_type = gbuiltintypes._sinttype;
		p->_u._limits._max._sint = ones(8 * p->_type->_size) >> 1;
		p->_u._limits._min._sint = -p->_u._limits._max._sint - 1;
	}
	{
		p = cc_symbol_install("unsigned int", &gTypes, SCOPE_GLOBAL, CC_MM_PERMPOOL);
		gbuiltintypes._uinttype = cc_type_new(Type_UInteger, NULL, m->_intmetric._size, m->_intmetric._align, p);
		p->_type = gbuiltintypes._uinttype;
		p->_u._limits._max._sint = ones(8 * p->_type->_size);
		p->_u._limits._min._sint = 0;
	}
	{
		p = cc_symbol_install("signed long", &gTypes, SCOPE_GLOBAL, CC_MM_PERMPOOL);
		gbuiltintypes._slongtype = cc_type_new(Type_SInteger, NULL, m->_longmetric._size, m->_longmetric._align, p);
		p->_type = gbuiltintypes._slongtype;
		p->_u._limits._max._sint = ones(8 * p->_type->_size) >> 1;
		p->_u._limits._min._sint = -p->_u._limits._max._sint - 1;
	}
	{
		p = cc_symbol_install("unsigned long", &gTypes, SCOPE_GLOBAL, CC_MM_PERMPOOL);
		gbuiltintypes._ulongtype = cc_type_new(Type_UInteger, NULL, m->_longmetric._size, m->_longmetric._align, p);
		p->_type = gbuiltintypes._ulongtype;
		p->_u._limits._max._sint = ones(8 * p->_type->_size);
		p->_u._limits._min._sint = 0;
	}
	{
		p = cc_symbol_install("signed long long", &gTypes, SCOPE_GLOBAL, CC_MM_PERMPOOL);
		gbuiltintypes._sllongtype = cc_type_new(Type_SInteger, NULL, m->_longlongmetric._size, m->_longlongmetric._align, p);
		p->_type = gbuiltintypes._sllongtype;
		p->_u._limits._max._sint = ones(8 * p->_type->_size) >> 1;
		p->_u._limits._min._sint = -p->_u._limits._max._sint - 1;
	}
	{
		p = cc_symbol_install("unsigned long long", &gTypes, SCOPE_GLOBAL, CC_MM_PERMPOOL);
		gbuiltintypes._ullongtype = cc_type_new(Type_UInteger, NULL, m->_longlongmetric._size, m->_longlongmetric._align, p);
		p->_type = gbuiltintypes._ullongtype;
		p->_u._limits._max._sint = ones(8 * p->_type->_size);
		p->_u._limits._min._sint = 0;
	}
	{
		p = cc_symbol_install("float", &gTypes, SCOPE_GLOBAL, CC_MM_PERMPOOL);
		gbuiltintypes._floattype = cc_type_new(Type_Float, NULL, m->_floatmetric._size, m->_floatmetric._align, p);
		p->_type = gbuiltintypes._floattype;
		p->_u._limits._max._float = FLT_MAX;
		p->_u._limits._min._float = FLT_MIN;
		p->_addressed = 1;
	}
	{
		p = cc_symbol_install("double", &gTypes, SCOPE_GLOBAL, CC_MM_PERMPOOL);
		gbuiltintypes._doubletype = cc_type_new(Type_Float, NULL, m->_doublemetric._size, m->_doublemetric._align, p);
		p->_type = gbuiltintypes._doubletype;
		p->_u._limits._max._float = DBL_MAX;
		p->_u._limits._min._float = DBL_MIN;
		p->_addressed = 1;
	}
	{
		p = cc_symbol_install("long double", &gTypes, SCOPE_GLOBAL, CC_MM_PERMPOOL);
		gbuiltintypes._ldoubletype = cc_type_new(Type_Float, NULL, m->_longdoublemetric._size, m->_longdoublemetric._align, p);
		p->_type = gbuiltintypes._ldoubletype;
		p->_u._limits._max._float = LDBL_MAX;
		p->_u._limits._min._float = LDBL_MIN;
		p->_addressed = 1;
	}
	{
		p = cc_symbol_install("void", &gTypes, SCOPE_GLOBAL, CC_MM_PERMPOOL);
		gbuiltintypes._voidtype = cc_type_new(Type_Void, NULL, 0, 0, p); /* assume size to 0 byte */
	}
	{
		p = cc_symbol_install("...", &gTypes, SCOPE_GLOBAL, CC_MM_PERMPOOL);
		gbuiltintypes._ellipsistype = cc_type_new(Type_Ellipsis, NULL, 0, 0, p);
	}

	sPointersym = cc_symbol_install("T*", &gTypes, SCOPE_GLOBAL, CC_MM_PERMPOOL);
	gbuiltintypes._ptrvoidtype = cc_type_ptr(gbuiltintypes._voidtype);
}

FCCType* cc_type_tmp(int16_t op, FCCType* ty)
{
	FCCType* tmp;

	tmp = (FCCType*)mm_alloc_area(sizeof(FCCType), CC_MM_TEMPPOOL);
	if (!tmp) {
		logger_output_s("out of memory at %s:%d\n", __FILE__, __LINE__);
		return NULL;
	}

	util_memset(tmp, 0, sizeof(*tmp));

	tmp->_op = op;
	tmp->_type = ty;
	return tmp;
}

FCCType* cc_type_qual(FCCType* ty, int16_t	op)
{
	if (!op) {
		return ty;
	}

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

int16_t cc_type_getqual(FCCType* ty)
{
	if (IsArray(ty)) {
		return cc_type_getqual(ty->_type);
	}

	if (IsQual(ty)) {
		return ty->_op;
	}

	return 0;
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
		return cc_type_newarray(gbuiltintypes._sinttype, elecnt, align);
	}
	if (IsArray(ty) && ty->_size == 0) {
		logger_output_s("error: missing array size.\n");
		return NULL;
	}
	if (ty->_size == 0) {
		if (UnQual(ty) == gbuiltintypes._voidtype) {
			logger_output_s("error: illegal type 'array of void'.\n");
			return NULL;
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
	util_memset(p, 0, sizeof(*p));

	*q = p;
	p->_name = name;
	p->_next = NULL;
	p->_type = fty;
	p->_loc = *loc;
	p->_offset = 0;
	p->_bitsize = 0;
	p->_lsb = 0;

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

BOOL cc_type_has_cfields(FCCType* sty)
{
	return sty->_u._symbol->_u._s._cfields;
}

FCCType* cc_type_func(FCCType* ret, int convention, FCCType** proto, int ellipsis)
{
	FCCType* ty = NULL;

	if (ret && (IsArray(ret) || IsFunction(ret)))
	{
		logger_output_s("illegal return type: %t \n", ret);
	}

	ty = cc_type_new(Type_Function, ret, 0, 0, NULL);
	ty->_u._f._convention = convention;
	ty->_u._f._protos = proto;
	ty->_u._f._has_ellipsis = ellipsis;

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
		return fn->_u._f._has_ellipsis;
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
	case Type_SInteger:
	case Type_UInteger:
	case Type_Float:
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
			int convention1 = ty1->_u._f._convention, convention2 = ty2->_u._f._convention;

			assert(p1 && p2);
			assert(convention1 != Type_Defcall && convention2 != Type_Defcall);

			if (convention1 != convention2) { return FALSE; }

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

BOOL cc_type_iscompatible(FCCType* ty1, FCCType* ty2)
{
	ty1 = UnQual(ty1);
	ty2 = UnQual(ty2);

	return IsPtr(ty1) && !IsFunction(ty1->_type) && IsPtr(ty2) && !IsFunction(ty2->_type)
		&& cc_type_isequal(UnQual(ty1->_type), UnQual(ty2->_type), FALSE);
}

FCCType* cc_type_promote(FCCType* ty)
{
	ty = UnQual(ty);
	switch (ty->_op)
	{
	case Type_Enum:
		return gbuiltintypes._sinttype;
	case Type_SInteger:
		if (ty->_size < gbuiltintypes._sinttype->_size) {
			return gbuiltintypes._sinttype;
		}
		break;
	case Type_UInteger:
		if (ty->_size < gbuiltintypes._sinttype->_size) {
			return gbuiltintypes._sinttype;
		}
		if (ty->_size < gbuiltintypes._uinttype->_size) {
			return gbuiltintypes._uinttype;
		}
		break;
	case Type_Float:
		if (ty->_size < gbuiltintypes._doubletype->_size) {
			return gbuiltintypes._doubletype;
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

FCCType* cc_type_compose(FCCType* ty1, FCCType* ty2)
{
	if (ty1 == ty2) {
		return ty1;
	}

	assert(ty1->_op == ty2->_op);
	switch (ty1->_op)
	{
	case Type_Pointer:
		return cc_type_ptr(cc_type_compose(ty1->_type, ty2->_type));
	case Type_Const:
	case Type_Volatile:
	case (Type_Const | Type_Volatile):
		return cc_type_qual(cc_type_compose(ty1->_type, ty2->_type), ty1->_op);
	case Type_Array:
	{
		FCCType* ty = cc_type_compose(ty1->_type, ty2->_type);
		if (ty1->_size && ((ty1->_type->_size && ty2->_size == 0) || ty1->_size == ty2->_size))
		{
			return cc_type_newarray(ty, ty1->_size / ty1->_type->_size, ty1->_align);
		}
		if (ty2->_size && ty2->_type->_size && ty1->_size == 0)
		{
			return cc_type_newarray(ty, ty2->_size / ty2->_type->_size, ty2->_align);
		}
		return cc_type_newarray(ty, 0, 0);
	}
	case Type_Function:
		return ty1;
	default:
		break;
	}

	assert(0);
	return NULL;
}

FCCType* cc_type_select(FCCType* ty1, FCCType* ty2)
{
#define XX(t)	if (ty1 == t || ty2 == t) { return t; }

	ty1 = UnQual(ty1);
	ty2 = UnQual(ty2);

	XX(gbuiltintypes._ldoubletype);
	XX(gbuiltintypes._doubletype);
	XX(gbuiltintypes._floattype);
	XX(gbuiltintypes._ullongtype);
	XX(gbuiltintypes._sllongtype);
	XX(gbuiltintypes._slongtype);
	XX(gbuiltintypes._ulongtype);
	if ((ty1 == gbuiltintypes._slongtype && ty2 == gbuiltintypes._uinttype)
		|| (ty1 == gbuiltintypes._uinttype && ty2 == gbuiltintypes._slongtype))
	{
		if (gbuiltintypes._slongtype->_size > gbuiltintypes._uinttype->_size)
			return gbuiltintypes._slongtype;
		else
			return gbuiltintypes._ulongtype;
	}

	XX(gbuiltintypes._slongtype);
	XX(gbuiltintypes._uinttype);
	return gbuiltintypes._sinttype;
#undef XX
}

BOOL cc_type_cancast(FCCType* to, FCCType* from)
{
	if (IsPtr(to) && IsArray(from)) {
		return TRUE;
	}

	if (IsVoid(to) || !IsScalar(to)) {
		return FALSE;
	}

	if (IsVoid(from) || !IsScalar(from)) {
		return FALSE;
	}

	return TRUE;
}
