/* \brief
 *		statement parsing.
 */


#include "stmt.h"
#include "lexer/token.h"
#include "logger.h"
#include "parser.h"
#include <string.h>
#include <assert.h>


FCCStatement* cc_stmt_new(enum EStmtType k)
{
	FCCStatement* stmt = mm_alloc_area(sizeof(FCCStatement), CC_MM_TEMPPOOL);
	if (stmt) {
		memset(stmt, 0, sizeof(FCCStatement));
		stmt->_kind = k;
	}
	else {
		logger_output_s("error: out of memory at %s:%d\n", __FILE__, __LINE__);
	}

	return stmt;
}

BOOL cc_stmt_statement(struct tagCCContext* ccctx, struct tagCCStmtContext* stmtctx, struct tagCCStatement** outstmt)
{
	BOOL bSuccess = FALSE;
	
	*outstmt = NULL;
	switch (ccctx->_currtk._type)
	{
	case TK_IF:
		bSuccess = cc_stmt_ifelse(ccctx, stmtctx, outstmt); break;
	case TK_DO:
		bSuccess = cc_stmt_dowhile(ccctx, stmtctx, outstmt); break;
	case TK_WHILE:
		bSuccess = cc_stmt_while(ccctx, stmtctx, outstmt); break;
	case TK_FOR:
		bSuccess = cc_stmt_for(ccctx, stmtctx, outstmt); break;
	case TK_BREAK:
		bSuccess = cc_stmt_break(ccctx, stmtctx, outstmt); break;
	case TK_CONTINUE:
		bSuccess = cc_stmt_continue(ccctx, stmtctx, outstmt); break;
	case TK_SWITCH:
		bSuccess = cc_stmt_switch(ccctx, stmtctx, outstmt); break;
	case TK_CASE:
		bSuccess = cc_stmt_case(ccctx, stmtctx, outstmt); break;
	case TK_DEFAULT:
		bSuccess = cc_stmt_default(ccctx, stmtctx, outstmt); break;
	case TK_RETURN:
		bSuccess = cc_stmt_return(ccctx, stmtctx, outstmt); break;
	case TK_GOTO:
		bSuccess = cc_stmt_goto(ccctx, stmtctx, outstmt); break;
	case TK_LBRACE: /* '{' */
		bSuccess = cc_stmt_compound(ccctx, stmtctx, outstmt); break;
	case TK_SEMICOLON: /* ';' */
		bSuccess = cc_read_token(ccctx, &ccctx->_currtk); break;
	case TK_ID:
	{
		FCCToken aheadtk;
		cc_lookahead_token(ccctx, &aheadtk);
		if (aheadtk._type == TK_COLON) { /* ':' */
			bSuccess = cc_stmt_label(ccctx, stmtctx, outstmt);
			break;
		}
	}
		/* go through */
	default:
		if (cc_parser_is_stmtspecifier(ccctx->_currtk._type)) {
			bSuccess = cc_stmt_expression(ccctx, stmtctx, outstmt);
		}
		else {
			logger_output_s("error: expect statement token. at %w\n", &ccctx->_currtk._loc);
		}
	}

	return bSuccess;
}

BOOL cc_stmt_compound(struct tagCCContext* ccctx, struct tagCCStmtContext* stmtctx, struct tagCCStatement** outstmt)
{
	struct tagCCStatement* stmt;

	if (!cc_parser_expect(ccctx, TK_LBRACE)) /* '{' */
	{
		return FALSE;
	}

	if (!(stmt = cc_stmt_new(ESTMT_Compound))) {
		return FALSE;
	}

	cc_symbol_enterscope();

	while (cc_parser_is_typename(&ccctx->_currtk))
	{
		if (!cc_parser_declaration(ccctx, &cc_parser_decllocal))
		{
			cc_symbol_exitscope();
			return FALSE;
		}
	} /* end locals */

	/* statements */
	while (cc_parser_is_stmtspecifier(ccctx->_currtk._type) 
		|| ccctx->_currtk._type == TK_LBRACE
		|| ccctx->_currtk._type == TK_SEMICOLON)
	{
		FCCStatement* kidstmt;
		if (!cc_stmt_statement(ccctx, stmtctx, &kidstmt)) {
			return FALSE;
		}

		if (kidstmt) {
			if (stmt->_u._compound._last) {
				stmt->_u._compound._last->_link = kidstmt;
				stmt->_u._compound._last = kidstmt;
			}
			else {
				assert(stmt->_u._compound._first == NULL);
				stmt->_u._compound._first = kidstmt;
				stmt->_u._compound._last = kidstmt;
			}
		}
	} /* end while */

	cc_symbol_exitscope();
	if (!cc_parser_expect(ccctx, TK_RBRACE)) { /* '}' */
		return FALSE;
	}

	*outstmt = stmt;
	return TRUE;
}

BOOL cc_stmt_label(struct tagCCContext* ccctx, struct tagCCStmtContext* stmtctx, struct tagCCStatement** outstmt)
{
	assert(ccctx->_currtk._type == TK_ID);
	// TODO:

	return TRUE;
}

BOOL cc_stmt_case(struct tagCCContext* ccctx, struct tagCCStmtContext* stmtctx, struct tagCCStatement** outstmt)
{
	return TRUE;
}

BOOL cc_stmt_default(struct tagCCContext* ccctx, struct tagCCStmtContext* stmtctx, struct tagCCStatement** outstmt)
{
	return TRUE;
}

BOOL cc_stmt_expression(struct tagCCContext* ccctx, struct tagCCStmtContext* stmtctx, struct tagCCStatement** outstmt)
{
	return TRUE;
}

BOOL cc_stmt_ifelse(struct tagCCContext* ccctx, struct tagCCStmtContext* stmtctx, struct tagCCStatement** outstmt)
{
	return TRUE;
}

BOOL cc_stmt_switch(struct tagCCContext* ccctx, struct tagCCStmtContext* stmtctx, struct tagCCStatement** outstmt)
{
	return TRUE;
}

BOOL cc_stmt_while(struct tagCCContext* ccctx, struct tagCCStmtContext* stmtctx, struct tagCCStatement** outstmt)
{
	return TRUE;
}

BOOL cc_stmt_dowhile(struct tagCCContext* ccctx, struct tagCCStmtContext* stmtctx, struct tagCCStatement** outstmt)
{
	return TRUE;
}

BOOL cc_stmt_for(struct tagCCContext* ccctx, struct tagCCStmtContext* stmtctx, struct tagCCStatement** outstmt)
{
	return TRUE;
}

BOOL cc_stmt_goto(struct tagCCContext* ccctx, struct tagCCStmtContext* stmtctx, struct tagCCStatement** outstmt)
{
	return TRUE;
}

BOOL cc_stmt_continue(struct tagCCContext* ccctx, struct tagCCStmtContext* stmtctx, struct tagCCStatement** outstmt)
{
	return TRUE;
}

BOOL cc_stmt_break(struct tagCCContext* ccctx, struct tagCCStmtContext* stmtctx, struct tagCCStatement** outstmt)
{
	return TRUE;
}

BOOL cc_stmt_return(struct tagCCContext* ccctx, struct tagCCStmtContext* stmtctx, struct tagCCStatement** outstmt)
{
	return TRUE;
}
