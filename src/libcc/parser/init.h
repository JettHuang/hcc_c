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
		struct tagVarInitializer** _kids; /* end with null */
	} _u;
} FVarInitializer;


FVarInitializer* cc_varinit_new();
BOOL cc_parser_initializer(struct tagCCContext* ctx, FVarInitializer** outinit);


#endif /* __CC_INITIALIZER_H__ */
