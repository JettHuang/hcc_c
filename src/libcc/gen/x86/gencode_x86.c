/*\brief
 *	generate x86 assemble codes.
 */

#include "utils.h"
#include "mm.h"
#include "cc.h"
#include "parser/symbols.h"
#include "parser/types.h"
#include "ir/ir.h"
#include "gen/assem.h"
#include "opcodes_x86.h"


#define ASM_INSTRUCT(op, str)	{ op,  str},

/* assemble code set */
struct FX86AsmCode {
	unsigned int _op;
	const char* _text;
} X86AsmCodes[] = 
{
#include "opcodes_x86.inl"
};


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

#define REQUIRES_BIT0()		(0)
#define REQUIRES_BIT1(b0)	(REQUIRES_BIT0() | (1 << b0))
#define REQUIRES_BIT2(b0, b1)	(REQUIRES_BIT1(b0) | (1 << b1))
#define REQUIRES_BIT3(b0, b1, b2)	(REQUIRES_BIT2(b0, b1) | (1 << b2))
#define REQUIRES_BIT4(b0, b1, b2, b3)	(REQUIRES_BIT3(b0, b1, b2) | (1 << b3))


typedef struct tagCCGenCodeContext {
	unsigned int _localspace; /* bytes */
} FCCGenCodeContext;

#define MAKE_LOOKTAG2(addr1, ty)	(((ty) << 16) | (addr1))
#define MAKE_LOOKTAG3(addr0, addr1, ty)	(((ty) << 16) | ((addr0) << 8) | (addr1))
#define MAKE_LOOKTAG4(cmp, addr0, addr1, ty)	( ((cmp) << 24) | ((ty) << 16) | ((addr0) << 8) | (addr1))

typedef struct tagCCASLookupEntry {
	int _tag;
	enum X86ASMCode _asop;
} FCCASLookupEntry;

static void emit(FCCASCodeList* asmlist, FCCASCode* asm);
static BOOL munch_expr(FCCGenCodeContext* ctx, FCCIRDagNode* dag,  FCCASCodeList* asmlist, int requires, struct tagCCASOperand* operand, enum EMMArea where);
static BOOL munch_stmt(FCCGenCodeContext* ctx, FCCIRCode* stmt, FCCASCodeList* asmlist, enum EMMArea where);

static void block_enter(FCCGenCodeContext* ctx, int level);
static void block_leave(FCCGenCodeContext* ctx, int level);
static void local_variable(FCCGenCodeContext* ctx, struct tagCCSymbol* id);

static enum X86ASMCode lookup_asmcode(const int tag, const FCCASLookupEntry* v, int count)
{
	while (--count >= 0)
	{
		if (v[count]._tag == tag)
		{
			return v[count]._asop;
		}
	} /* end while */
	
	assert(0);
	return X86_INS_INVALID;
}

static void block_enter(FCCGenCodeContext* ctx, int level)
{
	// TODO:
}
static void block_leave(FCCGenCodeContext* ctx, int level)
{
	// TODO:
}

static void local_variable(FCCGenCodeContext* ctx, struct tagCCSymbol* id)
{
	// TODO:
}

static void emit(FCCASCodeList* asmlist, FCCASCode* asm)
{
	cc_as_codelist_append(asmlist, asm);
}

static BOOL emit_argv(FCCASCodeList* asmlist, FCCASOperand* src, int bytescnt, enum EMMArea where)
{
	static const FCCASLookupEntry lookup[] =
	{
		{ MAKE_LOOKTAG2(FormatReg, IR_S32), X86_PUSH_I4R },
		{ MAKE_LOOKTAG2(FormatReg, IR_S64), X86_PUSH_I8R },
		{ MAKE_LOOKTAG2(FormatReg, IR_U32), X86_PUSH_U4R },
		{ MAKE_LOOKTAG2(FormatReg, IR_U64), X86_PUSH_U8R },
		{ MAKE_LOOKTAG2(FormatReg, IR_F32), X86_PUSH_F4R },
		{ MAKE_LOOKTAG2(FormatReg, IR_F64), X86_PUSH_F8R },
		{ MAKE_LOOKTAG2(FormatReg, IR_PTR), X86_PUSH_P4R },

		{ MAKE_LOOKTAG2(FormatInSIB, IR_S32), X86_PUSH_I4M },
		{ MAKE_LOOKTAG2(FormatInSIB, IR_S64), X86_PUSH_I8M },
		{ MAKE_LOOKTAG2(FormatInSIB, IR_U32), X86_PUSH_U4M },
		{ MAKE_LOOKTAG2(FormatInSIB, IR_U64), X86_PUSH_U8M },
		{ MAKE_LOOKTAG2(FormatInSIB, IR_F32), X86_PUSH_F4M },
		{ MAKE_LOOKTAG2(FormatInSIB, IR_F64), X86_PUSH_F8M },
		{ MAKE_LOOKTAG2(FormatInSIB, IR_PTR), X86_PUSH_P4M },
		{ MAKE_LOOKTAG2(FormatInSIB, IR_BLK), X86_PUSH_BM  },

		{ MAKE_LOOKTAG2(FormatImm, IR_S32), X86_PUSH_I4I },
		{ MAKE_LOOKTAG2(FormatImm, IR_S64), X86_PUSH_I8I },
		{ MAKE_LOOKTAG2(FormatImm, IR_U32), X86_PUSH_U4I },
		{ MAKE_LOOKTAG2(FormatImm, IR_U64), X86_PUSH_U8I },
		{ MAKE_LOOKTAG2(FormatImm, IR_PTR), X86_PUSH_P4I }
	};
	FCCASCode* asm;

	if (!(asm = cc_as_newcode(where))) {
		return FALSE;
	}

	asm->_opcode = lookup_asmcode(MAKE_LOOKTAG2(src->_format, src->_tycode), lookup, ELEMENTSCNT(lookup));
	if (asm->_opcode == X86_INS_INVALID) {
		return FALSE;
	}

	asm->_src = *src;
	asm->_count = bytescnt;
	emit(asmlist, asm);

	return TRUE;
}

static BOOL emit_jmp(FCCASCodeList* asmlist, FCCSymbol* target, enum EMMArea where)
{
	FCCASCode* asm;

	if (!(asm = cc_as_newcode(where))) {
		return FALSE;
	}

	asm->_opcode = X86_JMP;
	asm->_target = target;

	emit(asmlist, asm);

	return TRUE;
}

static BOOL emit_cjmp(FCCASCodeList* asmlist, int ircmp, FCCASOperand* dst, FCCASOperand* src, FCCSymbol* tlabel, FCCSymbol* flabel, enum EMMArea where)
{
	static const FCCASLookupEntry lookup[] =
	{
		{ MAKE_LOOKTAG4(IR_EQUAL, FormatReg, FormatReg, IR_S32), X86_JE_I4RR },
		{ MAKE_LOOKTAG4(IR_EQUAL, FormatReg, FormatInSIB, IR_S32), X86_JE_I4RM },
		{ MAKE_LOOKTAG4(IR_EQUAL, FormatReg, FormatImm, IR_S32), X86_JE_I4RI },
		{ MAKE_LOOKTAG4(IR_EQUAL, FormatReg, FormatReg, IR_U32), X86_JE_U4RR },
		{ MAKE_LOOKTAG4(IR_EQUAL, FormatReg, FormatInSIB, IR_U32), X86_JE_U4RM },
		{ MAKE_LOOKTAG4(IR_EQUAL, FormatReg, FormatImm, IR_U32), X86_JE_U4RI },
		{ MAKE_LOOKTAG4(IR_EQUAL, FormatInSIB, FormatInSIB, IR_F32), X86_JE_F4MM },
		{ MAKE_LOOKTAG4(IR_EQUAL, FormatInSIB, FormatInSIB, IR_F64), X86_JE_F8MM },

		{ MAKE_LOOKTAG4(IR_UNEQ, FormatReg, FormatReg, IR_S32), X86_JNE_I4RR },
		{ MAKE_LOOKTAG4(IR_UNEQ, FormatReg, FormatInSIB, IR_S32), X86_JNE_I4RM },
		{ MAKE_LOOKTAG4(IR_UNEQ, FormatReg, FormatImm, IR_S32), X86_JNE_I4RI },
		{ MAKE_LOOKTAG4(IR_UNEQ, FormatReg, FormatReg, IR_U32), X86_JNE_U4RR },
		{ MAKE_LOOKTAG4(IR_UNEQ, FormatReg, FormatInSIB, IR_U32), X86_JNE_U4RM },
		{ MAKE_LOOKTAG4(IR_UNEQ, FormatReg, FormatImm, IR_U32), X86_JNE_U4RI },
		{ MAKE_LOOKTAG4(IR_UNEQ, FormatInSIB, FormatInSIB, IR_F32), X86_JNE_F4MM },
		{ MAKE_LOOKTAG4(IR_UNEQ, FormatInSIB, FormatInSIB, IR_F64), X86_JNE_F8MM },

		{ MAKE_LOOKTAG4(IR_GREAT, FormatReg, FormatReg, IR_S32), X86_JG_I4RR },
		{ MAKE_LOOKTAG4(IR_GREAT, FormatReg, FormatInSIB, IR_S32), X86_JG_I4RM },
		{ MAKE_LOOKTAG4(IR_GREAT, FormatReg, FormatImm, IR_S32), X86_JG_I4RI },
		{ MAKE_LOOKTAG4(IR_GREAT, FormatReg, FormatReg, IR_U32), X86_JG_U4RR },
		{ MAKE_LOOKTAG4(IR_GREAT, FormatReg, FormatInSIB, IR_U32), X86_JG_U4RM },
		{ MAKE_LOOKTAG4(IR_GREAT, FormatReg, FormatImm, IR_U32), X86_JG_U4RI },
		{ MAKE_LOOKTAG4(IR_GREAT, FormatInSIB, FormatInSIB, IR_F32), X86_JG_F4MM },
		{ MAKE_LOOKTAG4(IR_GREAT, FormatInSIB, FormatInSIB, IR_F64), X86_JG_F8MM },

		{ MAKE_LOOKTAG4(IR_LESS, FormatReg, FormatReg, IR_S32), X86_JL_I4RR },
		{ MAKE_LOOKTAG4(IR_LESS, FormatReg, FormatInSIB, IR_S32), X86_JL_I4RM },
		{ MAKE_LOOKTAG4(IR_LESS, FormatReg, FormatImm, IR_S32), X86_JL_I4RI },
		{ MAKE_LOOKTAG4(IR_LESS, FormatReg, FormatReg, IR_U32), X86_JL_U4RR },
		{ MAKE_LOOKTAG4(IR_LESS, FormatReg, FormatInSIB, IR_U32), X86_JL_U4RM },
		{ MAKE_LOOKTAG4(IR_LESS, FormatReg, FormatImm, IR_U32), X86_JL_U4RI },
		{ MAKE_LOOKTAG4(IR_LESS, FormatInSIB, FormatInSIB, IR_F32), X86_JL_F4MM },
		{ MAKE_LOOKTAG4(IR_LESS, FormatInSIB, FormatInSIB, IR_F64), X86_JL_F8MM },

		{ MAKE_LOOKTAG4(IR_GEQ, FormatReg, FormatReg, IR_S32), X86_JGE_I4RR },
		{ MAKE_LOOKTAG4(IR_GEQ, FormatReg, FormatInSIB, IR_S32), X86_JGE_I4RM },
		{ MAKE_LOOKTAG4(IR_GEQ, FormatReg, FormatImm, IR_S32), X86_JGE_I4RI },
		{ MAKE_LOOKTAG4(IR_GEQ, FormatReg, FormatReg, IR_U32), X86_JGE_U4RR },
		{ MAKE_LOOKTAG4(IR_GEQ, FormatReg, FormatInSIB, IR_U32), X86_JGE_U4RM },
		{ MAKE_LOOKTAG4(IR_GEQ, FormatReg, FormatImm, IR_U32), X86_JGE_U4RI },
		{ MAKE_LOOKTAG4(IR_GEQ, FormatInSIB, FormatInSIB, IR_F32), X86_JGE_F4MM },
		{ MAKE_LOOKTAG4(IR_GEQ, FormatInSIB, FormatInSIB, IR_F64), X86_JGE_F8MM },

		{ MAKE_LOOKTAG4(IR_LEQ, FormatReg, FormatReg, IR_S32), X86_JLE_I4RR },
		{ MAKE_LOOKTAG4(IR_LEQ, FormatReg, FormatInSIB, IR_S32), X86_JLE_I4RM },
		{ MAKE_LOOKTAG4(IR_LEQ, FormatReg, FormatImm, IR_S32), X86_JLE_I4RI },
		{ MAKE_LOOKTAG4(IR_LEQ, FormatReg, FormatReg, IR_U32), X86_JLE_U4RR },
		{ MAKE_LOOKTAG4(IR_LEQ, FormatReg, FormatInSIB, IR_U32), X86_JLE_U4RM },
		{ MAKE_LOOKTAG4(IR_LEQ, FormatReg, FormatImm, IR_U32), X86_JLE_U4RI },
		{ MAKE_LOOKTAG4(IR_LEQ, FormatInSIB, FormatInSIB, IR_F32), X86_JLE_F4MM },
		{ MAKE_LOOKTAG4(IR_LEQ, FormatInSIB, FormatInSIB, IR_F64), X86_JLE_F8MM }
	};

	FCCASCode* asm;

	if (!(asm = cc_as_newcode(where))) {
		return FALSE;
	}

	asm->_opcode = lookup_asmcode(MAKE_LOOKTAG4(ircmp, dst->_format, src->_format, dst->_tycode), lookup, ELEMENTSCNT(lookup));
	if (asm->_opcode == X86_INS_INVALID) {
		return FALSE;
	}
	asm->_dst = *dst;
	asm->_src = *src;
	asm->_target = tlabel;
	emit(asmlist, asm);

	if (flabel && !emit_jmp(asmlist, flabel, where))
	{
		return FALSE;
	}

	return TRUE;
}

static BOOL emit_move(FCCASCodeList* asmlist, FCCASOperand* dst, FCCASOperand* src, int bytescnt, enum EMMArea where)
{
	static const FCCASLookupEntry lookup[] =
	{
		{ MAKE_LOOKTAG3(FormatSIB, FormatReg, IR_S8),  X86_MOV_I1MRI },
		{ MAKE_LOOKTAG3(FormatSIB, FormatReg, IR_S16), X86_MOV_I2MRI },
		{ MAKE_LOOKTAG3(FormatSIB, FormatReg, IR_S32), X86_MOV_I4MRI },
		{ MAKE_LOOKTAG3(FormatSIB, FormatReg, IR_U8),  X86_MOV_U1MRI },
		{ MAKE_LOOKTAG3(FormatSIB, FormatReg, IR_U16), X86_MOV_U2MRI },
		{ MAKE_LOOKTAG3(FormatSIB, FormatReg, IR_U32), X86_MOV_U4MRI },
		{ MAKE_LOOKTAG3(FormatSIB, FormatReg, IR_PTR), X86_MOV_P4MRI },

		{ MAKE_LOOKTAG3(FormatSIB, FormatImm, IR_S8),  X86_MOV_I1MRI },
		{ MAKE_LOOKTAG3(FormatSIB, FormatImm, IR_S16), X86_MOV_I2MRI },
		{ MAKE_LOOKTAG3(FormatSIB, FormatImm, IR_S32), X86_MOV_I4MRI },
		{ MAKE_LOOKTAG3(FormatSIB, FormatImm, IR_U8),  X86_MOV_U1MRI },
		{ MAKE_LOOKTAG3(FormatSIB, FormatImm, IR_U16), X86_MOV_U2MRI },
		{ MAKE_LOOKTAG3(FormatSIB, FormatImm, IR_U32), X86_MOV_U4MRI },
		{ MAKE_LOOKTAG3(FormatSIB, FormatImm, IR_PTR), X86_MOV_P4MRI },

		{ MAKE_LOOKTAG3(FormatReg, FormatReg, IR_S8),  X86_MOV_RRI },
		{ MAKE_LOOKTAG3(FormatReg, FormatReg, IR_S16), X86_MOV_RRI },
		{ MAKE_LOOKTAG3(FormatReg, FormatReg, IR_S32), X86_MOV_RRI },
		{ MAKE_LOOKTAG3(FormatReg, FormatReg, IR_U8),  X86_MOV_RRI },
		{ MAKE_LOOKTAG3(FormatReg, FormatReg, IR_U16), X86_MOV_RRI },
		{ MAKE_LOOKTAG3(FormatReg, FormatReg, IR_U32), X86_MOV_RRI },
		{ MAKE_LOOKTAG3(FormatReg, FormatReg, IR_PTR), X86_MOV_RRI },

		{ MAKE_LOOKTAG3(FormatReg, FormatImm, IR_S8),  X86_MOV_RRI },
		{ MAKE_LOOKTAG3(FormatReg, FormatImm, IR_S16), X86_MOV_RRI },
		{ MAKE_LOOKTAG3(FormatReg, FormatImm, IR_S32), X86_MOV_RRI },
		{ MAKE_LOOKTAG3(FormatReg, FormatImm, IR_U8),  X86_MOV_RRI },
		{ MAKE_LOOKTAG3(FormatReg, FormatImm, IR_U16), X86_MOV_RRI },
		{ MAKE_LOOKTAG3(FormatReg, FormatImm, IR_U32), X86_MOV_RRI },
		{ MAKE_LOOKTAG3(FormatReg, FormatImm, IR_PTR), X86_MOV_RRI },

		{ MAKE_LOOKTAG3(FormatReg, FormatSIB, IR_PTR), X86_ADDR    },
		{ MAKE_LOOKTAG3(FormatSIB, FormatInSIB, IR_BLK), X86_MOV_BMM }
	};

	FCCASCode* asm;

	if (!(asm = cc_as_newcode(where))) {
		return FALSE;
	}

	asm->_opcode = lookup_asmcode(MAKE_LOOKTAG3(dst->_format, src->_format, dst->_tycode), lookup, ELEMENTSCNT(lookup));
	if (asm->_opcode == X86_INS_INVALID) {
		return FALSE;
	}
	asm->_dst = *dst;
	asm->_src = *src;
	asm->_count = bytescnt;
	emit(asmlist, asm);

	return TRUE;
}

static BOOL emit_add(FCCASCodeList* asmlist, FCCIRDagNode* dag, FCCASOperand* dst, FCCASOperand* src, enum EMMArea where)
{
	return FALSE;
}

static BOOL emit_sub(FCCASCodeList* asmlist, FCCIRDagNode* dag, FCCASOperand* dst, FCCASOperand* src, enum EMMArea where)
{
	return FALSE;
}

static BOOL emit_mul(FCCASCodeList* asmlist, FCCIRDagNode* dag, FCCASOperand* dst, FCCASOperand* src, enum EMMArea where)
{
	return FALSE;
}

static BOOL emit_div(FCCASCodeList* asmlist, FCCIRDagNode* dag, FCCASOperand* dst, FCCASOperand* src, enum EMMArea where)
{
	return FALSE;
}

static BOOL emit_mod(FCCASCodeList* asmlist, FCCIRDagNode* dag, FCCASOperand* dst, FCCASOperand* src, enum EMMArea where)
{
	return FALSE;
}

static BOOL emit_lshift(FCCASCodeList* asmlist, FCCIRDagNode* dag, FCCASOperand* dst, FCCASOperand* src, enum EMMArea where)
{
	return FALSE;
}

static BOOL emit_rshift(FCCASCodeList* asmlist, FCCIRDagNode* dag, FCCASOperand* dst, FCCASOperand* src, enum EMMArea where)
{
	return FALSE;
}

static BOOL emit_bitand(FCCASCodeList* asmlist, FCCIRDagNode* dag, FCCASOperand* dst, FCCASOperand* src, enum EMMArea where)
{
	return FALSE;
}

static BOOL emit_bitor(FCCASCodeList* asmlist, FCCIRDagNode* dag, FCCASOperand* dst, FCCASOperand* src, enum EMMArea where)
{
	return FALSE;
}

static BOOL emit_bitxor(FCCASCodeList* asmlist, FCCIRDagNode* dag, FCCASOperand* dst, FCCASOperand* src, enum EMMArea where)
{
	return FALSE;
}

static BOOL emit_neg(FCCASCodeList* asmlist, FCCIRDagNode* dag, FCCASOperand* dst, enum EMMArea where)
{
	return FALSE;
}

static BOOL emit_bcom(FCCASCodeList* asmlist, FCCIRDagNode* dag, FCCASOperand* dst, enum EMMArea where)
{
	return FALSE;
}

static BOOL emit_cvt(FCCASCodeList* asmlist, FCCIRDagNode* dag, FCCASOperand* dst, FCCASOperand* src, enum EMMArea where)
{
	return FALSE;
}

static BOOL emit_call(FCCASCodeList* asmlist, FCCIRDagNode* dag, FCCASOperand* dst, enum EMMArea where)
{
	return FALSE;
}

/* generate code for expression */
static BOOL munch_expr(FCCGenCodeContext* ctx, FCCIRDagNode* dag, FCCASCodeList* asmlist, int requires, struct tagCCASOperand* operand, enum EMMArea where)
{
	struct tagCCASOperand dst = { 0 }, src = { 0 };

	switch (IR_OP(dag->_op))
	{
	case IR_CONST:
	{
		operand->_format = FormatImm;
		operand->_tycode = IR_OPTY0(dag->_op);
		operand->_u._imm = dag->_symbol;
	}
		break;
	case IR_ASSIGN:
	{
		if (!munch_expr(ctx, dag->_kids[1], asmlist, REQUIRES_BIT4(FormatReg, FormatImm, FormatInSIB, FormatSIB), &src, where)) {
			return FALSE;
		}

		if (!munch_expr(ctx, dag->_kids[0], asmlist, REQUIRES_BIT1(FormatInSIB), &dst, where)) {
			return FALSE;
		}

		if (!emit_move(asmlist, &dst, &src, dag->_kids[1]->_typesize, where)) {
			return FALSE;
		}
	}
		break;
	case IR_ADD:
	{
		if (!munch_expr(ctx, dag->_kids[0], asmlist, REQUIRES_BIT1(FormatReg), &dst, where)) {
			return FALSE;
		}
		if (!munch_expr(ctx, dag->_kids[1], asmlist, REQUIRES_BIT3(FormatReg, FormatImm, FormatInSIB), &src, where)) {
			return FALSE;
		}

		if (!emit_add(asmlist, dag, &dst, &src, where)) {
			return FALSE;
		}
	}
		break;
	case IR_SUB:
	{
		if (!munch_expr(ctx, dag->_kids[1], asmlist, REQUIRES_BIT3(FormatReg, FormatImm, FormatInSIB), &src, where)) {
			return FALSE;
		}

		if (!munch_expr(ctx, dag->_kids[0], asmlist, REQUIRES_BIT1(FormatReg), &dst, where)) {
			return FALSE;
		}

		if (!emit_sub(asmlist, dag, &dst, &src, where)) {
			return FALSE;
		}
	}
		break;
	case IR_MUL:
	{
		if (!munch_expr(ctx, dag->_kids[1], asmlist, REQUIRES_BIT3(FormatReg, FormatImm, FormatInSIB), &src, where)) {
			return FALSE;
		}

		if (!munch_expr(ctx, dag->_kids[0], asmlist, REQUIRES_BIT1(FormatReg), &dst, where)) {
			return FALSE;
		}

		if (!emit_mul(asmlist, dag, &dst, &src, where)) {
			return FALSE;
		}
	}
		break;
	case IR_DIV:
	{
		if (!munch_expr(ctx, dag->_kids[1], asmlist, REQUIRES_BIT3(FormatReg, FormatImm, FormatInSIB), &src, where)) {
			return FALSE;
		}

		if (!munch_expr(ctx, dag->_kids[0], asmlist, REQUIRES_BIT1(FormatReg), &dst, where)) {
			return FALSE;
		}

		if (!emit_div(asmlist, dag, &dst, &src, where)) {
			return FALSE;
		}
	}
		break;
	case IR_MOD:
	{
		if (!munch_expr(ctx, dag->_kids[1], asmlist, REQUIRES_BIT3(FormatReg, FormatImm, FormatInSIB), &src, where)) {
			return FALSE;
		}

		if (!munch_expr(ctx, dag->_kids[0], asmlist, REQUIRES_BIT1(FormatReg), &dst, where)) {
			return FALSE;
		}

		if (!emit_mod(asmlist, dag, &dst, &src, where)) {
			return FALSE;
		}
	}
		break;
	case IR_LSHIFT:
	{
		if (!munch_expr(ctx, dag->_kids[1], asmlist, REQUIRES_BIT3(FormatReg, FormatImm, FormatInSIB), &src, where)) {
			return FALSE;
		}

		if (!munch_expr(ctx, dag->_kids[0], asmlist, REQUIRES_BIT1(FormatReg), &dst, where)) {
			return FALSE;
		}

		if (!emit_lshift(asmlist, dag, &dst, &src, where)) {
			return FALSE;
		}
	}
		break;
	case IR_RSHIFT:
	{
		if (!munch_expr(ctx, dag->_kids[1], asmlist, REQUIRES_BIT3(FormatReg, FormatImm, FormatInSIB), &src, where)) {
			return FALSE;
		}

		if (!munch_expr(ctx, dag->_kids[0], asmlist, REQUIRES_BIT1(FormatReg), &dst, where)) {
			return FALSE;
		}

		if (!emit_rshift(asmlist, dag, &dst, &src, where)) {
			return FALSE;
		}
	}
		break;
	case IR_BITAND:
	{
		if (!munch_expr(ctx, dag->_kids[1], asmlist, REQUIRES_BIT3(FormatReg, FormatImm, FormatInSIB), &src, where)) {
			return FALSE;
		}

		if (!munch_expr(ctx, dag->_kids[0], asmlist, REQUIRES_BIT1(FormatReg), &dst, where)) {
			return FALSE;
		}

		if (!emit_bitand(asmlist, dag, &dst, &src, where)) {
			return FALSE;
		}
	}
		break;
	case IR_BITOR:
	{
		if (!munch_expr(ctx, dag->_kids[1], asmlist, REQUIRES_BIT3(FormatReg, FormatImm, FormatInSIB), &src, where)) {
			return FALSE;
		}

		if (!munch_expr(ctx, dag->_kids[0], asmlist, REQUIRES_BIT1(FormatReg), &dst, where)) {
			return FALSE;
		}

		if (!emit_bitor(asmlist, dag, &dst, &src, where)) {
			return FALSE;
		}
	}
		break;
	case IR_BITXOR:
	{
		if (!munch_expr(ctx, dag->_kids[1], asmlist, REQUIRES_BIT3(FormatReg, FormatImm, FormatInSIB), &src, where)) {
			return FALSE;
		}

		if (!munch_expr(ctx, dag->_kids[0], asmlist, REQUIRES_BIT1(FormatReg), &dst, where)) {
			return FALSE;
		}

		if (!emit_bitxor(asmlist, dag, &dst, &src, where)) {
			return FALSE;
		}
	}
		break;
	case IR_NEG:
	{
		if (!munch_expr(ctx, dag->_kids[0], asmlist, REQUIRES_BIT1(FormatReg), &dst, where)) {
			return FALSE;
		}

		if (!emit_neg(asmlist, dag, &dst, where)) {
			return FALSE;
		}
	}
		break;
	case IR_BCOM:
	{
		if (!munch_expr(ctx, dag->_kids[0], asmlist, REQUIRES_BIT1(FormatReg), &dst, where)) {
			return FALSE;
		}

		if (!emit_bcom(asmlist, dag, &dst, where)) {
			return FALSE;
		}
	}
		break;
	case IR_CVT:
	{
		if (!munch_expr(ctx, dag->_kids[0], asmlist, REQUIRES_BIT3(FormatReg, FormatImm, FormatInSIB), &src, where)) {
			return FALSE;
		}

		if (!emit_cvt(asmlist, dag, &dst, &src, where)) {
			return FALSE;
		}
	}
		break;
	case IR_INDIR:
	{
		if (!munch_expr(ctx, dag->_kids[0], asmlist, REQUIRES_BIT3(FormatReg, FormatImm, FormatSIB), &src, where)) {
			return FALSE;
		}

		operand->_format = FormatInSIB;
		operand->_tycode = IR_OPTY0(dag->_op);
		switch (src._format)
		{
		case FormatReg:
			operand->_u._sib._basereg = src._u._regs[0]; break;
		case FormatImm:
			operand->_u._sib._displacement2 = src._u._imm->_u._cnstval._pointer; break;
		case FormatSIB:
			operand->_u._sib = src._u._sib; break;
		default:
			assert(0); break;
		}
	}
		break;
	case IR_ADDRG:
	{
		operand->_format = FormatSIB;
		operand->_tycode = IR_OPTY0(dag->_op);
		operand->_u._sib._displacement = dag->_symbol;
	}
		break;
	case IR_ADDRF:
	case IR_ADDRL:
	{
		operand->_format = FormatSIB;
		operand->_tycode = IR_OPTY0(dag->_op);
		operand->_u._sib._basereg = X86_EBP;
		operand->_u._sib._displacement2 = dag->_symbol->_x._where._frame_offset;
	}
		break;
	case IR_CALL:
	{
		if (!munch_expr(ctx, dag->_kids[0], asmlist, REQUIRES_BIT3(FormatReg, FormatImm, FormatSIB), &dst, where)) {
			return FALSE;
		}

		if (!emit_call(asmlist, dag, &dst, where)) {
			return FALSE;
		}
	}
		break;
	default:
		assert(0); break;
	}

	return TRUE;
}

/* generate code for ir-code */
static BOOL munch_stmt(FCCGenCodeContext* ctx, FCCIRCode* ircode, FCCASCodeList* asmlist, enum EMMArea where)
{
	FCCASOperand dst = { 0 }, src = { 0 };
	FCCIRDagNode* dag;
	FCCASCode* asm;
	int tycode;


	switch (ircode->_op)
	{
	case IR_ARG:
	{
		dag = ircode->_u._expr->_dagnode;
		if (!munch_expr(ctx, dag, asmlist, REQUIRES_BIT3(FormatReg, FormatImm, FormatInSIB), &src, where))
		{
			return FALSE;
		}

		if (!emit_argv(asmlist, &src, dag->_typesize, where))
		{
			return FALSE;
		}
	}
		break;
	case IR_EXP:
	{
		dag = ircode->_u._expr->_dagnode;
		if (!munch_expr(ctx, dag, asmlist, REQUIRES_BIT0(), NULL, where))
		{
			return FALSE;
		}
	}
		break;
	case IR_JMP:
	{
		if (!emit_jmp(asmlist, ircode->_u._jmp._tlabel, where)) {
			return FALSE;
		}
	}
		break;
	case IR_CJMP:
	{
		FCCIRDagNode *lhs, *rhs;

		dag = ircode->_u._jmp._cond->_dagnode;
		lhs = dag->_kids[0];
		rhs = dag->_kids[1];
		tycode = IR_OPTY0(lhs->_op);

		if (tycode == IR_F32 || tycode == IR_F64)
		{
			if (!munch_expr(ctx, lhs, asmlist, REQUIRES_BIT1(FormatInSIB), &dst, where))
			{
				return FALSE;
			}
			if (!munch_expr(ctx, rhs, asmlist, REQUIRES_BIT1(FormatInSIB), &src, where))
			{
				return FALSE;
			}
		}
		else
		{
			if (!munch_expr(ctx, lhs, asmlist, REQUIRES_BIT1(FormatReg), &dst, where))
			{
				return FALSE;
			}

			if (!munch_expr(ctx, rhs, asmlist, REQUIRES_BIT3(FormatReg, FormatImm, FormatInSIB), &src, where))
			{
				return FALSE;
			}
		}

		if (!emit_cjmp(asmlist, IR_OP(dag->_op), &dst, &src, ircode->_u._jmp._tlabel, ircode->_u._jmp._flabel, where))
		{
			return FALSE;
		}
	}
		break;
	case IR_LABEL:
	{
		if (!(asm = cc_as_newcode(where))) {
			return FALSE;
		}

		asm->_opcode = X86_LABEL;
		asm->_target = ircode->_u._label;

		emit(asmlist, asm);
	}
		break;
	case IR_BLKBEG:
		block_enter(ctx, ircode->_u._blklevel); break;
	case IR_BLKEND:
		block_enter(ctx, ircode->_u._blklevel); break;
	case IR_LOCVAR:
		local_variable(ctx, ircode->_u._id); break;
	case IR_RET:
	{
		if (ircode->_u._ret._expr) 
		{
			dag = ircode->_u._ret._expr->_dagnode;
			if (!munch_expr(ctx, dag, asmlist, REQUIRES_BIT4(FormatReg, FormatImm, FormatInSIB, FormatSIB), &src, where))
			{
				return FALSE;
			}

			dst._format = FormatReg;
			dst._tycode = src._tycode;
			switch (dst._tycode)
			{
			case IR_S8:
			case IR_U8:
			case IR_S16:
			case IR_U16:
			case IR_S32:
			case IR_U32:
				dst._u._regs[0] = X86_EAX; dst._u._regs[1] = X86_NIL; break;
			case IR_S64:
			case IR_U64:
				dst._u._regs[0] = X86_EAX; dst._u._regs[1] = X86_EDX; break;
			case IR_F32:
			case IR_F64:
				dst._u._regs[0] = X86_ST0; dst._u._regs[1] = X86_NIL; break;
			case IR_PTR:
				dst._u._regs[0] = X86_EAX; dst._u._regs[1] = X86_NIL; break;
			default:
				assert(0);
			}

			if (!emit_move(asmlist, &dst, &src, dag->_typesize, where)) {
				return FALSE;
			}
		}

		if (ircode->_u._ret._exitlab && !emit_jmp(asmlist, ircode->_u._ret._exitlab, where))
		{
			return FALSE;
		}
	}
		break;
	case IR_ZERO:
	{
		if (!(asm = cc_as_newcode(where))) {
			return FALSE;
		}

		if (!munch_expr(ctx, ircode->_u._zero._addr->_dagnode, asmlist, REQUIRES_BIT1(FormatSIB), &asm->_dst, where))
		{
			return FALSE;
		}
		asm->_opcode = X86_ZERO_M;
		asm->_count = ircode->_u._zero._bytes;

		emit(asmlist, asm);
	}
		break;
	case IR_FENTER:
	{
		if (!(asm = cc_as_newcode(where))) {
			return FALSE;
		}

		asm->_opcode = X86_PROLOGUE;
		emit(asmlist, asm);
	}
		break;
	case IR_FEXIT:
	{
		if (!(asm = cc_as_newcode(where))) {
			return FALSE;
		}

		asm->_opcode = X86_EPILOGUE;
		emit(asmlist, asm);
	}
		break;
	default:
		assert(0); break;
	}

	return TRUE;
}

static BOOL cc_gen_asmcodes_irlist(struct tagCCSymbol* func, FCCIRCodeList* irlist, struct tagCCASCodeList* asmlist, enum EMMArea where)
{
	FCCIRCode* c = irlist->_head;

	for (; c; c = c->_next)
	{
		
	} /* end for */

	return TRUE;
}

BOOL cc_gen_asmcodes(struct tagCCSymbol* func, struct tagCCIRBasicBlock* bb, struct tagCCASCodeList* asmlist, enum EMMArea where)
{
	for (; bb; bb = bb->_next)
	{
		if (!cc_gen_asmcodes_irlist(func, &bb->_codes, asmlist, where)) {
			return FALSE;
		}
	} /* end for bb */

	return TRUE;
}
