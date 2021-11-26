/* \brief
 *	C compiler expression parsing.
 */

#ifndef __CC_EXPR_H__
#define __CC_EXPR_H__

#include "utils.h"
#include "mm.h"
#include "cc.h"
#include "symbols.h"


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
	EXPR_ADDRG, /* & */
	EXPR_ADDRF,
	EXPR_ADDRL,
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
	EXPR_CONSTANT,
	EXPR_CONSTANT_STR,

	EXPR_CALL, /* function call */

	EXPR_MAX
};

/* expression tree */
typedef struct tagCCExprTree {
	int _op;
	FLocation _loc;
	struct tagCCType* _ty; /* the derivation type of this expression */

	union {
		struct tagCCExprTree* _kids[3];
		struct tagCCSymbol* _symbol;
		struct tagStructField* _field;
		struct {
			struct tagCCExprTree* _lhs;
			struct tagCCExprTree** _args; /* end with null */
		} _f;
	} _u;
	
	/* auxiliary flags */
	int _isbitfield : 1;
	int _islvalue : 1;
	int _isconstant : 1; /* constant expressions can be used in initializers. */
} FCCExprTree;


FCCExprTree* cc_expr_new(enum EMMArea where);
BOOL cc_expr_parse_assignment(struct tagCCContext* ctx, FCCExprTree** outexpr, enum EMMArea where);
BOOL cc_expr_parse_expression(struct tagCCContext* ctx, FCCExprTree** outexpr, enum EMMArea where);
BOOL cc_expr_parse_constant_expression(struct tagCCContext* ctx, FCCExprTree** outexpr, enum EMMArea where);
BOOL cc_expr_parse_constant_int(struct tagCCContext* ctx, int* val);

struct tagCCType* cc_expr_assigntype(struct tagCCType* lhs, struct tagCCExprTree* expr);
FCCExprTree* cc_expr_cast(struct tagCCContext* ctx, struct tagCCType* castty, FCCExprTree* expr, enum EMMArea where);
FCCExprTree* cc_expr_constant(struct tagCCContext* ctx, struct tagCCType* cnstty, FCCConstVal cnstval, FLocation loc, enum EMMArea where);
FCCExprTree* cc_expr_constant_str(struct tagCCContext* ctx, struct tagCCType* cnstty, FCCConstVal cnstval, FLocation loc, enum EMMArea where);
BOOL cc_expr_is_modifiable(FCCExprTree* expr);
BOOL cc_expr_is_nullptr(FCCExprTree* expr);
FCCExprTree* cc_expr_to_pointer(FCCExprTree* expr);
const char* cc_expr_opname(enum EExprOp op);


#endif /* _CC_EXPR_H__ */
