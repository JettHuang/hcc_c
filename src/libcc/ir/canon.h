/* \brief
 *		canon.h  
 * 1. convert the ir-tree to code-list (linearized).
 * 2. convert code-list to basic-blocks. 
 */

#ifndef __CANON_H__
#define __CANON_H__

#include "ir.h"


BOOL cc_canon_expr_linearize(FCCIRCodeList* list, FCCIRTree* expr, FCCSymbol* tlab, FCCSymbol* flab, FCCIRTree** outexpr, enum EMMArea where);


#endif /* __CANON_H__ */

