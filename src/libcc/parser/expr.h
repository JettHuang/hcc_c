/* \brief
 *	C compiler expression parsing.
 */

#ifndef __CC_EXPR_H__
#define __CC_EXPR_H__

#include "utils.h"
#include "mm.h"
#include "cc.h"
#include "symbols.h"
#include "types.h"
#include "ir/ir.h"


/* parsing apis */
BOOL cc_expr_parse_assignment(FCCContext* ctx, FCCIRTree** outexpr, enum EMMArea where);
BOOL cc_expr_parse_expression(FCCContext* ctx, FCCIRTree** outexpr, enum EMMArea where);
BOOL cc_expr_parse_constant_expression(FCCContext* ctx, FCCIRTree** outexpr, enum EMMArea where);
BOOL cc_expr_parse_constant_int(FCCContext* ctx, int* val);

/* ir tree operating apis */
FCCType* cc_expr_assigntype(FCCType* lhs, FCCIRTree* expr);
BOOL cc_expr_is_modifiable(FCCIRTree* expr);
BOOL cc_expr_is_nullptr(FCCIRTree* expr);
BOOL cc_expr_is_constant(FCCIRTree* expr);

/* generate ir tree apis */
FCCIRTree* cc_expr_change_rettype(FCCIRTree* expr, FCCType* newty, enum EMMArea where);
FCCIRTree* cc_expr_adjust(FCCIRTree* expr, enum EMMArea where);
FCCIRTree* cc_expr_id(FCCSymbol* p, FLocation* loc, enum EMMArea where);
FCCIRTree* cc_expr_addr(FCCIRTree* expr, FLocation* loc, enum EMMArea where);
FCCIRTree* cc_expr_indir(FCCIRTree* expr, FLocation* loc, enum EMMArea where);
FCCIRTree* cc_expr_constant(FCCType* ty, int tycode, FLocation *loc, enum EMMArea where, ...);
FCCIRTree* cc_expr_cast(FCCType* ty, FCCIRTree* expr, FLocation* loc, enum EMMArea where);
FCCIRTree* cc_expr_add(FCCType* ty, FCCIRTree* lhs, FCCIRTree* rhs, FLocation* loc, enum EMMArea where);
FCCIRTree* cc_expr_sub(FCCType* ty, FCCIRTree* lhs, FCCIRTree* rhs, FLocation* loc, enum EMMArea where);
FCCIRTree* cc_expr_mul(FCCType* ty, FCCIRTree* lhs, FCCIRTree* rhs, FLocation* loc, enum EMMArea where);
FCCIRTree* cc_expr_div(FCCType* ty, FCCIRTree* lhs, FCCIRTree* rhs, FLocation* loc, enum EMMArea where);
FCCIRTree* cc_expr_mod(FCCType* ty, FCCIRTree* lhs, FCCIRTree* rhs, FLocation* loc, enum EMMArea where);
FCCIRTree* cc_expr_call(FCCType* fty, FCCIRTree* expr, FArray* args, FLocation* loc, enum EMMArea where);
FCCIRTree* cc_expr_seq(FCCType* ty, FCCIRTree* expr0, FCCIRTree* expr1, FLocation* loc, enum EMMArea where);
FCCIRTree* cc_expr_assign(FCCType* ty, FCCIRTree* lhs, FCCIRTree* rhs, FLocation* loc, enum EMMArea where);
FCCIRTree* cc_expr_neg(FCCType* ty, FCCIRTree* expr, FLocation* loc, enum EMMArea where);
FCCIRTree* cc_expr_bitcom(FCCType* ty, FCCIRTree* expr, FLocation* loc, enum EMMArea where);
FCCIRTree* cc_expr_not(FCCType* ty, FCCIRTree* expr, FLocation* loc, enum EMMArea where);
FCCIRTree* cc_expr_lshift(FCCType* ty, FCCIRTree* lhs, FCCIRTree* rhs, FLocation* loc, enum EMMArea where);
FCCIRTree* cc_expr_rshift(FCCType* ty, FCCIRTree* lhs, FCCIRTree* rhs, FLocation* loc, enum EMMArea where);
FCCIRTree* cc_expr_less(FCCType* ty, FCCIRTree* lhs, FCCIRTree* rhs, FLocation* loc, enum EMMArea where);
FCCIRTree* cc_expr_lequal(FCCType* ty, FCCIRTree* lhs, FCCIRTree* rhs, FLocation* loc, enum EMMArea where);
FCCIRTree* cc_expr_great(FCCType* ty, FCCIRTree* lhs, FCCIRTree* rhs, FLocation* loc, enum EMMArea where);
FCCIRTree* cc_expr_gequal(FCCType* ty, FCCIRTree* lhs, FCCIRTree* rhs, FLocation* loc, enum EMMArea where);
FCCIRTree* cc_expr_equal(FCCType* ty, FCCIRTree* lhs, FCCIRTree* rhs, FLocation* loc, enum EMMArea where);
FCCIRTree* cc_expr_unequal(FCCType* ty, FCCIRTree* lhs, FCCIRTree* rhs, FLocation* loc, enum EMMArea where);
FCCIRTree* cc_expr_bitand(FCCType* ty, FCCIRTree* lhs, FCCIRTree* rhs, FLocation* loc, enum EMMArea where);
FCCIRTree* cc_expr_bitxor(FCCType* ty, FCCIRTree* lhs, FCCIRTree* rhs, FLocation* loc, enum EMMArea where);
FCCIRTree* cc_expr_bitor(FCCType* ty, FCCIRTree* lhs, FCCIRTree* rhs, FLocation* loc, enum EMMArea where);
FCCIRTree* cc_expr_logicand(FCCType* ty, FCCIRTree* lhs, FCCIRTree* rhs, FLocation* loc, enum EMMArea where);
FCCIRTree* cc_expr_logicor(FCCType* ty, FCCIRTree* lhs, FCCIRTree* rhs, FLocation* loc, enum EMMArea where);
FCCIRTree* cc_expr_condition(FCCIRTree* expr0, FCCIRTree* expr1, FCCIRTree* expr2, FLocation *loc, enum EMMArea where);
/* to bool expression */
FCCIRTree* cc_expr_bool(FCCType* ty, FCCIRTree* expr, FLocation* loc, enum EMMArea where);
/* return the last expression of an expr-tree */
FCCIRTree* cc_expr_right(FCCIRTree* expr);
/* return value of expression */
FCCIRTree* cc_expr_value(FCCIRTree* expr, enum EMMArea where);

#endif /* _CC_EXPR_H__ */
