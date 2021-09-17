/* \brief
 *		statement parsing.
 */


#include "stmt.h"
#include "lexer/token.h"
#include "logger.h"
#include "parser.h"
#include <string.h>
#include <assert.h>


static struct tagCCStatement* cc_find_case(struct tagCCStatement* stmtswitch, int val)
{
	FArray* cases;
	int n;

	assert(stmtswitch->_kind == ESTMT_Switch);
	cases = &(stmtswitch->_u._switch._cases);
	for (n=0; n<cases->_elecount; n++)
	{
		FCCStatement* stmt = *(FCCStatement**)array_element(cases, n);
		assert(stmt->_kind == ESTMT_Case);
		if (stmt->_u._case._constval == val) {
			return stmt;
		}
	} /* end for */

	return NULL;
}

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
			logger_output_s("error: expect a statement. at %w\n", &ccctx->_currtk._loc);
		}
	}

	return bSuccess;
}

BOOL cc_stmt_compound(struct tagCCContext* ccctx, struct tagCCStmtContext* stmtctx, struct tagCCStatement** outstmt)
{
	struct tagCCStatement* stmt;

	if (!(stmt = cc_stmt_new(ESTMT_Compound))) {
		return FALSE;
	}
	stmt->_loc = ccctx->_currtk._loc;

	if (!cc_parser_expect(ccctx, TK_LBRACE)) /* '{' */
	{
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
	struct tagCCStatement* stmt;
	FCCSymbol* p;

	assert(ccctx->_currtk._type == TK_ID);
	if (!(stmt = cc_stmt_new(ESTMT_Label))) {
		return FALSE;
	}
	stmt->_loc = ccctx->_currtk._loc;

	p = cc_symbol_lookup(ccctx->_currtk._val._astr._str, gLabels);
	if (!p) {
		p = cc_symbol_install(ccctx->_currtk._val._astr._str, &gLabels, SCOPE_LABEL, CC_MM_TEMPPOOL);
		p->_loc = ccctx->_currtk._loc;
	}
	if (p->_defined) {
		logger_output_s("error: redefinition of label '%s' at %w, previously defined at %w.\n", ccctx->_currtk._val._astr._str, &stmt->_loc, &p->_loc);
		return FALSE;
	}
	p->_defined = 1;
	stmt->_u._label._id = p;

	cc_read_token(ccctx, &ccctx->_currtk);
	if (!cc_parser_expect(ccctx, TK_COLON)) { /* ':' */
		return FALSE;
	}

	if (!cc_stmt_statement(ccctx, stmtctx, &stmt->_link)) {
		logger_output_s("error: expect a statement after label '%s' at %w\n", p->_name, &p->_loc);
		return FALSE;
	}

	*outstmt = stmt;
	return TRUE;
}

BOOL cc_stmt_case(struct tagCCContext* ccctx, struct tagCCStmtContext* stmtctx, struct tagCCStatement** outstmt)
{
	struct tagCCStatement* stmt;
	int constval;


	if (!(stmt = cc_stmt_new(ESTMT_Case))) {
		return FALSE;
	}
	stmt->_loc = ccctx->_currtk._loc;

	if (!cc_parser_expect(ccctx, TK_CASE)) { /* 'case' */
		return FALSE;
	}

	if (!stmtctx->_switch) {
		logger_output_s("error: 'case' should in a switch statement, at %w.\n", &stmt->_loc);
		return FALSE;
	}

	if (!cc_parser_intexpression(ccctx, &constval))
	{
		logger_output_s("error: case label must be a constant value, at %w.\n", &stmt->_loc);
		return FALSE;
	}

	if (!cc_parser_expect(ccctx, TK_COLON)) { /* ':' */
		return FALSE;
	}

	if (!cc_stmt_statement(ccctx, stmtctx, &stmt->_link)) {
		logger_output_s("error: expect a statement after 'case' at %w\n", &stmt->_loc);
		return FALSE;
	}

	*outstmt = stmt;
	return TRUE;
}

BOOL cc_stmt_default(struct tagCCContext* ccctx, struct tagCCStmtContext* stmtctx, struct tagCCStatement** outstmt)
{
	struct tagCCStatement* stmt;

	if (!(stmt = cc_stmt_new(ESTMT_Default))) {
		return FALSE;
	}
	stmt->_loc = ccctx->_currtk._loc;

	if (!cc_parser_expect(ccctx, TK_DEFAULT)) { /* 'default' */
		return FALSE;
	}

	if (!stmtctx->_switch) {
		logger_output_s("error: 'default' should in a switch statement, at %w.\n", &stmt->_loc);
		return FALSE;
	}

	if (!cc_parser_expect(ccctx, TK_COLON)) { /* ':' */
		return FALSE;
	}

	if (!cc_stmt_statement(ccctx, stmtctx, &stmt->_link)) {
		logger_output_s("error: expect a statement after 'default' at %w\n", &stmt->_loc);
		return FALSE;
	}

	*outstmt = stmt;
	return TRUE;
}

BOOL cc_stmt_expression(struct tagCCContext* ccctx, struct tagCCStmtContext* stmtctx, struct tagCCStatement** outstmt)
{
	struct tagCCStatement* stmt;
	FCCExprTree* expr;

	if (!(stmt = cc_stmt_new(ESTMT_Expr))) {
		return FALSE;
	}
	stmt->_loc = ccctx->_currtk._loc;

	if (!cc_expr_expression(ccctx, &expr, CC_MM_TEMPPOOL)) {
		return FALSE;
	}

	if (!cc_parser_expect(ccctx, TK_SEMICOLON)) { /* ';' */
		return FALSE;
	}

	stmt->_u._expression._expr = expr;
	*outstmt = stmt;
	return TRUE;
}

BOOL cc_stmt_ifelse(struct tagCCContext* ccctx, struct tagCCStmtContext* stmtctx, struct tagCCStatement** outstmt)
{
	struct tagCCStatement* stmt;

	if (!(stmt = cc_stmt_new(ESTMT_If))) {
		return FALSE;
	}
	stmt->_loc = ccctx->_currtk._loc;

	if (!cc_parser_expect(ccctx, TK_IF)) { /* if */
		return FALSE;
	}

	if (!cc_parser_expect(ccctx, TK_LPAREN)) { /* '(' */
		return FALSE;
	}

	if (!cc_expr_expression(ccctx, &(stmt->_u._ifelse._expr), CC_MM_TEMPPOOL)) {
		return FALSE;
	}

	if (!cc_parser_expect(ccctx, TK_RPAREN)) { /* ')' */
		return FALSE;
	}

	if (!cc_stmt_statement(ccctx, stmtctx, &stmt->_u._ifelse._if)) {
		return FALSE;
	}

	if (ccctx->_currtk._type == TK_ELSE) { /* 'else' */
		cc_read_token(ccctx, &ccctx->_currtk);
		if (!cc_stmt_statement(ccctx, stmtctx, &stmt->_u._ifelse._else)) {
			return FALSE;
		}
	}

	*outstmt = stmt;
	return TRUE;
}

BOOL cc_stmt_switch(struct tagCCContext* ccctx, struct tagCCStmtContext* stmtctx, struct tagCCStatement** outstmt)
{
	struct tagCCStmtContext currstmtctx;
	struct tagCCStatement* stmt;
	
	currstmtctx = *stmtctx;
	if (!(stmt = cc_stmt_new(ESTMT_Switch))) {
		return FALSE;
	}
	stmt->_loc = ccctx->_currtk._loc;

	if (!cc_parser_expect(ccctx, TK_LPAREN)) { /* '(' */
		return FALSE;
	}

	if (!cc_expr_expression(ccctx, &(stmt->_u._switch._expr), CC_MM_TEMPPOOL)) {
		return FALSE;
	}

	if (!cc_parser_expect(ccctx, TK_RPAREN)) { /* ')' */
		return FALSE;
	}

	currstmtctx._switch = stmt;
	if (!cc_stmt_statement(ccctx, &currstmtctx, &stmt->_u._switch._stmt)) {
		return FALSE;
	}

	*outstmt = stmt;
	return TRUE;
}

BOOL cc_stmt_while(struct tagCCContext* ccctx, struct tagCCStmtContext* stmtctx, struct tagCCStatement** outstmt)
{
	struct tagCCStmtContext currstmtctx;
	struct tagCCStatement* stmt;

	currstmtctx = *stmtctx;
	if (!(stmt = cc_stmt_new(ESTMT_While))) {
		return FALSE;
	}
	stmt->_loc = ccctx->_currtk._loc;

	if (!cc_parser_expect(ccctx, TK_LPAREN)) { /* '(' */
		return FALSE;
	}

	if (!cc_expr_expression(ccctx, &(stmt->_u._while._expr), CC_MM_TEMPPOOL)) {
		return FALSE;
	}

	if (!cc_parser_expect(ccctx, TK_RPAREN)) { /* ')' */
		return FALSE;
	}

	currstmtctx._loop = stmt;
	if (!cc_stmt_statement(ccctx, &currstmtctx, &stmt->_u._while._stmt)) {
		return FALSE;
	}

	*outstmt = stmt;
	return TRUE;
}

BOOL cc_stmt_dowhile(struct tagCCContext* ccctx, struct tagCCStmtContext* stmtctx, struct tagCCStatement** outstmt)
{
	struct tagCCStmtContext currstmtctx;
	struct tagCCStatement* stmt;

	currstmtctx = *stmtctx;
	if (!(stmt = cc_stmt_new(ESTMT_DoWhile))) {
		return FALSE;
	}
	stmt->_loc = ccctx->_currtk._loc;

	if (!cc_parser_expect(ccctx, TK_DO)) {
		return FALSE;
	}

	currstmtctx._loop = stmt;
	if (!cc_stmt_statement(ccctx, &currstmtctx, &stmt->_u._dowhile._stmt)) {
		return FALSE;
	}

	if (!cc_parser_expect(ccctx, TK_LPAREN)) { /* '(' */
		return FALSE;
	}

	if (!cc_expr_expression(ccctx, &(stmt->_u._dowhile._expr), CC_MM_TEMPPOOL)) {
		return FALSE;
	}

	if (!cc_parser_expect(ccctx, TK_RPAREN)) { /* ')' */
		return FALSE;
	}
	if (!cc_parser_expect(ccctx, TK_SEMICOLON)) { /* ';' */
		return FALSE;
	}

	*outstmt = stmt;
	return TRUE;
}

BOOL cc_stmt_for(struct tagCCContext* ccctx, struct tagCCStmtContext* stmtctx, struct tagCCStatement** outstmt)
{
	struct tagCCStmtContext currstmtctx;
	struct tagCCStatement* stmt;

	currstmtctx = *stmtctx;
	if (!(stmt = cc_stmt_new(ESTMT_For))) {
		return FALSE;
	}
	stmt->_loc = ccctx->_currtk._loc;

	if (!cc_parser_expect(ccctx, TK_FOR)) {
		return FALSE;
	}
	if (!cc_parser_expect(ccctx, TK_LPAREN)) { /* '(' */
		return FALSE;
	}

	if (ccctx->_currtk._type == TK_SEMICOLON) {
		stmt->_u._for._expr0 = NULL;
	}
	else if (!cc_expr_expression(ccctx, &(stmt->_u._for._expr0), CC_MM_TEMPPOOL)) {
		return FALSE;
	}

	if (!cc_parser_expect(ccctx, TK_SEMICOLON)) { /* ';' */
		return FALSE;
	}

	if (ccctx->_currtk._type == TK_SEMICOLON) {
		stmt->_u._for._expr1 = NULL;
	}
	else if (!cc_expr_expression(ccctx, &(stmt->_u._for._expr1), CC_MM_TEMPPOOL)) {
		return FALSE;
	}

	if (!cc_parser_expect(ccctx, TK_SEMICOLON)) { /* ';' */
		return FALSE;
	}

	if (ccctx->_currtk._type == TK_SEMICOLON) {
		stmt->_u._for._expr2 = NULL;
	}
	else if (!cc_expr_expression(ccctx, &(stmt->_u._for._expr2), CC_MM_TEMPPOOL)) {
		return FALSE;
	}
	if (!cc_parser_expect(ccctx, TK_RPAREN)) { /* ')' */
		return FALSE;
	}

	currstmtctx._loop = stmt;
	if (!cc_stmt_statement(ccctx, &currstmtctx, &stmt->_u._for._stmt)) {
		return FALSE;
	}

	*outstmt = stmt;
	return TRUE;
}

BOOL cc_stmt_goto(struct tagCCContext* ccctx, struct tagCCStmtContext* stmtctx, struct tagCCStatement** outstmt)
{
	struct tagCCStatement* stmt;
	FCCSymbol* p;

	if (!(stmt = cc_stmt_new(ESTMT_Goto))) {
		return FALSE;
	}
	stmt->_loc = ccctx->_currtk._loc;

	if (ccctx->_currtk._type == TK_ID) {
		logger_output_s("error: illegal goto syntax, expect a label at %w\n", &stmt->_loc);
		return FALSE;
	}
	
	p = cc_symbol_lookup(ccctx->_currtk._val._astr._str, gLabels);
	if (!p) {
		p = cc_symbol_install(ccctx->_currtk._val._astr._str, &gLabels, SCOPE_LABEL, CC_MM_TEMPPOOL);
	}
	stmt->_u._goto._id = p;

	cc_read_token(ccctx, &ccctx->_currtk);
	if (!cc_parser_expect(ccctx, TK_SEMICOLON)) { /* ';' */
		return FALSE;
	}

	*outstmt = stmt;
	return TRUE;
}

BOOL cc_stmt_continue(struct tagCCContext* ccctx, struct tagCCStmtContext* stmtctx, struct tagCCStatement** outstmt)
{
	struct tagCCStatement* stmt;
	FCCSymbol* p;


	if (!(stmt = cc_stmt_new(ESTMT_Continue))) {
		return FALSE;
	}
	stmt->_loc = ccctx->_currtk._loc;

	if (!stmtctx->_loop) {
		logger_output_s("error: 'continue' should in a iteration statement, at %w.\n", &stmt->_loc);
		return FALSE;
	}
	// TODO: 根据loop类型, 设置跳转目标

	if (!cc_parser_expect(ccctx, TK_CONTINUE)) { /* 'continue' */
		return FALSE;
	}
	if (!cc_parser_expect(ccctx, TK_SEMICOLON)) { /* ';' */
		return FALSE;
	}

	*outstmt = stmt;
	return TRUE;
}

BOOL cc_stmt_break(struct tagCCContext* ccctx, struct tagCCStmtContext* stmtctx, struct tagCCStatement** outstmt)
{
	struct tagCCStatement* stmt;
	FCCSymbol* p;


	if (!(stmt = cc_stmt_new(ESTMT_Break))) {
		return FALSE;
	}
	stmt->_loc = ccctx->_currtk._loc;

	if (!stmtctx->_loop) {
		logger_output_s("error: 'break' should in a iteration statement, at %w.\n", &stmt->_loc);
		return FALSE;
	}
	// TODO: 根据loop类型, 设置跳转目标

	if (!cc_parser_expect(ccctx, TK_BREAK)) { /* 'break' */
		return FALSE;
	}
	if (!cc_parser_expect(ccctx, TK_SEMICOLON)) { /* ';' */
		return FALSE;
	}

	*outstmt = stmt;
	return TRUE;
}

BOOL cc_stmt_return(struct tagCCContext* ccctx, struct tagCCStmtContext* stmtctx, struct tagCCStatement** outstmt)
{
	struct tagCCStatement* stmt;
	FCCSymbol* p;


	if (!(stmt = cc_stmt_new(ESTMT_Return))) {
		return FALSE;
	}
	stmt->_loc = ccctx->_currtk._loc;
	// TODO: 设置跳转目标

	if (!cc_parser_expect(ccctx, TK_RETURN)) {
		return FALSE;
	}
	if (ccctx->_currtk._type == TK_SEMICOLON) { /* ';' */
		stmt->_u._return._expr = NULL;
	}
	else if(!cc_expr_expression(ccctx, &(stmt->_u._return._expr), CC_MM_TEMPPOOL)) {
		return FALSE;
	}

	if (!cc_parser_expect(ccctx, TK_SEMICOLON)) { /* ';' */
		return FALSE;
	}

	*outstmt = stmt;
	return TRUE;
}
