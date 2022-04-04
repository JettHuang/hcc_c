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

	struct tagCCASCode* _adjust_esp;
} FCCGenCodeContext;

typedef struct tagCCTripleCode {
	int _opcode;
	struct tagCCDagNode* _res;
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

static void gen_context_init(struct tagCCGenCodeContext* ctx, struct tagCCASCodeList* asmlist, enum EMMArea where)
{
	ctx->_asmlist = asmlist;
	ctx->_where = where;
	ctx->_curlocalbytes = 0;
	ctx->_maxlocalbytes = 0;
	ctx->_adjust_esp = NULL;
	
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
	cc_as_codelist_append(ctx->_asmlist, asm);
	
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

/* generate code for expression */
static BOOL munch_expr(struct tagCCTripleCodeContext* ctx, FCCIRDagNode* dag)
{
	struct tagCCTripleCode* triple;

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
		dag->_kids[0]->_lastref = triple->_seqid;
		dag->_kids[1]->_lastref = triple->_seqid;
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
			dag->_kids[0]->_lastref = triple->_seqid;
			dag->_kids[1]->_lastref = triple->_seqid;

			triple->_res = dag;
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
		dag->_kids[0]->_lastref = triple->_seqid;
		triple->_res = dag;
		triple->_kids[0] = dag->_kids[0];
	}
		break;
	case IR_INDIR:
	{
		if (!munch_expr(ctx, dag->_kids[0])) { return FALSE; }
		dag->_x._recalable = dag->_kids[0]->_x._recalable;
	}
		break;
	case IR_ADDRG:
	case IR_ADDRF:
	case IR_ADDRL:
	{
		dag->_x._recalable = 1;
	}
		break;
	case IR_CALL:
	{
		if (!munch_expr(ctx, dag->_kids[0])) { return FALSE; }
		if (!(triple = emit_triple(ctx, X86_CALL))) { return FALSE; }
		dag->_x._isemitted = 1;
		dag->_kids[0]->_lastref = triple->_seqid;
		triple->_res = dag;
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
		dag->_lastref = triple->_seqid;
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
		dag->_kids[0]->_lastref = triple->_seqid;
		dag->_kids[1]->_lastref = triple->_seqid;
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
			dag->_lastref = triple->_seqid;
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
		dag->_lastref = triple->_seqid;
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

static BOOL cc_gen_asmcodes_irlist(struct tagCCTripleCodeContext*ctx, FCCIRCodeList* irlist)
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
	struct tagCCTripleCodeContext tripctx;
	struct tagCCGenCodeContext genctx;
	int offset, i;

	tripctx._code_seqid = 0;
	tripctx._head = tripctx._tail = NULL;
	for (; bb; bb = bb->_next)
	{
		if (!cc_gen_asmcodes_irlist(&tripctx, &bb->_codes)) {
			return FALSE;
		}
	} /* end for bb */

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

	return TRUE;
}

BOOL cc_gen_dumpfunction(struct tagCCContext* ctx, struct tagCCSymbol* func, FArray* caller, FArray* callee, struct tagCCIRBasicBlock* body)
{
	struct tagCCASCodeList asmlist = { NULL, NULL };

	return TRUE;

	if (!cc_gen_asmcodes(caller, callee, body, &asmlist, CC_MM_TEMPPOOL)) {
		logger_output_s("failed to generate assemble codes for function '%s'\n", func->_name);
		return FALSE;
	}

	return FALSE;
}
