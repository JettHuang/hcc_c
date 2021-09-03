/* \brief
 *	C compiler statements parsing.
 */

#ifndef __CC_STMT_H__
#define __CC_STMT_H__

#include "utils.h"
#include "mm.h"
#include "cc.h"
#include "symbols.h"
#include "expr.h"


/* statement types */
enum EStmtType
{
	ESTMT_Expr = 0,
	ESTMT_Compound,
	
	ESTMT_If,
	ESTMT_Switch,
	
	ESTMT_While,
	ESTMT_DoWhile,
	ESTMT_For,

	ESTMT_Label,
	ESTMT_Case,
	ESTMT_Default,

	ESTMT_Goto,
	ESTMT_Continue,
	ESTMT_Break,
	ESTMT_Return,

	ESTMT_Max
};


typedef struct tagCCStatement {
	enum EStmtType	_kind;
	FLocation _loc;
	struct tagCCStatement* _link;

	union
	{
		/* labeled statement */
		struct {
			struct tagCCSymbol* _id;
		} _label;

		struct {
			int _constval;
			struct tagCCSymbol* _id; /* target */
		} _case;

		struct {
			struct tagCCSymbol* _id; /* target */
		} _default;

		/* compound statement */
		struct {
			struct tagCCStatement* _first, *_last;
		} _compound;

		/* expression statement */
		struct {
			struct tagCCExprTree* _expr;
		} _expression;

		/* selection statement */
		struct {
			struct tagCCExprTree* _expr;
			struct tagCCStatement* _if;
			struct tagCCStatement* _else;
		} _ifelse;

		struct {
			struct tagCCExprTree* _expr;
			struct tagCCStatement* _stmt;
			FArray _cases;
		} _switch;

		/* iteration statement */
		struct {
			struct tagCCExprTree* _expr;
			struct tagCCStatement* _stmt;
		} _while, _dowhile;

		struct {
			struct tagCCExprTree* _expr0, *_expr1, *_expr2;
			struct tagCCStatement* _stmt;
		} _for;

		/* jump statement */
		struct {
			struct tagCCSymbol* _id; /* goto target */
		} _goto;

		struct {
			struct tagCCSymbol* _id; /* continue target */
		} _continue;

		struct {
			struct tagCCSymbol* _id; /* break target */
		} _break;

		struct {
			struct tagCCExprTree* _expr;
			struct tagCCSymbol* _id; /* jump target */
		} _return;

	} _u;
} FCCStatement;


/* context of statement parsing */
typedef struct tagCCStmtContext {
	struct tagCCStatement* _switch;
	struct tagCCStatement* _loop;
} FCCStmtContext;


FCCStatement* cc_stmt_new(enum EStmtType k);
BOOL cc_stmt_statement(struct tagCCContext* ccctx, struct tagCCStmtContext* stmtctx, struct tagCCStatement** outstmt);
BOOL cc_stmt_compound(struct tagCCContext* ccctx, struct tagCCStmtContext* stmtctx, struct tagCCStatement** outstmt);
BOOL cc_stmt_label(struct tagCCContext* ccctx, struct tagCCStmtContext* stmtctx, struct tagCCStatement** outstmt);
BOOL cc_stmt_case(struct tagCCContext* ccctx, struct tagCCStmtContext* stmtctx, struct tagCCStatement** outstmt);
BOOL cc_stmt_default(struct tagCCContext* ccctx, struct tagCCStmtContext* stmtctx, struct tagCCStatement** outstmt);
BOOL cc_stmt_expression(struct tagCCContext* ccctx, struct tagCCStmtContext* stmtctx, struct tagCCStatement** outstmt);
BOOL cc_stmt_ifelse(struct tagCCContext* ccctx, struct tagCCStmtContext* stmtctx, struct tagCCStatement** outstmt);
BOOL cc_stmt_switch(struct tagCCContext* ccctx, struct tagCCStmtContext* stmtctx, struct tagCCStatement** outstmt);
BOOL cc_stmt_while(struct tagCCContext* ccctx, struct tagCCStmtContext* stmtctx, struct tagCCStatement** outstmt);
BOOL cc_stmt_dowhile(struct tagCCContext* ccctx, struct tagCCStmtContext* stmtctx, struct tagCCStatement** outstmt);
BOOL cc_stmt_for(struct tagCCContext* ccctx, struct tagCCStmtContext* stmtctx, struct tagCCStatement** outstmt);
BOOL cc_stmt_goto(struct tagCCContext* ccctx, struct tagCCStmtContext* stmtctx, struct tagCCStatement** outstmt);
BOOL cc_stmt_continue(struct tagCCContext* ccctx, struct tagCCStmtContext* stmtctx, struct tagCCStatement** outstmt);
BOOL cc_stmt_break(struct tagCCContext* ccctx, struct tagCCStmtContext* stmtctx, struct tagCCStatement** outstmt);
BOOL cc_stmt_return(struct tagCCContext* ccctx, struct tagCCStmtContext* stmtctx, struct tagCCStatement** outstmt);


#endif /* __CC_STMT_H__ */
