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
#include "ir/ir.h"


typedef struct tagCCLoopContext {
	int _level;
	struct tagCCSymbol* _lab_test, *_lab_cont, *_lab_exit;
} FCCLoopContext;

typedef struct tagCCSwitchCase {
	struct tagCCSymbol* _label;
	int	_value;
} FCCSwitchCase;

typedef struct tagCCSwitchContext {
	int _level;
	struct tagCCSymbol* _lab_exit;
	struct tagCCSymbol* _default;
	FArray _cases; /* array of FCCSwitchCase */
} FCCSwitchContext;


BOOL cc_stmt_statement(struct tagCCContext* ccctx,  struct tagCCIRCodeList* list, struct tagCCLoopContext* loop, struct tagCCSwitchContext* swtch);
BOOL cc_stmt_compound(struct tagCCContext* ccctx,  struct tagCCIRCodeList* list, struct tagCCLoopContext* loop, struct tagCCSwitchContext* swtch);
BOOL cc_stmt_label(struct tagCCContext* ccctx,  struct tagCCIRCodeList* list, struct tagCCLoopContext* loop, struct tagCCSwitchContext* swtch);
BOOL cc_stmt_case(struct tagCCContext* ccctx,  struct tagCCIRCodeList* list, struct tagCCLoopContext* loop, struct tagCCSwitchContext* swtch);
BOOL cc_stmt_default(struct tagCCContext* ccctx,  struct tagCCIRCodeList* list, struct tagCCLoopContext* loop, struct tagCCSwitchContext* swtch);
BOOL cc_stmt_expression(struct tagCCContext* ccctx,  struct tagCCIRCodeList* list, struct tagCCLoopContext* loop, struct tagCCSwitchContext* swtch);
BOOL cc_stmt_ifelse(struct tagCCContext* ccctx,  struct tagCCIRCodeList* list, struct tagCCLoopContext* loop, struct tagCCSwitchContext* swtch);
BOOL cc_stmt_switch(struct tagCCContext* ccctx,  struct tagCCIRCodeList* list, struct tagCCLoopContext* loop, struct tagCCSwitchContext* swtch);
BOOL cc_stmt_while(struct tagCCContext* ccctx,  struct tagCCIRCodeList* list, struct tagCCLoopContext* loop, struct tagCCSwitchContext* swtch);
BOOL cc_stmt_dowhile(struct tagCCContext* ccctx,  struct tagCCIRCodeList* list, struct tagCCLoopContext* loop, struct tagCCSwitchContext* swtch);
BOOL cc_stmt_for(struct tagCCContext* ccctx,  struct tagCCIRCodeList* list, struct tagCCLoopContext* loop, struct tagCCSwitchContext* swtch);
BOOL cc_stmt_goto(struct tagCCContext* ccctx,  struct tagCCIRCodeList* list, struct tagCCLoopContext* loop, struct tagCCSwitchContext* swtch);
BOOL cc_stmt_continue(struct tagCCContext* ccctx,  struct tagCCIRCodeList* list, struct tagCCLoopContext* loop, struct tagCCSwitchContext* swtch);
BOOL cc_stmt_break(struct tagCCContext* ccctx,  struct tagCCIRCodeList* list, struct tagCCLoopContext* loop, struct tagCCSwitchContext* swtch);
BOOL cc_stmt_return(struct tagCCContext* ccctx,  struct tagCCIRCodeList* list, struct tagCCLoopContext* loop, struct tagCCSwitchContext* swtch);


#endif /* __CC_STMT_H__ */
