/* \brief
 *	representation of C types
 *	for example:
 *		int a;         type is int
 *		int const a;   type is int with qualifier const
 *		int* const a;  type is pointer with qualifier const, then chain to int
 *		const int* const a;  type is pointer with qualifier const, and chain to type pointer, chain to type int with qualifier const.
 *
 */

#ifndef __CC_TYPES_H__
#define __CC_TYPES_H__

#include "utils.h"
#include "mm.h"


/* types(& qualifiers) */
#define Type_Unknown	0
#define Type_SInteger	1  /* int8 short int long  long-long */
#define Type_UInteger	2  /* uint8 unsigned integer */
#define Type_Float		3  /* float, double ,long double */
#define Type_Enum		4
#define Type_Void		5
#define Type_Pointer	6
/* aggregate types */
#define Type_Array		7
#define Type_Union		8
#define	Type_Struct		9
#define Type_Function	10

/* special types */
#define Type_Defined	11
#define Type_Ellipsis	12

/* qualifiers */
#define	Type_Const		0x0100
#define Type_Restrict	0x0200
#define Type_Volatile	0x0400

/* sign */
#define Sign_Unknown	0
#define Sign_Signed		1
#define Sign_Unsigned	2
/* size */
#define Size_Unknown	0
#define Size_Short		1
#define Size_Long		2
#define Size_Longlong	3


/* utilities macros */
#define IsQual(t)	((t)->_op & (Type_Const|Type_Restrict|Type_Volatile))
#define UnQual(t)	(IsQual(t) ? (t)->_type : (t))
#define IsVolatile(t)	((t)->_op & Type_Volatile)
#define IsConst(t)		((t)->_op & Type_Const)
#define IsArray(t)	(UnQual(t)->_op == Type_Array)
#define IsStruct(t)	(UnQual(t)->_op == Type_Struct || UnQual(t)->_op == Type_Union)
#define IsUnion(t)	(UnQual(t)->_op == Type_Union)
#define IsFunction(t)	(UnQual(t)->_op == Type_Function)
#define IsPtr(t)	(UnQual(t)->_op == Type_Pointer)
#define IsEnum(t)	(UnQual(t)->_op == Type_Enum)
#define IsChar(t)	(UnQual(t)->_op == Type_Char && (t)->_size == 1)
#define IsWideChar(t)	(UnQual(t)->_op == Type_Char && (t)->_size == 2)
#define IsInt(t)	(UnQual(t)->_op == Type_SInteger || UnQual(t)->_op == Type_UInteger)
#define IsFloat(t)	(UnQual(t)->_op == Type_Float)
#define IsDouble(t)	(UnQual(t)->_op == Type_Double)
#define IsScalar(t) (UnQual(t)->_op <= Type_Pointer)


/* cc type */
typedef struct tagCCType
{
	int16_t	_op;
	int16_t _align;		/* align in bytes */
	int32_t	_size;		/* size in bytes */
	
	struct tagCCType   *_type;	/* pointer to successor type */

	union {
		struct tagCCSymbol* _symbol; /* for other types */
		struct
		{
			struct tagCCType** _protos; /* params type array, end with NULL */
		} _f; /* function proto */
	} _u;
	
} FCCType;

typedef struct tagStructField
{
	const char* _name;
	FLocation   _loc;
	FCCType*    _type;

	int32_t		_offset;
	int16_t		_bitsize;
	int16_t		_lsb; /* least significant bit */

	struct tagStructField* _next; /* pointer to next sibling field */
} FCCField;


/* type metrics */

typedef struct tagCCMetrics
{
	int8_t	_size;
	int8_t  _align;
} FCCMetrics;

typedef struct tagCCTypeMetrics
{
	FCCMetrics _charmetric;
	FCCMetrics _wcharmetric;
	FCCMetrics _shortmetric;
	FCCMetrics _intmetric;
	FCCMetrics _longmetric;
	FCCMetrics _longlongmetric;
	FCCMetrics _floatmetric;
	FCCMetrics _doublemetric;
	FCCMetrics _longdoublemetric;
	FCCMetrics _ptrmetric;
	FCCMetrics _structmetric;
} FCCTypeMetrics;

/* types context */
typedef struct tagCCBuiltinTypes
{
	/* build-in or common types */
	FCCType* _chartype;
	FCCType* _wchartype;
	FCCType* _schartype;
	FCCType* _uchartype;
	FCCType* _sshorttype;
	FCCType* _ushorttype;
	FCCType* _sinttype;
	FCCType* _uinttype;
	FCCType* _slongtype;
	FCCType* _ulongtype;
	FCCType* _sllongtype;
	FCCType* _ullongtype;
	FCCType* _floattype;
	FCCType* _doubletype;
	FCCType* _ldoubletype;
	FCCType* _voidtype;
	FCCType* _ellipsistype; /* ... */
} FCCBuiltinTypes;

extern FCCBuiltinTypes gBuiltinTypes;

void cc_type_init(const FCCTypeMetrics* m);

/* create a type for temporary use */
FCCType* cc_type_tmp(int16_t op, FCCType* ty);

/* qualify a type */
FCCType* cc_type_qual(FCCType* ty, int16_t	op);

/* new a pointer to type */
FCCType* cc_type_ptr(FCCType* ty);
/* get the dereferenced type */
FCCType* cc_type_deref(FCCType* ty);

/* create a array type */
FCCType* cc_type_newarray(FCCType*ty, int elecnt, int align);
/* translate a array to pointer type */
FCCType* cc_type_arraytoptr(FCCType* ty);

/* new a struct type (STRUCT, UNION, ENUM)*/
FCCType* cc_type_newstruct(int op, const char* name, const FLocation *loc, int level);
/* add a field (type: fty) to struct(sty) */
FCCField* cc_type_newfield(const char* name, const FLocation* loc, FCCType* sty, FCCType* fty);
FCCField* cc_type_findfield(const char* name, FCCType* sty);
/* get fields list */
FCCField* cc_type_fields(FCCType* sty);

/* new a function type */
FCCType* cc_type_func(FCCType* ret, FCCType** proto);
FCCType* cc_type_rettype(FCCType* fn);
/* check a function has variance parameter */
BOOL cc_type_isvariance(FCCType* fn);

BOOL cc_type_isequal(FCCType* ty1, FCCType* ty2, BOOL option);
FCCType* cc_type_promote(FCCType* ty);
void cc_type_remove(int level);

FCCType* cc_type_compose(FCCType* ty1, FCCType* ty2);

#endif /* __CC_TYPES_H__ */
