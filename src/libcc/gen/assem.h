/* \brief
 *		abstract assembly code.
 *
 */

#ifndef __CC_ASSEM_H__
#define __CC_ASSEM_H__

#include "ir/dag.h"


enum ASOperandFormat 
{
	FormatReg,
	FormatImm,
	FormatSIB,
	FormatInSIB
};

typedef struct tagCCASOperand {
	int _format;
	int _tycode; /* operand type code */
	union {
		short _regs[2]; /* NOTE: for 64bits regs[1]:regs[0] */
		struct tagCCSymbol* _imm; /* immediate constant */
		struct {
			struct tagCCSymbol* _displacement;
			int _displacement2;

			short _basereg;
			short _indexreg;
			short _scale; /* 1, 2, 4, 8 */
		} _sib;
	} _u;
} FCCASOperand;

typedef struct tagCCASCode {
	unsigned int _opcode;
	struct tagCCASOperand _dst;
	struct tagCCASOperand _src;
	struct tagCCSymbol*	  _target;
	int _count;

	struct tagCCASCode* _prev, * _next;
} FCCASCode;

typedef struct tagCCASCodeList {
	struct tagCCASCode* _head;
	struct tagCCASCode* _tail;
} FCCASCodeList;


FCCASCode* cc_as_newcode(enum EMMArea where);
void cc_as_codelist_append(FCCASCodeList* l, FCCASCode* c);
/* remove & return next item */
FCCASCode* cc_as_codelist_remove(FCCASCodeList* l, FCCASCode* c);

#endif /* __CC_ASSEM_H__ */
