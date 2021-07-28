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
#define Type_Char		1  /* signed or unsigned depends on platform */
#define Type_SInteger	2  /* int8 short int long  long-long */
#define Type_UInteger	3  /* uint8 unsigned integer */
#define Type_Float		4
#define Type_Double		5
#define Type_Enum		6
#define Type_Void		7
/* aggregate types */
#define Type_Array		8
#define Type_Union		9
#define	Type_Struct		10
#define Type_Function	11
#define Type_Pointer	12
/* special types */
#define Type_Defined	13
#define Type_Variable	14

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


/* cc type */
typedef struct tagCCType
{
	int16_t	_op;
	int16_t _align;		/* align in bytes */
	int16_t	_size;		/* size in bytes */
	
	struct tagCCSymbol *_symbol;
	struct tagCCType   *_next;	/* pointer to successor type */

	union {
		struct tagSymbolListNode* _enums; /* for enum type */
		int32_t		_elecount; /* for array */
		struct
		{
			struct tagStructField* _next;
			uint8_t	_cfields : 1; /* has const field? */
			uint8_t	_vfields : 1; /* has volatile field? */
		} s; /* for struct(union) */

		struct  
		{
			struct tagCCType* _ret;
			struct tagSymbolListNode* _parameters; /* ... means ellipsis parameter */
		} f;
	} u;
	
} FCCType;

typedef struct tagStructField
{
	struct tagStructField *_next;
	struct tagCCSymbol *_symbol;
	int32_t			_offset;
	int16_t			_bitsize;
	int16_t			_lsb; /* least significant bit */
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
typedef struct tagCCTypesContext
{
	/* build-in or common types */
	FCCType* _chartype;
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
} FCCTypesContext;

#endif /* __CC_TYPES_H__ */
