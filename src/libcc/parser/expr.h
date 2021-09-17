/* \brief
 *	C compiler expression parsing.
 */

#ifndef __CC_EXPR_H__
#define __CC_EXPR_H__

#include "utils.h"
#include "mm.h"
#include "cc.h"


/* expression op type */
enum EExprOp
{
	EXPR_COMMA = 0,
	EXPR_ASSIGN, /* = *= /= %= += -= <<= >>= &= ^= |= */
	EXPR_ASSIGN_MUL,
	EXPR_ASSIGN_DIV,
	EXPR_ASSIGN_MOD,
	EXPR_ASSIGN_ADD,
	EXPR_ASSIGN_SUB,
	EXPR_ASSIGN_LSHIFT,
	EXPR_ASSIGN_RSHIFT,
	EXPR_ASSIGN_BITAND,
	EXPR_ASSIGN_BITXOR,
	EXPR_ASSIGN_BITOR,
	EXPR_COND, /* ? : */ 
	EXPR_LOGOR,
	EXPR_LOGAND,
	EXPR_BITOR,
	EXPR_BITXOR,
	EXPR_BITAND,
	EXPR_EQ,
	EXPR_UNEQ,
	EXPR_LESS,
	EXPR_GREAT,
	EXPR_LESSEQ,
	EXPR_GREATEQ,
	EXPR_LSHFIT,
	EXPR_RSHFIT,
	EXPR_ADD,
	EXPR_SUB,
	EXPR_MUL,
	EXPR_DIV,
	EXPR_MOD,

	/* unary prefix */
	EXPR_DEREF,/* '*' */
	EXPR_ADDR, /* & */
	EXPR_NEG,  /* - */
	EXPR_POS,  /* + */
	EXPR_NOT,  /* ! */
	EXPR_COMPLEMENT,  /* ~ complement of one's */
	EXPR_PREINC,
	EXPR_PREDEC,
	EXPR_SIZEOF,
	EXPR_TYPECAST,

	/* unary postfix */
	EXPR_POSINC,
	EXPR_POSDEC,

	/* value */
	EXPR_ID,
	EXPR_CONSTANT,

	EXPR_PTRFIELD, /* struct field -> */
	EXPR_DOTFIELD, /* struct field . */
	EXPR_CALL, /* function call */
	EXPR_ARGS, /* function args */
	EXPR_ARRAYSUB, /* array subscripting */

	EXPR_MAX
};


/* expression tree */
typedef struct tagCCExprTree {
	int _op;
	FLocation _loc;
	struct tagCCType* _ty; /* the derivational type of this expression */

	union {
		struct tagCCExprTree* _kids[3];
		struct tagCCSymbol* _symbol;
		struct {
			struct tagCCExprTree* _lhs;
			struct tagStructField* _field;
		} _s;
		struct {
			struct tagCCExprTree* _lhs;
			struct tagCCExprTree** _args; /* end while null */
		} _f;
	} _u;
} FCCExprTree;


FCCExprTree* cc_expr_new(enum EMMArea where);
BOOL cc_expr_assignment(struct tagCCContext* ctx, FCCExprTree** outexpr, enum EMMArea where);
BOOL cc_expr_expression(struct tagCCContext* ctx, FCCExprTree** outexpr, enum EMMArea where);
BOOL cc_expr_constant_expression(struct tagCCContext* ctx, FCCExprTree** outexpr, enum EMMArea where);

BOOL cc_expr_constant_int(struct tagCCContext* ctx, int* val);



#endif /* _CC_EXPR_H__ */
