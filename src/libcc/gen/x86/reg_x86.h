/* \brief
 *		register alloc.
 */

#ifndef __CC_REG_X86_H__
#define __CC_REG_X86_H__

#include "utils.h"
#include "mm.h"


#define REG_BIT(r)	(0x01 << (r))

/* x86 generic registers */
#define X86_NIL		0x00
#define X86_EAX		0x01
#define X86_EBX		0x02
#define X86_ECX		0x03
#define X86_EDX		0x04
#define X86_ESI		0x05
#define X86_EDI		0x06
#define X86_EBP		0x07
#define X86_ESP		0x08
#define X86_ST0		0x10
#define X86_ST1		0x11
#define X86_MAX		0x12

#define NORMAL_X86REGS	(REG_BIT(X86_EAX) | REG_BIT(X86_EBX) | REG_BIT(X86_ECX) | REG_BIT(X86_EDX))
#define NORMAL_ADDR_X86REGS	(NORMAL_X86REGS | REG_BIT(X86_ESI) | REG_BIT(X86_EDI))

/* reset registers */
void cc_reg_reset();

/* get register for dag */
int cc_reg_alloc(struct tagCCGenCodeContext* ctx, int currseqid, int regflags);

/* free register */
BOOL cc_reg_free(struct tagCCGenCodeContext* ctx, int regid, int curseqid);

/* discard register */
void cc_reg_discard(int regid);

/* spill data to memory */
BOOL cc_reg_spill(struct tagCCGenCodeContext* ctx, struct tagCCDagNode* dag, int curseqid);

/* associate dag with register */
BOOL cc_reg_make_associated(int regid, struct tagCCDagNode* dag, int part);
BOOL cc_reg_make_unassociated(int regid, struct tagCCDagNode* dag, int part);

/* mark register used flags */
void cc_reg_markused(int regid);
void cc_reg_unmarkused(int regid);

const char* cc_reg_name(int regid, int size);

#endif /* __CC_REG_X86_H__ */
