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


#define REQUIRES_BIT0()		(0)
#define REQUIRES_BIT1(b0)	(REQUIRES_BIT0() | (1 << (b0)))
#define REQUIRES_BIT2(b0, b1)	(REQUIRES_BIT1(b0) | (1 << (b1)))
#define REQUIRES_BIT3(b0, b1, b2)	(REQUIRES_BIT2(b0, b1) | (1 << (b2)))
#define REQUIRES_BIT4(b0, b1, b2, b3)	(REQUIRES_BIT3(b0, b1, b2) | (1 << (b3)))

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
	struct tagCCASCodeList _asmlist;
	enum EMMArea _where;

	FArray _localuses;
	int _curlocalbytes; /* current local space bytes used */
	int _maxlocalbytes; /* max local space bytes used */
} FCCGenCodeContext;

typedef struct tagCCTripleCode {
	int _opcode;
	struct tagCCDagNode* _result;
	struct tagCCDagNode *_kids[2];
	struct tagCCSymbol* _target;
	int _count;

	int _seqid;
	struct tagCCTripleCode* _prev, * _next;
} FCCTripleCode;


typedef struct tagCCTripleCodeContext {
	struct tagCCTripleCode* _head;
	struct tagCCTripleCode* _tail;
	int _code_seqid;
} FCCTripleCodeContext;

static struct tagCCTripleCode* emit_triple(struct tagCCTripleCodeContext *ctx, int opcode);
static BOOL munch_expr(struct tagCCTripleCodeContext* ctx, FCCIRDagNode* dag);
static BOOL munch_stmt(struct tagCCTripleCodeContext* ctx, FCCIRCode* stmt);

static void block_enter(struct tagCCGenCodeContext* ctx, int seqid, int level);
static void block_leave(struct tagCCGenCodeContext* ctx, int seqid, int level);
static void local_variable(struct tagCCGenCodeContext* ctx, struct tagCCSymbol* id);

static struct tagCCTripleCode* emit_triple(struct tagCCTripleCodeContext* ctx, int opcode)
{
	struct tagCCTripleCode* c;

	c = mm_alloc_area(sizeof(struct tagCCTripleCode), CC_MM_TEMPPOOL);
	if (!c) {
		logger_output_s("error: out of memory at %s:%d\n", __FILE__, __LINE__);
		return NULL;
	}

	util_memset(c, 0, sizeof(struct tagCCTripleCode));

	c->_opcode = opcode;
	c->_seqid = ctx->_code_seqid++;

	/* append to list */
	if (ctx->_tail) {
		c->_prev = ctx->_tail;
		ctx->_tail->_next = c;
	}
	else {
		ctx->_head = c;
	}

	ctx->_tail = c;
	return c;
}

static void gen_context_init(struct tagCCGenCodeContext* ctx, enum EMMArea where)
{
	ctx->_asmlist._head = NULL;
	ctx->_asmlist._tail = NULL;
	ctx->_where = where;
	ctx->_curlocalbytes = 0;
	ctx->_maxlocalbytes = 0;
	
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

static struct tagCCASCode* emit_as(struct tagCCGenCodeContext* ctx, int opcode)
{
	struct tagCCASCode* asm;

	if (!(asm = cc_as_newcode(ctx->_where))) {
		return NULL;
	}

	asm->_opcode = opcode;
	cc_as_codelist_append(&ctx->_asmlist, asm);
	
	return asm;
}

BOOL emit_store_regvalue(struct tagCCGenCodeContext* ctx, struct tagCCDagNode* dag, int tmpoffset)
{
	struct tagCCASCode* asm;

	if (!(asm = emit_as(ctx, X86_MOV))) { return FALSE; }

	asm->_dst._format = FormatSIB;
	asm->_dst._tycode = IR_OPTY0(dag->_op);
	asm->_dst._u._sib._basereg = X86_EBP;
	asm->_dst._u._sib._displacement2 = tmpoffset;
	asm->_src._format = FormatReg;
	asm->_src._tycode = IR_OPTY0(dag->_op);
	asm->_src._u._regs[0] = dag->_x._loc._registers[0];
	asm->_src._u._regs[1] = dag->_x._loc._registers[1];

	return TRUE;
}

BOOL emit_restore_tmpvalue(struct tagCCGenCodeContext* ctx, struct tagCCDagNode* dag, int regs[2])
{
	struct tagCCASCode* asm;
	int i, tmpoffset = -1;

	for (i = 0; i < ctx->_localuses._elecount; ++i)
	{
		struct tagCCLocalUseEntry* p;

		p = array_element(&ctx->_localuses, i);
		if (p->_kind == LOCAL_TMP)
		{
			if (p->_x._tmp._dag == dag)
			{
				tmpoffset = p->_x._tmp._offset;
				p->_x._tmp._dag = NULL;
				break;
			}
		}
	} /* end for */

	if (!(asm = emit_as(ctx, X86_MOV))) { return FALSE; }
	asm->_src._format = FormatInSIB;
	asm->_src._tycode = IR_OPTY0(dag->_op);
	asm->_src._u._sib._basereg = X86_EBP;
	asm->_src._u._sib._displacement2 = tmpoffset;
	asm->_dst._format = FormatReg;
	asm->_dst._tycode = IR_OPTY0(dag->_op);
	asm->_dst._u._regs[0] = dag->_x._loc._registers[0];
	asm->_dst._u._regs[1] = dag->_x._loc._registers[1];

	return TRUE;
}

/* mark dag last reference id */
static void mark_lastref(FCCIRDagNode* dag, int seqid)
{
	dag->_lastref = seqid;
	if (IR_OP(dag->_op) == IR_INDIR)
	{
		dag->_kids[0]->_lastref = seqid;
	}
}

/* generate code for expression */
static BOOL munch_expr(struct tagCCTripleCodeContext* ctx, FCCIRDagNode* dag)
{
	struct tagCCTripleCode* triple;

	if (dag->_x._isemitted) 
	{ 
		return TRUE; 
	}

	switch (IR_OP(dag->_op))
	{
	case IR_CONST:
		dag->_x._recalable = 1;
		break;
	case IR_ASSIGN:
	{
		if (!munch_expr(ctx, dag->_kids[1])) { return FALSE; }
		if (!munch_expr(ctx, dag->_kids[0])) { return FALSE; }

		if (!(triple = emit_triple(ctx, X86_MOV))) { return FALSE; }
		mark_lastref(dag->_kids[0], triple->_seqid);
		mark_lastref(dag->_kids[1], triple->_seqid);
		triple->_kids[0] = dag->_kids[0];
		triple->_kids[1] = dag->_kids[1];
	}
		break;
	case IR_ADD:
	case IR_SUB:
	case IR_MUL:
	case IR_DIV:
	case IR_MOD:
	case IR_LSHIFT:
	case IR_RSHIFT:
	case IR_BITAND:
	case IR_BITOR:
	case IR_BITXOR:
	{
		int opcode, irop0, irop1;

		switch (IR_OP(dag->_op))
		{
		case IR_ADD: opcode = X86_ADD; break;
		case IR_SUB: opcode = X86_SUB; break;
		case IR_MUL: opcode = X86_MUL; break;
		case IR_DIV: opcode = X86_DIV; break;
		case IR_MOD: opcode = X86_MOD; break;
		case IR_LSHIFT: opcode = X86_LSH; break;
		case IR_RSHIFT: opcode = X86_RSH; break;
		case IR_BITAND: opcode = X86_AND; break;
		case IR_BITOR:  opcode = X86_OR; break;
		case IR_BITXOR: opcode = X86_XOR; break;
		}

		if (!munch_expr(ctx, dag->_kids[0])) { return FALSE; }
		if (!munch_expr(ctx, dag->_kids[1])) { return FALSE; }

		irop0 = dag->_kids[0]->_op;
		irop1 = dag->_kids[1]->_op;
		if ((opcode == X86_ADD || opcode == X86_SUB)
			&& (IsAddrOp(irop0) && IR_OP(irop1) == IR_CONST))
		{
			dag->_x._recalable = 1;
		}
		else
		{
			if (!(triple = emit_triple(ctx, opcode))) { return FALSE; }

			dag->_x._isemitted = 1;
			mark_lastref(dag, triple->_seqid);
			mark_lastref(dag->_kids[0], triple->_seqid);
			mark_lastref(dag->_kids[1], triple->_seqid);

			triple->_result = dag;
			triple->_kids[0] = dag->_kids[0];
			triple->_kids[1] = dag->_kids[1];
		}
	}
		break;
	case IR_NEG:
	case IR_BCOM:
	case IR_CVT:
	{
		int opcode;

		switch (IR_OP(dag->_op))
		{
		case IR_NEG: opcode = X86_NEG; break;
		case IR_BCOM:opcode = X86_NOT; break;
		case IR_CVT: opcode = X86_CVT; break;
		}

		if (!munch_expr(ctx, dag->_kids[0])) { return FALSE; }
		if (!(triple = emit_triple(ctx, opcode))) { return FALSE; }
		dag->_x._isemitted = 1;
		mark_lastref(dag, triple->_seqid);
		mark_lastref(dag->_kids[0], triple->_seqid);
		triple->_result = dag;
		triple->_kids[0] = dag->_kids[0];
	}
		break;
	case IR_INDIR:
	{
		FCCIRDagNode* kid = dag->_kids[0];
		if (IR_OP(kid->_op) == IR_INDIR)
		{
			if (!kid->_x._isemitted)
			{
				if (!munch_expr(ctx, kid->_kids[0])) { return FALSE; }
				if (!(triple = emit_triple(ctx, X86_LOAD))) { return FALSE; }
				kid->_x._isemitted = 1;
				mark_lastref(kid, triple->_seqid);
				mark_lastref(kid->_kids[0], triple->_seqid);
				triple->_result = kid;
				triple->_kids[0] = kid->_kids[0];
			}
		}
		else
		{
			if (!munch_expr(ctx, kid)) { return FALSE; }
			dag->_x._recalable = kid->_x._recalable;
		}
	}
		break;
	case IR_ADDRG:
	case IR_ADDRF:
		dag->_x._recalable = 1; break;
	case IR_ADDRL:
	{
		dag->_x._recalable = 1;
		if (dag->_symbol->_temporary && !dag->_symbol->_defined)
		{
			dag->_symbol->_defined = 1;
			if (!(triple = emit_triple(ctx, X86_LOCALVAR))) { return FALSE; }
			triple->_target = dag->_symbol;
		}
	}
		break;
	case IR_CALL:
	{
		if (!munch_expr(ctx, dag->_kids[0])) { return FALSE; }
		if (!(triple = emit_triple(ctx, X86_CALL))) { return FALSE; }
		dag->_x._isemitted = 1;
		mark_lastref(dag, triple->_seqid);
		mark_lastref(dag->_kids[0], triple->_seqid);
		triple->_result = dag;
		triple->_kids[0] = dag->_kids[0];
	}
		break;
	default:
		assert(0); break;
	}

	return TRUE;
}

/* generate code for ir-code */
static BOOL munch_stmt(struct tagCCTripleCodeContext* ctx, FCCIRCode* ircode)
{
	struct tagCCTripleCode* triple;
	struct tagCCDagNode* dag;

	switch (ircode->_op)
	{
	case IR_ARG:
	{
		dag = ircode->_u._expr->_dagnode;
		if (!munch_expr(ctx, dag)) { return FALSE; }
		if (!(triple = emit_triple(ctx, X86_PUSH))) { return FALSE; }

		triple->_kids[0] = dag;
		mark_lastref(dag, triple->_seqid);
	}
		break;
	case IR_EXP:
	{
		dag = ircode->_u._expr->_dagnode;
		if (!munch_expr(ctx, dag)) { return FALSE; }
	}
		break;
	case IR_JMP:
	{
		if (!(triple = emit_triple(ctx, X86_JMP))) { return FALSE; }
		triple->_target = ircode->_u._jmp._tlabel;
	}
		break;
	case IR_CJMP:
	{
		int opcode;

		dag = ircode->_u._jmp._cond->_dagnode;
		switch (IR_OP(dag->_op))
		{
		case IR_EQUAL: opcode = X86_JE; break;
		case IR_UNEQ:  opcode = X86_JNE; break;
		case IR_GREAT: opcode = X86_JG; break;
		case IR_GEQ:   opcode = X86_JGE; break;
		case IR_LESS:  opcode = X86_JL; break;
		case IR_LEQ:   opcode = X86_JLE; break;
		}

		if (!munch_expr(ctx, dag->_kids[0])) { return FALSE; }
		if (!munch_expr(ctx, dag->_kids[1])) { return FALSE; }
		
		if (!(triple = emit_triple(ctx, opcode))) { return FALSE; }
		mark_lastref(dag->_kids[0], triple->_seqid);
		mark_lastref(dag->_kids[1], triple->_seqid);
		triple->_kids[0] = dag->_kids[0];
		triple->_kids[1] = dag->_kids[1];
		triple->_target = ircode->_u._jmp._tlabel;

		if (ircode->_u._jmp._flabel)
		{
			if (!(triple = emit_triple(ctx, X86_JMP))) { return FALSE; }
			triple->_target = ircode->_u._jmp._flabel;
		}
	}
		break;
	case IR_LABEL:
	{
		if (!(triple = emit_triple(ctx, X86_LABEL))) { return FALSE; }
		triple->_target = ircode->_u._label;
	}
		break;
	case IR_BLKBEG:
		{
			if (!(triple = emit_triple(ctx, X86_BLKENTER))) { return FALSE; }
			triple->_count = ircode->_u._blklevel;
		}
		break;
	case IR_BLKEND:
		{
			if (!(triple = emit_triple(ctx, X86_BLKLEAVE))) { return FALSE; }
			triple->_count = ircode->_u._blklevel;
		}
		break;
	case IR_LOCVAR:
		{
			if (!(triple = emit_triple(ctx, X86_LOCALVAR))) { return FALSE; }
			triple->_target = ircode->_u._id;
		}
		break;
	case IR_RET:
	{
		if (ircode->_u._ret._expr) 
		{
			dag = ircode->_u._ret._expr->_dagnode;
			if (!munch_expr(ctx, dag)) { return FALSE; }
			if (!(triple = emit_triple(ctx, X86_MOVERET))) { return FALSE; }
			mark_lastref(dag, triple->_seqid);
			triple->_kids[0] = dag;
		}

		if (ircode->_u._ret._exitlab)
		{
			if (!(triple = emit_triple(ctx, X86_JMP))) { return FALSE; }
			triple->_target = ircode->_u._ret._exitlab;
		}
	}
		break;
	case IR_ZERO:
	{
		dag = ircode->_u._zero._addr->_dagnode;
		if (!munch_expr(ctx, dag)) { return FALSE; }
		if (!(triple = emit_triple(ctx, X86_ZERO_M))) { return FALSE; }
		mark_lastref(dag, triple->_seqid);
		triple->_kids[0] = dag;
		triple->_count = ircode->_u._zero._bytes;
	}
		break;
	case IR_FENTER:
	{
		if (!(triple = emit_triple(ctx, X86_PROLOGUE))) { return FALSE; }
	}
		break;
	case IR_FEXIT:
	{
		if (!(triple = emit_triple(ctx, X86_EPILOGUE))) { return FALSE; }
	}
		break;
	default:
		assert(0); break;
	}

	return TRUE;
}

static BOOL cc_gen_triplecodes_irlist(struct tagCCTripleCodeContext*ctx, FCCIRCodeList* irlist)
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

static BOOL cc_gen_triplecodes(struct tagCCTripleCodeContext* ctx, struct tagCCIRBasicBlock* bb)
{
	for (; bb; bb = bb->_next)
	{
		if (!cc_gen_triplecodes_irlist(ctx, &bb->_codes)) {
			return FALSE;
		}
	} /* end for bb */

	return TRUE;
}

/* translate dag to asm-operand */
static BOOL cc_gen_dag2operand(struct tagCCDagNode *dag, struct tagCCASOperand *operand)
{
	int irop0, irop1;

	util_memset(operand, 0, sizeof(*operand));
	if (dag->_x._inregister)
	{
		operand->_format = FormatReg;
		operand->_tycode = IR_OPTY0(dag->_op);
		operand->_u._regs[0] = dag->_x._loc._registers[0];
		operand->_u._regs[1] = dag->_x._loc._registers[1];

		return TRUE;
	}
	else if (dag->_x._intemporary)
	{
		operand->_format = FormatInSIB;
		operand->_tycode = IR_OPTY0(dag->_op);
		operand->_u._sib._basereg = X86_EBP;
		operand->_u._sib._displacement2 = dag->_x._loc._temp_offset;
		
		return TRUE;
	}
	else
	{
		switch (IR_OP(dag->_op))
		{
		case IR_ADD:
		case IR_SUB:
		{
			irop0 = dag->_kids[0]->_op;
			irop1 = dag->_kids[1]->_op;
			if (IsAddrOp(irop0) && IR_OP(irop1) == IR_CONST)
			{
				struct tagCCSymbol* id = dag->_kids[0]->_symbol;
				struct tagCCSymbol* cnst = dag->_kids[1]->_symbol;
				int k = cnst->_u._cnstval._pointer;

				if (IR_OP(dag->_op) == IR_SUB) { k = -k; }
				if (IR_OP(irop0) == IR_ADDRG)
				{
					operand->_format = FormatSIB;
					operand->_tycode = IR_OPTY0(dag->_op);
					operand->_u._sib._displacement = id;
					operand->_u._sib._displacement2 = k;
				}
				else
				{
					operand->_format = FormatSIB;
					operand->_tycode = IR_OPTY0(dag->_op);
					operand->_u._sib._basereg = X86_EBP;
					operand->_u._sib._displacement2 = id->_x._frame_offset + k;
				}

				return TRUE;
			}
		}
			break;
		case IR_CONST:
		{
			operand->_format = FormatImm;
			operand->_tycode = IR_OPTY0(dag->_op);
			operand->_u._imm = dag->_symbol;
			return TRUE;
		}
			break;
		case IR_INDIR:
		{
			struct tagCCASOperand addr = { 0 };
			if (!cc_gen_dag2operand(dag->_kids[0], &addr)) { return FALSE; }
			if (addr._format == FormatReg)
			{
				operand->_format = FormatInSIB;
				operand->_tycode = IR_OPTY0(dag->_op);
				operand->_u._sib._basereg = operand->_u._regs[0];
				
				return TRUE;
			}
			if (addr._format == FormatSIB)
			{
				operand->_format = FormatInSIB;
				operand->_tycode = IR_OPTY0(dag->_op);
				operand->_u._sib = addr._u._sib;
				
				return TRUE;
			}
			if (addr._format == FormatInSIB)
			{
				operand->_format = FormatInSIB2;
				operand->_tycode = IR_OPTY0(dag->_op);
				operand->_u._sib = addr._u._sib;
				
				return TRUE;
			}
			if (addr._format == FormatImm)
			{
				operand->_format = FormatInSIB;
				operand->_tycode = IR_OPTY0(dag->_op);
				operand->_u._sib._displacement2 = addr._u._imm->_u._cnstval._pointer;
				
				return TRUE;
			}
		}
			break;
		default:
			break;
		}
	}

	return FALSE;
}

static BOOL cc_gen_InInSIB2InSIB(struct tagCCGenCodeContext* ctx, struct tagCCDagNode* dag, struct tagCCASOperand* operand, int seqid)
{
	if (operand->_format == FormatInSIB2)
	{
		struct tagCCDagNode* kid = dag->_kids[0];
		int regs[2] = { X86_NIL, X86_NIL};
		
		assert(IR_OP(kid->_op) == IR_INDIR && kid->_x._intemporary);
		regs[0] = cc_reg_get(ctx, seqid, NORMAL_ADDR_X86REGS, kid, 0);
		if (regs[0] == X86_NIL) { return FALSE; }
		if (!emit_restore_tmpvalue(ctx, kid, regs)) { return FALSE; }
		dag->_x._inregister = 1;
	
		operand->_format = FormatInSIB;
		operand->_tycode = IR_OPTY0(dag->_op);
		operand->_u._sib._basereg = regs[0];
		operand->_u._sib._indexreg = X86_NIL;
		operand->_u._sib._scale = 0;
		operand->_u._sib._displacement2 = 0;
		operand->_u._sib._displacement = NULL;
	}

	return TRUE;
}

static BOOL cc_gen_InSIB2SIB(struct tagCCGenCodeContext* ctx, struct tagCCDagNode* dag, struct tagCCASOperand* operand, int seqid)
{
	struct tagCCASCode* as;

	if (operand->_format == FormatInSIB)
	{
		struct tagCCDagNode* kid = dag->_kids[0];
		int regs[2] = { X86_NIL, X86_NIL };

		regs[0] = cc_reg_get(ctx, seqid, NORMAL_ADDR_X86REGS, kid, 0);
		if (regs[0] == X86_NIL) { return FALSE; }
		if (!(as = emit_as(ctx, X86_MOV))) { return FALSE; }
		as->_src = *operand;
		as->_dst._format = FormatReg;
		as->_dst._tycode = IR_OPTY0(dag->_op);
		as->_dst._u._regs[0] = regs[0];
		as->_dst._u._regs[1] = regs[1];
		as->_count = kid->_typesize;

		dag->_x._inregister = 1;
		operand->_format = FormatSIB;
		operand->_tycode = IR_OPTY0(dag->_op);
		operand->_u._sib._basereg = regs[0];
		operand->_u._sib._indexreg = X86_NIL;
		operand->_u._sib._scale = 0;
		operand->_u._sib._displacement2 = 0;
		operand->_u._sib._displacement = NULL;
	}

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
static BOOL cc_gen_triple_to_x86(struct tagCCGenCodeContext* ctx, struct tagCCTripleCode* triple)
{
	const int seqid = triple->_seqid;
	struct tagCCASCode* as;
	struct tagCCDagNode* result, *lhs, *rhs;
	int tycode;

	switch (triple->_opcode)
	{
	case X86_BLKENTER:
		block_enter(ctx, seqid, triple->_count); break;
	case X86_BLKLEAVE:
		block_leave(ctx, seqid, triple->_count); break;
	case X86_LOCALVAR:
		local_variable(ctx, triple->_target); break;
	case X86_MOVERET:
	{
		lhs = triple->_kids[0];
		tycode = IR_OPTY0(lhs->_op);
		switch (tycode)
		{
		case IR_S8:
		case IR_U8:
		case IR_S16:
		case IR_U16:
		case IR_S32:
		case IR_U32:
		case IR_PTR:
		{
			struct tagCCASOperand src = { 0 };
			if (!cc_gen_dag2operand(lhs, &src)) { return FALSE; }
			if (!cc_gen_InInSIB2InSIB(ctx, lhs, &src, seqid)) { return FALSE; }
			if (!(as = emit_as(ctx, X86_MOV))) { return FALSE; }
			as->_dst._format = FormatReg;
			as->_dst._tycode = src._tycode;
			as->_dst._u._regs[0] = X86_EAX;
			as->_dst._u._regs[1] = X86_NIL;
			as->_src = src;
			as->_count = lhs->_typesize;
		}
			break;	
		case IR_S64:
		case IR_U64:
		{
			struct tagCCASOperand src = { 0 };
			if (!cc_gen_dag2operand(lhs, &src)) { return FALSE; }
			if (!cc_gen_InInSIB2InSIB(ctx, lhs, &src, seqid)) { return FALSE; }
			if (!(as = emit_as(ctx, X86_MOV))) { return FALSE; }
			as->_dst._format = FormatReg;
			as->_dst._tycode = src._tycode;
			as->_dst._u._regs[0] = X86_EAX;
			as->_dst._u._regs[1] = X86_EDX;
			as->_src = src;
			as->_count = lhs->_typesize;
		}
			break;
		case IR_F32:
		case IR_F64:
		{
			struct tagCCASOperand src = { 0 };
			if (!cc_gen_dag2operand(lhs, &src)) { return FALSE; }
			if (!cc_gen_InInSIB2InSIB(ctx, lhs, &src, seqid)) { return FALSE; }
			if (!(as = emit_as(ctx, X86_MOV))) { return FALSE; }
			as->_dst._format = FormatReg;
			as->_dst._tycode = src._tycode;
			as->_dst._u._regs[0] = X86_ST0;
			as->_dst._u._regs[1] = X86_NIL;
			as->_src = src;
			as->_count = lhs->_typesize;
		}
			break;
		default:
			assert(0); break;
		}
	}
		break;
	case X86_LOAD:
	{
		lhs = triple->_kids[0];
		tycode = IR_OPTY0(lhs->_op);
		switch (tycode)
		{
		case IR_S8:
		case IR_U8:
		case IR_S16:
		case IR_U16:
		case IR_S32:
		case IR_U32:
		case IR_PTR:
		{
			struct tagCCASOperand src = { 0 };
			int regs[2] = { X86_NIL, X86_NIL };

			if (lhs->_x._inregister) { return TRUE; }
			if (!cc_gen_dag2operand(lhs, &src)) { return FALSE; }
			if (!cc_gen_InInSIB2InSIB(ctx, lhs, &src, seqid)) { return FALSE; }

			regs[0] = cc_reg_get(ctx, seqid, NORMAL_ADDR_X86REGS, lhs, 0);
			if (regs[0] == X86_NIL) { return FALSE; }
			if (!(as = emit_as(ctx, X86_MOV))) { return FALSE; }
			as->_src = src;
			as->_dst._format = FormatReg;
			as->_dst._tycode = tycode;
			as->_dst._u._regs[0] = regs[0];
			as->_dst._u._regs[1] = regs[1];
			as->_count = lhs->_typesize;
		}
			break;
		default:
			assert(0); break;
		}
	}
		break;
	case X86_LABEL:
	{
		if (!(as = emit_as(ctx, X86_LABEL))) { return FALSE; }
		as->_target = triple->_target;
	}
		break;
	case X86_OR:
	case X86_XOR:
	case X86_AND:
	case X86_LSH:
	case X86_RSH:
	case X86_ADD:
	case X86_SUB:
	case X86_MUL:
	case X86_DIV:
	case X86_MOD:
	case X86_NEG:
	case X86_NOT:

	case X86_JZ:
	case X86_JNZ:
	case X86_JE:
	case X86_JNE:
	case X86_JG:
	case X86_JL:
	case X86_JGE:
	case X86_JLE:
	case X86_JMP:

	/* convert */
	case X86_CVT:
	case X86_MOV:

	case X86_PUSH:
	case X86_CALL:

	case X86_PROLOGUE:
	case X86_EPILOGUE:

	/* set memory to zero */
	case X86_ZERO_M:

	default:
		assert(0); break;
	}

	return TRUE;
}

static BOOL cc_gen_asmcodes(struct tagCCGenCodeContext* ctx, FArray* caller, FArray* callee, struct tagCCTripleCode* triple)
{
	int offset, i;
	
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

	for (; triple; triple=triple->_next)
	{
		if (!cc_gen_triple_to_x86(ctx, triple)) { return FALSE; }
	} /* end for */

	return TRUE;
}

BOOL cc_gen_dumpfunction(struct tagCCContext* ctx, struct tagCCSymbol* func, FArray* caller, FArray* callee, struct tagCCIRBasicBlock* body)
{
	struct tagCCTripleCodeContext triplectx;
	struct tagCCGenCodeContext genctx;

	return TRUE;

	triplectx._head = NULL;
	triplectx._tail = NULL;
	triplectx._code_seqid = 0;
	if (!cc_gen_triplecodes(&triplectx, body)) {
		logger_output_s("failed to generate triple codes for function '%s'\n", func->_name);
		return FALSE;
	}

	gen_context_init(&genctx, CC_MM_TEMPPOOL);
	if (!cc_gen_asmcodes(&genctx, caller, callee, triplectx._head)) {
		logger_output_s("failed to generate x86 codes for function '%s'\n", func->_name);
		return FALSE;
	}

	return TRUE;
}
