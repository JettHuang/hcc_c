/* \brief
 *		abstract assembly code.
 *
 */

#ifndef __CC_ASSEM_H__
#define __CC_ASSEM_H__

#include "ir/dag.h"


enum ASOperandType /* bytes */
{
	S1,
	U1,
	S2,
	U2,
	S4,
	U4,
	S8,
	U8,
	F4,
	F8
};

enum ASAddressingMode 
{
	Reg,
	Mem,
	Imm
};

typedef struct tagCCASOperand {
	unsigned int _addrmode;
	union {
		struct tagCCDagNode* _reg;
		struct tagCCDagNode* _imm;
		struct {
			struct tagCCDagNode* _base;
			struct tagCCDagNode* _index;
			unsigned int _scale; /* 1, 2, 4, 8 */
			struct tagCCDagNode* _displacement;
		} _mem;
	} _u;
} FCCASOperand;

typedef struct tagCCASCode {
	unsigned int _opcode;
	struct tagCCASOperand _dst;
	struct tagCCASOperand _src;
	struct tagCCSymbol*	  _target;

	struct tagCCASCode* _prev, * _next;
} FCCASCode;

typedef struct tagCCASCodeList {
	struct tagCCASCode* _head;
	struct tagCCASCode* _tail;
} FCCASCodeList;


FCCASCode* cc_as_newcode(unsigned int op, enum EMMArea where);
void cc_as_codelist_append(FCCASCodeList* l, FCCASCode* c);
/* remove & return next item */
FCCASCode* cc_as_codelist_remove(FCCASCodeList* l, FCCASCode* c);

#endif /* __CC_ASSEM_H__ */
