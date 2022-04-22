/* \brief
 *		parsing variable initializer
 * this version only support simple initializer : { 0, 1, { 20, 30 }}
 */

#ifndef __CC_INITIALIZER_H__
#define __CC_INITIALIZER_H__

#include "utils.h"
#include "mm.h"
#include "cc.h"


 /* variable initializer */
typedef struct tagVarInitializer {
	FLocation _loc;
	int _isblock : 1;
	union {
		struct tagCCExprTree* _expr;
		struct {
			struct tagVarInitializer** _kids;
			int _cnt;
		} _kids;
	} _u;
} FVarInitializer;


FVarInitializer* cc_varinit_new(enum EMMArea where);
BOOL cc_parser_initializer(struct tagCCContext* ctx, FVarInitializer** outinit, BOOL bExpectConstant, enum EMMArea where);
BOOL cc_varinit_check(struct tagCCContext* ctx, struct tagCCType* ty, FVarInitializer* init, BOOL bExpectConstant, enum EMMArea where);

/* generate initialization codes for local variable */
BOOL cc_gen_localvar_initcodes(struct tagCCIRCodeList* list, struct tagCCSymbol* p);

#endif /* __CC_INITIALIZER_H__ */
