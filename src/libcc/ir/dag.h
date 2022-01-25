/* \brief
 *		DAG operation for expressions. used for Extracting common expressions.
 */

#ifndef __CC_DAG_H__
#define __CC_CANON_H__

#include "ir.h"

/* new a DAG node (not in the shared pool) */
FCCIRDagNode* cc_dag_newnode(unsigned int op, FCCIRDagNode* lhs, FCCIRDagNode* rhs, FCCSymbol* sym, enum EMMArea where);
FCCIRDagNode* cc_dag_newnode_inpool(unsigned int op, FCCIRDagNode* lhs, FCCIRDagNode* rhs, FCCSymbol* sym, enum EMMArea where);
void cc_dag_rmnodes_inpool(FCCSymbol* p);
void cc_dag_reset_pool();

/* generate DAG nodes for basic blocks */
BOOL cc_dag_translate_basicblocks(FCCIRBasicBlock* first, enum EMMArea where);
BOOL cc_dag_translate_codelist(FCCIRCodeList* list, enum EMMArea where);

#endif /* __CC_CANON_H__ */
