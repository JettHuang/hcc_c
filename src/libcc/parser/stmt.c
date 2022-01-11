/* \brief
 *		statement parsing.
 */


#include "stmt.h"
#include "lexer/token.h"
#include "logger.h"
#include "parser.h"
#include "generator/gen.h"
#include <string.h>
#include <assert.h>


static struct tagCCSymbol* cc_find_case(struct tagCCSwitchContext* swtch, int val)
{
	int n;

	for (n = 0; n < swtch->_cases._elecount; n++)
	{
		FCCSwitchCase* swcase = array_element(&swtch->_cases, n);
		if (swcase->_value == val) {
			return swcase->_label;
		}
	} /* end for */

	return NULL;
}

BOOL cc_stmt_statement(struct tagCCContext* ccctx,  struct tagCCIRCodeList* list, struct tagCCLoopContext* loop, struct tagCCSwitchContext* swtch)
{
	BOOL bSuccess = FALSE;
	
	switch (ccctx->_currtk._type)
	{
	case TK_IF:
		bSuccess = cc_stmt_ifelse(ccctx, list, loop, swtch); break;
	case TK_DO:
		bSuccess = cc_stmt_dowhile(ccctx, list, loop, swtch); break;
	case TK_WHILE:
		bSuccess = cc_stmt_while(ccctx, list, loop, swtch); break;
	case TK_FOR:
		bSuccess = cc_stmt_for(ccctx, list, loop, swtch); break;
	case TK_BREAK:
		bSuccess = cc_stmt_break(ccctx, list, loop, swtch); break;
	case TK_CONTINUE:
		bSuccess = cc_stmt_continue(ccctx, list, loop, swtch); break;
	case TK_SWITCH:
		bSuccess = cc_stmt_switch(ccctx, list, loop, swtch); break;
	case TK_CASE:
		bSuccess = cc_stmt_case(ccctx, list, loop, swtch); break;
	case TK_DEFAULT:
		bSuccess = cc_stmt_default(ccctx, list, loop, swtch); break;
	case TK_RETURN:
		bSuccess = cc_stmt_return(ccctx, list, loop, swtch); break;
	case TK_GOTO:
		bSuccess = cc_stmt_goto(ccctx, list, loop, swtch); break;
	case TK_LBRACE: /* '{' */
		bSuccess = cc_stmt_compound(ccctx, list, loop, swtch); break;
	case TK_SEMICOLON: /* ';' */
		bSuccess = cc_read_token(ccctx, &ccctx->_currtk); break;
	case TK_ID:
	{
		FCCToken aheadtk;
		cc_lookahead_token(ccctx, &aheadtk);
		if (aheadtk._type == TK_COLON) { /* ':' */
			bSuccess = cc_stmt_label(ccctx, list, loop, swtch);
			break;
		}
	}
		/* go through */
	default:
		if (cc_parser_is_stmtspecifier(ccctx->_currtk._type)) {
			bSuccess = cc_stmt_expression(ccctx, list, loop, swtch);
		}
		else {
			logger_output_s("error: expect a statement. at %w\n", &ccctx->_currtk._loc);
		}
	}

	return bSuccess;
}

BOOL cc_stmt_compound(struct tagCCContext* ccctx,  struct tagCCIRCodeList* list, struct tagCCLoopContext* loop, struct tagCCSwitchContext* swtch)
{
	if (!cc_parser_expect(ccctx, TK_LBRACE)) /* '{' */
	{
		return FALSE;
	}
	cc_symbol_enterscope();

	cc_ir_codelist_append(list, cc_ir_newcode_blk(TRUE, gCurrentLevel, CC_MM_TEMPPOOL));

	for (;;) 
	{
		/* is a declaration? */
		if (cc_parser_is_typename(&ccctx->_currtk))
		{
			if (!cc_parser_declaration(ccctx, &cc_parser_decllocal))
			{
				cc_symbol_exitscope();
				return FALSE;
			}
		} /* end locals */
		else if (cc_parser_is_stmtspecifier(ccctx->_currtk._type)
			|| ccctx->_currtk._type == TK_LBRACE
			|| ccctx->_currtk._type == TK_SEMICOLON)
		{
			if (!cc_stmt_statement(ccctx, list, loop, swtch)) {
				return FALSE;
			}
		} /* end statement */
		else
		{
			break;
		}
	}

	cc_ir_codelist_append(list, cc_ir_newcode_blk(FALSE, gCurrentLevel, CC_MM_TEMPPOOL));
	cc_symbol_exitscope();
	if (!cc_parser_expect(ccctx, TK_RBRACE)) { /* '}' */
		return FALSE;
	}

	return TRUE;
}

BOOL cc_stmt_label(struct tagCCContext* ccctx,  struct tagCCIRCodeList* list, struct tagCCLoopContext* loop, struct tagCCSwitchContext* swtch)
{
	FCCSymbol* p;
	const char* id;

	assert(ccctx->_currtk._type == TK_ID);
	id = ccctx->_currtk._val._astr._str;
	p = cc_symbol_lookup(id, gLabels);
	if (!p) {
		p = cc_symbol_label(id, &(ccctx->_currtk._loc), CC_MM_TEMPPOOL);
	}
	if (p->_defined) {
		logger_output_s("error: redefinition of label '%s' at %w, previously defined at %w.\n", id, &ccctx->_currtk._loc, &p->_loc);
		return FALSE;
	}
	p->_defined = 1;
	
	cc_read_token(ccctx, &ccctx->_currtk);
	if (!cc_parser_expect(ccctx, TK_COLON)) { /* ':' */
		return FALSE;
	}

	if (!cc_stmt_statement(ccctx, list, loop, swtch)) {
		logger_output_s("error: expect a statement after label '%s' at %w\n", p->_name, &p->_loc);
		return FALSE;
	}

	return TRUE;
}

BOOL cc_stmt_case(struct tagCCContext* ccctx,  struct tagCCIRCodeList* list, struct tagCCLoopContext* loop, struct tagCCSwitchContext* swtch)
{
	FCCSwitchCase swcase;
	FCCSymbol* p;
	FLocation loc;
	int constval;

	loc = ccctx->_currtk._loc;
	if (!cc_parser_expect(ccctx, TK_CASE)) { /* 'case' */
		return FALSE;
	}

	if (!swtch) {
		logger_output_s("error: 'case' should in a switch statement at %w.\n", &loc);
		return FALSE;
	}

	if (!cc_expr_parse_constant_int(ccctx, &constval))
	{
		logger_output_s("error: case label must be a constant value at %w.\n", &loc);
		return FALSE;
	}

	if ((p = cc_find_case(swtch, constval))) {
		logger_output_s("error: case value at %w is same with previous at %w.\n", &loc, &p->_loc);
		return FALSE;
	}

	if (!cc_parser_expect(ccctx, TK_COLON)) { /* ':' */
		return FALSE;
	}

	/* add label code */
	p = cc_symbol_label(NULL, &loc, CC_MM_TEMPPOOL);
	
	swcase._label = p;
	swcase._value = constval;
	array_append(&swtch->_cases, &swcase);
	cc_ir_codelist_append(list, cc_ir_newcode_label(p, CC_MM_TEMPPOOL));

	if (!cc_stmt_statement(ccctx, list, loop, swtch)) {
		logger_output_s("error: expect a statement after 'case' at %w\n", &loc);
		return FALSE;
	}

	return TRUE;
}

BOOL cc_stmt_default(struct tagCCContext* ccctx,  struct tagCCIRCodeList* list, struct tagCCLoopContext* loop, struct tagCCSwitchContext* swtch)
{
	FCCSymbol* p;
	FLocation loc;

	loc = ccctx->_currtk._loc;
	if (!cc_parser_expect(ccctx, TK_DEFAULT)) { /* 'default' */
		return FALSE;
	}

	if (!swtch) {
		logger_output_s("error: 'default' should in a switch statement at %w.\n", &loc);
		return FALSE;
	}

	if (swtch->_default) {
		logger_output_s("error: case value at %w is same with previous at %w.\n", &loc, &swtch->_default->_loc);
		return FALSE;
	}

	if (!cc_parser_expect(ccctx, TK_COLON)) { /* ':' */
		return FALSE;
	}

	/* add label code */
	p = cc_symbol_label(NULL, &loc, CC_MM_TEMPPOOL);

	swtch->_default = p;
	cc_ir_codelist_append(list, cc_ir_newcode_label(p, CC_MM_TEMPPOOL));

	if (!cc_stmt_statement(ccctx, list, loop, swtch)) {
		logger_output_s("error: expect a statement after 'default' at %w\n", &loc);
		return FALSE;
	}

	return TRUE;
}

BOOL cc_stmt_expression(struct tagCCContext* ccctx,  struct tagCCIRCodeList* list, struct tagCCLoopContext* loop, struct tagCCSwitchContext* swtch)
{
	FCCIRTree* expr;

	if (!cc_expr_parse_expression(ccctx, &expr, CC_MM_TEMPPOOL)) {
		return FALSE;
	}

	if (!cc_parser_expect(ccctx, TK_SEMICOLON)) { /* ';' */
		return FALSE;
	}

	cc_ir_codelist_append(list, cc_ir_newcode_expr(expr, CC_MM_TEMPPOOL));
	return TRUE;
}

BOOL cc_stmt_ifelse(struct tagCCContext* ccctx,  struct tagCCIRCodeList* list, struct tagCCLoopContext* loop, struct tagCCSwitchContext* swtch)
{
	FCCIRTree* cond;
	FCCSymbol* ifbody, *elbody, *ifelend;

	if (!cc_parser_expect(ccctx, TK_IF)) { /* if */
		return FALSE;
	}

	if (!cc_parser_expect(ccctx, TK_LPAREN)) { /* '(' */
		return FALSE;
	}

	if (!cc_expr_parse_expression(ccctx, &cond, CC_MM_TEMPPOOL)) {
		return FALSE;
	}
	if (!(cond = cc_expr_bool(gbuiltintypes._sinttype, cond, NULL, CC_MM_TEMPPOOL))) {
		return FALSE;
	}

	if (!cc_parser_expect(ccctx, TK_RPAREN)) { /* ')' */
		return FALSE;
	}

	/*  if (cond)
	 *  _if_body:
	 *			....
	 *  _el_body:
	 *			....
	 *
	 *  _end:
	 */

	ifbody = cc_symbol_label(NULL, &(ccctx->_currtk._loc), CC_MM_TEMPPOOL);
	elbody = cc_symbol_label(NULL, &(ccctx->_currtk._loc), CC_MM_TEMPPOOL);
	ifelend = cc_symbol_label(NULL, &(ccctx->_currtk._loc), CC_MM_TEMPPOOL);

	cc_ir_codelist_append(list, cc_ir_newcode_jump(cond, ifbody, elbody, CC_MM_TEMPPOOL));
	cc_ir_codelist_append(list, cc_ir_newcode_label(ifbody, CC_MM_TEMPPOOL));
	if (!cc_stmt_statement(ccctx, list, loop, swtch)) {
		return FALSE;
	}
	cc_ir_codelist_append(list, cc_ir_newcode_jump(NULL, ifelend, NULL, CC_MM_TEMPPOOL));

	elbody->_loc = ccctx->_currtk._loc;
	cc_ir_codelist_append(list, cc_ir_newcode_label(elbody, CC_MM_TEMPPOOL));
	if (ccctx->_currtk._type == TK_ELSE) { /* 'else' */
		cc_read_token(ccctx, &ccctx->_currtk);
		if (!cc_stmt_statement(ccctx, list, loop, swtch)) {
			return FALSE;
		}
	}

	/* ending of if-else */
	ifelend->_loc = ccctx->_currtk._loc;
	cc_ir_codelist_append(list, cc_ir_newcode_label(ifelend, CC_MM_TEMPPOOL));

	return TRUE;
}

BOOL cc_stmt_switch(struct tagCCContext* ccctx,  struct tagCCIRCodeList* list, struct tagCCLoopContext* loop, struct tagCCSwitchContext* swtch)
{
	struct tagCCSwitchContext thisswtch;
	FCCIRTree* intexpr;
	FCCSymbol* swend;
	FCCIRCode* iafter, *cjmp, *cflab;
	int n, tycode;

	if (!cc_parser_expect(ccctx, TK_SWITCH)) { /* switch */
		return FALSE;
	}

	if (!cc_parser_expect(ccctx, TK_LPAREN)) { /* '(' */
		return FALSE;
	}

	if (!cc_expr_parse_expression(ccctx, &intexpr, CC_MM_TEMPPOOL)) {
		return FALSE;
	}

	if (!IsInt(intexpr->_ty)) {
		logger_output_s("error: integer is expected for switch(expr) at %w\n", &intexpr->_loc);
		return FALSE;
	}

	if (!cc_parser_expect(ccctx, TK_RPAREN)) { /* ')' */
		return FALSE;
	}

	if (!(intexpr = cc_expr_cast(gbuiltintypes._sinttype, intexpr, NULL, CC_MM_TEMPPOOL))) {
		return FALSE;
	}

	/*
	 *   cjmp _case_0
	 *   cjmp _case_1
	 *   cjmp _case_2
	 *   jmp  _default
	 *
	 * _case_0:
	 *       .....
	 * _case_1:
	 *       .....
	 * _case_2:
	 *       .....
	 * _default:
	 *       .....
	 * _swend:
	 * 
	 */

	swend = cc_symbol_label(NULL, &(ccctx->_currtk._loc), CC_MM_TEMPPOOL);

	thisswtch._level = 0;
	thisswtch._lab_exit = swend;
	thisswtch._default = NULL;
	array_init(&thisswtch._cases, 32, sizeof(struct tagCCSwitchCase), CC_MM_TEMPPOOL);
	if (loop) { thisswtch._level = loop->_level; }
	if (swtch) { thisswtch._level = MAX(thisswtch._level, swtch->_level); }
	thisswtch._level++;

	iafter = list->_tail;
	if (!cc_stmt_statement(ccctx, list, loop, &thisswtch)) {
		return FALSE;
	}

	/* insert compare codes. */
	tycode = cc_ir_typecode(gbuiltintypes._sinttype);
	for (n = 0; n < thisswtch._cases._elecount; n++)
	{
		FCCIRTree* cmpexpr, *rhs;
		FCCSwitchCase* swcase;
		FCCSymbol* flab;

		swcase = array_element(&thisswtch._cases, n);
		rhs = cc_expr_constant(gbuiltintypes._sinttype, tycode, NULL, CC_MM_TEMPPOOL, swcase->_value);
		cmpexpr = cc_expr_equal(gbuiltintypes._sinttype, intexpr, rhs, NULL, CC_MM_TEMPPOOL);
		
		flab = cc_symbol_label(NULL, NULL, CC_MM_TEMPPOOL);

		cjmp = cc_ir_newcode_jump(cmpexpr, swcase->_label, flab, CC_MM_TEMPPOOL);
		cc_ir_codelist_insert_after(list, iafter, cjmp);
		cflab = cc_ir_newcode_label(flab, CC_MM_TEMPPOOL);
		cc_ir_codelist_insert_after(list, cjmp, cflab);
		iafter = cflab;
	} /* end for */

	if (thisswtch._default) {
		cjmp = cc_ir_newcode_jump(NULL, thisswtch._default, NULL, CC_MM_TEMPPOOL);
		cc_ir_codelist_insert_after(list, iafter, cjmp);
		iafter = cjmp;
	}
	else {
		cjmp = cc_ir_newcode_jump(NULL, swend, NULL, CC_MM_TEMPPOOL);
		cc_ir_codelist_insert_after(list, iafter, cjmp);
		iafter = cjmp;
	}

/* label of switch ending */
	swend->_loc = ccctx->_currtk._loc;
	cc_ir_codelist_append(list, cc_ir_newcode_label(swend, CC_MM_TEMPPOOL));

	return TRUE;
}

BOOL cc_stmt_while(struct tagCCContext* ccctx,  struct tagCCIRCodeList* list, struct tagCCLoopContext* loop, struct tagCCSwitchContext* swtch)
{
	struct tagCCLoopContext thisloop;
	FLocation loc;
	FCCIRTree* cond;
	FCCSymbol* test, *body, *end;

	loc = ccctx->_currtk._loc;
	if (!cc_parser_expect(ccctx, TK_WHILE)) { /* 'while' */
		return FALSE;
	}
	if (!cc_parser_expect(ccctx, TK_LPAREN)) { /* '(' */
		return FALSE;
	}

	if (!cc_expr_parse_expression(ccctx, &cond, CC_MM_TEMPPOOL)) {
		return FALSE;
	}
	if (!(cond = cc_expr_bool(gbuiltintypes._sinttype, cond, NULL, CC_MM_TEMPPOOL))) {
		return FALSE;
	}

	if (!cc_parser_expect(ccctx, TK_RPAREN)) { /* ')' */
		return FALSE;
	}

	/*    
	 *  _label_test:
	 *			while (cond)
	 *  _label_body:
	 *			{
	 *				....
	 *			}
	 *  _label_e:
	 */

	test = cc_symbol_label(NULL, &loc, CC_MM_TEMPPOOL);
	body = cc_symbol_label(NULL, &loc, CC_MM_TEMPPOOL);
	end = cc_symbol_label(NULL, &loc, CC_MM_TEMPPOOL);

/* label cond testing */
	cc_ir_codelist_append(list, cc_ir_newcode_label(test, CC_MM_TEMPPOOL));
	cc_ir_codelist_append(list, cc_ir_newcode_jump(cond, body, end, CC_MM_TEMPPOOL));
	cc_ir_codelist_append(list, cc_ir_newcode_label(body, CC_MM_TEMPPOOL));

	thisloop._level = 0;
	thisloop._lab_test = test;
	thisloop._lab_cont = test;
	thisloop._lab_exit = end;
	if (loop) { thisloop._level = loop->_level; }
	if (swtch) { thisloop._level = MAX(thisloop._level, swtch->_level); }
	thisloop._level++;

	if (!cc_stmt_statement(ccctx, list, &thisloop, swtch)) {
		return FALSE;
	}
	cc_ir_codelist_append(list, cc_ir_newcode_jump(NULL, test, NULL, CC_MM_TEMPPOOL));

/* label of ending while */
	end->_loc = ccctx->_currtk._loc;
	cc_ir_codelist_append(list, cc_ir_newcode_label(end, CC_MM_TEMPPOOL));

	return TRUE;
}

BOOL cc_stmt_dowhile(struct tagCCContext* ccctx,  struct tagCCIRCodeList* list, struct tagCCLoopContext* loop, struct tagCCSwitchContext* swtch)
{
	struct tagCCLoopContext thisloop;
	FCCIRTree* cond;
	FCCSymbol* test, * body, * end;

	if (!cc_parser_expect(ccctx, TK_DO)) {
		return FALSE;
	}

	/*
	*		do {
	*  _label_body:
	*			....
	*		} 
	*  _label_test:
	*		while (cond);
	*  _label_end:
	*/

	test = cc_symbol_label(NULL, &(ccctx->_currtk._loc), CC_MM_TEMPPOOL);
	body = cc_symbol_label(NULL, &(ccctx->_currtk._loc), CC_MM_TEMPPOOL);
	end = cc_symbol_label(NULL, &(ccctx->_currtk._loc), CC_MM_TEMPPOOL);

	/* label body */
	cc_ir_codelist_append(list, cc_ir_newcode_label(body, CC_MM_TEMPPOOL));

	thisloop._level = 0;
	thisloop._lab_test = test;
	thisloop._lab_cont = test;
	thisloop._lab_exit = end;
	if (loop) { thisloop._level = loop->_level; }
	if (swtch) { thisloop._level = MAX(thisloop._level, swtch->_level); }
	thisloop._level++;

	if (!cc_stmt_statement(ccctx, list, &thisloop, swtch)) {
		return FALSE;
	}

	if (!cc_parser_expect(ccctx, TK_WHILE)) { /* 'while' */
		return FALSE;
	}
	if (!cc_parser_expect(ccctx, TK_LPAREN)) { /* '(' */
		return FALSE;
	}
	if (!cc_expr_parse_expression(ccctx, &cond, CC_MM_TEMPPOOL)) {
		return FALSE;
	}
	if (!(cond = cc_expr_bool(gbuiltintypes._sinttype, cond, NULL, CC_MM_TEMPPOOL))) {
		return FALSE;
	}
	if (!cc_parser_expect(ccctx, TK_RPAREN)) { /* ')' */
		return FALSE;
	}
	if (!cc_parser_expect(ccctx, TK_SEMICOLON)) { /* ';' */
		return FALSE;
	}

	/* label test */
	cc_ir_codelist_append(list, cc_ir_newcode_label(test, CC_MM_TEMPPOOL));
	cc_ir_codelist_append(list, cc_ir_newcode_jump(cond, body, end, CC_MM_TEMPPOOL));
	/* label end */
	cc_ir_codelist_append(list, cc_ir_newcode_label(end, CC_MM_TEMPPOOL));

	return TRUE;
}

BOOL cc_stmt_for(struct tagCCContext* ccctx,  struct tagCCIRCodeList* list, struct tagCCLoopContext* loop, struct tagCCSwitchContext* swtch)
{
	struct tagCCLoopContext thisloop;
	FCCIRTree* expr0, * cond, * expr2;
	FCCSymbol* test, * body, *cont, *end;

	if (!cc_parser_expect(ccctx, TK_FOR)) {
		return FALSE;
	}
	if (!cc_parser_expect(ccctx, TK_LPAREN)) { /* '(' */
		return FALSE;
	}

	if (ccctx->_currtk._type == TK_SEMICOLON) {
		expr0 = NULL;
	}
	else if (!cc_expr_parse_expression(ccctx, &expr0, CC_MM_TEMPPOOL)) {
		return FALSE;
	}

	if (!cc_parser_expect(ccctx, TK_SEMICOLON)) { /* ';' */
		return FALSE;
	}

	if (ccctx->_currtk._type == TK_SEMICOLON) {
		cond = cc_expr_constant(gbuiltintypes._sinttype, cc_ir_typecode(gbuiltintypes._sinttype), &ccctx->_currtk._loc, CC_MM_TEMPPOOL, 1);
	}
	else if (!cc_expr_parse_expression(ccctx, &cond, CC_MM_TEMPPOOL)) {
		return FALSE;
	}

	if (!(cond = cc_expr_bool(gbuiltintypes._sinttype, cond, NULL, CC_MM_TEMPPOOL))) {
		return FALSE;
	}

	if (!cc_parser_expect(ccctx, TK_SEMICOLON)) { /* ';' */
		return FALSE;
	}

	if (ccctx->_currtk._type == TK_RPAREN) {
		expr2 = NULL;
	}
	else if (!cc_expr_parse_expression(ccctx, &expr2, CC_MM_TEMPPOOL)) {
		return FALSE;
	}
	if (!cc_parser_expect(ccctx, TK_RPAREN)) { /* ')' */
		return FALSE;
	}

	test = cc_symbol_label(NULL, &(ccctx->_currtk._loc), CC_MM_TEMPPOOL);
	cont = cc_symbol_label(NULL, &(ccctx->_currtk._loc), CC_MM_TEMPPOOL);
	body = cc_symbol_label(NULL, &(ccctx->_currtk._loc), CC_MM_TEMPPOOL);
	end = cc_symbol_label(NULL, &(ccctx->_currtk._loc), CC_MM_TEMPPOOL);

	/*  for (expr0; cond; expr2)
	 *		...
	 *=>
	 * 
	 *		expr0;
	 *  _label_test:
	 *		cond
	 *  _label_body:
	 *		...
	 *  _label_cont:
	 *		expr2;
	 *		jmp test;
	 *  _label_end:
	 */
	if (expr0) {
		cc_ir_codelist_append(list, cc_ir_newcode_expr(expr0, CC_MM_TEMPPOOL));
	}
	/* label test */
	cc_ir_codelist_append(list, cc_ir_newcode_label(test, CC_MM_TEMPPOOL));
	cc_ir_codelist_append(list, cc_ir_newcode_jump(cond, body, end, CC_MM_TEMPPOOL));

	thisloop._level = 0;
	thisloop._lab_test = test;
	thisloop._lab_cont = cont;
	thisloop._lab_exit = end;
	if (loop) { thisloop._level = loop->_level; }
	if (swtch) { thisloop._level = MAX(thisloop._level, swtch->_level); }
	thisloop._level++;

	/* label body */
	cc_ir_codelist_append(list, cc_ir_newcode_label(body, CC_MM_TEMPPOOL));
	if (!cc_stmt_statement(ccctx, list, &thisloop, swtch)) {
		return FALSE;
	}

	/* label continue */
	cc_ir_codelist_append(list, cc_ir_newcode_label(cont, CC_MM_TEMPPOOL));
	if (expr2) {
		cc_ir_codelist_append(list, cc_ir_newcode_expr(expr2, CC_MM_TEMPPOOL));
	}
	cc_ir_codelist_append(list, cc_ir_newcode_jump(NULL, test, NULL, CC_MM_TEMPPOOL));
	/* label end */
	end->_loc = ccctx->_currtk._loc;
	cc_ir_codelist_append(list, cc_ir_newcode_label(end, CC_MM_TEMPPOOL));
	
	return TRUE;
}

BOOL cc_stmt_goto(struct tagCCContext* ccctx,  struct tagCCIRCodeList* list, struct tagCCLoopContext* loop, struct tagCCSwitchContext* swtch)
{
	FCCSymbol* p;
	const char* id;

	if (!cc_parser_expect(ccctx, TK_GOTO)) {
		return FALSE;
	}
	if (ccctx->_currtk._type != TK_ID) {
		logger_output_s("error: illegal goto syntax, expect a label at %w\n", &ccctx->_currtk._loc);
		return FALSE;
	}
	
	id = ccctx->_currtk._val._astr._str;
	p = cc_symbol_lookup(id, gLabels);
	if (!p) {
		p = cc_symbol_install(id, &gLabels, SCOPE_LABEL, CC_MM_TEMPPOOL);
		p->_loc = ccctx->_currtk._loc;
	}
	
	cc_read_token(ccctx, &ccctx->_currtk);
	if (!cc_parser_expect(ccctx, TK_SEMICOLON)) { /* ';' */
		return FALSE;
	}

	/* jump label */
	cc_ir_codelist_append(list, cc_ir_newcode_jump(NULL, p, NULL, CC_MM_TEMPPOOL));
	return TRUE;
}

BOOL cc_stmt_continue(struct tagCCContext* ccctx,  struct tagCCIRCodeList* list, struct tagCCLoopContext* loop, struct tagCCSwitchContext* swtch)
{
	if (!loop) {
		logger_output_s("error: 'continue' should in a iteration statement, at %w.\n", &ccctx->_currtk._loc);
		return FALSE;
	}

	if (!cc_parser_expect(ccctx, TK_CONTINUE)) { /* 'continue' */
		return FALSE;
	}
	if (!cc_parser_expect(ccctx, TK_SEMICOLON)) { /* ';' */
		return FALSE;
	}

	assert(loop->_lab_cont);
	/* jump label */
	cc_ir_codelist_append(list, cc_ir_newcode_jump(NULL, loop->_lab_cont, NULL, CC_MM_TEMPPOOL));

	return TRUE;
}

BOOL cc_stmt_break(struct tagCCContext* ccctx,  struct tagCCIRCodeList* list, struct tagCCLoopContext* loop, struct tagCCSwitchContext* swtch)
{
	FCCSymbol* target = NULL;

	if (!loop && !swtch) {
		logger_output_s("error: 'break' should in a iteration or switch statement, at %w.\n", &ccctx->_currtk._loc);
		return FALSE;
	}
	if (!cc_parser_expect(ccctx, TK_BREAK)) { /* 'break' */
		return FALSE;
	}
	if (!cc_parser_expect(ccctx, TK_SEMICOLON)) { /* ';' */
		return FALSE;
	}

	/* check the nearest loop or switch */
	if (loop) {
		target = loop->_lab_exit;
	}
	if (swtch) {
		if (!loop || loop->_level <= swtch->_level)
		{
			assert(!loop || loop->_level != swtch->_level);
			target = swtch->_lab_exit;
		}
	}

	/* jump label */
	cc_ir_codelist_append(list, cc_ir_newcode_jump(NULL, target, NULL, CC_MM_TEMPPOOL));
	return TRUE;
}

BOOL cc_stmt_return(struct tagCCContext* ccctx,  struct tagCCIRCodeList* list, struct tagCCLoopContext* loop, struct tagCCSwitchContext* swtch)
{
	FLocation loc;
	FCCIRTree* expr;
	FCCType* rty;

	loc = ccctx->_currtk._loc;
	if (!cc_parser_expect(ccctx, TK_RETURN)) {
		return FALSE;
	}
	if (ccctx->_currtk._type == TK_SEMICOLON) { /* ';' */
		expr = NULL;
	}
	else if(!cc_expr_parse_expression(ccctx, &expr, CC_MM_TEMPPOOL)) {
		return FALSE;
	}

	if (!(expr = cc_expr_value(expr, CC_MM_TEMPPOOL))) {
		return FALSE;
	}

	if (!cc_parser_expect(ccctx, TK_SEMICOLON)) { /* ';' */
		return FALSE;
	}

	rty = ccctx->_function->_type->_type;
	if (IsVoid(rty)) {
		if (expr && !IsVoid(expr->_ty)) {
			logger_output_s("error: return type is invalid at %w, type %t is expected for function '%s'.\n", &loc, rty, ccctx->_function->_name);
			return FALSE;
		}
	}
	else {
		FCCType* ty;

		if (!expr || !(ty = cc_expr_assigntype(rty, expr))) {
			logger_output_s("error: return type is invalid at %w, type %t is expected for function '%s'.\n", &loc, rty, ccctx->_function->_name);
			return FALSE;
		}

		if (!(expr = cc_expr_cast(ty, expr, NULL, CC_MM_TEMPPOOL))) {
			logger_output_s("error: cast failed at %s:%d.\n", __FILE__, __LINE__);
			return FALSE;
		}
	}

	/* jump label */
	cc_ir_codelist_append(list, cc_ir_newcode_ret(expr, CC_MM_TEMPPOOL));
	return TRUE;
}
