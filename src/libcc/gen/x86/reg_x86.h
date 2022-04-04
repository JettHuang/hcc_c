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


/* reset registers */
void cc_reg_reset();

/* get register for dag */
BOOL cc_reg_get(struct tagCCGenCodeContext* ctx, int seqid, int requires, struct tagCCDagNode* dag, int part);

/* put register allocated for dag */
BOOL cc_reg_put(int regid);

#endif /* __CC_REG_X86_H__ */
