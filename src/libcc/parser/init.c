/* \brief
 *		variable initializer parsing.
 */

#include "init.h"
#include "expr.h"
#include "lexer/token.h"
#include "logger.h"
#include "parser.h"


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
			if (!cc_expr_constant_expression(ctx, &initializer->_u._expr, where)) {
				return FALSE;
			}
		}
		else {
			if (!cc_expr_assignment(ctx, &initializer->_u._expr, where)) {
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
static BOOL cc_varinit_check_inner(struct tagCCContext* ctx, struct tagCCType* ty, FVarInitializer* init, int* outerindex, BOOL bUsingOuterBlock);

static BOOL cc_varinit_check_scalar(struct tagCCContext* ctx, struct tagCCType* ty, FVarInitializer* init, int* outerindex, BOOL bUsingOuterBlock)
{
	FVarInitializer* thisinit;
	FCCExprTree* expr;

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
	expr = thisinit->_u._expr;
	if (!cc_expr_assigntype(ty, expr)) {
		logger_output_s("error: initialize assign failed at %w.\n", &expr->_loc);
		return FALSE;
	}

	return TRUE;
}

static BOOL cc_varinit_check_union(struct tagCCContext* ctx, struct tagCCType* ty, FVarInitializer* init, int* outerindex, BOOL bUsingOuterBlock)
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

		if (!cc_varinit_check_inner(ctx, first->_type, thisinit, &innerindex, TRUE))
		{
			return FALSE;
		}

		if (innerindex < thisinit->_u._kids._cnt) {
			logger_output_s("error: too many initializers in block %w\n", &thisinit->_loc);
			return FALSE;
		}
	}
	else {
		if (!cc_varinit_check_inner(ctx, first->_type, thisinit, outerindex, TRUE))
		{
			return FALSE;
		}
	}

	return TRUE;
}

static BOOL cc_varinit_check_struct(struct tagCCContext* ctx, struct tagCCType* ty, FVarInitializer* init, int* outerindex, BOOL bUsingOuterBlock)
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
			if (!cc_varinit_check_inner(ctx, field->_type, thisinit, &innerindex, TRUE))
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
			if (!cc_varinit_check_inner(ctx, field->_type, thisinit, outerindex, TRUE))
			{
				return FALSE;
			}
		}
	}

	return TRUE;
}

static BOOL cc_varinit_check_array(struct tagCCContext* ctx, struct tagCCType* ty, FVarInitializer* init, int* outerindex, BOOL bUsingOuterBlock)
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
		ischararray = (elety == gBuiltinTypes._chartype) || (elety == gBuiltinTypes._wchartype);

		if (thisinit->_isblock)
		{
			if (ty->_size > 0) {
				for (innerindex = arraysize = 0; ty->_size > arraysize && innerindex < thisinit->_u._kids._cnt; arraysize += elety->_size)
				{
					tmpinit = *(thisinit->_u._kids._kids + innerindex);
					iscnststr = !tmpinit->_isblock && tmpinit->_u._expr->_op == EXPR_CONSTANT && IsArray(tmpinit->_u._expr->_ty);

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
					else if (!cc_varinit_check_inner(ctx, elety, thisinit, &innerindex, TRUE))
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
					iscnststr = !tmpinit->_isblock && tmpinit->_u._expr->_op == EXPR_CONSTANT && IsArray(tmpinit->_u._expr->_ty);

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
					else if (!cc_varinit_check_inner(ctx, elety, thisinit, &innerindex, TRUE))
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
			iscnststr = !tmpinit->_isblock && tmpinit->_u._expr->_op == EXPR_CONSTANT && IsArray(tmpinit->_u._expr->_ty);
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
		ischararray = (elety == gBuiltinTypes._chartype) || (elety == gBuiltinTypes._wchartype);

		if (ty->_size > 0) {
			for (arraysize = 0; ty->_size > arraysize && *outerindex < thisinit->_u._kids._cnt; arraysize += elety->_size)
			{
				tmpinit = *(thisinit->_u._kids._kids + *outerindex);
				iscnststr = !tmpinit->_isblock && tmpinit->_u._expr->_op == EXPR_CONSTANT && IsArray(tmpinit->_u._expr->_ty);

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
					(*outerindex)++;
					break; /* don't continue */
				}
				else if (!cc_varinit_check_inner(ctx, elety, thisinit, outerindex, TRUE))
				{
					return FALSE;
				}
			} /* end for */
		}
		else {
			for (arraysize = 0; *outerindex < thisinit->_u._kids._cnt; arraysize += elety->_size)
			{
				tmpinit = *(thisinit->_u._kids._kids + *outerindex);
				iscnststr = !tmpinit->_isblock && tmpinit->_u._expr->_op == EXPR_CONSTANT && IsArray(tmpinit->_u._expr->_ty);

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
				else if (!cc_varinit_check_inner(ctx, elety, thisinit, outerindex, TRUE))
				{
					return FALSE;
				}
			} /* end for */

			ty->_size = arraysize;
		}
	}

	return TRUE;
}

static BOOL cc_varinit_check_inner(struct tagCCContext* ctx, struct tagCCType* ty, FVarInitializer* init, int* outerindex, BOOL bUsingOuterBlock)
{
	int tmp = 0;

	if (IsScalar(ty)) {
		return cc_varinit_check_scalar(ctx, ty, init, outerindex, bUsingOuterBlock);
	}
	else if (IsUnion(ty)) {
		return cc_varinit_check_union(ctx, ty, init, outerindex, bUsingOuterBlock);
	}
	else if (IsStruct(ty)) {
		return cc_varinit_check_struct(ctx, ty, init, outerindex, bUsingOuterBlock);
	}
	else if (IsArray(ty)) {
		return cc_varinit_check_array(ctx, ty, init, outerindex, bUsingOuterBlock);
	}
	else {
		assert(0);
	}

	return TRUE;
}

BOOL cc_varinit_check(struct tagCCContext* ctx, struct tagCCType* ty, FVarInitializer* init)
{
	return cc_varinit_check_inner(ctx, ty, init, NULL, FALSE);
}
