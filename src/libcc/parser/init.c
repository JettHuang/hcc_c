/* \brief
 *		variable initializer parsing.
 */

#include "init.h"
#include "expr.h"
#include "lexer/token.h"
#include "logger.h"
#include "parser.h"
#include "ir/ir.h"
#include "ir/canon.h"


FVarInitializer* cc_varinit_new(enum EMMArea where)
{
	FVarInitializer* p;

	if ((p = mm_alloc_area(sizeof(FVarInitializer), where))) {
		p->_isblock = 0;
		p->_u._expr = NULL;
	}
	else {
		logger_output_s("error: out of memory at %s:%d\n", __FILE__, __LINE__);
	}

	return p;
}


BOOL cc_parser_initializer(struct tagCCContext* ctx, FVarInitializer** outinit, BOOL bExpectConstant, enum EMMArea where)
{
	FVarInitializer* initializer, * kidinit;

	if (!(initializer = cc_varinit_new(where))) {
		return FALSE;
	}

	initializer->_loc = ctx->_currtk._loc;
	if (ctx->_currtk._type == TK_LBRACE) /* '{' */
	{
		FArray kids;

		initializer->_isblock = 1;
		array_init(&kids, 32, sizeof(FVarInitializer*), where);

		cc_read_token(ctx, &ctx->_currtk);
		for (;;)
		{
			if (!cc_parser_initializer(ctx, &kidinit, bExpectConstant, where)) {
				return FALSE;
			}

			array_append(&kids, &kidinit);
			if (ctx->_currtk._type != TK_COMMA) /* ',' */
			{
				break;
			}
			cc_read_token(ctx, &ctx->_currtk);
		} /* end for ;; */

		initializer->_u._kids._kids = kids._data;
		initializer->_u._kids._cnt = kids._elecount;
		if (!cc_parser_expect(ctx, TK_RBRACE)) /* '}' */
		{
			return FALSE;
		}
	}
	else {
		initializer->_isblock = 0;
		if (bExpectConstant) {
			if (!cc_expr_parse_constant_expression(ctx, &initializer->_u._expr, where)) {
				return FALSE;
			}
		}
		else {
			if (!cc_expr_parse_assignment(ctx, &initializer->_u._expr, where)) {
				return FALSE;
			}
		}

	}

	*outinit = initializer;
	return TRUE;
}

/*-------------------------------------------------------------
 *  initializer checker
 *-------------------------------------------------------------*/
static BOOL cc_varinit_check_inner(struct tagCCContext* ctx, struct tagCCType* ty, FVarInitializer* init, int* outerindex, BOOL bUsingOuterBlock, enum EMMArea where);

static BOOL cc_varinit_check_scalar(struct tagCCContext* ctx, struct tagCCType* ty, FVarInitializer* init, int* outerindex, BOOL bUsingOuterBlock, enum EMMArea where)
{
	FVarInitializer* thisinit;
	FCCIRTree* expr;

	assert(!bUsingOuterBlock || init->_isblock);
	if (bUsingOuterBlock) {
		thisinit = *(init->_u._kids._kids + *outerindex);
		(*outerindex)++;
	}
	else {
		thisinit = init;
	}

	if (thisinit->_isblock) {
		if (thisinit->_u._kids._cnt > 1) {
			logger_output_s("error: too many initializers at %w.\n", &init->_loc);
			return FALSE;
		}

		thisinit = *(thisinit->_u._kids._kids + 0);
		if (thisinit->_isblock) {
			logger_output_s("error: only one level of braces is allowed on an initializer at %w.\n", &thisinit->_loc);
			return FALSE;
		}
	}

	assert(!thisinit->_isblock);
	expr = cc_expr_value(thisinit->_u._expr, where);
	expr = cc_expr_adjust(expr, where);
	if (!(ty = cc_expr_assigntype(ty, expr))) {
		logger_output_s("error: initialize assign failed at %w.\n", &expr->_loc);
		return FALSE;
	}
	
	if (!(expr = cc_expr_cast(ty, expr, NULL, where))) {
		logger_output_s("error: cast failed at %s:%d.\n", __FILE__, __LINE__);
		return FALSE;
	}
	if (!cc_expr_is_constant(expr)) {
		logger_output_s("error: constant expression expected at %w.\n", &expr->_loc);
		return FALSE;
	}

	thisinit->_u._expr = expr;

	return TRUE;
}

static BOOL cc_varinit_check_union(struct tagCCContext* ctx, struct tagCCType* ty, FVarInitializer* init, int* outerindex, BOOL bUsingOuterBlock, enum EMMArea where)
{
	FVarInitializer* thisinit, * tmpinit;
	FCCField* first;
	BOOL bnewblock = TRUE;

	assert(!bUsingOuterBlock || init->_isblock);
	thisinit = init;
	if (bUsingOuterBlock) {
		tmpinit = *(init->_u._kids._kids + *outerindex);
		if (tmpinit->_isblock) {
			(*outerindex)++;
			thisinit = tmpinit;
		}
		else {
			bnewblock = FALSE;
		}
	}
	if (!thisinit->_isblock) {
		logger_output_s("error: initializer for union need brace '{', at %w\n", &thisinit->_loc);
		return FALSE;
	}

	first = cc_type_fields(ty);
	if (bnewblock) {
		int innerindex = 0;

		if (!cc_varinit_check_inner(ctx, first->_type, thisinit, &innerindex, TRUE, where))
		{
			return FALSE;
		}

		if (innerindex < thisinit->_u._kids._cnt) {
			logger_output_s("error: too many initializers in block %w\n", &thisinit->_loc);
			return FALSE;
		}
	}
	else {
		if (!cc_varinit_check_inner(ctx, first->_type, thisinit, outerindex, TRUE, where))
		{
			return FALSE;
		}
	}

	return TRUE;
}

static BOOL cc_varinit_check_struct(struct tagCCContext* ctx, struct tagCCType* ty, FVarInitializer* init, int* outerindex, BOOL bUsingOuterBlock, enum EMMArea where)
{
	FVarInitializer* thisinit, * tmpinit;
	FCCField* field;
	BOOL bnewblock = TRUE;

	assert(!bUsingOuterBlock || init->_isblock);
	thisinit = init;
	if (bUsingOuterBlock) {
		tmpinit = *(init->_u._kids._kids + *outerindex);
		if (tmpinit->_isblock) {
			(*outerindex)++;
			thisinit = tmpinit;
		}
		else {
			bnewblock = FALSE;
		}
	}
	if (!thisinit->_isblock) {
		logger_output_s("error: initializer for struct need brace '{', at %w\n", &thisinit->_loc);
		return FALSE;
	}

	if (bnewblock) {
		int innerindex = 0;

		for (field = cc_type_fields(ty); field && innerindex < thisinit->_u._kids._cnt; field = field->_next) {
			if (!cc_varinit_check_inner(ctx, field->_type, thisinit, &innerindex, TRUE, where))
			{
				return FALSE;
			}
		}

		if (innerindex < thisinit->_u._kids._cnt) {
			logger_output_s("error: too many initializers in block %w\n", &thisinit->_loc);
			return FALSE;
		}
	}
	else {
		for (field = cc_type_fields(ty); field && *outerindex < thisinit->_u._kids._cnt; field = field->_next) {
			if (!cc_varinit_check_inner(ctx, field->_type, thisinit, outerindex, TRUE, where))
			{
				return FALSE;
			}
		}
	}

	return TRUE;
}

static BOOL cc_varinit_check_array(struct tagCCContext* ctx, struct tagCCType* ty, FVarInitializer* init, int* outerindex, BOOL bUsingOuterBlock, enum EMMArea where)
{
	FVarInitializer* thisinit, * tmpinit;
	BOOL bnewblock = TRUE;

	thisinit = init;
	if (bUsingOuterBlock) {
		tmpinit = *(init->_u._kids._kids + *outerindex);
		if (tmpinit->_isblock) {
			(*outerindex)++;
			thisinit = tmpinit;
		}
		else {
			bnewblock = FALSE;
		}
	}

	/* check if new block */
	if (bnewblock)
	{
		/*  ie:
		 *    int a[] = { 0, 1, 2}
		 *    char s0[] = { "abc" }
		 *    char s1[] = { 'a', 'b', "cd" }
		 *
		 */

		FCCType* elety, * strty;
		BOOL ischararray, iscnststr;
		int arraysize, innerindex;

		elety = UnQual(ty->_type);
		ischararray = (elety == gbuiltintypes._chartype) || (elety == gbuiltintypes._wchartype);

		if (thisinit->_isblock)
		{
			if (ty->_size > 0) {
				for (innerindex = arraysize = 0; ty->_size > arraysize && innerindex < thisinit->_u._kids._cnt; arraysize += elety->_size)
				{
					tmpinit = *(thisinit->_u._kids._kids + innerindex);
					iscnststr = !tmpinit->_isblock && (tmpinit->_u._expr->_op == IR_MKOP1(IR_CONST, IR_STRA) || tmpinit->_u._expr->_op == IR_MKOP1(IR_CONST, IR_STRW));

					if (ischararray && iscnststr)
					{
						strty = UnQual(tmpinit->_u._expr->_ty->_type);
						if (strty != elety) {
							logger_output_s("error: mismatch type for array initializer at %w.\n", &tmpinit->_loc);
							return FALSE;
						}

						if ((ty->_size - arraysize) < tmpinit->_u._expr->_ty->_size) {
							logger_output_s("warning:  array initializer will be truncated at %w.\n", &tmpinit->_u._expr->_loc);
							arraysize = ty->_size;
						}
						else {
							arraysize += tmpinit->_u._expr->_ty->_size;
						}

						innerindex++;
						break; /* don't continue */
					}
					else if (!cc_varinit_check_inner(ctx, elety, thisinit, &innerindex, TRUE, where))
					{
						return FALSE;
					}
				} /* end for */

				if (innerindex < thisinit->_u._kids._cnt) {
					logger_output_s("error: too many initializers in block %w\n", &thisinit->_loc);
					return FALSE;
				}
			}
			else {
				for (innerindex = arraysize = 0; innerindex < thisinit->_u._kids._cnt; arraysize += elety->_size)
				{
					tmpinit = *(thisinit->_u._kids._kids + innerindex);
					iscnststr = !tmpinit->_isblock && (tmpinit->_u._expr->_op == IR_MKOP1(IR_CONST, IR_STRA) || tmpinit->_u._expr->_op == IR_MKOP1(IR_CONST, IR_STRW));

					if (ischararray && iscnststr)
					{
						strty = UnQual(tmpinit->_u._expr->_ty->_type);
						if (strty != elety) {
							logger_output_s("error: mismatch type for array initializer at %w.\n", &tmpinit->_loc);
							return FALSE;
						}

						arraysize += tmpinit->_u._expr->_ty->_size;
						innerindex++;
						break; /* don't continue */
					}
					else if (!cc_varinit_check_inner(ctx, elety, thisinit, &innerindex, TRUE, where))
					{
						return FALSE;
					}
				} /* end for */

				if (innerindex < thisinit->_u._kids._cnt) {
					logger_output_s("error: too many initializers in block %w\n", &thisinit->_loc);
					return FALSE;
				}
				ty->_size = arraysize;
			}
		}
		else
		{
			tmpinit = thisinit;
			iscnststr = !tmpinit->_isblock && (tmpinit->_u._expr->_op == IR_MKOP1(IR_CONST, IR_STRA) || tmpinit->_u._expr->_op == IR_MKOP1(IR_CONST, IR_STRW));
			if (ischararray && iscnststr)
			{
				strty = UnQual(tmpinit->_u._expr->_ty->_type);
				if (strty != elety) {
					logger_output_s("error: mismatch type for array initializer at %w.\n", &tmpinit->_loc);
					return FALSE;
				}

				if (ty->_size > 0) {
					if (ty->_size < tmpinit->_u._expr->_ty->_size) {
						logger_output_s("warning:  array initializer will be truncated at %w.\n", &tmpinit->_u._expr->_loc);
					}
				}
				else {
					ty->_size = tmpinit->_u._expr->_ty->_size;
				}
			}
			else
			{
				logger_output_s("error: initializer for struct need brace '{', at %w\n", &tmpinit->_loc);
				return FALSE;
			}
		}
	}
	else /* !bnewblock */
	{
		FCCType* elety, * strty;
		BOOL ischararray, iscnststr;
		int arraysize;

		assert(thisinit->_isblock);
		elety = UnQual(ty->_type);
		ischararray = (elety == gbuiltintypes._chartype) || (elety == gbuiltintypes._wchartype);

		if (ty->_size > 0) {
			for (arraysize = 0; ty->_size > arraysize && *outerindex < thisinit->_u._kids._cnt; arraysize += elety->_size)
			{
				tmpinit = *(thisinit->_u._kids._kids + *outerindex);
				iscnststr = !tmpinit->_isblock && (tmpinit->_u._expr->_op == IR_MKOP1(IR_CONST, IR_STRA) || tmpinit->_u._expr->_op == IR_MKOP1(IR_CONST, IR_STRW));

				if (ischararray && iscnststr)
				{
					/* ie: "ABC" will be ignore for str[4].
						struct Struct {
							int a;
							int b;
							char str[4];
						};

						struct Struct s = { 1, 2, 'a', 'b', "ABC" };
					*/
					if (arraysize <= 0) {
						(*outerindex)++;
					}

					break; /* don't continue */
				}
				else if (!cc_varinit_check_inner(ctx, elety, thisinit, outerindex, TRUE, where))
				{
					return FALSE;
				}
			} /* end for */
		}
		else {
			for (arraysize = 0; *outerindex < thisinit->_u._kids._cnt; arraysize += elety->_size)
			{
				tmpinit = *(thisinit->_u._kids._kids + *outerindex);
				iscnststr = !tmpinit->_isblock && (tmpinit->_u._expr->_op == IR_MKOP1(IR_CONST, IR_STRA) || tmpinit->_u._expr->_op == IR_MKOP1(IR_CONST, IR_STRW));

				if (ischararray && iscnststr)
				{
					strty = UnQual(tmpinit->_u._expr->_ty->_type);
					if (strty != elety) {
						logger_output_s("error: mismatch type for array initializer at %w.\n", &tmpinit->_loc);
						return FALSE;
					}

					if ((ty->_size - arraysize) < tmpinit->_u._expr->_ty->_size) {
						logger_output_s("warning:  array initializer will be truncated at %w.\n", &tmpinit->_u._expr->_loc);
						arraysize = ty->_size;
					}
					else {
						arraysize += tmpinit->_u._expr->_ty->_size;
					}

					(*outerindex)++;
					break; /* don't continue */
				}
				else if (!cc_varinit_check_inner(ctx, elety, thisinit, outerindex, TRUE, where))
				{
					return FALSE;
				}
			} /* end for */

			ty->_size = arraysize;
		}
	}

	return TRUE;
}

static BOOL cc_varinit_check_inner(struct tagCCContext* ctx, struct tagCCType* ty, FVarInitializer* init, int* outerindex, BOOL bUsingOuterBlock, enum EMMArea where)
{
	if (IsScalar(ty)) {
		return cc_varinit_check_scalar(ctx, ty, init, outerindex, bUsingOuterBlock, where);
	}
	else if (IsUnion(ty)) {
		return cc_varinit_check_union(ctx, ty, init, outerindex, bUsingOuterBlock, where);
	}
	else if (IsStruct(ty)) {
		return cc_varinit_check_struct(ctx, ty, init, outerindex, bUsingOuterBlock, where);
	}
	else if (IsArray(ty)) {
		return cc_varinit_check_array(ctx, ty, init, outerindex, bUsingOuterBlock, where);
	}
	else {
		assert(0);
	}

	return TRUE;
}

BOOL cc_varinit_check(struct tagCCContext* ctx, struct tagCCType* ty, FVarInitializer* init, enum EMMArea where)
{
	return cc_varinit_check_inner(ctx, ty, init, NULL, FALSE, where);
}

/*-------------------------------------------------------------
 * generate initialization codes for local variable 
 *------------------------------------------------------------*/
static int gSpaceZeroBytes = 0;
static BOOL cc_gen_localvar_initcodes_inner(struct tagCCIRCodeList* list, FCCIRTree* baseAddr, int offset, struct tagCCType* ty, FVarInitializer* init, int* outerindex, BOOL bUsingOuterBlock);

static BOOL cc_gen_localvar_assign(struct tagCCIRCodeList* list, FCCIRTree* baseAddr, int offset, FCCIRTree* rhs)
{
	FCCIRTree*lhs, *assign;

	lhs = cc_expr_constant(gbuiltintypes._sinttype, IR_S32, NULL, CC_MM_TEMPPOOL, offset);
	lhs = cc_expr_add(cc_type_ptr(rhs->_ty), baseAddr, lhs, &rhs->_loc, CC_MM_TEMPPOOL);
	lhs = cc_expr_indir(lhs, &rhs->_loc, CC_MM_TEMPPOOL);
	assign = cc_expr_assign(rhs->_ty, lhs, rhs, &rhs->_loc, CC_MM_TEMPPOOL);

	return cc_canon_expr_linearize(list, assign, NULL, NULL, NULL, CC_MM_TEMPPOOL);
}

static BOOL cc_gen_localvar_initcodes_scalar(struct tagCCIRCodeList* list, FCCIRTree* baseAddr, int offset, struct tagCCType* ty, FVarInitializer* init, int* outerindex, BOOL bUsingOuterBlock)
{
	FVarInitializer* thisinit;
	
	assert(!bUsingOuterBlock || init->_isblock);
	if (bUsingOuterBlock) {
		thisinit = *(init->_u._kids._kids + *outerindex);
		(*outerindex)++;
	}
	else {
		thisinit = init;
	}

	if (thisinit->_isblock) {
		if (thisinit->_u._kids._cnt > 1) {
			logger_output_s("error: too many initializers at %w.\n", &init->_loc);
			return FALSE;
		}

		thisinit = *(thisinit->_u._kids._kids + 0);
	}

	assert(!thisinit->_isblock);
	
	return cc_gen_localvar_assign(list, baseAddr, offset, thisinit->_u._expr);
}

static BOOL cc_localvarinit_get_scalar_int(struct tagCCType* ty, FVarInitializer* init, int* outerindex, BOOL bUsingOuterBlock, FCCIRTree** outExpr)
{
	FVarInitializer* thisinit;

	assert(!bUsingOuterBlock || init->_isblock);
	if (bUsingOuterBlock) {
		thisinit = *(init->_u._kids._kids + *outerindex);
		(*outerindex)++;
	}
	else {
		thisinit = init;
	}

	if (thisinit->_isblock) {
		if (thisinit->_u._kids._cnt > 1) {
			logger_output_s("error: too many initializers at %w.\n", &init->_loc);
			return FALSE;
		}

		thisinit = *(thisinit->_u._kids._kids + 0);
	}

	assert(!thisinit->_isblock);
	*outExpr = thisinit->_u._expr;

	return TRUE;
}

static BOOL cc_gen_localvar_initcodes_union(struct tagCCIRCodeList* list, FCCIRTree* baseAddr, int offset, struct tagCCType* ty, FVarInitializer* init, int* outerindex, BOOL bUsingOuterBlock)
{
	FVarInitializer* thisinit, * tmpinit;
	FCCField* first;
	BOOL bnewblock = TRUE;

	assert(!bUsingOuterBlock || init->_isblock);
	thisinit = init;
	if (bUsingOuterBlock) {
		tmpinit = *(init->_u._kids._kids + *outerindex);
		if (tmpinit->_isblock) {
			(*outerindex)++;
			thisinit = tmpinit;
		}
		else {
			bnewblock = FALSE;
		}
	}
	if (!thisinit->_isblock) {
		logger_output_s("error: initializer for union need brace '{', at %w\n", &thisinit->_loc);
		return FALSE;
	}

	first = cc_type_fields(ty);
	if (bnewblock) {
		int innerindex = 0;

		if (first->_lsb > 0) {
			FCCIRTree* intExpr, *shiftcnt;

			if (!cc_localvarinit_get_scalar_int(first->_type, thisinit, &innerindex, TRUE, &intExpr))
			{
				return FALSE;
			}

			/* bitsval = bitsval << (first->_lsb - 1) */
			shiftcnt = cc_expr_constant(gbuiltintypes._sinttype, IR_S32, &intExpr->_loc, CC_MM_TEMPPOOL, (first->_lsb - 1));
			intExpr = cc_expr_lshift(intExpr->_ty, intExpr, shiftcnt, &intExpr->_loc, CC_MM_TEMPPOOL);
			
			if (!cc_gen_localvar_assign(list, baseAddr, offset, intExpr))
			{
				return FALSE;
			}
		}
		else {
			if (!cc_gen_localvar_initcodes_inner(list, baseAddr, offset, first->_type, thisinit, &innerindex, TRUE))
			{
				return FALSE;
			}
		}

		if (innerindex < thisinit->_u._kids._cnt) {
			logger_output_s("error: too many initializers in block %w\n", &thisinit->_loc);
			return FALSE;
		}
	}
	else {
		if (first->_lsb > 0) {
			FCCIRTree* intExpr, * shiftcnt;

			if (!cc_localvarinit_get_scalar_int(first->_type, thisinit, outerindex, TRUE, &intExpr))
			{
				return FALSE;
			}

			/* bitsval = bitsval << (first->_lsb - 1) */
			shiftcnt = cc_expr_constant(gbuiltintypes._sinttype, IR_S32, &intExpr->_loc, CC_MM_TEMPPOOL, (first->_lsb - 1));
			intExpr = cc_expr_lshift(intExpr->_ty, intExpr, shiftcnt, &intExpr->_loc, CC_MM_TEMPPOOL);

			if (!cc_gen_localvar_assign(list, baseAddr, offset, intExpr))
			{
				return FALSE;
			}
		}
		else {
			if (!cc_gen_localvar_initcodes_inner(list, baseAddr, offset, first->_type, thisinit, outerindex, TRUE))
			{
				return FALSE;
			}
		}
	}

	return TRUE;
}

static BOOL cc_gen_localvar_initcodes_struct(struct tagCCIRCodeList* list, FCCIRTree* baseAddr, int structOffset, struct tagCCType* ty, FVarInitializer* init, int* outerindex, BOOL bUsingOuterBlock)
{
	FVarInitializer* thisinit, * tmpinit;
	FCCField* field;
	BOOL bnewblock = TRUE;

	assert(!bUsingOuterBlock || init->_isblock);
	thisinit = init;
	if (bUsingOuterBlock) {
		tmpinit = *(init->_u._kids._kids + *outerindex);
		if (tmpinit->_isblock) {
			(*outerindex)++;
			thisinit = tmpinit;
		}
		else {
			bnewblock = FALSE;
		}
	}

	assert(thisinit->_isblock);

	if (bnewblock) {
		int offset = 0, innerindex = 0;

		for (field = cc_type_fields(ty); field; )
		{
			offset = field->_offset;

			if (field->_lsb > 0) {
				FCCIRTree *result, * intExpr, * shiftcnt;

				assert(field->_type->_size == gbuiltintypes._sinttype->_size);
				result = cc_expr_constant(UnQual(field->_type), cc_ir_typecode(UnQual(field->_type)), NULL, CC_MM_TEMPPOOL, 0);
				for (; field; field = field->_next) {
					if (field->_offset != offset) {
						break;
					}
					if (innerindex < thisinit->_u._kids._cnt) {
						if (!cc_localvarinit_get_scalar_int(field->_type, thisinit, &innerindex, TRUE, &intExpr))
						{
							return FALSE;
						}

						/* bitsval |= val << (field->_lsb - 1) */
						shiftcnt = cc_expr_constant(gbuiltintypes._sinttype, IR_S32, &intExpr->_loc, CC_MM_TEMPPOOL, (field->_lsb - 1));
						intExpr = cc_expr_lshift(intExpr->_ty, intExpr, shiftcnt, &intExpr->_loc, CC_MM_TEMPPOOL);
						result = cc_expr_bitor(intExpr->_ty, result, intExpr, &intExpr->_loc, CC_MM_TEMPPOOL);
					}
				} /* end for */

				if (!cc_gen_localvar_assign(list, baseAddr, structOffset + offset, result))
				{
					return FALSE;
				}
			}
			else {
				if (innerindex < thisinit->_u._kids._cnt) {
					if (!cc_gen_localvar_initcodes_inner(list, baseAddr, structOffset + offset, field->_type, thisinit, &innerindex, TRUE))
					{
						return FALSE;
					}
				}
				else {
					/* space zero */
					gSpaceZeroBytes += field->_type->_size;
				}

				field = field->_next;
			}
		} /* end for */
	}
	else {
		int offset = 0;

		for (field = cc_type_fields(ty); field; )
		{
			offset = field->_offset;

			if (field->_lsb > 0) {
				FCCIRTree* result, * intExpr, * shiftcnt;

				assert(field->_type->_size == gbuiltintypes._sinttype->_size);
				result = cc_expr_constant(UnQual(field->_type), cc_ir_typecode(UnQual(field->_type)), NULL, CC_MM_TEMPPOOL, 0);

				for (; field; field = field->_next) {
					if (field->_offset != offset) {
						break;
					}
					if (*outerindex < thisinit->_u._kids._cnt) {
						if (!cc_localvarinit_get_scalar_int(field->_type, thisinit, outerindex, TRUE, &intExpr))
						{
							return FALSE;
						}

						/* bitsval |= val << (field->_lsb - 1) */
						shiftcnt = cc_expr_constant(gbuiltintypes._sinttype, IR_S32, &intExpr->_loc, CC_MM_TEMPPOOL, (field->_lsb - 1));
						intExpr = cc_expr_lshift(intExpr->_ty, intExpr, shiftcnt, &intExpr->_loc, CC_MM_TEMPPOOL);
						result = cc_expr_bitor(intExpr->_ty, result, intExpr, &intExpr->_loc, CC_MM_TEMPPOOL);
					}
				} /* end for */

				if (!cc_gen_localvar_assign(list, baseAddr, structOffset + offset, result))
				{
					return FALSE;
				}
			}
			else {
				if (*outerindex < thisinit->_u._kids._cnt) {
					if (!cc_gen_localvar_initcodes_inner(list, baseAddr, structOffset + offset, field->_type, thisinit, outerindex, TRUE))
					{
						return FALSE;
					}
				}
				else {
					/* space zero */
					gSpaceZeroBytes += field->_type->_size;
				}

				field = field->_next;
			}
		} /* end for */
	}

	return TRUE;
}

static BOOL cc_gen_localvar_initcodes_array(struct tagCCIRCodeList* list, FCCIRTree* baseAddr, int offset, struct tagCCType* ty, FVarInitializer* init, int* outerindex, BOOL bUsingOuterBlock)
{
	FVarInitializer* thisinit, * tmpinit;
	BOOL bnewblock = TRUE;

	thisinit = init;
	if (bUsingOuterBlock) {
		tmpinit = *(init->_u._kids._kids + *outerindex);
		if (tmpinit->_isblock) {
			(*outerindex)++;
			thisinit = tmpinit;
		}
		else {
			bnewblock = FALSE;
		}
	}

	/* check if new block */
	if (bnewblock)
	{
		/*  ie:
		 *    int a[] = { 0, 1, 2}
		 *    char s0[] = { "abc" }
		 *    char s1[] = { 'a', 'b', "cd" }
		 *
		 */

		FCCType* elety;
		BOOL ischararray, iscnststr;
		int arraysize, innerindex, tycode;

		elety = UnQual(ty->_type);
		tycode = cc_ir_typecode(elety);
		ischararray = (elety == gbuiltintypes._chartype) || (elety == gbuiltintypes._wchartype);

		if (thisinit->_isblock)
		{
			assert(ty->_size > 0);
			for (innerindex = arraysize = 0; ty->_size > arraysize && innerindex < thisinit->_u._kids._cnt; arraysize += elety->_size)
			{
				tmpinit = *(thisinit->_u._kids._kids + innerindex);
				iscnststr = !tmpinit->_isblock && (tmpinit->_u._expr->_op == IR_MKOP1(IR_CONST, IR_STRA) || tmpinit->_u._expr->_op == IR_MKOP1(IR_CONST, IR_STRW));

				if (ischararray && iscnststr)
				{
					int n, chcnt;
					const void* str;

					str = tmpinit->_u._expr->_u._val._payload;
					chcnt = (tmpinit->_u._expr->_ty->_size) / (tmpinit->_u._expr->_ty->_type->_size);
					if (elety == gbuiltintypes._chartype) {
						for (n = 0; ty->_size > arraysize && n < chcnt; n++, arraysize += elety->_size)
						{
							FCCIRTree* cnstchar = cc_expr_constant(elety, tycode, NULL, CC_MM_TEMPPOOL, ((const uint8_t*)str)[n]);

							if (!cc_gen_localvar_assign(list, baseAddr, offset + arraysize, cnstchar))
							{
								return FALSE;
							}
						} /* end for */
					}
					else {
						for (n = 0; ty->_size > arraysize && n < chcnt; n++, arraysize += elety->_size)
						{
							FCCIRTree* cnstwchar = cc_expr_constant(elety, tycode, NULL, CC_MM_TEMPPOOL, ((const uint16_t*)str)[n]);

							if (!cc_gen_localvar_assign(list, baseAddr, offset + arraysize, cnstwchar))
							{
								return FALSE;
							}
						}
					}

					innerindex++;
					break; /* don't continue */
				}
				else if (!cc_gen_localvar_initcodes_inner(list, baseAddr, offset + arraysize, elety, thisinit, &innerindex, TRUE))
				{
					return FALSE;
				}
			} /* end for */

			if (ty->_size > arraysize) {
				/* un-initialized data */
				gSpaceZeroBytes += (ty->_size - arraysize);
			}
		}
		else
		{
			tmpinit = thisinit;
			iscnststr = !tmpinit->_isblock && (tmpinit->_u._expr->_op == IR_MKOP1(IR_CONST, IR_STRA) || tmpinit->_u._expr->_op == IR_MKOP1(IR_CONST, IR_STRW));
			if (ischararray && iscnststr)
			{
				int n, chcnt;
				const void* str;

				str = tmpinit->_u._expr->_u._val._payload;
				chcnt = (tmpinit->_u._expr->_ty->_size) / (tmpinit->_u._expr->_ty->_type->_size);
				if (elety == gbuiltintypes._chartype) {
					for (n = arraysize = 0; ty->_size > arraysize && n < chcnt; n++, arraysize += elety->_size)
					{
						FCCIRTree* cnstchar = cc_expr_constant(elety, tycode, NULL, CC_MM_TEMPPOOL, ((const uint8_t*)str)[n]);

						if (!cc_gen_localvar_assign(list, baseAddr, offset + n * elety->_size, cnstchar))
						{
							return FALSE;
						}
					}
				}
				else {
					for (n = arraysize = 0; ty->_size > arraysize && n < chcnt; n++, arraysize += elety->_size)
					{
						FCCIRTree* cnstwchar = cc_expr_constant(elety, tycode, NULL, CC_MM_TEMPPOOL, ((const uint16_t*)str)[n]);

						if (!cc_gen_localvar_assign(list, baseAddr, offset + n * elety->_size, cnstwchar))
						{
							return FALSE;
						}
					}
				}
			}
			else
			{
				logger_output_s("error: initializer for array need brace '{', at %w\n", &tmpinit->_loc);
				return FALSE;
			}
		}
	}
	else /* !bnewblock */
	{
		FCCType* elety;
		BOOL ischararray, iscnststr;
		int arraysize, tycode;

		assert(thisinit->_isblock);
		elety = UnQual(ty->_type);
		tycode = cc_ir_typecode(elety);
		ischararray = (elety == gbuiltintypes._chartype) || (elety == gbuiltintypes._wchartype);

		assert(ty->_size > 0);
		for (arraysize = 0; ty->_size > arraysize && *outerindex < thisinit->_u._kids._cnt; arraysize += elety->_size)
		{
			tmpinit = *(thisinit->_u._kids._kids + *outerindex);
			iscnststr = !tmpinit->_isblock && (tmpinit->_u._expr->_op == IR_MKOP1(IR_CONST, IR_STRA) || tmpinit->_u._expr->_op == IR_MKOP1(IR_CONST, IR_STRW));

			if (ischararray && iscnststr)
			{
				/* ie: "ABC" will be ignore for str[4].
					struct Struct {
						int a;
						int b;
						char str[4];
					};

					struct Struct s = { 1, 2, 'a', 'b', "ABC" };
				*/

				int n, chcnt;
				const void* str;

				if (arraysize > 0) {
					break; /* "ABC" will be ignore for str[4]. */
				}

				str = tmpinit->_u._expr->_u._val._payload;
				chcnt = (tmpinit->_u._expr->_ty->_size) / (tmpinit->_u._expr->_ty->_type->_size);
				if (elety == gbuiltintypes._chartype) {
					for (n = 0; ty->_size > arraysize; n++, arraysize += elety->_size)
					{
						FCCIRTree* cnstchar = cc_expr_constant(elety, tycode, NULL, CC_MM_TEMPPOOL, (n < chcnt) ? ((const uint8_t*)str)[n] : 0);

						if (!cc_gen_localvar_assign(list, baseAddr, offset + arraysize, cnstchar))
						{
							return FALSE;
						}
					} /* end for */
				}
				else {
					for (n = 0; ty->_size > arraysize; n++, arraysize += elety->_size)
					{
						FCCIRTree* cnstwchar = cc_expr_constant(elety, tycode, NULL, CC_MM_TEMPPOOL, (n < chcnt) ? ((const uint16_t*)str)[n] : 0);

						if (!cc_gen_localvar_assign(list, baseAddr, offset + arraysize, cnstwchar))
						{
							return FALSE;
						}
					}
				}

				(*outerindex)++;
				break; /* don't continue */
			}
			else if (!cc_gen_localvar_initcodes_inner(list, baseAddr, offset + arraysize, elety, thisinit, outerindex, TRUE))
			{
				return FALSE;
			}
		} /* end for */

		if (ty->_size > arraysize)
		{
			/* un-initialized data */
			gSpaceZeroBytes += (ty->_size - arraysize);
		}
	}

	return TRUE;
}

static BOOL cc_gen_localvar_initcodes_inner(struct tagCCIRCodeList* list, FCCIRTree* baseAddr, int offset, struct tagCCType* ty, FVarInitializer* init, int* outerindex, BOOL bUsingOuterBlock)
{
	if (IsScalar(ty)) {
		return cc_gen_localvar_initcodes_scalar(list, baseAddr, offset, ty, init, outerindex, bUsingOuterBlock);
	}
	else if (IsUnion(ty)) {
		return cc_gen_localvar_initcodes_union(list, baseAddr, offset, ty, init, outerindex, bUsingOuterBlock);
	}
	else if (IsStruct(ty)) {
		return cc_gen_localvar_initcodes_struct(list, baseAddr, offset, ty, init, outerindex, bUsingOuterBlock);
	}
	else if (IsArray(ty)) {
		return cc_gen_localvar_initcodes_array(list, baseAddr, offset, ty, init, outerindex, bUsingOuterBlock);
	}
	else {
		assert(0);
	}

	return TRUE;
}

BOOL cc_gen_localvar_initcodes(struct tagCCIRCodeList* list, struct tagCCSymbol* p)
{
	FCCIRCode* iafter, *zerocode;
	FCCIRTree* exprId, *baseaddr;
	
	assert(p->_u._initializer);

	iafter = list->_tail;
	gSpaceZeroBytes = 0;
	exprId = cc_expr_id(p, NULL, CC_MM_TEMPPOOL);
	if (IsArray(exprId->_ty)) {
		baseaddr = cc_expr_adjust(exprId, CC_MM_TEMPPOOL);
	}
	else {
		baseaddr = cc_expr_addr(exprId, NULL, CC_MM_TEMPPOOL);
	}
	if (!cc_gen_localvar_initcodes_inner(list, baseaddr, 0, p->_type, p->_u._initializer, NULL, FALSE))
	{
		return FALSE;
	}

	if (gSpaceZeroBytes > 0) {
		zerocode = cc_ir_newcode_setzero(baseaddr, p->_type->_size, CC_MM_TEMPPOOL);
		cc_ir_codelist_insert_after(list, iafter, zerocode);
	}

	return TRUE;
}
