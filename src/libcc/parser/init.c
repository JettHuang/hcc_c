/* \brief
 *		variable initializer parsing.
 */

#include "init.h"
#include "expr.h"
#include "lexer/token.h"
#include "logger.h"
#include "parser.h"


FVarInitializer* cc_varinit_new()
{
	FVarInitializer* p;

	if ((p = mm_alloc_area(sizeof(FVarInitializer), CC_MM_TEMPPOOL))) {
		p->_isblock = 0;
		p->_u._expr = NULL;
	}
	else {
		logger_output_s("error: out of memory at %s:%d\n", __FILE__, __LINE__);
	}

	return p;
}


BOOL cc_parser_initializer(struct tagCCContext* ctx, FVarInitializer** outinit)
{
	FVarInitializer* initializer, *kidinit;

	if (!(initializer = cc_varinit_new())) {
		return FALSE;
	}

	initializer->_loc = ctx->_currtk._loc;
	if (ctx->_currtk._type == TK_LBRACE) /* '{' */
	{
		FArray kids;

		initializer->_isblock = 1;
		array_init(&kids, 32, sizeof(FVarInitializer*), CC_MM_TEMPPOOL);

		for (;;)
		{
			if (!cc_parser_initializer(ctx, &kidinit)) {
				return FALSE;
			}

			array_append(&kids, &kidinit);
			if (ctx->_currtk._type != TK_COMMA) /* ',' */
			{
				break;
			}
			cc_read_token(ctx, &ctx->_currtk);
		} /* end for ;; */

		array_append_zeroed(&kids);
		initializer->_u._kids = kids._data;
		if (!cc_parser_expect(ctx, TK_RBRACE)) /* '}' */
		{
			return FALSE;
		}
	}
	else {
		initializer->_isblock = 0;
		if (!cc_expr_assignment(ctx, &initializer->_u._expr)) {
			return FALSE;
		}
	}

	*outinit = initializer;
	return TRUE;
}
