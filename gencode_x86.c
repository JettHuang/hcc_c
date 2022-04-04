/*\brief
 *	generate x86 assemble codes.
 */

#include "utils.h"
#include "mm.h"
#include "cc.h"
#include "logger.h"
#include "parser/symbols.h"
#include "parser/types.h"
#include "ir/ir.h"
#include "gen/assem.h"
#include "opcodes_x86.h"
#include "reg_x86.h"


#define ASM_INSTRUCT(op, str)	{ op,  str},

/* assemble code set */
struct FX86AsmCode {
	unsigned int _op;
	const char* _text;
} X86AsmCodes[] = 
{
#include "opcodes_x86.inl"
};


#define REQUIRES_BIT0()		(0)
#define REQUIRES_BIT1(b0)	(REQUIRES_BIT0() | (1 << b0))
#define REQUIRES_BIT2(b0, b1)	(REQUIRES_BIT1(b0) | (1 << b1))
#define REQUIRES_BIT3(b0, b1, b2)	(REQUIRES_BIT2(b0, b1) | (1 << b2))
#define REQUIRES_BIT4(b0, b1, b2, b3)	(REQUIRES_BIT3(b0, b1, b2) | (1 << b3))


enum ELocalUseEntryKind { LOCAL_BLOCK, LOCAL_VAR, LOCAL_TMP};
typedef struct tagCCLocalUseEntry {
	int _kind;
	union {
		struct {
			int _level; /* block level */
			int _offset;
		} _blk;
		struct {
			struct tagCCDagNode* _dag;
			int _offset;
			int _size;
		} _tmp;
	} _x;
} FCCLocalUseEntry;

typedef struct tagCCGenCodeContext {
	struct tagCCASCodeList* _asmlist;
	enum EMMArea _where;

	FArray _localuses;
	int _curlocalbytes; /* current local space bytes used */
	int _maxlocalbytes; /* max local space bytes used */
} FCCGenCodeContext;

typedef struct tagCCASLookupEntry {
	int _tag;
	enum X86ASMCode _asop;
} FCCASLookupEntry;

static struct tagCCASCode* emit(struct tagCCGenCodeContext* ctx, int opcode);
static BOOL munch_expr(struct tagCCGenCodeContext* ctx, FCCIRDagNode* dag, struct tagCCASOperand* operand);
static BOOL munch_stmt(struct tagCCGenCodeContext* ctx, FCCIRCode* stmt);

static void block_enter(struct tagCCGenCodeContext* ctx, int seqid, int level);
static void block_leave(struct tagCCGenCodeContext* ctx, int seqid, int level);
static void local_variable(struct tagCCGenCodeContext* ctx, struct tagCCSymbol* id);


static void gen_context_init(struct tagCCGenCodeContext* ctx, struct tagCCASCodeList* asmlist, enum EMMArea where)
{
	ctx->_asmlist = asmlist;
	ctx->_where = where;
	ctx->_curlocalbytes = 0;
	ctx->_maxlocalbytes = 0;
	ctx->_asm_seqid = 0;

	array_init(&ctx->_localuses, 128, sizeof(struct tagCCLocalUseEntry), where);
}

static void block_enter(struct tagCCGenCodeContext* ctx, int seqid, int level)
{
	struct tagCCLocalUseEntry e;

	e._kind = LOCAL_BLOCK;
	e._x._blk._level = level;
	e._x._blk._offset = ctx->_curlocalbytes;

	array_append(&ctx->_localuses, &e);
}

static void block_leave(struct tagCCGenCodeContext* ctx, int seqid, int level)
{
	int i;

	for (i=ctx->_localuses._elecount-1; i>=0; --i)
	{
		struct tagCCLocalUseEntry* p;
		
		p = array_element(&ctx->_localuses, i);
		if (p->_kind == LOCAL_TMP)
		{
			if (p->_x._tmp._dag && p->_x._tmp._dag->_lastref > seqid)
			{
				break; /* keep it living */
			}

			array_popback(&ctx->_localuses);
		}
		else if (p->_kind == LOCAL_BLOCK)
		{
			if (p->_x._blk._level >= level)
			{
				ctx->_curlocalbytes = p->_x._blk._offset;
				array_popback(&ctx->_localuses);
			}
			if (p->_x._blk._level == level)
			{
				break;
			}
		}
	} /* end for */
}

static void local_variable(struct tagCCGenCodeContext* ctx, struct tagCCSymbol* id)
{
	int spacebytes = util_roundup(id->_type->_size, 4);
	
	ctx->_curlocalbytes += spacebytes;
	id->_x._frame_offset = -ctx->_curlocalbytes;

	if (ctx->_curlocalbytes > ctx->_maxlocalbytes)
	{
		ctx->_maxlocalbytes = ctx->_curlocalbytes;
	}
}

int gen_get_tempspace(struct tagCCGenCodeContext* ctx, int seqid, struct tagCCDagNode* dag)
{
	struct tagCCLocalUseEntry e;
	int i, spacebytes;

	for (i=0; i < ctx->_localuses._elecount; ++i)
	{
		struct tagCCLocalUseEntry* p;

		p = array_element(&ctx->_localuses, i);
		if (p->_kind == LOCAL_TMP)
		{
			if (p->_x._tmp._size >= dag->_typesize &&
				(!p->_x._tmp._dag || p->_x._tmp._dag->_lastref < seqid))
			{
				p->_x._tmp._dag = dag;
				return p->_x._tmp._offset;
			}
		}
	} /* end for */

	/* create new temp */
	spacebytes = util_roundup(dag->_typesize, 4);
	ctx->_curlocalbytes += spacebytes;
	if (ctx->_curlocalbytes > ctx->_maxlocalbytes)
	{
		ctx->_maxlocalbytes = ctx->_curlocalbytes;
	}

	e._kind = LOCAL_TMP;
	e._x._tmp._offset = -ctx->_curlocalbytes;
	e._x._tmp._size = spacebytes;
	e._x._tmp._dag = dag;
	array_append(&ctx->_localuses, &e);
	
	return e._x._tmp._offset;
}

static void emit(struct tagCCGenCodeContext* ctx, FCCASCode* asm)
{
	asm->_seqid = ++ctx->_asm_seqid;
	cc_as_codelist_append(ctx->_asmlist, asm);
}

static BOOL emit_argv(FCCASCodeList* asmlist, const FCCASOperand* src, int bytescnt, enum EMMArea where)
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

static BOOL emit_cjmp(FCCASCodeList* asmlist, int ircmp, const FCCASOperand* dst, const FCCASOperand* src, FCCSymbol* tlabel, FCCSymbol* flabel, enum EMMArea where)
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

static FCCASCode* emit_move(FCCASCodeList* asmlist, const FCCASOperand* dst, const FCCASOperand* src, int bytescnt, enum EMMArea where)
{
	static const FCCASLookupEntry lookup[] =
	{
		{ MAKE_LOOKTAG3(FormatSIB, FormatReg, IR_S8),  X86_MOV_I1MRI },
		{ MAKE_LOOKTAG3(FormatSIB, FormatReg, IR_S16), X86_MOV_I2MRI },
		{ MAKE_LOOKTAG3(FormatSIB, FormatReg, IR_S32), X86_MOV_I4MRI },
		{ MAKE_LOOKTAG3(FormatSIB, FormatReg, IR_S64), X86_MOV_I8MR  },
		{ MAKE_LOOKTAG3(FormatSIB, FormatReg, IR_U8),  X86_MOV_U1MRI },
		{ MAKE_LOOKTAG3(FormatSIB, FormatReg, IR_U16), X86_MOV_U2MRI },
		{ MAKE_LOOKTAG3(FormatSIB, FormatReg, IR_U32), X86_MOV_U4MRI },
		{ MAKE_LOOKTAG3(FormatSIB, FormatReg, IR_U64), X86_MOV_U8MR  },
		{ MAKE_LOOKTAG3(FormatSIB, FormatReg, IR_PTR), X86_MOV_P4MRI },

		{ MAKE_LOOKTAG3(FormatSIB, FormatImm, IR_S8),  X86_MOV_I1MRI },
		{ MAKE_LOOKTAG3(FormatSIB, FormatImm, IR_S16), X86_MOV_I2MRI },
		{ MAKE_LOOKTAG3(FormatSIB, FormatImm, IR_S32), X86_MOV_I4MRI },
		{ MAKE_LOOKTAG3(FormatSIB, FormatImm, IR_S64), X86_MOV_I8MI  },
		{ MAKE_LOOKTAG3(FormatSIB, FormatImm, IR_U8),  X86_MOV_U1MRI },
		{ MAKE_LOOKTAG3(FormatSIB, FormatImm, IR_U16), X86_MOV_U2MRI },
		{ MAKE_LOOKTAG3(FormatSIB, FormatImm, IR_U32), X86_MOV_U4MRI },
		{ MAKE_LOOKTAG3(FormatSIB, FormatImm, IR_U64), X86_MOV_U8MI  },
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
		return NULL;
	}

	asm->_opcode = lookup_asmcode(MAKE_LOOKTAG3(dst->_format, src->_format, dst->_tycode), lookup, ELEMENTSCNT(lookup));
	if (asm->_opcode == X86_INS_INVALID) {
		return NULL;
	}
	asm->_dst = *dst;
	asm->_src = *src;
	asm->_count = bytescnt;
	emit(asmlist, asm);

	return TRUE;
}

BOOL emit_store_regvalue(struct tagCCGenCodeContext* ctx, struct tagCCDagNode* dag, int tmpoffset)
{
	struct tagCCASOperand dst = { 0 }, src = { 0 };

	dst._format = FormatSIB;
	dst._tycode = IR_OPTY0(dag->_op);
	dst._u._sib._basereg = X86_EBP;
	dst._u._sib._displacement2 = tmpoffset;
	src._format = FormatReg;
	src._tycode = IR_OPTY0(dag->_op);
	src._u._regs[0] = dag->_x._loc._registers[0];
	src._u._regs[1] = dag->_x._loc._registers[1];

	return emit_move(ctx->_asmlist, &dst, &src, dag->_typesize, ctx->_where);
}

static BOOL emit_add(FCCASCodeList* asmlist, FCCIRDagNode* dag, const FCCASOperand* dst, const FCCASOperand* src, enum EMMArea where)
{
	static const FCCASLookupEntry lookup[] =
	{
		{ MAKE_LOOKTAG3(FormatReg, FormatReg, IR_S32), X86_ADD_I4RR },
		{ MAKE_LOOKTAG3(FormatReg, FormatImm, IR_S32), X86_ADD_I4RI },
		{ MAKE_LOOKTAG3(FormatReg, FormatInSIB, IR_S32),  X86_ADD_I4RM },
		{ MAKE_LOOKTAG3(FormatReg, FormatReg, IR_U32), X86_ADD_U4RR },
		{ MAKE_LOOKTAG3(FormatReg, FormatImm, IR_U32), X86_ADD_U4RI },
		{ MAKE_LOOKTAG3(FormatReg, FormatInSIB, IR_U32),  X86_ADD_U4RM },

		{ MAKE_LOOKTAG3(FormatReg, FormatInSIB, IR_F32),  X86_ADD_F4RM },
		{ MAKE_LOOKTAG3(FormatReg, FormatInSIB, IR_F64),  X86_ADD_F8RM }
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
	emit(asmlist, asm);

	return TRUE;
}

static BOOL emit_sub(FCCASCodeList* asmlist, FCCIRDagNode* dag, const FCCASOperand* dst, const FCCASOperand* src, enum EMMArea where)
{
	static const FCCASLookupEntry lookup[] =
	{
		{ MAKE_LOOKTAG3(FormatReg, FormatReg, IR_S32), X86_SUB_I4RR },
		{ MAKE_LOOKTAG3(FormatReg, FormatImm, IR_S32), X86_SUB_I4RI },
		{ MAKE_LOOKTAG3(FormatReg, FormatInSIB, IR_S32),  X86_SUB_I4RM },
		{ MAKE_LOOKTAG3(FormatReg, FormatReg, IR_U32), X86_SUB_U4RR },
		{ MAKE_LOOKTAG3(FormatReg, FormatImm, IR_U32), X86_SUB_U4RI },
		{ MAKE_LOOKTAG3(FormatReg, FormatInSIB, IR_U32),  X86_SUB_U4RM },

		{ MAKE_LOOKTAG3(FormatReg, FormatInSIB, IR_F32),  X86_SUB_F4RM },
		{ MAKE_LOOKTAG3(FormatReg, FormatInSIB, IR_F64),  X86_SUB_F8RM }
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
	emit(asmlist, asm);

	return TRUE;
}

static BOOL emit_mul(FCCASCodeList* asmlist, FCCIRDagNode* dag, const FCCASOperand* dst, const FCCASOperand* src, enum EMMArea where)
{
	static const FCCASLookupEntry lookup[] =
	{
		{ MAKE_LOOKTAG3(FormatReg, FormatReg, IR_S32), X86_MUL_I4RM },
		{ MAKE_LOOKTAG3(FormatReg, FormatInSIB, IR_S32),  X86_MUL_I4RM },
		{ MAKE_LOOKTAG3(FormatReg, FormatReg, IR_U32), X86_MUL_U4RR },
		{ MAKE_LOOKTAG3(FormatReg, FormatInSIB, IR_U32),  X86_MUL_U4RM },

		{ MAKE_LOOKTAG3(FormatReg, FormatInSIB, IR_F32),  X86_MUL_F4RM },
		{ MAKE_LOOKTAG3(FormatReg, FormatInSIB, IR_F64),  X86_MUL_F8RM }
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
	emit(asmlist, asm);

	return TRUE;
}

static BOOL emit_div(FCCASCodeList* asmlist, FCCIRDagNode* dag, const FCCASOperand* dst, const FCCASOperand* src, enum EMMArea where)
{
	static const FCCASLookupEntry lookup[] =
	{
		{ MAKE_LOOKTAG3(FormatReg, FormatReg, IR_S32), X86_DIV_I4RR },
		{ MAKE_LOOKTAG3(FormatReg, FormatInSIB, IR_S32),  X86_DIV_I4RM },
		{ MAKE_LOOKTAG3(FormatReg, FormatReg, IR_U32), X86_DIV_U4RR },
		{ MAKE_LOOKTAG3(FormatReg, FormatInSIB, IR_U32),  X86_DIV_U4RM },

		{ MAKE_LOOKTAG3(FormatReg, FormatInSIB, IR_F32),  X86_DIV_F4RM },
		{ MAKE_LOOKTAG3(FormatReg, FormatInSIB, IR_F64),  X86_DIV_F8RM }
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
	emit(asmlist, asm);

	return TRUE;
}

static BOOL emit_mod(FCCASCodeList* asmlist, FCCIRDagNode* dag, const FCCASOperand* dst, const FCCASOperand* src, enum EMMArea where)
{
	static const FCCASLookupEntry lookup[] =
	{
		{ MAKE_LOOKTAG3(FormatReg, FormatReg, IR_S32), X86_MOD_I4RR },
		{ MAKE_LOOKTAG3(FormatReg, FormatInSIB, IR_S32),  X86_MOD_I4RM },
		{ MAKE_LOOKTAG3(FormatReg, FormatReg, IR_U32), X86_MOD_U4RR },
		{ MAKE_LOOKTAG3(FormatReg, FormatInSIB, IR_U32),  X86_MOD_U4RM }
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
	emit(asmlist, asm);

	return TRUE;
}

static BOOL emit_lshift(FCCASCodeList* asmlist, FCCIRDagNode* dag, const FCCASOperand* dst, const FCCASOperand* src, enum EMMArea where)
{
	static const FCCASLookupEntry lookup[] =
	{
		{ MAKE_LOOKTAG3(FormatReg, FormatReg, IR_S32), X86_LSH_I4RR },
		{ MAKE_LOOKTAG3(FormatReg, FormatImm, IR_S32),  X86_LSH_I4RI },
		{ MAKE_LOOKTAG3(FormatReg, FormatReg, IR_U32), X86_LSH_U4RR },
		{ MAKE_LOOKTAG3(FormatReg, FormatImm, IR_U32),  X86_LSH_U4RI }
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
	emit(asmlist, asm);

	return TRUE;
}

static BOOL emit_rshift(FCCASCodeList* asmlist, FCCIRDagNode* dag, const FCCASOperand* dst, const FCCASOperand* src, enum EMMArea where)
{
	static const FCCASLookupEntry lookup[] =
	{
		{ MAKE_LOOKTAG3(FormatReg, FormatReg, IR_S32), X86_RSH_I4RR },
		{ MAKE_LOOKTAG3(FormatReg, FormatImm, IR_S32),  X86_RSH_I4RI },
		{ MAKE_LOOKTAG3(FormatReg, FormatReg, IR_U32), X86_RSH_U4RR },
		{ MAKE_LOOKTAG3(FormatReg, FormatImm, IR_U32),  X86_RSH_U4RI }
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
	emit(asmlist, asm);

	return TRUE;
}

static BOOL emit_bitand(FCCASCodeList* asmlist, FCCIRDagNode* dag, const FCCASOperand* dst, const FCCASOperand* src, enum EMMArea where)
{
	static const FCCASLookupEntry lookup[] =
	{
		{ MAKE_LOOKTAG3(FormatReg, FormatReg, IR_S32), X86_AND_I4RR },
		{ MAKE_LOOKTAG3(FormatReg, FormatImm, IR_S32),  X86_AND_I4RI },
		{ MAKE_LOOKTAG3(FormatReg, FormatInSIB, IR_S32),  X86_AND_I4RM },
		{ MAKE_LOOKTAG3(FormatReg, FormatReg, IR_U32), X86_AND_U4RR },
		{ MAKE_LOOKTAG3(FormatReg, FormatImm, IR_U32),  X86_AND_U4RI },
		{ MAKE_LOOKTAG3(FormatReg, FormatInSIB, IR_U32),  X86_AND_U4RM }
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
	emit(asmlist, asm);

	return TRUE;
}

static BOOL emit_bitor(FCCASCodeList* asmlist, FCCIRDagNode* dag, const FCCASOperand* dst, const FCCASOperand* src, enum EMMArea where)
{
	static const FCCASLookupEntry lookup[] =
	{
		{ MAKE_LOOKTAG3(FormatReg, FormatReg, IR_S32), X86_OR_I4RR },
		{ MAKE_LOOKTAG3(FormatReg, FormatImm, IR_S32),  X86_OR_I4RI },
		{ MAKE_LOOKTAG3(FormatReg, FormatInSIB, IR_S32),  X86_OR_I4RM },
		{ MAKE_LOOKTAG3(FormatReg, FormatReg, IR_U32), X86_OR_U4RR },
		{ MAKE_LOOKTAG3(FormatReg, FormatImm, IR_U32),  X86_OR_U4RI },
		{ MAKE_LOOKTAG3(FormatReg, FormatInSIB, IR_U32),  X86_OR_U4RM }
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
	emit(asmlist, asm);

	return TRUE;
}

static BOOL emit_bitxor(FCCASCodeList* asmlist, FCCIRDagNode* dag, const FCCASOperand* dst, const FCCASOperand* src, enum EMMArea where)
{
	static const FCCASLookupEntry lookup[] =
	{
		{ MAKE_LOOKTAG3(FormatReg, FormatReg, IR_S32), X86_XOR_I4RR },
		{ MAKE_LOOKTAG3(FormatReg, FormatImm, IR_S32),  X86_XOR_I4RI },
		{ MAKE_LOOKTAG3(FormatReg, FormatInSIB, IR_S32),  X86_XOR_I4RM },
		{ MAKE_LOOKTAG3(FormatReg, FormatReg, IR_U32), X86_XOR_U4RR },
		{ MAKE_LOOKTAG3(FormatReg, FormatImm, IR_U32),  X86_XOR_U4RI },
		{ MAKE_LOOKTAG3(FormatReg, FormatInSIB, IR_U32),  X86_XOR_U4RM }
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
	emit(asmlist, asm);

	return TRUE;
}

static BOOL emit_neg(FCCASCodeList* asmlist, FCCIRDagNode* dag, const FCCASOperand* dst, enum EMMArea where)
{
	static const FCCASLookupEntry lookup[] =
	{
		{ MAKE_LOOKTAG2(FormatReg, IR_S32), X86_NEG_I4R },
		{ MAKE_LOOKTAG2(FormatInSIB, IR_S32),  X86_NEG_I4M },
		{ MAKE_LOOKTAG2(FormatReg, IR_U32), X86_NEG_U4R },
		{ MAKE_LOOKTAG2(FormatInSIB, IR_U32),  X86_NEG_U4M },
		{ MAKE_LOOKTAG2(FormatReg, IR_F32),  X86_NEG_F4R },
		{ MAKE_LOOKTAG2(FormatReg, IR_F64),  X86_NEG_F8R }
	};

	FCCASCode* asm;

	if (!(asm = cc_as_newcode(where))) {
		return FALSE;
	}

	asm->_opcode = lookup_asmcode(MAKE_LOOKTAG2(dst->_format, dst->_tycode), lookup, ELEMENTSCNT(lookup));
	if (asm->_opcode == X86_INS_INVALID) {
		return FALSE;
	}
	asm->_dst = *dst;
	emit(asmlist, asm);

	return TRUE;
}

static BOOL emit_bcom(FCCASCodeList* asmlist, FCCIRDagNode* dag, const FCCASOperand* dst, enum EMMArea where)
{
	static const FCCASLookupEntry lookup[] =
	{
		{ MAKE_LOOKTAG2(FormatReg, IR_S32), X86_NOT_I4R },
		{ MAKE_LOOKTAG2(FormatInSIB, IR_S32),  X86_NOT_I4M },
		{ MAKE_LOOKTAG2(FormatReg, IR_U32), X86_NOT_U4R },
		{ MAKE_LOOKTAG2(FormatInSIB, IR_U32),  X86_NOT_U4M }
	};

	FCCASCode* asm;

	if (!(asm = cc_as_newcode(where))) {
		return FALSE;
	}

	asm->_opcode = lookup_asmcode(MAKE_LOOKTAG2(dst->_format, dst->_tycode), lookup, ELEMENTSCNT(lookup));
	if (asm->_opcode == X86_INS_INVALID) {
		return FALSE;
	}
	asm->_dst = *dst;
	emit(asmlist, asm);

	return TRUE;
}

static BOOL emit_cvt(FCCASCodeList* asmlist, FCCIRDagNode* dag, const FCCASOperand* dst, const FCCASOperand* src, enum EMMArea where)
{
	static const FCCASLookupEntry lookup[] =
	{
		/* load & extension byte & word */
		{ MAKE_LOOKTAG4(FormatReg, IR_S32, FormatInSIB, IR_S8),  X86_EXT_RI4MI1 },
		{ MAKE_LOOKTAG4(FormatReg, IR_S32, FormatInSIB, IR_S16), X86_EXT_RI4MI2 },
		{ MAKE_LOOKTAG4(FormatReg, IR_U32, FormatInSIB, IR_U8),  X86_EXT_RU4MU1 },
		{ MAKE_LOOKTAG4(FormatReg, IR_U32, FormatInSIB, IR_U16), X86_EXT_RU4MU2 },

		/* convert integer to float */
		{ MAKE_LOOKTAG4(FormatInSIB, IR_F32, FormatReg, IR_S32),  X86_CVT_MF4RI4 },
		{ MAKE_LOOKTAG4(FormatInSIB, IR_F64, FormatReg, IR_S32),  X86_CVT_MF8RI4 },
		{ MAKE_LOOKTAG4(FormatInSIB, IR_F32, FormatReg, IR_U32),  X86_CVT_MF4RU4 },
		{ MAKE_LOOKTAG4(FormatInSIB, IR_F64, FormatReg, IR_U32),  X86_CVT_MF8RU4 },

		/* convert double -- float */
		{ MAKE_LOOKTAG4(FormatInSIB, IR_F64, FormatInSIB, IR_F32),  X86_CVT_MF8MF4 },
		{ MAKE_LOOKTAG4(FormatInSIB, IR_F32, FormatInSIB, IR_F64),  X86_CVT_MF4MF8 },

		/* convert float to integer (in eax) */
		{ MAKE_LOOKTAG4(FormatReg, IR_S32, FormatInSIB, IR_F32),  X86_CVT_RI4MF4 },
		{ MAKE_LOOKTAG4(FormatReg, IR_U32, FormatInSIB, IR_F32),  X86_CVT_RU4MF4 },
		{ MAKE_LOOKTAG4(FormatReg, IR_S32, FormatInSIB, IR_F64),  X86_CVT_RI4MF8 },
		{ MAKE_LOOKTAG4(FormatReg, IR_U32, FormatInSIB, IR_F64),  X86_CVT_RU4MF8 }
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
	emit(asmlist, asm);

	return TRUE;
}

static BOOL emit_call(FCCASCodeList* asmlist, FCCIRDagNode* dag, const FCCASOperand* dst, enum EMMArea where)
{
	static const FCCASLookupEntry lookup[] =
	{
		{ MAKE_LOOKTAG1(FormatReg),  X86_CALL },
		{ MAKE_LOOKTAG1(FormatImm),  X86_CALL },
		{ MAKE_LOOKTAG1(FormatSIB),  X86_CALL }, 
		{ MAKE_LOOKTAG1(FormatInSIB),  X86_ICALL }
	};

	FCCASCode* asm;

	if (!(asm = cc_as_newcode(where))) {
		return FALSE;
	}

	asm->_opcode = lookup_asmcode(MAKE_LOOKTAG1(dst->_format), lookup, ELEMENTSCNT(lookup));
	if (asm->_opcode == X86_INS_INVALID) {
		return FALSE;
	}
	asm->_dst = *dst;
	emit(asmlist, asm);

	return TRUE;
}

/* generate code for expression */
static BOOL munch_expr(struct tagCCGenCodeContext* ctx, FCCIRDagNode* dag, struct tagCCASOperand* operand)
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
		if (!munch_expr(ctx, dag->_kids[1], REQUIRES_BIT4(FormatReg, FormatImm, FormatInSIB, FormatSIB), &src)) {
			return FALSE;
		}

		if (!munch_expr(ctx, dag->_kids[0], REQUIRES_BIT1(FormatInSIB), &dst)) {
			return FALSE;
		}

		if (!emit_move(ctx->_asmlist, &dst, &src, dag->_kids[1]->_typesize, ctx->_where)) {
			return FALSE;
		}
	}
		break;
	case IR_ADD:
	{
		if (!munch_expr(ctx, dag->_kids[0], REQUIRES_BIT1(FormatReg), &dst)) {
			return FALSE;
		}
		if (!munch_expr(ctx, dag->_kids[1], REQUIRES_BIT3(FormatReg, FormatImm, FormatInSIB), &src)) {
			return FALSE;
		}

		if (!emit_add(ctx->_asmlist, dag, &dst, &src, ctx->_where)) {
			return FALSE;
		}
	}
		break;
	case IR_SUB:
	{
		if (!munch_expr(ctx, dag->_kids[1], REQUIRES_BIT3(FormatReg, FormatImm, FormatInSIB), &src)) {
			return FALSE;
		}

		if (!munch_expr(ctx, dag->_kids[0], REQUIRES_BIT1(FormatReg), &dst)) {
			return FALSE;
		}

		if (!emit_sub(ctx->_asmlist, dag, &dst, &src, ctx->_where)) {
			return FALSE;
		}
	}
		break;
	case IR_MUL:
	{
		if (!munch_expr(ctx, dag->_kids[1], REQUIRES_BIT3(FormatReg, FormatImm, FormatInSIB), &src)) {
			return FALSE;
		}

		if (!munch_expr(ctx, dag->_kids[0], REQUIRES_BIT1(FormatReg), &dst)) {
			return FALSE;
		}

		if (!emit_mul(ctx->_asmlist, dag, &dst, &src, ctx->_where)) {
			return FALSE;
		}
	}
		break;
	case IR_DIV:
	{
		if (!munch_expr(ctx, dag->_kids[1], REQUIRES_BIT3(FormatReg, FormatImm, FormatInSIB), &src)) {
			return FALSE;
		}

		if (!munch_expr(ctx, dag->_kids[0], REQUIRES_BIT1(FormatReg), &dst)) {
			return FALSE;
		}

		if (!emit_div(ctx->_asmlist, dag, &dst, &src, ctx->_where)) {
			return FALSE;
		}
	}
		break;
	case IR_MOD:
	{
		if (!munch_expr(ctx, dag->_kids[1], REQUIRES_BIT3(FormatReg, FormatImm, FormatInSIB), &src)) {
			return FALSE;
		}

		if (!munch_expr(ctx, dag->_kids[0], REQUIRES_BIT1(FormatReg), &dst)) {
			return FALSE;
		}

		if (!emit_mod(ctx->_asmlist, dag, &dst, &src, ctx->_where)) {
			return FALSE;
		}
	}
		break;
	case IR_LSHIFT:
	{
		if (!munch_expr(ctx, dag->_kids[1], REQUIRES_BIT3(FormatReg, FormatImm, FormatInSIB), &src)) {
			return FALSE;
		}

		if (!munch_expr(ctx, dag->_kids[0], REQUIRES_BIT1(FormatReg), &dst)) {
			return FALSE;
		}

		if (!emit_lshift(ctx->_asmlist, dag, &dst, &src, ctx->_where)) {
			return FALSE;
		}
	}
		break;
	case IR_RSHIFT:
	{
		if (!munch_expr(ctx, dag->_kids[1], REQUIRES_BIT3(FormatReg, FormatImm, FormatInSIB), &src)) {
			return FALSE;
		}

		if (!munch_expr(ctx, dag->_kids[0], REQUIRES_BIT1(FormatReg), &dst)) {
			return FALSE;
		}

		if (!emit_rshift(ctx->_asmlist, dag, &dst, &src, ctx->_where)) {
			return FALSE;
		}
	}
		break;
	case IR_BITAND:
	{
		if (!munch_expr(ctx, dag->_kids[1], REQUIRES_BIT3(FormatReg, FormatImm, FormatInSIB), &src)) {
			return FALSE;
		}

		if (!munch_expr(ctx, dag->_kids[0], REQUIRES_BIT1(FormatReg), &dst)) {
			return FALSE;
		}

		if (!emit_bitand(ctx->_asmlist, dag, &dst, &src, ctx->_where)) {
			return FALSE;
		}
	}
		break;
	case IR_BITOR:
	{
		if (!munch_expr(ctx, dag->_kids[1], REQUIRES_BIT3(FormatReg, FormatImm, FormatInSIB), &src)) {
			return FALSE;
		}

		if (!munch_expr(ctx, dag->_kids[0], REQUIRES_BIT1(FormatReg), &dst)) {
			return FALSE;
		}

		if (!emit_bitor(ctx->_asmlist, dag, &dst, &src, ctx->_where)) {
			return FALSE;
		}
	}
		break;
	case IR_BITXOR:
	{
		if (!munch_expr(ctx, dag->_kids[1], REQUIRES_BIT3(FormatReg, FormatImm, FormatInSIB), &src)) {
			return FALSE;
		}

		if (!munch_expr(ctx, dag->_kids[0], REQUIRES_BIT1(FormatReg), &dst)) {
			return FALSE;
		}

		if (!emit_bitxor(ctx->_asmlist, dag, &dst, &src, ctx->_where)) {
			return FALSE;
		}
	}
		break;
	case IR_NEG:
	{
		if (!munch_expr(ctx, dag->_kids[0], REQUIRES_BIT1(FormatReg), &dst)) {
			return FALSE;
		}

		if (!emit_neg(ctx->_asmlist, dag, &dst, ctx->_where)) {
			return FALSE;
		}
	}
		break;
	case IR_BCOM:
	{
		if (!munch_expr(ctx, dag->_kids[0], REQUIRES_BIT1(FormatReg), &dst)) {
			return FALSE;
		}

		if (!emit_bcom(ctx->_asmlist, dag, &dst, ctx->_where)) {
			return FALSE;
		}
	}
		break;
	case IR_CVT:
	{
		if (!munch_expr(ctx, dag->_kids[0], REQUIRES_BIT3(FormatReg, FormatImm, FormatInSIB), &src)) {
			return FALSE;
		}

		if (!emit_cvt(ctx->_asmlist, dag, &dst, &src, ctx->_where)) {
			return FALSE;
		}
	}
		break;
	case IR_INDIR:
	{
		if (!munch_expr(ctx, dag->_kids[0], REQUIRES_BIT3(FormatReg, FormatImm, FormatSIB), &src)) {
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
		operand->_u._sib._displacement2 = dag->_symbol->_x._frame_offset;
	}
		break;
	case IR_CALL:
	{
		if (!munch_expr(ctx, dag->_kids[0], REQUIRES_BIT3(FormatReg, FormatImm, FormatSIB), &dst)) {
			return FALSE;
		}

		if (!emit_call(ctx->_asmlist, dag, &dst, ctx->_where)) {
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
static BOOL munch_stmt(struct tagCCGenCodeContext* ctx, FCCIRCode* ircode)
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
		if (!munch_expr(ctx, dag, REQUIRES_BIT3(FormatReg, FormatImm, FormatInSIB), &src))
		{
			return FALSE;
		}

		if (!emit_argv(ctx->_asmlist, &src, dag->_typesize, ctx->_where))
		{
			return FALSE;
		}
	}
		break;
	case IR_EXP:
	{
		dag = ircode->_u._expr->_dagnode;
		if (!munch_expr(ctx, dag, REQUIRES_BIT0(), NULL))
		{
			return FALSE;
		}
	}
		break;
	case IR_JMP:
	{
		if (!emit_jmp(ctx->_asmlist, ircode->_u._jmp._tlabel, ctx->_where)) {
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
			if (!munch_expr(ctx, lhs, REQUIRES_BIT1(FormatInSIB), &dst))
			{
				return FALSE;
			}
			if (!munch_expr(ctx, rhs, REQUIRES_BIT1(FormatInSIB), &src))
			{
				return FALSE;
			}
		}
		else
		{
			if (!munch_expr(ctx, lhs, REQUIRES_BIT1(FormatReg), &dst))
			{
				return FALSE;
			}

			if (!munch_expr(ctx, rhs, REQUIRES_BIT3(FormatReg, FormatImm, FormatInSIB), &src))
			{
				return FALSE;
			}
		}

		if (!emit_cjmp(ctx->_asmlist, IR_OP(dag->_op), &dst, &src, ircode->_u._jmp._tlabel, ircode->_u._jmp._flabel, ctx->_where))
		{
			return FALSE;
		}
	}
		break;
	case IR_LABEL:
	{
		if (!(asm = cc_as_newcode(ctx->_where))) {
			return FALSE;
		}

		asm->_opcode = X86_LABEL;
		asm->_target = ircode->_u._label;

		emit(ctx->_asmlist, asm);
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
			if (!(asm = cc_as_newcode(ctx->_where))) {
				return FALSE;
			}
			if (!munch_expr(ctx, ircode->_u._ret._expr->_dagnode, &asm->_src))
			{
				return FALSE;
			}

			asm->_opcode = X86_MOVERET;
			emit(ctx, asm);
		}

		if (ircode->_u._ret._exitlab && !emit_jmp(ctx->_asmlist, ircode->_u._ret._exitlab, ctx->_where))
		{
			return FALSE;
		}
	}
		break;
	case IR_ZERO:
	{
		if (!(asm = cc_as_newcode(ctx->_where))) {
			return FALSE;
		}
		if (!munch_expr(ctx, ircode->_u._zero._addr->_dagnode, &asm->_dst))
		{
			return FALSE;
		}

		asm->_opcode = X86_ZERO_M;
		asm->_count = ircode->_u._zero._bytes;
		emit(ctx, asm);
	}
		break;
	case IR_FENTER:
	{
		if (!(asm = cc_as_newcode(ctx->_where))) {
			return FALSE;
		}

		asm->_opcode = X86_PROLOGUE;
		emit(ctx, asm);
	}
		break;
	case IR_FEXIT:
	{
		if (!(asm = cc_as_newcode(ctx->_where))) {
			return FALSE;
		}

		asm->_opcode = X86_EPILOGUE;
		emit(ctx, asm);
	}
		break;
	default:
		assert(0); break;
	}

	return TRUE;
}

static BOOL cc_gen_asmcodes_irlist(struct tagCCGenCodeContext *ctx, FCCIRCodeList* irlist)
{
	FCCIRCode* c = irlist->_head;

	for (; c; c = c->_next)
	{
		if (!munch_stmt(ctx, c)) {
			return FALSE;
		}
	} /* end for */

	return TRUE;
}

/*
       Frame Layout
	      ...
     |             |
	 +-------------+
	 |   param2    |
	 +-------------+
	 |   param1    |
	 +-------------+
	 |   param0    |
	 +-------------+ +8
	 |   ret-addr  |
	 +-------------+ +4
	 |   ebp       |
	 +-------------+ +0 <--- ebp
	 |   local     |
	 |   temps     |
	 +-------------+
	 |callee-saved |
	 +-------------+
     |             |
 */
BOOL cc_gen_asmcodes(FArray* caller, FArray* callee, struct tagCCIRBasicBlock* bb, struct tagCCASCodeList* asmlist, enum EMMArea where)
{
	struct tagCCGenCodeContext genctx;
	int offset, i;

	gen_context_init(&genctx, asmlist, where);
	cc_reg_reset();

	/* calculate parameters stack offset */
	offset = 8;
	for (i = 0; i < caller->_elecount; i++)
	{
		FCCSymbol* p0, * p1;

		p0 = *(FCCSymbol**)array_element(caller, i);
		p1 = *(FCCSymbol**)array_element(callee, i);

		p0->_x._frame_offset = offset;
		p1->_x._frame_offset = offset;

		assert((p0->_type->_size & 0x03) == 0);
		offset += p0->_type->_size;
	} /* end for i */

	for (; bb; bb = bb->_next)
	{
		if (!cc_gen_asmcodes_irlist(&genctx, &bb->_codes)) {
			return FALSE;
		}
	} /* end for bb */

	return TRUE;
}

BOOL cc_gen_dumpfunction(struct tagCCContext* ctx, struct tagCCSymbol* func, FArray* caller, FArray* callee, struct tagCCIRBasicBlock* body)
{
	struct tagCCASCodeList asmlist = { NULL, NULL };

	return TRUE;

	if (!cc_gen_asmcodes(caller, callee, body, &asmlist, CC_MM_TEMPPOOL)) {
		cc_logger_outs("failed to generate assemble codes for function '%s'\n", func->_name);
		return FALSE;
	}

	return FALSE;
}
