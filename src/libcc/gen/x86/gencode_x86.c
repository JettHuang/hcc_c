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
	struct tagCCSymbol* _frame_base;

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
static BOOL munch_expr(struct tagCCTripleCodeContext* ctx, struct tagCCDagNode* dag);
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

static BOOL gen_context_init(struct tagCCGenCodeContext* ctx, enum EMMArea where)
{
	struct tagCCSymbol* p;

	ctx->_asmlist._head = NULL;
	ctx->_asmlist._tail = NULL;
	ctx->_where = where;
	ctx->_curlocalbytes = 0;
	ctx->_maxlocalbytes = 0;
	
	array_init(&ctx->_localuses, 128, sizeof(struct tagCCLocalUseEntry), where);

	p = mm_alloc_area(sizeof(struct tagCCSymbol), CC_MM_TEMPPOOL);
	if (!p) { return FALSE; }

	util_memset(p, 0, sizeof(*p));
	p->_name = "framebase";
	p->_sclass = SC_Auto;
	p->_scope = SCOPE_LOCAL;
	p->_generated = 1;

	ctx->_frame_base = p;
	return TRUE;
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

void alloc_temporary_space(struct tagCCGenCodeContext* ctx, struct tagCCDagNode* dag, int curseqid)
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
				(!p->_x._tmp._dag || p->_x._tmp._dag->_lastref < curseqid))
			{
				if (p->_x._tmp._dag)
				{
					p->_x._tmp._dag->_x._inmemory = 0;
					p->_x._tmp._dag->_x._loc._mm._var = NULL;
					p->_x._tmp._dag->_x._loc._mm._offset = 0;
				}

				p->_x._tmp._dag = dag;
				dag->_x._inmemory = 1;
				dag->_x._loc._mm._var = ctx->_frame_base;
				dag->_x._loc._mm._offset = p->_x._tmp._offset;
				return;
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
	
	dag->_x._inmemory = 1;
	dag->_x._loc._mm._var = ctx->_frame_base;
	dag->_x._loc._mm._offset = e._x._tmp._offset;
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

BOOL emit_store_r2staticmem(struct tagCCGenCodeContext* ctx, struct tagCCDagNode* dag)
{
	struct tagCCASCode* asm;
	struct tagCCSymbol* var;

	if (!(asm = emit_as(ctx, X86_MOV))) { return FALSE; }

	assert(dag->_x._inmemory);
	assert(dag->_x._inregister);

	var = dag->_x._loc._mm._var;
	asm->_dst._format = FormatSIB;
	asm->_dst._tycode = IR_OPTY0(dag->_op);
	if (var->_scope == SCOPE_PARAM || var->_scope == SCOPE_LOCAL) {
		asm->_dst._u._sib._basereg = X86_EBP;
		asm->_dst._u._sib._displacement2 = var->_x._frame_offset + dag->_x._loc._mm._offset;
	}
	else {
		asm->_dst._u._sib._displacement = var;
		asm->_dst._u._sib._displacement2 = dag->_x._loc._mm._offset;
	}
	asm->_src._format = FormatReg;
	asm->_src._tycode = IR_OPTY0(dag->_op);
	asm->_src._u._regs[0] = dag->_x._loc._regs[0];
	asm->_src._u._regs[1] = dag->_x._loc._regs[1];
	asm->_count = dag->_typesize;

	return TRUE;
}

BOOL emit_load_staticmem2r(struct tagCCGenCodeContext* ctx, struct tagCCDagNode* dag)
{
	struct tagCCASCode* asm;
	struct tagCCSymbol* var;

	if (!(asm = emit_as(ctx, X86_MOV))) { return FALSE; }

	assert(dag->_x._inmemory);
	assert(dag->_x._inregister);

	var = dag->_x._loc._mm._var;
	asm->_src._format = FormatInSIB;
	asm->_src._tycode = IR_OPTY0(dag->_op);
	if (var->_scope == SCOPE_PARAM || var->_scope == SCOPE_LOCAL) {
		asm->_src._u._sib._basereg = X86_EBP;
		asm->_src._u._sib._displacement2 = var->_x._frame_offset + dag->_x._loc._mm._offset;
	}
	else {
		asm->_src._u._sib._displacement = var;
		asm->_src._u._sib._displacement2 = dag->_x._loc._mm._offset;
	}
	asm->_dst._format = FormatReg;
	asm->_dst._tycode = IR_OPTY0(dag->_op);
	asm->_dst._u._regs[0] = dag->_x._loc._regs[0];
	asm->_dst._u._regs[1] = dag->_x._loc._regs[1];
	asm->_count = dag->_typesize;

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

static void check_static_indir(struct tagCCDagNode* dag)
{
	if (IR_OP(dag->_op) == IR_INDIR && dag->_x._recalable)
	{
		struct tagCCDagNode* kid = dag->_kids[0];

		switch (IR_OP(kid->_op))
		{
		case IR_ADD:
		case IR_SUB:
		{
			struct tagCCDagNode* lhs, * rhs;

			lhs = kid->_kids[0];
			rhs = kid->_kids[1];
			if (IsAddrOp(lhs->_op) && IR_OP(rhs->_op) == IR_CONST)
			{
				struct tagCCSymbol* var = lhs->_symbol;
				struct tagCCSymbol* cnst = rhs->_symbol;
				int k = cnst->_u._cnstval._pointer;

				if (IR_OP(kid->_op) == IR_SUB) { k = -k; }

				dag->_x._inmemory = 1;
				dag->_x._loc._mm._var = var;
				dag->_x._loc._mm._offset = k;
			}
		}
		break;
		case IR_CONST:
		{
			dag->_x._inmemory = 1;
			dag->_x._loc._mm._var = NULL;
			dag->_x._loc._mm._offset = kid->_symbol->_u._cnstval._pointer;
		}
		break;
		default:
			break;
		}
	} /* end if */
}

/* generate code for expression */
static BOOL munch_expr(struct tagCCTripleCodeContext* ctx, struct tagCCDagNode* dag)
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
		struct tagCCDagNode* lhs, * rhs;
		int opcode;

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

		lhs = dag->_kids[0];
		rhs = dag->_kids[1];
		if (!munch_expr(ctx, lhs)) { return FALSE; }
		if (!munch_expr(ctx, rhs)) { return FALSE; }

		if ((opcode == X86_ADD || opcode == X86_SUB)
			&& (IsAddrOp(lhs->_op) && IR_OP(rhs->_op) == IR_CONST))
		{
			dag->_x._recalable = 1;
		}
		else
		{
			if (!(triple = emit_triple(ctx, opcode))) { return FALSE; }

			dag->_x._isemitted = 1;
			mark_lastref(dag, triple->_seqid);
			mark_lastref(lhs, triple->_seqid);
			mark_lastref(rhs, triple->_seqid);

			triple->_result = dag;
			triple->_kids[0] = lhs;
			triple->_kids[1] = rhs;
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
				if (!munch_expr(ctx, kid)) { return FALSE; }
				if (!(triple = emit_triple(ctx, X86_LOAD))) { return FALSE; }
				kid->_x._isemitted = 1;
				mark_lastref(kid, triple->_seqid);
				triple->_result = kid;
				triple->_kids[0] = kid;
			}
		}
		else
		{
			if (!munch_expr(ctx, kid)) { return FALSE; }
			dag->_x._recalable = kid->_x._recalable;
			check_static_indir(dag);
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
static BOOL cc_get_operand(struct tagCCDagNode *dag, struct tagCCASOperand *operand)
{
	util_memset(operand, 0, sizeof(*operand));
	if (dag->_x._inregister)
	{
		operand->_format = FormatReg;
		operand->_tycode = IR_OPTY0(dag->_op);
		operand->_u._regs[0] = dag->_x._loc._regs[0];
		operand->_u._regs[1] = dag->_x._loc._regs[1];
		return TRUE;
	}
	else if (dag->_x._inmemory)
	{
		struct tagCCSymbol* var = dag->_x._loc._mm._var;

		operand->_format = FormatInSIB;
		operand->_tycode = IR_OPTY0(dag->_op);
		if (var->_scope == SCOPE_PARAM || var->_scope == SCOPE_LOCAL) {
			operand->_u._sib._basereg = X86_EBP;
			operand->_u._sib._displacement2 = var->_x._frame_offset + dag->_x._loc._mm._offset;
		}
		else {
			operand->_u._sib._displacement = var;
			operand->_u._sib._displacement2 = dag->_x._loc._mm._offset;
		}

		return TRUE;
	}
	else
	{
		switch (IR_OP(dag->_op))
		{
		case IR_ADD:
		case IR_SUB:
		{
			struct tagCCDagNode* lhs, * rhs;

			lhs = dag->_kids[0];
			rhs = dag->_kids[1];
			if (IsAddrOp(lhs->_op) && IR_OP(rhs->_op) == IR_CONST)
			{
				struct tagCCSymbol* id = lhs->_symbol;
				struct tagCCSymbol* cnst = rhs->_symbol;
				int k = cnst->_u._cnstval._pointer;

				if (IR_OP(dag->_op) == IR_SUB) { k = -k; }
				if (IR_OP(lhs->_op) == IR_ADDRG)
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
		case IR_ADDRG:
		{
			operand->_format = FormatSIB;
			operand->_tycode = IR_OPTY0(dag->_op);
			operand->_u._sib._displacement = dag->_symbol;
			
			return TRUE;
		}
			break;
		case IR_ADDRF:
		case IR_ADDRL:
		{
			operand->_format = FormatSIB;
			operand->_tycode = IR_OPTY0(dag->_op);
			operand->_u._sib._basereg = X86_EBP;
			operand->_u._sib._displacement2 = dag->_symbol->_x._frame_offset;

			return TRUE;
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
			if (!cc_get_operand(dag->_kids[0], &addr)) { return FALSE; }
			if (addr._format == FormatReg)
			{
				operand->_format = FormatInSIB;
				operand->_tycode = IR_OPTY0(dag->_op);
				operand->_u._sib._basereg = addr._u._regs[0];
				
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

static int cc_get_regflags_bytype(int tycode)
{
	switch (tycode)
	{
	case IR_S8:
	case IR_U8:
	case IR_S16:
	case IR_U16:
		return NORMAL_X86REGS;
	case IR_S32:
	case IR_U32:
	case IR_PTR:
		return NORMAL_ADDR_X86REGS;
	default:
		break;
	}

	return NORMAL_ADDR_X86REGS;
}

static BOOL cc_convert_insib2_to_insib(struct tagCCGenCodeContext* ctx, struct tagCCDagNode* dag, struct tagCCASOperand* operand, int seqid)
{
	if (operand->_format == FormatInSIB2)
	{
		struct tagCCDagNode* kid = dag->_kids[0];
		int regs[2] = { X86_NIL, X86_NIL};
		
		assert(IR_OP(kid->_op) == IR_INDIR && kid->_x._inmemory);
		regs[0] = cc_reg_alloc(ctx, seqid, NORMAL_ADDR_X86REGS);
		if (regs[0] == X86_NIL) { return FALSE; }

		cc_reg_make_associated(regs[0], kid, 0);
		if (!emit_load_staticmem2r(ctx, kid)) { return FALSE; }
	
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

static BOOL cc_convert_insib_reg_imm_to_sib(struct tagCCGenCodeContext* ctx, struct tagCCDagNode* dag, struct tagCCASOperand* operand, int seqid)
{
	struct tagCCASCode* as;

	if (operand->_format == FormatInSIB)
	{
		int regs[2] = { X86_NIL, X86_NIL };

		regs[0] = cc_reg_alloc(ctx, seqid, NORMAL_ADDR_X86REGS);
		if (regs[0] == X86_NIL) { return FALSE; }

		cc_reg_make_associated(regs[0], dag, 0);
		if (!(as = emit_as(ctx, X86_MOV))) { return FALSE; }

		as->_src = *operand;
		as->_dst._format = FormatReg;
		as->_dst._tycode = IR_OPTY0(dag->_op);
		as->_dst._u._regs[0] = regs[0];
		as->_count = dag->_typesize;

		operand->_format = FormatSIB;
		operand->_tycode = IR_OPTY0(dag->_op);
		operand->_u._sib._basereg = regs[0];
		operand->_u._sib._indexreg = X86_NIL;
		operand->_u._sib._scale = 0;
		operand->_u._sib._displacement2 = 0;
		operand->_u._sib._displacement = NULL;
	}
	else if (operand->_format == FormatReg)
	{
		int regid = operand->_u._regs[0];

		operand->_format = FormatSIB;
		operand->_tycode = IR_OPTY0(dag->_op);
		operand->_u._sib._basereg = regid;
		operand->_u._sib._indexreg = X86_NIL;
		operand->_u._sib._scale = 0;
		operand->_u._sib._displacement2 = 0;
		operand->_u._sib._displacement = NULL;
	}
	else if (operand->_format == FormatImm)
	{
		int ptr = operand->_u._imm->_u._cnstval._pointer;

		operand->_format = FormatSIB;
		operand->_tycode = IR_OPTY0(dag->_op);
		operand->_u._sib._basereg = X86_NIL;
		operand->_u._sib._indexreg = X86_NIL;
		operand->_u._sib._scale = 0;
		operand->_u._sib._displacement2 = ptr;
		operand->_u._sib._displacement = NULL;
	}

	return TRUE;
}

static BOOL cc_convert_insib_sib_imm_to_reg(struct tagCCGenCodeContext* ctx, struct tagCCDagNode* dag, struct tagCCASOperand* operand, int seqid)
{
	struct tagCCASCode* as;
	int tycode;

	tycode = IR_OPTY0(dag->_op);
	if (operand->_format == FormatInSIB || operand->_format == FormatImm || operand->_format == FormatSIB)
	{
		int regs[2] = { X86_NIL, X86_NIL };

		regs[0] = cc_reg_alloc(ctx, seqid, NORMAL_ADDR_X86REGS);
		if (regs[0] == X86_NIL) { return FALSE; }
		cc_reg_make_associated(regs[0], dag, 0);
		if (tycode == IR_S64 || tycode == IR_U64)
		{
			regs[1] = cc_reg_alloc(ctx, seqid, NORMAL_ADDR_X86REGS);
			if (regs[1] == X86_NIL) { return FALSE; }
			cc_reg_make_associated(regs[1], dag, 1);
		}

		if (!(as = emit_as(ctx, X86_MOV))) { return FALSE; }

		as->_src = *operand;
		as->_dst._format = FormatReg;
		as->_dst._tycode = IR_OPTY0(dag->_op);
		as->_dst._u._regs[0] = regs[0];
		as->_dst._u._regs[1] = regs[1];
		as->_count = dag->_typesize;

		*operand = as->_dst;
	}

	return TRUE;
}

static BOOL cc_is_integer(int tycode)
{
	switch (tycode)
	{
	case IR_S8:
	case IR_U8:
	case IR_S16:
	case IR_U16:
	case IR_S32:
	case IR_U32:
	case IR_S64:
	case IR_U64:
	case IR_PTR:
		return TRUE;
	case IR_F32:
	case IR_F64:
		return FALSE;
	default:
		assert(0); break;
	}

	return FALSE;
}

static int cc_type_size(int tycode)
{
	switch (tycode)
	{
	case IR_S8:
	case IR_U8:
		return 1;
	case IR_S16:
	case IR_U16:
		return 2;
	case IR_S32:
	case IR_U32:
	case IR_PTR:
		return 4;
	case IR_S64:
	case IR_U64:
		return 8;
	case IR_F32:
		return 4;
	case IR_F64:
		return 8;
	default:
		break;
	}

	return 0;
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
	const int curseqid = triple->_seqid;
	struct tagCCASCode* as;
	struct tagCCDagNode* result, *lhs, *rhs;
	int tycode;

	switch (triple->_opcode)
	{
	case X86_BLKENTER:
		block_enter(ctx, curseqid, triple->_count); break;
	case X86_BLKLEAVE:
		block_leave(ctx, curseqid, triple->_count); break;
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

			if (!cc_get_operand(lhs, &src)) { return FALSE; }
			if (src._format == FormatInSIB2 && !cc_convert_insib2_to_insib(ctx, lhs, &src, curseqid)) { return FALSE; }
			if (!(as = emit_as(ctx, X86_MOV))) { return FALSE; }
			as->_dst._format = FormatReg;
			as->_dst._tycode = tycode;
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

			if (!cc_get_operand(lhs, &src)) { return FALSE; }
			if (src._format == FormatInSIB2 && !cc_convert_insib2_to_insib(ctx, lhs, &src, curseqid)) { return FALSE; }
			if (!(as = emit_as(ctx, X86_MOV))) { return FALSE; }
			as->_dst._format = FormatReg;
			as->_dst._tycode = tycode;
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

			if (!cc_get_operand(lhs, &src)) { return FALSE; }
			if (src._format == FormatInSIB2 && !cc_convert_insib2_to_insib(ctx, lhs, &src, curseqid)) { return FALSE; }
			if (!(as = emit_as(ctx, X86_MOV))) { return FALSE; }
			as->_dst._format = FormatReg;
			as->_dst._tycode = tycode;
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
			if (!cc_get_operand(lhs, &src)) { return FALSE; }
			if (src._format == FormatInSIB2 && !cc_convert_insib2_to_insib(ctx, lhs, &src, curseqid)) { return FALSE; }

			regs[0] = cc_reg_alloc(ctx, curseqid, NORMAL_ADDR_X86REGS);
			if (regs[0] == X86_NIL) { return FALSE; }
			cc_reg_make_associated(regs[0], lhs, 0);
			if (!(as = emit_as(ctx, X86_MOV))) { return FALSE; }

			as->_src = src;
			as->_dst._format = FormatReg;
			as->_dst._tycode = IR_OPTY0(lhs->_op);
			as->_dst._u._regs[0] = regs[0];
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
	{
		struct tagCCASOperand dst = { 0 }, src = { 0 };
		int regs[2] = { X86_NIL, X86_NIL };

		result = triple->_result;
		lhs = triple->_kids[0];
		rhs = triple->_kids[1];

		if (!cc_get_operand(lhs, &dst)) { return FALSE; }
		if (dst._format == FormatInSIB2 && !cc_convert_insib2_to_insib(ctx, lhs, &dst, curseqid)) { return FALSE; }
		if (!cc_get_operand(rhs, &src)) { return FALSE; }
		if (src._format == FormatInSIB2 && !cc_convert_insib2_to_insib(ctx, rhs, &src, curseqid)) { return FALSE; }

		switch (IR_OPTY0(lhs->_op))
		{
		case IR_S32:
		case IR_U32:
		case IR_S64:
		case IR_U64:
			if (lhs->_x._inregister)
			{
				regs[0] = lhs->_x._loc._regs[0];
				regs[1] = lhs->_x._loc._regs[1];
				cc_reg_free(ctx, regs[0], curseqid);
				cc_reg_make_associated(regs[0], result, 0);
				if (regs[1] != X86_NIL) 
				{
					cc_reg_free(ctx, regs[1], curseqid);
					cc_reg_make_associated(regs[1], result, 1);
				}
			}
			else
			{
				/* load to register */
				regs[0] = cc_reg_alloc(ctx, curseqid, NORMAL_X86REGS);
				if (regs[0] == X86_NIL) { return FALSE; }
				if (IR_OPTY0(lhs->_op) == IR_S64 || IR_OPTY0(lhs->_op) == IR_U64)
				{
					regs[1] = cc_reg_alloc(ctx, curseqid, NORMAL_X86REGS);
					if (regs[1] == X86_NIL) { return FALSE; }
				}

				if (!(as = emit_as(ctx, X86_MOV))) { return FALSE; }

				as->_src = dst;
				as->_dst._format = FormatReg;
				as->_dst._tycode = IR_OPTY0(lhs->_op);
				as->_dst._u._regs[0] = regs[0];
				as->_dst._u._regs[1] = regs[1];
				as->_count = lhs->_typesize;

				dst = as->_dst;
				cc_reg_make_associated(regs[0], result, 0);
				cc_reg_make_associated(regs[1], result, 1);
				if (lhs == rhs) { src = dst; }
			}
			break;
		default:
			assert(0); break;
		}

		if (!(as = emit_as(ctx, triple->_opcode))) { return FALSE; }
		as->_dst = dst;
		as->_src = src;
	}
		break;
	case X86_ADD:
	case X86_SUB:
	{
		struct tagCCASOperand dst = { 0 }, src = { 0 };
		int regs[2] = { X86_NIL, X86_NIL };

		result = triple->_result;
		lhs = triple->_kids[0];
		rhs = triple->_kids[1];

		if (!cc_get_operand(lhs, &dst)) { return FALSE; }
		if (dst._format == FormatInSIB2 && !cc_convert_insib2_to_insib(ctx, lhs, &dst, curseqid)) { return FALSE; }
		if (!cc_get_operand(rhs, &src)) { return FALSE; }
		if (src._format == FormatInSIB2 && !cc_convert_insib2_to_insib(ctx, rhs, &src, curseqid)) { return FALSE; }

		switch (IR_OPTY0(lhs->_op))
		{
		case IR_S32:
		case IR_U32:
		case IR_PTR:
		case IR_S64:
		case IR_U64:
		{
			if (lhs->_x._inregister)
			{
				regs[0] = lhs->_x._loc._regs[0];
				regs[1] = lhs->_x._loc._regs[1];
				cc_reg_free(ctx, regs[0], curseqid);
				cc_reg_make_associated(regs[0], result, 0);
				if (regs[1] != X86_NIL)
				{
					cc_reg_free(ctx, regs[1], curseqid);
					cc_reg_make_associated(regs[1], result, 1);
				}
			}
			else
			{
				/* load to register */
				regs[0] = cc_reg_alloc(ctx, curseqid, NORMAL_X86REGS);
				if (regs[0] == X86_NIL) { return FALSE; }
				if (IR_OPTY0(lhs->_op) == IR_S64 || IR_OPTY0(lhs->_op) == IR_U64)
				{
					regs[1] = cc_reg_alloc(ctx, curseqid, NORMAL_X86REGS);
					if (regs[1] == X86_NIL) { return FALSE; }
				}

				if (!(as = emit_as(ctx, X86_MOV))) { return FALSE; }

				as->_src = dst;
				as->_dst._format = FormatReg;
				as->_dst._tycode = IR_OPTY0(lhs->_op);
				as->_dst._u._regs[0] = regs[0];
				as->_dst._u._regs[1] = regs[1];
				as->_count = lhs->_typesize;

				dst = as->_dst;
				cc_reg_make_associated(regs[0], result, 0);
				cc_reg_make_associated(regs[1], result, 1);
				if (lhs == rhs) { src = dst; }
			}
		}
			break;
		case IR_F32:
		case IR_F64:
		{
			if (lhs->_x._inregister)
			{
				regs[0] = lhs->_x._loc._regs[0];
				assert(regs[0] == X86_ST0);
				cc_reg_free(ctx, regs[0], curseqid);
				cc_reg_make_associated(regs[0], result, 0);
			}
			else
			{
				/* load to register */
				if (rhs->_x._inregister && !rhs->_x._inmemory) {
					if (!cc_reg_spill(ctx, rhs, curseqid)) { return FALSE; }
				}
				cc_reg_free(ctx, X86_ST0, curseqid);
				cc_reg_make_associated(X86_ST0, result, 0);

				if (!(as = emit_as(ctx, X86_MOV))) { return FALSE; }

				as->_src = dst;
				as->_dst._format = FormatReg;
				as->_dst._tycode = IR_OPTY0(lhs->_op);
				as->_dst._u._regs[0] = X86_ST0;
				as->_count = lhs->_typesize;

				if (!cc_get_operand(result, &dst)) { return FALSE; }
				if (lhs == rhs) { src = dst; }
			}
		}
			break;
		default:
			assert(0); break;
		}

		if (!(as = emit_as(ctx, triple->_opcode))) { return FALSE; }
		as->_dst = dst;
		as->_src = src;
	}
		break;
	case X86_MUL:
	{
		struct tagCCASOperand dst = { 0 }, src = { 0 };
		int regs[2] = { X86_NIL, X86_NIL };

		result = triple->_result;
		lhs = triple->_kids[0];
		rhs = triple->_kids[1];

		if (!cc_get_operand(lhs, &dst)) { return FALSE; }
		if (dst._format == FormatInSIB2 && !cc_convert_insib2_to_insib(ctx, lhs, &dst, curseqid)) { return FALSE; }
		if (!cc_get_operand(rhs, &src)) { return FALSE; }
		if (src._format == FormatInSIB2 && !cc_convert_insib2_to_insib(ctx, rhs, &src, curseqid)) { return FALSE; }

		switch (IR_OPTY0(lhs->_op))
		{
		case IR_S32:
		case IR_U32:
		{
			if (lhs->_x._inregister)
			{
				regs[0] = lhs->_x._loc._regs[0];
				cc_reg_free(ctx, regs[0], curseqid);
				cc_reg_make_associated(regs[0], result, 0);
			}
			else
			{
				/* load to register */
				regs[0] = cc_reg_alloc(ctx, curseqid, NORMAL_X86REGS);
				if (regs[0] == X86_NIL) { return FALSE; }

				if (!(as = emit_as(ctx, X86_MOV))) { return FALSE; }
				as->_src = dst;
				as->_dst._format = FormatReg;
				as->_dst._tycode = IR_OPTY0(lhs->_op);
				as->_dst._u._regs[0] = regs[0];
				as->_count = lhs->_typesize;

				dst = as->_dst;
				cc_reg_make_associated(regs[0], result, 0);
				if (lhs == rhs) { src = dst; }
			}
		}
			break;
		case IR_S64:
		case IR_U64:
		{
			if (!(as = emit_as(ctx, X86_PUSH))) { return FALSE; }
			as->_dst = src;
			as->_count = rhs->_typesize;

			if (!(as = emit_as(ctx, X86_PUSH))) { return FALSE; }
			as->_dst = dst;
			as->_count = lhs->_typesize;

			cc_reg_free(ctx, X86_EAX, curseqid);
			cc_reg_free(ctx, X86_ECX, curseqid);
			cc_reg_free(ctx, X86_EDX, curseqid);

			cc_reg_make_associated(X86_EAX, result, 0);
			cc_reg_make_associated(X86_EDX, result, 1);
		}
			break;
		case IR_F32:
		case IR_F64:
		{
			if (lhs->_x._inregister)
			{
				regs[0] = lhs->_x._loc._regs[0];
				assert(regs[0] == X86_ST0);
				cc_reg_free(ctx, regs[0], curseqid);
				cc_reg_make_associated(regs[0], result, 0);
			}
			else
			{
				/* load to register */
				if (rhs->_x._inregister && !rhs->_x._inmemory) {
					if (!cc_reg_spill(ctx, rhs, curseqid)) { return FALSE; }
				}
				cc_reg_free(ctx, X86_ST0, curseqid);
				cc_reg_make_associated(X86_ST0, result, 0);

				if (!(as = emit_as(ctx, X86_MOV))) { return FALSE; }

				as->_src = dst;
				as->_dst._format = FormatReg;
				as->_dst._tycode = IR_OPTY0(lhs->_op);
				as->_dst._u._regs[0] = X86_ST0;
				as->_count = lhs->_typesize;

				if (!cc_get_operand(result, &dst)) { return FALSE; }
				if (lhs == rhs) { src = dst; }
			}
		}
			break;
		default:
			assert(0); break;
		}

		if (!(as = emit_as(ctx, triple->_opcode))) { return FALSE; }
		as->_dst = dst;
		as->_src = src;
	}
		break;

	case X86_LSH:
	case X86_RSH:
	{
		struct tagCCASOperand dst = { 0 }, src = { 0 };
		int regs[2] = { X86_NIL, X86_NIL };

		result = triple->_result;
		lhs = triple->_kids[0];
		rhs = triple->_kids[1];

		if (!cc_get_operand(lhs, &dst)) { return FALSE; }
		if (dst._format == FormatInSIB2 && !cc_convert_insib2_to_insib(ctx, lhs, &dst, curseqid)) { return FALSE; }
		if (!cc_get_operand(rhs, &src)) { return FALSE; }
		if (src._format == FormatInSIB2 && !cc_convert_insib2_to_insib(ctx, rhs, &src, curseqid)) { return FALSE; }

		switch (IR_OPTY0(lhs->_op))
		{
		case IR_S32:
		case IR_U32:
		{
			if (lhs->_x._inregister && lhs->_lastref <= curseqid && (lhs->_x._loc._regs[0] != X86_ECX || src._format == FormatImm))
			{
				regs[0] = lhs->_x._loc._regs[0];
				cc_reg_free(ctx, regs[0], curseqid);
				cc_reg_make_associated(regs[0], result, 0);
			}
			else
			{
				/* load to register */
				regs[0] = cc_reg_alloc(ctx, curseqid, (REG_BIT(X86_EAX) | REG_BIT(X86_EBX) | REG_BIT(X86_EDX)));
				if (regs[0] == X86_NIL) { return FALSE; }
				if (!(as = emit_as(ctx, X86_MOV))) { return FALSE; }

				as->_src = dst;
				as->_dst._format = FormatReg;
				as->_dst._tycode = IR_OPTY0(lhs->_op);
				as->_dst._u._regs[0] = regs[0];
				as->_count = lhs->_typesize;

				cc_reg_make_associated(regs[0], result, 0);
			}
			assert(result->_x._inregister);

			if (src._format != FormatImm && (src._format != FormatReg || src._u._regs[0] != X86_ECX))
			{
				cc_reg_free(ctx, X86_ECX, curseqid);
				if (!(as = emit_as(ctx, X86_MOV))) { return FALSE; }
				as->_dst = src;
				as->_src._format = FormatReg;
				as->_src._tycode = src._tycode;
				as->_src._u._regs[0] = X86_ECX;

				cc_reg_make_associated(X86_ECX, rhs, 0);
			}

			if (!cc_get_operand(result, &dst)) { return FALSE; }
			if (!cc_get_operand(rhs, &src)) { return FALSE; }
		}
			break;
		case IR_S64:
		case IR_U64:
		{
			if (rhs->_x._inregister && (rhs->_x._loc._regs[0] == X86_EAX || rhs->_x._loc._regs[0] == X86_EDX) 
				&& !rhs->_x._inmemory) {
				if (!cc_reg_spill(ctx, rhs, curseqid)) { return FALSE; }
			}

			cc_reg_free(ctx, X86_EAX, curseqid);
			cc_reg_free(ctx, X86_ECX, curseqid);
			cc_reg_free(ctx, X86_EDX, curseqid);

			if (!(as = emit_as(ctx, X86_MOV))) { return FALSE; }
			as->_src = dst;
			as->_dst._format = FormatReg;
			as->_dst._tycode = IR_OPTY0(lhs->_op);
			as->_dst._u._regs[0] = X86_EAX;
			as->_dst._u._regs[1] = X86_EDX;
			as->_count = lhs->_typesize;

			if (!(as = emit_as(ctx, X86_MOV))) { return FALSE; }
			as->_src = src;
			as->_dst._format = FormatReg;
			as->_dst._tycode = IR_OPTY0(rhs->_op);
			as->_dst._u._regs[0] = X86_ECX;
			as->_dst._u._regs[1] = X86_NIL;
			as->_count = rhs->_typesize;

			cc_reg_make_associated(X86_EAX, result, 0);
			cc_reg_make_associated(X86_EDX, result, 1);
		}
			break;
		default:
			assert(0);  break;
		}

		if (!(as = emit_as(ctx, triple->_opcode))) { return FALSE; }
		as->_dst = dst;
		as->_src = src;
	}
		break;
	case X86_DIV:
	case X86_MOD:
	{
		struct tagCCASOperand dst = { 0 }, src = { 0 };
		int regs[2] = { X86_NIL, X86_NIL };

		result = triple->_result;
		lhs = triple->_kids[0];
		rhs = triple->_kids[1];

		if (!cc_get_operand(lhs, &dst)) { return FALSE; }
		if (dst._format == FormatInSIB2 && !cc_convert_insib2_to_insib(ctx, lhs, &dst, curseqid)) { return FALSE; }
		if (!cc_get_operand(rhs, &src)) { return FALSE; }
		if (src._format == FormatInSIB2 && !cc_convert_insib2_to_insib(ctx, rhs, &src, curseqid)) { return FALSE; }

		switch (IR_OPTY0(lhs->_op))
		{
		case IR_S32:
		case IR_U32:
		{
			/* mark eax edx is using */
			cc_reg_markused(X86_EDX);
			cc_reg_markused(X86_EAX);

			if (!cc_convert_insib_sib_imm_to_reg(ctx, rhs, &src, curseqid)) { return FALSE; }
			if (!lhs->_x._inregister || (lhs->_x._loc._regs[0] != X86_EAX))
			{
				cc_reg_free(ctx, X86_EAX, curseqid);
				if (!(as = emit_as(ctx, X86_MOV))) { return FALSE; }

				as->_src = dst;
				as->_dst._format = FormatReg;
				as->_dst._tycode = IR_OPTY0(lhs->_op);
				as->_dst._u._regs[0] = X86_EAX;
				as->_count = lhs->_typesize;
				
				dst = as->_dst;
			}

			cc_reg_free(ctx, X86_EDX, curseqid);
			cc_reg_unmarkused(X86_EDX);
			cc_reg_unmarkused(X86_EAX);
			
			cc_reg_make_associated(triple->_opcode == X86_DIV ? X86_EAX : X86_EDX, result, 0);
		}
			break;
		case IR_S64:
		case IR_U64:
		{
			if (!(as = emit_as(ctx, X86_PUSH))) { return FALSE; }
			as->_dst = src;
			as->_count = rhs->_typesize;

			if (!(as = emit_as(ctx, X86_PUSH))) { return FALSE; }
			as->_dst = dst;
			as->_count = lhs->_typesize;

			cc_reg_free(ctx, X86_EAX, curseqid);
			cc_reg_free(ctx, X86_ECX, curseqid);
			cc_reg_free(ctx, X86_EDX, curseqid);

			cc_reg_make_associated(X86_EAX, result, 0);
			cc_reg_make_associated(X86_EDX, result, 1);
		}
			break;
		case IR_F32:
		case IR_F64:
		{
			if (lhs->_x._inregister)
			{
				regs[0] = lhs->_x._loc._regs[0];
				assert(regs[0] == X86_ST0);
				cc_reg_free(ctx, regs[0], curseqid);
				cc_reg_make_associated(regs[0], result, 0);
			}
			else
			{
				/* load to register */
				if (rhs->_x._inregister && !rhs->_x._inmemory) {
					if (!cc_reg_spill(ctx, rhs, curseqid)) { return FALSE; }
				}
				cc_reg_free(ctx, X86_ST0, curseqid);
				cc_reg_make_associated(X86_ST0, result, 0);

				if (!(as = emit_as(ctx, X86_MOV))) { return FALSE; }

				as->_src = dst;
				as->_dst._format = FormatReg;
				as->_dst._tycode = IR_OPTY0(lhs->_op);
				as->_dst._u._regs[0] = X86_ST0;
				as->_count = lhs->_typesize;

				dst = as->_dst;
				if (lhs == rhs) { src = dst; }
			}
		}
		break;
		default:
			assert(0); break;
		}

		if (!(as = emit_as(ctx, triple->_opcode))) { return FALSE; }
		as->_dst = dst;
		as->_src = src;
	}
		break;
	case X86_NEG:
	{
		struct tagCCASOperand dst = { 0 }, src = { 0 };
		int regs[2] = { X86_NIL, X86_NIL };

		result = triple->_result;
		lhs = triple->_kids[0];

		if (!cc_get_operand(lhs, &dst)) { return FALSE; }
		if (dst._format == FormatInSIB2 && !cc_convert_insib2_to_insib(ctx, lhs, &dst, curseqid)) { return FALSE; }

		switch (IR_OPTY0(lhs->_op))
		{
		case IR_S32:
		case IR_U32:
		case IR_S64:
		case IR_U64:
		{
			if (!cc_convert_insib_sib_imm_to_reg(ctx, lhs, &dst, curseqid)) { return FALSE; }

			regs[0] = lhs->_x._loc._regs[0];
			regs[1] = lhs->_x._loc._regs[1];
		
			cc_reg_free(ctx, regs[0], curseqid);
			cc_reg_make_associated(regs[0], result, 0);
			if (regs[1] != X86_NIL) {
				cc_reg_free(ctx, regs[1], curseqid);
				cc_reg_make_associated(regs[1], result, 1);
			}
		}
		break;
		case IR_F32:
		case IR_F64:
		{
			if (lhs->_x._inregister)
			{
				regs[0] = lhs->_x._loc._regs[0];
				assert(regs[0] == X86_ST0);
				cc_reg_free(ctx, regs[0], curseqid);
				cc_reg_make_associated(regs[0], result, 0);
			}
			else
			{
				/* load to register */
				cc_reg_free(ctx, X86_ST0, curseqid);
				if (!(as = emit_as(ctx, X86_MOV))) { return FALSE; }

				as->_src = dst;
				as->_dst._format = FormatReg;
				as->_dst._tycode = IR_OPTY0(lhs->_op);
				as->_dst._u._regs[0] = X86_ST0;
				as->_count = lhs->_typesize;

				dst = as->_dst;
				cc_reg_make_associated(X86_ST0, result, 0);
			}
		}
		break;
		default:
			assert(0); break;
		}

		if (!(as = emit_as(ctx, triple->_opcode))) { return FALSE; }
		as->_dst = dst;
	}
		break;
	case X86_NOT:
	{
		struct tagCCASOperand dst = { 0 }, src = { 0 };
		int regs[2] = { X86_NIL, X86_NIL };

		result = triple->_result;
		lhs = triple->_kids[0];

		if (!cc_get_operand(lhs, &dst)) { return FALSE; }
		if (dst._format == FormatInSIB2 && !cc_convert_insib2_to_insib(ctx, lhs, &dst, curseqid)) { return FALSE; }
		
		switch (IR_OPTY0(lhs->_op))
		{
		case IR_S32:
		case IR_U32:
		case IR_S64:
		case IR_U64:
		{
			if (!cc_convert_insib_sib_imm_to_reg(ctx, lhs, &dst, curseqid)) { return FALSE; }
			regs[0] = lhs->_x._loc._regs[0];
			regs[1] = lhs->_x._loc._regs[1];

			cc_reg_free(ctx, regs[0], curseqid);
			cc_reg_make_associated(regs[0], result, 0);
			if (regs[1] != X86_NIL) {
				cc_reg_free(ctx, regs[1], curseqid);
				cc_reg_make_associated(regs[1], result, 1);
			}
		}
			break;
		default:
			assert(0); break;
		}

		if (!(as = emit_as(ctx, triple->_opcode))) { return FALSE; }
		as->_dst = dst;
	}
		break;
	case X86_JE:
	case X86_JNE:
	case X86_JG:
	case X86_JL:
	case X86_JGE:
	case X86_JLE:
	{
		struct tagCCASOperand dst = { 0 }, src = { 0 };
		int regs[2] = { X86_NIL, X86_NIL };

		lhs = triple->_kids[0];
		rhs = triple->_kids[1];

		if (!cc_get_operand(lhs, &dst)) { return FALSE; }
		if (dst._format == FormatInSIB2 && !cc_convert_insib2_to_insib(ctx, lhs, &dst, curseqid)) { return FALSE; }
		if (!cc_get_operand(rhs, &src)) { return FALSE; }
		if (src._format == FormatInSIB2 && !cc_convert_insib2_to_insib(ctx, rhs, &src, curseqid)) { return FALSE; }

		switch (IR_OPTY0(lhs->_op))
		{
		case IR_S32:
		case IR_U32:
		case IR_S64:
		case IR_U64:
		{
			if (!cc_convert_insib_sib_imm_to_reg(ctx, lhs, &dst, curseqid)) { return FALSE; }
			assert(lhs->_x._inregister);
		}
			break;
		case IR_F32:
		case IR_F64:
		{
			cc_reg_free(ctx, X86_EAX, curseqid);

			if (lhs->_x._inregister)
			{
				regs[0] = lhs->_x._loc._regs[0];
				assert(regs[0] == X86_ST0);
			}
			else
			{
				/* load to register */
				if (rhs->_x._inregister && !rhs->_x._inmemory) {
					if (!cc_reg_spill(ctx, rhs, curseqid)) { return FALSE; }
				}
				cc_reg_free(ctx, X86_ST0, curseqid);
				cc_reg_make_associated(X86_ST0, lhs, 0);

				if (!(as = emit_as(ctx, X86_MOV))) { return FALSE; }

				as->_src = dst;
				as->_dst._format = FormatReg;
				as->_dst._tycode = IR_OPTY0(lhs->_op);
				as->_dst._u._regs[0] = X86_ST0;
				as->_count = lhs->_typesize;

				dst = as->_dst;
				if (lhs == rhs) { src = dst; }
			}
		}
			break;
		default:
			assert(0); break;
		}

		if (!(as = emit_as(ctx, triple->_opcode))) { return FALSE; }
		as->_dst = dst;
		as->_src = src;
		as->_target = triple->_target;
	}
		break;
	case X86_JMP:
	{
		if (!(as = emit_as(ctx, X86_JMP))) { return FALSE; }
		as->_target = triple->_target;
	}
		break;
	/* convert */
	case X86_CVT:
	{
		struct tagCCASOperand dst = { 0 }, src = { 0 };
		int regs[2] = { X86_NIL, X86_NIL };
		int dstty, srcty;
		BOOL isdstfilled = FALSE;
		BOOL isreused = FALSE;

		lhs = triple->_result;
		rhs = triple->_kids[0];
		dstty = IR_OPTY0(lhs->_op);
		srcty = IR_OPTY0(rhs->_op);

		if (!cc_get_operand(rhs, &src)) { return FALSE; }
		if (src._format == FormatInSIB2 && !cc_convert_insib2_to_insib(ctx, rhs, &src, curseqid)) { return FALSE; }
		if (src._format == FormatSIB && !cc_convert_insib_sib_imm_to_reg(ctx, rhs, &src, curseqid)) { return FALSE; }
		if (cc_is_integer(dstty) && cc_is_integer(srcty))
		{
			if (lhs->_typesize == rhs->_typesize)
			{
				if (!rhs->_x._inregister)
				{
					if (!cc_convert_insib_sib_imm_to_reg(ctx, rhs, &src, curseqid)) { return FALSE; }
				}
				assert(rhs->_x._inregister);

				cc_reg_make_associated(rhs->_x._loc._regs[0], lhs, 0);
				cc_reg_make_associated(rhs->_x._loc._regs[1], lhs, 1);
				isreused = TRUE;
			}
			else
			{
				if (dstty != IR_U64 && dstty != IR_S64)
				{
					/* alloc register */
					regs[0] = cc_reg_alloc(ctx, curseqid, (REG_BIT(X86_EAX) | REG_BIT(X86_EBX) | REG_BIT(X86_ECX) | REG_BIT(X86_EDX)));
					if (regs[0] == X86_NIL) { return FALSE; }

					cc_reg_make_associated(regs[0], lhs, 0);
				}
				else
				{
					cc_reg_free(ctx, X86_EAX, curseqid);
					cc_reg_free(ctx, X86_EDX, curseqid);

					cc_reg_make_associated(X86_EAX, lhs, 0);
					cc_reg_make_associated(X86_EDX, lhs, 1);

					if (src._format != FormatReg && !cc_convert_insib_sib_imm_to_reg(ctx, rhs, &src, curseqid)) { return FALSE; }
				}
			}
		}
		else if (cc_is_integer(dstty) && !cc_is_integer(srcty))
		{
			cc_reg_free(ctx, X86_EAX, curseqid);
			cc_reg_make_associated(X86_EAX, lhs, 0);

			if (dstty == IR_U64 || dstty == IR_S64)
			{
				cc_reg_free(ctx, X86_EDX, curseqid);
				cc_reg_make_associated(X86_EDX, lhs, 1);
			}

			assert(src._format == FormatReg || src._format == FormatInSIB);
		}
		else if (!cc_is_integer(dstty) && cc_is_integer(srcty))
		{
			/* alloc temp */
			alloc_temporary_space(ctx, lhs, curseqid);

			if (srcty != IR_U64 && srcty != IR_S64)
			{
				if (srcty == IR_S8 || srcty == IR_U8 || srcty == IR_S16 || srcty == IR_U16)
				{
					/* alloc intermediate register */
					regs[0] = cc_reg_alloc(ctx, curseqid, (REG_BIT(X86_EAX) | REG_BIT(X86_EBX) | REG_BIT(X86_ECX) | REG_BIT(X86_EDX)));
					if (regs[0] == X86_NIL) { return FALSE; }

					if (!(as = emit_as(ctx, X86_CVT))) { return FALSE; }
					as->_dst._format = FormatReg;
					as->_dst._tycode = (srcty == IR_S8 || srcty == IR_S16) ? IR_S32 : IR_U32;
					as->_dst._u._regs[0] = regs[0];
					as->_dst._u._regs[1] = X86_NIL;
					as->_src = src;

					dst = as->_dst;
					isdstfilled = TRUE;
				}
			}
		}
		else if (!cc_is_integer(dstty) && !cc_is_integer(srcty))
		{
			/* alloc temp */
			alloc_temporary_space(ctx, lhs, curseqid);
		}

		if (!isreused) 
		{
			if (!isdstfilled && !cc_get_operand(lhs, &dst)) { return FALSE; }
			if (!(as = emit_as(ctx, X86_CVT))) { return FALSE; }
			as->_dst = dst;
			as->_src = src;
		}
	}
		break;
	case X86_MOV:
	{
		struct tagCCASOperand dst = { 0 }, src = { 0 };
		int regs[2] = { X86_NIL, X86_NIL };

		lhs = triple->_kids[0];
		rhs = triple->_kids[1];

		if (!cc_get_operand(lhs, &dst)) { return FALSE; }
		if (dst._format == FormatInSIB2 && !cc_convert_insib2_to_insib(ctx, lhs, &dst, curseqid)) { return FALSE; }
		if (!cc_convert_insib_reg_imm_to_sib(ctx, lhs, &dst, curseqid)) { return FALSE; }

		if (!cc_get_operand(rhs, &src)) { return FALSE; }
		if (src._format == FormatInSIB2 && !cc_convert_insib2_to_insib(ctx, rhs, &src, curseqid)) { return FALSE; }
		if (src._tycode == IR_BLK)
		{
			assert(src._format == FormatInSIB);

			cc_reg_free(ctx, X86_EDI, curseqid);
			cc_reg_free(ctx, X86_ESI, curseqid);
			cc_reg_free(ctx, X86_ECX, curseqid);

			if (!(as = emit_as(ctx, X86_MOV))) { return FALSE; }
			as->_dst = dst;
			as->_src = src;
			as->_count = rhs->_typesize;
		}
		else
		{
			if (src._format == FormatInSIB || src._format == FormatSIB)
			{
				if (!cc_convert_insib_sib_imm_to_reg(ctx, rhs, &src, curseqid)) { return FALSE; }
			}

			if (!(as = emit_as(ctx, X86_MOV))) { return FALSE; }
			as->_dst = dst;
			as->_src = src;

			cc_reg_make_associated(rhs->_x._loc._regs[0], lhs, 0);
			cc_reg_make_associated(rhs->_x._loc._regs[1], lhs, 1);
		}
	}
		break;
	case X86_PUSH:
	{
		struct tagCCASOperand dst = { 0 };
		int regs[2] = { X86_NIL, X86_NIL };

		lhs = triple->_kids[0];
	
		if (!cc_get_operand(lhs, &dst)) { return FALSE; }
		if (dst._format == FormatInSIB2 && !cc_convert_insib2_to_insib(ctx, lhs, &dst, curseqid)) { return FALSE; }
		if (dst._tycode == IR_BLK)
		{
			assert(dst._format == FormatInSIB);

			cc_reg_free(ctx, X86_EDI, curseqid);
			cc_reg_free(ctx, X86_ESI, curseqid);
			cc_reg_free(ctx, X86_ECX, curseqid);

			if (!(as = emit_as(ctx, X86_PUSH))) { return FALSE; }
			as->_dst = dst;
			as->_count = lhs->_typesize;
		}
		else
		{
			if (dst._format == FormatSIB)
			{
				if (!cc_convert_insib_sib_imm_to_reg(ctx, lhs, &dst, curseqid)) { return FALSE; }
			}

			if (!(as = emit_as(ctx, X86_PUSH))) { return FALSE; }
			as->_dst = dst;
		}
	}
		break;
	case X86_CALL:
	{
		struct tagCCASOperand dst = { 0 };

		result = triple->_result;
		lhs = triple->_kids[0];
		if (!cc_get_operand(lhs, &dst)) { return FALSE; }
		if (dst._format == FormatInSIB2 && !cc_convert_insib2_to_insib(ctx, lhs, &dst, curseqid)) { return FALSE; }

		cc_reg_free(ctx, X86_EAX, curseqid);
		cc_reg_free(ctx, X86_ECX, curseqid);
		cc_reg_free(ctx, X86_EDX, curseqid);
		cc_reg_free(ctx, X86_ST0, curseqid);

		if (!(as = emit_as(ctx, X86_CALL))) { return FALSE; }
		as->_dst = dst;
		as->_count = result->_argsbytes;

		tycode = IR_OPTY0(result->_op);
		switch (tycode)
		{
		case IR_S8:
		case IR_U8:
		case IR_S16:
		case IR_U16:
		case IR_S32:
		case IR_U32:
		case IR_PTR:
			cc_reg_make_associated(X86_EAX, result, 0);
			break;
		case IR_S64:
		case IR_U64:
			cc_reg_make_associated(X86_EAX, result, 0);
			cc_reg_make_associated(X86_EDX, result, 1);
			break;
		case IR_F32:
		case IR_F64:
			cc_reg_make_associated(X86_ST0, result, 0);
			break;
		default:
			break;
		}
	}
		break;
	case X86_PROLOGUE:
	case X86_EPILOGUE:
	{
		if (!(as = emit_as(ctx, triple->_opcode))) { return FALSE; }
	}
		break;
	/* set memory to zero */
	case X86_ZERO_M:
	{
		struct tagCCASOperand dst = { 0 };
		
		lhs = triple->_kids[0];

		if (!cc_get_operand(lhs, &dst)) { return FALSE; }
		if (dst._format == FormatInSIB2 && !cc_convert_insib2_to_insib(ctx, lhs, &dst, curseqid)) { return FALSE; }
		if (!cc_convert_insib_reg_imm_to_sib(ctx, lhs, &dst, curseqid)) { return FALSE; }

		cc_reg_free(ctx, X86_EDI, curseqid);
		cc_reg_free(ctx, X86_EAX, curseqid);
		cc_reg_free(ctx, X86_ECX, curseqid);

		if (!(as = emit_as(ctx, X86_ZERO_M))) { return FALSE; }
		as->_dst = dst;
		as->_count = triple->_count;
	}
		break;
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
	ctx->_frame_base->_x._frame_offset = 0;

	for (; triple; triple=triple->_next)
	{
		if (!cc_gen_triple_to_x86(ctx, triple)) { return FALSE; }
	} /* end for */

	return TRUE;
}

/* get operand text */
static const char* cc_operand_text(const struct tagCCASOperand* operand, const int part, char szbuf[256])
{
	switch (operand->_format)
	{
	case FormatReg:
	{
		const char* reg0;

		reg0 = cc_reg_name(operand->_u._regs[part], cc_type_size(operand->_tycode));
		sprintf(szbuf, "%s", reg0);
	}
		break;
	case FormatImm:
	{
		uint64_t k = operand->_u._imm->_u._cnstval._uint;
		int32_t val;

		if (cc_type_size(operand->_tycode) < 8) {
			val = (int)k;
		} else if (part == 0) {
			val = (int)k;
		} else {
			val = (int)(k >> 32);
		}

		sprintf(szbuf, "%d", val);
	}
		break;
	case FormatSIB:
	case FormatInSIB:
	{
		int displacement2, len;

		len = 0;
		displacement2 = part * 4;
		if (operand->_u._sib._displacement) {
			len += sprintf(szbuf + len, "%s", operand->_u._sib._displacement->_x._name);
		}
		if (operand->_u._sib._basereg) {
			len += sprintf(szbuf + len, len ? "+%s" : "%s", cc_reg_name(operand->_u._sib._basereg, 4));
		}
		displacement2 += operand->_u._sib._displacement2;
		if (displacement2 != 0) {
			len += sprintf(szbuf + len, "%+d", displacement2);
		}
	}
		break;
	default:
		assert(0); break;
	}

	return szbuf;
}

/* output assemble text to file */
static BOOL cc_output_asmcodes(struct tagCCContext* ctx, struct tagCCSymbol* func, struct tagCCGenCodeContext* genctx)
{
	const struct tagCCASCode* as;
	const struct tagCCASOperand* dst, * src;
	struct tagCCASOperand tmp;
	char szdst[256], szsrc[256];

	for (as = genctx->_asmlist._head; as; as = as->_next)
	{
		dst = &as->_dst;
		src = &as->_src;

		switch (as->_opcode)
		{
		case X86_LABEL:
			fprintf(ctx->_outfp, "%s:\n", as->_target->_x._name); break;
		case X86_OR:
		case X86_XOR:
		case X86_AND:
		{
			const char* opname;

			switch (as->_opcode)
			{
			case X86_OR:
				opname = "or"; break;
			case X86_XOR:
				opname = "xor"; break;
			case X86_AND:
				opname = "and"; break;
			default:
				opname = NULL; break;
			}
			switch (dst->_tycode)
			{
			case IR_S32:
			case IR_U32:
			case IR_PTR:
				if (as->_src._format == FormatInSIB) {
					fprintf(ctx->_outfp, "\t%s %s, dword ptr [%s]\n", opname, cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc));
				}
				else {
					fprintf(ctx->_outfp, "\t%s %s, %s\n", opname, cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc));
				}
				break;
			case IR_S64:
			case IR_U64:
				if (as->_src._format == FormatInSIB) {
					fprintf(ctx->_outfp, "\t%s %s, dword ptr [%s]\n", opname, cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc));
					fprintf(ctx->_outfp, "\t%s %s, dword ptr [%s]\n", opname, cc_operand_text(dst, 1, szdst), cc_operand_text(src, 1, szsrc));
				}
				else {
					fprintf(ctx->_outfp, "\t%s %s, %s\n", opname, cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc));
					fprintf(ctx->_outfp, "\t%s %s, %s\n", opname, cc_operand_text(dst, 1, szdst), cc_operand_text(src, 1, szsrc));
				}
				break;
			default:
				assert(0); break;
			}
		}
			break;
		case X86_ADD:
		{
			switch (dst->_tycode)
			{
			case IR_S32:
			case IR_U32:
			case IR_PTR:
				if (as->_src._format == FormatInSIB) {
					fprintf(ctx->_outfp, "\tadd %s, dword ptr [%s] \n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc));
				}
				else {
					fprintf(ctx->_outfp, "\tadd %s, %s\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc));
				}
				break;
			case IR_S64:
			case IR_U64:
				if (as->_src._format == FormatInSIB) {
					fprintf(ctx->_outfp, "\tadd %s, dword ptr [%s] \n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc));
					fprintf(ctx->_outfp, "\tadc %s, dword ptr [%s] \n", cc_operand_text(dst, 1, szdst), cc_operand_text(src, 1, szsrc));
				}
				else {
					fprintf(ctx->_outfp, "\tadd %s, %s\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc));
					fprintf(ctx->_outfp, "\tadc %s, %s\n", cc_operand_text(dst, 1, szdst), cc_operand_text(src, 1, szsrc));
				}
				break;
			case IR_F32:
				if (as->_src._format == FormatInSIB) {
					fprintf(ctx->_outfp, "\tfadd dword ptr [%s] \n", cc_operand_text(src, 0, szsrc));
				}
				else {
					fprintf(ctx->_outfp, "\tfadd st(0), %s\n", cc_operand_text(src, 0, szsrc));
				}
				break;
			case IR_F64:
				if (as->_src._format == FormatInSIB) {
					fprintf(ctx->_outfp, "\tfadd qword ptr [%s] \n", cc_operand_text(src, 0, szsrc));
				}
				else {
					fprintf(ctx->_outfp, "\tfadd st(0), %s\n", cc_operand_text(src, 0, szsrc));
				}
				break;
			default:
				assert(0); break;
			}
		}
			break;
		case X86_SUB:
		{
			switch (dst->_tycode)
			{
			case IR_S32:
			case IR_U32:
			case IR_PTR:
				if (as->_src._format == FormatInSIB) {
					fprintf(ctx->_outfp, "\tsub %s, dword ptr [%s] \n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc));
				}
				else {
					fprintf(ctx->_outfp, "\tsub %s, %s\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc));
				}
				break;
			case IR_S64:
			case IR_U64:
				if (as->_src._format == FormatInSIB) {
					fprintf(ctx->_outfp, "\tsub %s, dword ptr [%s] \n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc));
					fprintf(ctx->_outfp, "\tsbb %s, dword ptr [%s] \n", cc_operand_text(dst, 1, szdst), cc_operand_text(src, 1, szsrc));
				}
				else {
					fprintf(ctx->_outfp, "\tadd %s, %s\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc));
					fprintf(ctx->_outfp, "\tsbb %s, %s\n", cc_operand_text(dst, 1, szdst), cc_operand_text(src, 1, szsrc));
				}
				break;
			case IR_F32:
				if (as->_src._format == FormatInSIB) {
					fprintf(ctx->_outfp, "\tfsub dword ptr [%s] \n", cc_operand_text(src, 0, szsrc));
				}
				else {
					fprintf(ctx->_outfp, "\tfsub st(0), %s\n", cc_operand_text(src, 0, szsrc));
				}
				break;
			case IR_F64:
				if (as->_src._format == FormatInSIB) {
					fprintf(ctx->_outfp, "\tfsub qword ptr [%s] \n", cc_operand_text(src, 0, szsrc));
				}
				else {
					fprintf(ctx->_outfp, "\tfsub st(0), %s\n", cc_operand_text(src, 0, szsrc));
				}
				break;
			default:
				assert(0); break;
			}
		}
			break;
		case X86_MUL:
		{
			switch (dst->_tycode)
			{
			case IR_S32:
			case IR_U32:
			case IR_PTR:
				if (as->_src._format == FormatInSIB) {
					fprintf(ctx->_outfp, "\timul %s, dword ptr [%s] \n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc));
				}
				else {
					fprintf(ctx->_outfp, "\timul %s, %s\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc));
				}
				break;
			case IR_S64:
			case IR_U64:
				fprintf(ctx->_outfp, "\tcall __allmul\n");
				break;
			case IR_F32:
				if (as->_src._format == FormatInSIB) {
					fprintf(ctx->_outfp, "\tfmul dword ptr [%s] \n", cc_operand_text(src, 0, szsrc));
				}
				else {
					fprintf(ctx->_outfp, "\tfmul st(0), %s\n", cc_operand_text(src, 0, szsrc));
				}
				break;
			case IR_F64:
				if (as->_src._format == FormatInSIB) {
					fprintf(ctx->_outfp, "\tfmul qword ptr [%s] \n", cc_operand_text(src, 0, szsrc));
				}
				else {
					fprintf(ctx->_outfp, "\tfmul st(0), %s\n", cc_operand_text(src, 0, szsrc));
				}
				break;
			default:
				assert(0); break;
			}
		}
			break;
		case X86_LSH:
		{
			switch (dst->_tycode)
			{
			case IR_S32:
			case IR_U32:
			case IR_PTR:
				if (as->_src._format == FormatReg) {
					fprintf(ctx->_outfp, "\tshl %s, cl\n", cc_operand_text(dst, 0, szdst));
				}
				else {
					fprintf(ctx->_outfp, "\tshl %s, %s\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc));
				}
				break;
			case IR_S64:
			case IR_U64:
				fprintf(ctx->_outfp, "\tcall __allshl\n");
				break;
			default:
				assert(0); break;
			}
		}
		break;
		case X86_RSH:
		{
			switch (dst->_tycode)
			{
			case IR_S32:
				if (as->_src._format == FormatReg) {
					fprintf(ctx->_outfp, "\tsar %s, cl\n", cc_operand_text(dst, 0, szdst));
				}
				else {
					fprintf(ctx->_outfp, "\tsar %s, %s\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc));
				}
				break;
			case IR_U32:
				if (as->_src._format == FormatReg) {
					fprintf(ctx->_outfp, "\tshr %s, cl\n", cc_operand_text(dst, 0, szdst));
				}
				else {
					fprintf(ctx->_outfp, "\tshr %s, %s\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc));
				}
				break;
			case IR_S64:
				fprintf(ctx->_outfp, "\tcall __allshr\n");
				break;
			case IR_U64:
				fprintf(ctx->_outfp, "\tcall __aullshr\n");
				break;
			default:
				assert(0); break;
			}
		}
			break;
		case X86_DIV:
		case X86_MOD:
		{
			switch (dst->_tycode)
			{
			case IR_S32:
				if (as->_src._format == FormatInSIB) { /* edx:eax / %1 => eax:quotient edx:remainder */
					fprintf(ctx->_outfp, "\tcdq\n\tidiv dword ptr [%s]\n", cc_operand_text(src, 0, szsrc));
				}
				else {
					fprintf(ctx->_outfp, "\tcdq\n\tidiv %s\n", cc_operand_text(src, 0, szsrc));
				}
				break;
			case IR_U32:
				if (as->_src._format == FormatInSIB) {
					fprintf(ctx->_outfp, "\txor edx, edx\n\tdiv dword ptr [%s]\n", cc_operand_text(src, 0, szsrc));
				}
				else {
					fprintf(ctx->_outfp, "\txor edx, edx\n\tdiv %s\n", cc_operand_text(src, 0, szsrc));
				}
				break;
			case IR_S64:
				if (as->_opcode == X86_DIV) {
					fprintf(ctx->_outfp, "\tcall __alldiv\n");
				}
				else {
					fprintf(ctx->_outfp, "\tcall __allrem\n");
				}
				break;
			case IR_U64:
				if (as->_opcode == X86_DIV) {
					fprintf(ctx->_outfp, "\tcall __aulldiv\n");
				}
				else {
					fprintf(ctx->_outfp, "\tcall __aullrem\n");
				}
				break;
			case IR_F32:
				if (as->_src._format == FormatInSIB) {
					fprintf(ctx->_outfp, "\tfdiv dword ptr [%s] \n", cc_operand_text(src, 0, szsrc));
				}
				else {
					fprintf(ctx->_outfp, "\tfdiv st(0), %s\n", cc_operand_text(src, 0, szsrc));
				}
				break;
			case IR_F64:
				if (as->_src._format == FormatInSIB) {
					fprintf(ctx->_outfp, "\tfdiv qword ptr [%s] \n", cc_operand_text(src, 0, szsrc));
				}
				else {
					fprintf(ctx->_outfp, "\tfdiv st(0), %s\n", cc_operand_text(src, 0, szsrc));
				}
				break;
			default:
				assert(0); break;
			}
		}
			break;
		case X86_NEG:
		{
			switch (dst->_tycode)
			{
			case IR_S32:
			case IR_U32:
				if (dst->_format == FormatInSIB) { 
					fprintf(ctx->_outfp, "\tneg dword ptr [%s]\n", cc_operand_text(dst, 0, szdst));
				}
				else {
					fprintf(ctx->_outfp, "\tneg %s\n", cc_operand_text(dst, 0, szdst));
				}
				break;
			case IR_S64:
			case IR_U64:
				assert(dst->_format == FormatReg);
				cc_operand_text(dst, 0, szdst);
				cc_operand_text(dst, 1, szsrc);
				fprintf(ctx->_outfp, "\tneg %s\n\tadc %s, 0\n\tneg %s\n", szdst, szsrc, szsrc);
				break;
			case IR_F32: /* Complements the sign bit of ST(0) */
			case IR_F64:
				fprintf(ctx->_outfp, "\tfchs\n");
				break;
			default:
				assert(0); break;
			}
		}
			break;
		case X86_NOT:
		{
			switch (dst->_tycode)
			{
			case IR_S32:
			case IR_U32:
				fprintf(ctx->_outfp, "\tnot %s\n", cc_operand_text(dst, 0, szdst));
				break;
			case IR_S64:
			case IR_U64:
				fprintf(ctx->_outfp, "\tnot %s\n\tnot %s\n", cc_operand_text(dst, 0, szdst), cc_operand_text(dst, 1, szsrc));
				break;
			default:
				assert(0); break;
			}
		}
			break;
		case X86_JE:
		{
			switch (dst->_tycode)
			{
			case IR_S32:
			case IR_U32:
				if (src->_format == FormatInSIB) {
					fprintf(ctx->_outfp, "\tcmp %s, dword ptr[%s]\n\tje %s\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc), as->_target->_x._name);
				}
				else {
					fprintf(ctx->_outfp, "\tcmp %s, %s\n\tje %s\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc), as->_target->_x._name);
				}
				break;
			case IR_S64:
			case IR_U64:
				if (src->_format == FormatInSIB) {
					fprintf(ctx->_outfp, "\tcmp %s, dword ptr[%s]\n\tjne @F\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc));
					fprintf(ctx->_outfp, "\tcmp %s, dword ptr[%s]\n\tje %s\n@@:\n", cc_operand_text(dst, 1, szdst), cc_operand_text(src, 1, szsrc), as->_target->_x._name);
				}
				else {
					fprintf(ctx->_outfp, "\tcmp %s, %s\n\tjne @F\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc));
					fprintf(ctx->_outfp, "\tcmp %s, %s\n\tje %s\n@@:\n", cc_operand_text(dst, 1, szdst), cc_operand_text(src, 1, szsrc), as->_target->_x._name);
				}
				break;
			case IR_F32:
				fprintf(ctx->_outfp, "\tfcom dword ptr[%s]\n\tfnstsw ax\n\ttest ah, 44h\n\tjnp %s\n", cc_operand_text(src, 0, szsrc), as->_target->_x._name);
				break;
			case IR_F64:
				fprintf(ctx->_outfp, "\tfcom qword ptr[%s]\n\tfnstsw ax\n\ttest ah, 44h\n\tjnp %s\n", cc_operand_text(src, 0, szsrc), as->_target->_x._name);
				break;
			default:
				assert(0); break;
			}
		}
			break;
		case X86_JNE:
		{
			switch (dst->_tycode)
			{
			case IR_S32:
			case IR_U32:
				if (src->_format == FormatInSIB) {
					fprintf(ctx->_outfp, "\tcmp %s, dword ptr[%s]\n\tjne %s\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc), as->_target->_x._name);
				}
				else {
					fprintf(ctx->_outfp, "\tcmp %s, %s\n\tjne %s\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc), as->_target->_x._name);
				}
				break;
			case IR_S64:
			case IR_U64:
				if (src->_format == FormatInSIB) {
					fprintf(ctx->_outfp, "\tcmp %s, dword ptr[%s]\n\tjne %s\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc), as->_target->_x._name);
					fprintf(ctx->_outfp, "\tcmp %s, dword ptr[%s]\n\tjne %s\n", cc_operand_text(dst, 1, szdst), cc_operand_text(src, 1, szsrc), as->_target->_x._name);
				}
				else {
					fprintf(ctx->_outfp, "\tcmp %s, %s\n\tjne %s\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc), as->_target->_x._name);
					fprintf(ctx->_outfp, "\tcmp %s, %s\n\tjne %s\n", cc_operand_text(dst, 1, szdst), cc_operand_text(src, 1, szsrc), as->_target->_x._name);
				}
				break;
			case IR_F32:
				fprintf(ctx->_outfp, "\tfcom dword ptr[%s]\n\tfnstsw ax\n\ttest ah, 44h\n\tjp %s\n", cc_operand_text(src, 0, szsrc), as->_target->_x._name);
				break;
			case IR_F64:
				fprintf(ctx->_outfp, "\tfcom qword ptr[%s]\n\tfnstsw ax\n\ttest ah, 44h\n\tjp %s\n", cc_operand_text(src, 0, szsrc), as->_target->_x._name);
				break;
			default:
				assert(0); break;
			}
		}
			break;
		case X86_JG:
		{
			switch (dst->_tycode)
			{
			case IR_S32:
				if (src->_format == FormatInSIB) {
					fprintf(ctx->_outfp, "\tcmp %s, dword ptr[%s]\n\tjg %s\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc), as->_target->_x._name);
				}
				else {
					fprintf(ctx->_outfp, "\tcmp %s, %s\n\tjg %s\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc), as->_target->_x._name);
				}
				break;
			case IR_U32:
				if (src->_format == FormatInSIB) {
					fprintf(ctx->_outfp, "\tcmp %s, dword ptr[%s]\n\tja %s\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc), as->_target->_x._name);
				}
				else {
					fprintf(ctx->_outfp, "\tcmp %s, %s\n\tja %s\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc), as->_target->_x._name);
				}
				break;
			case IR_S64:
				if (src->_format == FormatInSIB) {
					fprintf(ctx->_outfp, "\tcmp %s, dword ptr[%s]\n\tjl @F\n\tjg %s\n", cc_operand_text(dst, 1, szdst), cc_operand_text(src, 1, szsrc), as->_target->_x._name);
					fprintf(ctx->_outfp, "\tcmp %s, dword ptr[%s]\n\tja %s\n@@:\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc), as->_target->_x._name);
				}
				else {
					fprintf(ctx->_outfp, "\tcmp %s, %s\n\tjl @F\n\tjg %s\n", cc_operand_text(dst, 1, szdst), cc_operand_text(src, 1, szsrc), as->_target->_x._name);
					fprintf(ctx->_outfp, "\tcmp %s, %s\n\tja %s\n@@:\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc), as->_target->_x._name);
				}
				break;
			case IR_U64:
				if (src->_format == FormatInSIB) {
					fprintf(ctx->_outfp, "\tcmp %s, dword ptr[%s]\n\tja %s\n\tjb @F\n", cc_operand_text(dst, 1, szdst), cc_operand_text(src, 1, szsrc), as->_target->_x._name);
					fprintf(ctx->_outfp, "\tcmp %s, dword ptr[%s]\n\tja %s\n@@:\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc), as->_target->_x._name);
				}
				else {
					fprintf(ctx->_outfp, "\tcmp %s, %s\n\tja %s\n\tjb @F\n", cc_operand_text(dst, 1, szdst), cc_operand_text(src, 1, szsrc), as->_target->_x._name);
					fprintf(ctx->_outfp, "\tcmp %s, %s\n\tja %s\n@@:\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc), as->_target->_x._name);
				}
				break;
			case IR_F32:
				fprintf(ctx->_outfp, "\tfcom dword ptr[%s]\n\tfnstsw ax\n\ttest ah, 1h\n\tjne %s\n", cc_operand_text(src, 0, szsrc), as->_target->_x._name);
				break;
			case IR_F64:
				fprintf(ctx->_outfp, "\tfcom qword ptr[%s]\n\tfnstsw ax\n\ttest ah, 1h\n\tjne %s\n", cc_operand_text(src, 0, szsrc), as->_target->_x._name);
				break;
			default:
				assert(0); break;
			}
		}
			break;
		case X86_JL:
		{
			switch (dst->_tycode)
			{
			case IR_S32:
				if (src->_format == FormatInSIB) {
					fprintf(ctx->_outfp, "\tcmp %s, dword ptr[%s]\n\tjl %s\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc), as->_target->_x._name);
				}
				else {
					fprintf(ctx->_outfp, "\tcmp %s, %s\n\tjl %s\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc), as->_target->_x._name);
				}
				break;
			case IR_U32:
				if (src->_format == FormatInSIB) {
					fprintf(ctx->_outfp, "\tcmp %s, dword ptr[%s]\n\tjb %s\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc), as->_target->_x._name);
				}
				else {
					fprintf(ctx->_outfp, "\tcmp %s, %s\n\tjb %s\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc), as->_target->_x._name);
				}
				break;
			case IR_S64:
				if (src->_format == FormatInSIB) {
					fprintf(ctx->_outfp, "\tcmp %s, dword ptr[%s]\n\tjl %s\n\tjg @F\n", cc_operand_text(dst, 1, szdst), cc_operand_text(src, 1, szsrc), as->_target->_x._name);
					fprintf(ctx->_outfp, "\tcmp %s, dword ptr[%s]\n\tjb %s\n\t@@:\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc), as->_target->_x._name);
				}
				else {
					fprintf(ctx->_outfp, "\tcmp %s, %s\n\tjl %s\n\tjg @F\n", cc_operand_text(dst, 1, szdst), cc_operand_text(src, 1, szsrc), as->_target->_x._name);
					fprintf(ctx->_outfp, "\tcmp %s, %s\n\tjb %s\n@@:\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc), as->_target->_x._name);
				}
				break;
			case IR_U64:
				if (src->_format == FormatInSIB) {
					fprintf(ctx->_outfp, "\tcmp %s, dword ptr[%s]\n\tjb %s\n\tja @F\n", cc_operand_text(dst, 1, szdst), cc_operand_text(src, 1, szsrc), as->_target->_x._name);
					fprintf(ctx->_outfp, "\tcmp %s, dword ptr[%s]\n\tjb %s\n@@:\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc), as->_target->_x._name);
				}
				else {
					fprintf(ctx->_outfp, "\tcmp %s, %s\n\tjb %s\n\tja @F\n", cc_operand_text(dst, 1, szdst), cc_operand_text(src, 1, szsrc), as->_target->_x._name);
					fprintf(ctx->_outfp, "\tcmp %s, %s\n\tjb %s\n@@:\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc), as->_target->_x._name);
				}
				break;
			case IR_F32:
				fprintf(ctx->_outfp, "\tfcom dword ptr[%s]\n\tfnstsw ax\n\ttest ah, 41h\n\tjp %s\n", cc_operand_text(src, 0, szsrc), as->_target->_x._name);
				break;
			case IR_F64:
				fprintf(ctx->_outfp, "\tfcom qword ptr[%s]\n\tfnstsw ax\n\ttest ah, 41h\n\tjp %s\n", cc_operand_text(src, 0, szsrc), as->_target->_x._name);
				break;
			default:
				assert(0); break;
			}
		}
			break;
		case X86_JGE:
		{
			switch (dst->_tycode)
			{
			case IR_S32:
				if (src->_format == FormatInSIB) {
					fprintf(ctx->_outfp, "\tcmp %s, dword ptr[%s]\n\tjge %s\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc), as->_target->_x._name);
				}
				else {
					fprintf(ctx->_outfp, "\tcmp %s, %s\n\tjge %s\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc), as->_target->_x._name);
				}
				break;
			case IR_U32:
				if (src->_format == FormatInSIB) {
					fprintf(ctx->_outfp, "\tcmp %s, dword ptr[%s]\n\tjae %s\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc), as->_target->_x._name);
				}
				else {
					fprintf(ctx->_outfp, "\tcmp %s, %s\n\tjae %s\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc), as->_target->_x._name);
				}
				break;
			case IR_S64:
				if (src->_format == FormatInSIB) {
					fprintf(ctx->_outfp, "\tcmp %s, dword ptr[%s]\n\tjg %s\n\tjl @F\n", cc_operand_text(dst, 1, szdst), cc_operand_text(src, 1, szsrc), as->_target->_x._name);
					fprintf(ctx->_outfp, "\tcmp %s, dword ptr[%s]\n\tjae %s\n@@:\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc), as->_target->_x._name);
				}
				else {
					fprintf(ctx->_outfp, "\tcmp %s, %s\n\tjg %s\n\tjl @F\n", cc_operand_text(dst, 1, szdst), cc_operand_text(src, 1, szsrc), as->_target->_x._name);
					fprintf(ctx->_outfp, "\tcmp %s, %s\n\tjae %s\n@@:\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc), as->_target->_x._name);
				}
				break;
			case IR_U64:
				if (src->_format == FormatInSIB) {
					fprintf(ctx->_outfp, "\tcmp %s, dword ptr[%s]\n\tja %s\n\tjb @F\n", cc_operand_text(dst, 1, szdst), cc_operand_text(src, 1, szsrc), as->_target->_x._name);
					fprintf(ctx->_outfp, "\tcmp %s, dword ptr[%s]\n\tjae %s\n@@:\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc), as->_target->_x._name);
				}
				else {
					fprintf(ctx->_outfp, "\tcmp %s, %s\n\tja %s\n\tjb @F\n", cc_operand_text(dst, 1, szdst), cc_operand_text(src, 1, szsrc), as->_target->_x._name);
					fprintf(ctx->_outfp, "\tcmp %s, %s\n\tjae %s\n@@:\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc), as->_target->_x._name);
				}
				break;
			case IR_F32:
				fprintf(ctx->_outfp, "\tfcom dword ptr[%s]\n\tfnstsw ax\n\ttest ah, 41h\n\tjne %s\n", cc_operand_text(src, 0, szsrc), as->_target->_x._name);
				break;
			case IR_F64:
				fprintf(ctx->_outfp, "\tfcom qword ptr[%s]\n\tfnstsw ax\n\ttest ah, 41h\n\tjne %s\n", cc_operand_text(src, 0, szsrc), as->_target->_x._name);
				break;
			default:
				assert(0); break;
			}
		}
			break;
		case X86_JLE:
		{
			switch (dst->_tycode)
			{
			case IR_S32:
				if (src->_format == FormatInSIB) {
					fprintf(ctx->_outfp, "\tcmp %s, dword ptr[%s]\n\tjle %s\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc), as->_target->_x._name);
				}
				else {
					fprintf(ctx->_outfp, "\tcmp %s, %s\n\tjle %s\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc), as->_target->_x._name);
				}
				break;
			case IR_U32:
				if (src->_format == FormatInSIB) {
					fprintf(ctx->_outfp, "\tcmp %s, dword ptr[%s]\n\tjbe %s\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc), as->_target->_x._name);
				}
				else {
					fprintf(ctx->_outfp, "\tcmp %s, %s\n\tjbe %s\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc), as->_target->_x._name);
				}
				break;
			case IR_S64:
				if (src->_format == FormatInSIB) {
					fprintf(ctx->_outfp, "\tcmp %s, dword ptr[%s]\n\tjl %s\n\tjg @F\n", cc_operand_text(dst, 1, szdst), cc_operand_text(src, 1, szsrc), as->_target->_x._name);
					fprintf(ctx->_outfp, "\tcmp %s, dword ptr[%s]\n\tjbe %s\n@@:\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc), as->_target->_x._name);
				}
				else {
					fprintf(ctx->_outfp, "\tcmp %s, %s\n\tjl %s\n\tjg @F\n", cc_operand_text(dst, 1, szdst), cc_operand_text(src, 1, szsrc), as->_target->_x._name);
					fprintf(ctx->_outfp, "\tcmp %s, %s\n\tjbe %s\n@@:\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc), as->_target->_x._name);
				}
				break;
			case IR_U64:
				if (src->_format == FormatInSIB) {
					fprintf(ctx->_outfp, "\tcmp %s, dword ptr[%s]\n\tjb %s\n\tja @F\n", cc_operand_text(dst, 1, szdst), cc_operand_text(src, 1, szsrc), as->_target->_x._name);
					fprintf(ctx->_outfp, "\tcmp %s, dword ptr[%s]\n\tjbe %s\n@@:\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc), as->_target->_x._name);
				}
				else {
					fprintf(ctx->_outfp, "\tcmp %s, %s\n\tjb %s\n\tja @F\n", cc_operand_text(dst, 1, szdst), cc_operand_text(src, 1, szsrc), as->_target->_x._name);
					fprintf(ctx->_outfp, "\tcmp %s, %s\n\tjbe %s\n@@:\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc), as->_target->_x._name);
				}
				break;
			case IR_F32:
				fprintf(ctx->_outfp, "\tfcom dword ptr[%s]\n\tfnstsw ax\n\ttest ah, 5h\n\tjp %s\n", cc_operand_text(src, 0, szsrc), as->_target->_x._name);
				break;
			case IR_F64:
				fprintf(ctx->_outfp, "\tfcom qword ptr[%s]\n\tfnstsw ax\n\ttest ah, 5h\n\tjp %s\n", cc_operand_text(src, 0, szsrc), as->_target->_x._name);
				break;
			default:
				assert(0); break;
			}
		}
			break;
		case X86_JMP:
			fprintf(ctx->_outfp, "\tjmp %s\n", as->_target->_x._name); 
			break;
		case X86_CVT: /* convert */
		{
			#define MAKE_CVT_ID(dstty, srcty, dstfmt, srcfmt)		(((dstty) << 24) | ((srcty) << 16) | ((dstfmt) << 8) | ((srcfmt)))

			int dsttycode = dst->_tycode;
			int srctycode = src->_tycode;
			
			if (dsttycode == IR_PTR) { dsttycode = IR_U32; }
			if (srctycode == IR_PTR) { srctycode = IR_U32; }
			
			switch (MAKE_CVT_ID(dsttycode, srctycode, dst->_format, src->_format))
			{
			case MAKE_CVT_ID(IR_S8, IR_S8, FormatReg, FormatReg):
			case MAKE_CVT_ID(IR_U8, IR_S8, FormatReg, FormatReg):
				assert(0);
			case MAKE_CVT_ID(IR_S16, IR_S8, FormatReg, FormatReg):
			case MAKE_CVT_ID(IR_U16, IR_S8, FormatReg, FormatReg):
			case MAKE_CVT_ID(IR_S32, IR_S8, FormatReg, FormatReg):
			case MAKE_CVT_ID(IR_U32, IR_S8, FormatReg, FormatReg):
				fprintf(ctx->_outfp, "\tmovsx %s, %s\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc)); break;
			case MAKE_CVT_ID(IR_S64, IR_S8, FormatReg, FormatReg):
			case MAKE_CVT_ID(IR_U64, IR_S8, FormatReg, FormatReg):
				fprintf(ctx->_outfp, "\tmovsx %s, %s\n\tcdq\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc)); break;
			case MAKE_CVT_ID(IR_S8, IR_S8, FormatReg, FormatInSIB):
			case MAKE_CVT_ID(IR_U8, IR_S8, FormatReg, FormatInSIB):
				assert(0);
			case MAKE_CVT_ID(IR_S16, IR_S8, FormatReg, FormatInSIB):
			case MAKE_CVT_ID(IR_U16, IR_S8, FormatReg, FormatInSIB):
			case MAKE_CVT_ID(IR_S32, IR_S8, FormatReg, FormatInSIB):
			case MAKE_CVT_ID(IR_U32, IR_S8, FormatReg, FormatInSIB):
				fprintf(ctx->_outfp, "\tmovsx %s, byte ptr [%s]\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc)); break;
			case MAKE_CVT_ID(IR_S64, IR_S8, FormatReg, FormatInSIB):
			case MAKE_CVT_ID(IR_U64, IR_S8, FormatReg, FormatInSIB):
				fprintf(ctx->_outfp, "\tmovsx %s, byte ptr [%s]\ncdq\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc)); break;

			case MAKE_CVT_ID(IR_S8, IR_U8, FormatReg, FormatReg):
			case MAKE_CVT_ID(IR_U8, IR_U8, FormatReg, FormatReg):
				assert(0);
			case MAKE_CVT_ID(IR_S16, IR_U8, FormatReg, FormatReg):
			case MAKE_CVT_ID(IR_U16, IR_U8, FormatReg, FormatReg):
			case MAKE_CVT_ID(IR_S32, IR_U8, FormatReg, FormatReg):
			case MAKE_CVT_ID(IR_U32, IR_U8, FormatReg, FormatReg):
				fprintf(ctx->_outfp, "\tmovzx %s, %s\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc)); break;
			case MAKE_CVT_ID(IR_S64, IR_U8, FormatReg, FormatReg):
			case MAKE_CVT_ID(IR_U64, IR_U8, FormatReg, FormatReg):
				fprintf(ctx->_outfp, "\tmovzx %s, %s\n\tcdq\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc)); break;

			case MAKE_CVT_ID(IR_S8, IR_U8, FormatReg, FormatInSIB):
			case MAKE_CVT_ID(IR_U8, IR_U8, FormatReg, FormatInSIB):
				assert(0);
			case MAKE_CVT_ID(IR_S16, IR_U8, FormatReg, FormatInSIB):
			case MAKE_CVT_ID(IR_U16, IR_U8, FormatReg, FormatInSIB):
			case MAKE_CVT_ID(IR_S32, IR_U8, FormatReg, FormatInSIB):
			case MAKE_CVT_ID(IR_U32, IR_U8, FormatReg, FormatInSIB):
				fprintf(ctx->_outfp, "\tmovzx %s, byte ptr [%s]\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc)); break;
			case MAKE_CVT_ID(IR_S64, IR_U8, FormatReg, FormatInSIB):
			case MAKE_CVT_ID(IR_U64, IR_U8, FormatReg, FormatInSIB):
				fprintf(ctx->_outfp, "\tmovzx %s, byte ptr [%s]\n\tcdq\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc)); break;

			case MAKE_CVT_ID(IR_S8, IR_S16, FormatReg, FormatReg):
			case MAKE_CVT_ID(IR_U8, IR_S16, FormatReg, FormatReg):
				tmp = *src; tmp._tycode = IR_S8;
				fprintf(ctx->_outfp, "\tmov %s, %s\n", cc_operand_text(dst, 0, szdst), cc_operand_text(&tmp, 0, szsrc)); break;
			case MAKE_CVT_ID(IR_S16, IR_S16, FormatReg, FormatReg):
			case MAKE_CVT_ID(IR_U16, IR_S16, FormatReg, FormatReg):
			case MAKE_CVT_ID(IR_S32, IR_S16, FormatReg, FormatReg):
			case MAKE_CVT_ID(IR_U32, IR_S16, FormatReg, FormatReg):
				fprintf(ctx->_outfp, "\tmovsx %s, %s\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc)); break;
			case MAKE_CVT_ID(IR_S64, IR_S16, FormatReg, FormatReg):
			case MAKE_CVT_ID(IR_U64, IR_S16, FormatReg, FormatReg):
				fprintf(ctx->_outfp, "\tmovsx %s, %s\n\tcdq\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc)); break;
			case MAKE_CVT_ID(IR_S8, IR_S16, FormatReg, FormatInSIB):
			case MAKE_CVT_ID(IR_U8, IR_S16, FormatReg, FormatInSIB):
				fprintf(ctx->_outfp, "\tmov %s, byte ptr[%s]\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc)); break;
			case MAKE_CVT_ID(IR_S16, IR_S16, FormatReg, FormatInSIB):
			case MAKE_CVT_ID(IR_U16, IR_S16, FormatReg, FormatInSIB):
			case MAKE_CVT_ID(IR_S32, IR_S16, FormatReg, FormatInSIB):
			case MAKE_CVT_ID(IR_U32, IR_S16, FormatReg, FormatInSIB):
				fprintf(ctx->_outfp, "\tmovsx %s, word ptr [%s]\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc)); break;
			case MAKE_CVT_ID(IR_S64, IR_S16, FormatReg, FormatInSIB):
			case MAKE_CVT_ID(IR_U64, IR_S16, FormatReg, FormatInSIB):
				fprintf(ctx->_outfp, "\tmovsx %s, word ptr [%s]\n\tcdq\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc)); break;

			case MAKE_CVT_ID(IR_S8, IR_U16, FormatReg, FormatReg):
			case MAKE_CVT_ID(IR_U8, IR_U16, FormatReg, FormatReg):
				tmp = *src; tmp._tycode = IR_S8;
				fprintf(ctx->_outfp, "\tmov %s, %s\n", cc_operand_text(dst, 0, szdst), cc_operand_text(&tmp, 0, szsrc)); break;
			case MAKE_CVT_ID(IR_S16, IR_U16, FormatReg, FormatReg):
			case MAKE_CVT_ID(IR_U16, IR_U16, FormatReg, FormatReg):
				assert(0);
			case MAKE_CVT_ID(IR_S32, IR_U16, FormatReg, FormatReg):
			case MAKE_CVT_ID(IR_U32, IR_U16, FormatReg, FormatReg):
				fprintf(ctx->_outfp, "\tmovzx %s, %s\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc)); break;
			case MAKE_CVT_ID(IR_S64, IR_U16, FormatReg, FormatReg):
			case MAKE_CVT_ID(IR_U64, IR_U16, FormatReg, FormatReg):
				fprintf(ctx->_outfp, "\tmovzx %s, %s\n\tcdq\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc)); break;

			case MAKE_CVT_ID(IR_S8, IR_U16, FormatReg, FormatInSIB):
			case MAKE_CVT_ID(IR_U8, IR_U16, FormatReg, FormatInSIB):
				fprintf(ctx->_outfp, "\tmov %s, byte ptr[%s]\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc)); break;
			case MAKE_CVT_ID(IR_S16, IR_U16, FormatReg, FormatInSIB):
			case MAKE_CVT_ID(IR_U16, IR_U16, FormatReg, FormatInSIB):
				assert(0);
			case MAKE_CVT_ID(IR_S32, IR_U16, FormatReg, FormatInSIB):
			case MAKE_CVT_ID(IR_U32, IR_U16, FormatReg, FormatInSIB):
				fprintf(ctx->_outfp, "\tmovzx %s, word ptr [%s]\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc)); break;
			case MAKE_CVT_ID(IR_S64, IR_U16, FormatReg, FormatInSIB):
			case MAKE_CVT_ID(IR_U64, IR_U16, FormatReg, FormatInSIB):
				fprintf(ctx->_outfp, "\tmovzx %s, word ptr [%s]\n\tcdq\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc)); break;

			case MAKE_CVT_ID(IR_S8,  IR_S32, FormatReg, FormatReg):
			case MAKE_CVT_ID(IR_U8,  IR_S32, FormatReg, FormatReg):
				tmp = *src; tmp._tycode = IR_S8;
				fprintf(ctx->_outfp, "\tmov %s, %s\n", cc_operand_text(dst, 0, szdst), cc_operand_text(&tmp, 0, szsrc)); break;
			case MAKE_CVT_ID(IR_S16, IR_S32, FormatReg, FormatReg):
			case MAKE_CVT_ID(IR_U16, IR_S32, FormatReg, FormatReg):
				tmp = *src; tmp._tycode = IR_S16;
				fprintf(ctx->_outfp, "\tmov %s, %s\n", cc_operand_text(dst, 0, szdst), cc_operand_text(&tmp, 0, szsrc)); break;
			case MAKE_CVT_ID(IR_S32, IR_S32, FormatReg, FormatReg):
			case MAKE_CVT_ID(IR_U32, IR_S32, FormatReg, FormatReg):
				assert(0);
			case MAKE_CVT_ID(IR_S64, IR_S32, FormatReg, FormatReg):
			case MAKE_CVT_ID(IR_U64, IR_S32, FormatReg, FormatReg):
				fprintf(ctx->_outfp, "\tmov %s, %s\n\tcdq\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc)); break;
			case MAKE_CVT_ID(IR_F32, IR_S32, FormatInSIB, FormatReg):
				fprintf(ctx->_outfp, "\tpush %s\n\tfild dword ptr [esp]\n\tfstp dword ptr [%s]\n\tadd esp, 4\n", 
					cc_operand_text(src, 0, szsrc), cc_operand_text(dst, 0, szdst)); 
				break;
			case MAKE_CVT_ID(IR_F32, IR_S32, FormatInSIB, FormatInSIB):
				fprintf(ctx->_outfp, "\tfild dword ptr [%s]\n\tfstp dword ptr [%s]\n",
					cc_operand_text(src, 0, szsrc), cc_operand_text(dst, 0, szdst));
				break;
			case MAKE_CVT_ID(IR_F64, IR_S32, FormatInSIB, FormatReg):
				fprintf(ctx->_outfp, "\tpush %s\n\tfild dword ptr [esp]\n\tfstp qword ptr [%s]\n\tadd esp, 4\n",
					cc_operand_text(src, 0, szsrc), cc_operand_text(dst, 0, szdst));
				break;
			case MAKE_CVT_ID(IR_F64, IR_S32, FormatInSIB, FormatInSIB):
				fprintf(ctx->_outfp, "\tfild dword ptr [%s]\n\tfstp qword ptr [%s]\n",
					cc_operand_text(src, 0, szsrc), cc_operand_text(dst, 0, szdst));
				break;

			case MAKE_CVT_ID(IR_S8, IR_S32, FormatReg, FormatInSIB):
			case MAKE_CVT_ID(IR_U8, IR_S32, FormatReg, FormatInSIB):
				fprintf(ctx->_outfp, "\tmov %s, byte ptr[%s]\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc)); break;
			case MAKE_CVT_ID(IR_S16, IR_S32, FormatReg, FormatInSIB):
			case MAKE_CVT_ID(IR_U16, IR_S32, FormatReg, FormatInSIB):
				fprintf(ctx->_outfp, "\tmov %s, word ptr[%s]\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc)); break;
			case MAKE_CVT_ID(IR_S32, IR_S32, FormatReg, FormatInSIB):
			case MAKE_CVT_ID(IR_U32, IR_S32, FormatReg, FormatInSIB):
				assert(0);
			case MAKE_CVT_ID(IR_S64, IR_S32, FormatReg, FormatInSIB):
			case MAKE_CVT_ID(IR_U64, IR_S32, FormatReg, FormatInSIB):
				fprintf(ctx->_outfp, "\tmov %s, dword ptr[%s]\n\tcdq\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc)); break;

			case MAKE_CVT_ID(IR_S8, IR_U32, FormatReg, FormatReg):
			case MAKE_CVT_ID(IR_U8, IR_U32, FormatReg, FormatReg):
				tmp = *src; tmp._tycode = IR_S8;
				fprintf(ctx->_outfp, "\tmov %s, %s\n", cc_operand_text(dst, 0, szdst), cc_operand_text(&tmp, 0, szsrc)); break;
			case MAKE_CVT_ID(IR_S16, IR_U32, FormatReg, FormatReg):
			case MAKE_CVT_ID(IR_U16, IR_U32, FormatReg, FormatReg):
				tmp = *src; tmp._tycode = IR_S16;
				fprintf(ctx->_outfp, "\tmov %s, %s\n", cc_operand_text(dst, 0, szdst), cc_operand_text(&tmp, 0, szsrc)); break;
			case MAKE_CVT_ID(IR_S32, IR_U32, FormatReg, FormatReg):
			case MAKE_CVT_ID(IR_U32, IR_U32, FormatReg, FormatReg):
				assert(0);
			case MAKE_CVT_ID(IR_S64, IR_U32, FormatReg, FormatReg):
			case MAKE_CVT_ID(IR_U64, IR_U32, FormatReg, FormatReg):
				fprintf(ctx->_outfp, "\tmov %s, %s\n\txor edx, edx\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc)); break;
			case MAKE_CVT_ID(IR_F32, IR_U32, FormatInSIB, FormatReg):
				fprintf(ctx->_outfp, "\tpush 0\n\tpush %s\n\tfild qword ptr [esp]\n\tfstp dword ptr [%s]\n\tadd esp, 8\n",
					cc_operand_text(src, 0, szsrc), cc_operand_text(dst, 0, szdst));
				break;
			case MAKE_CVT_ID(IR_F32, IR_U32, FormatInSIB, FormatInSIB):
				fprintf(ctx->_outfp, "\tpush 0\n\tpush dword ptr[%s]\n\tfild qword ptr [esp]\n\tfstp dword ptr [%s]\n\tadd esp, 8\n",
					cc_operand_text(src, 0, szsrc), cc_operand_text(dst, 0, szdst));
				break;
			case MAKE_CVT_ID(IR_F64, IR_U32, FormatInSIB, FormatReg):
				fprintf(ctx->_outfp, "\tpush 0\n\tpush %s\n\tfild qword ptr [esp]\n\tfstp qword ptr [%s]\n\tadd esp, 8\n",
					cc_operand_text(src, 0, szsrc), cc_operand_text(dst, 0, szdst));
				break;
			case MAKE_CVT_ID(IR_F64, IR_U32, FormatInSIB, FormatInSIB):
				fprintf(ctx->_outfp, "\tpush 0\n\tpush dword ptr[%s]\n\tfild qword ptr [esp]\n\tfstp qword ptr [%s]\n\tadd esp, 8\n",
					cc_operand_text(src, 0, szsrc), cc_operand_text(dst, 0, szdst));
				break;

			case MAKE_CVT_ID(IR_S8, IR_U32, FormatReg, FormatInSIB):
			case MAKE_CVT_ID(IR_U8, IR_U32, FormatReg, FormatInSIB):
				fprintf(ctx->_outfp, "\tmov %s, byte ptr[%s]\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc)); break;
			case MAKE_CVT_ID(IR_S16, IR_U32, FormatReg, FormatInSIB):
			case MAKE_CVT_ID(IR_U16, IR_U32, FormatReg, FormatInSIB):
				fprintf(ctx->_outfp, "\tmov %s, word ptr[%s]\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc)); break;
			case MAKE_CVT_ID(IR_S32, IR_U32, FormatReg, FormatInSIB):
			case MAKE_CVT_ID(IR_U32, IR_U32, FormatReg, FormatInSIB):
				assert(0);
			case MAKE_CVT_ID(IR_S64, IR_U32, FormatReg, FormatInSIB):
			case MAKE_CVT_ID(IR_U64, IR_U32, FormatReg, FormatInSIB):
				fprintf(ctx->_outfp, "\tmov %s, dword ptr[%s]\n\txor edx, edx\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc)); break;

			case MAKE_CVT_ID(IR_S8, IR_S64, FormatReg, FormatReg):
			case MAKE_CVT_ID(IR_U8, IR_S64, FormatReg, FormatReg):
			case MAKE_CVT_ID(IR_S8, IR_U64, FormatReg, FormatReg):
			case MAKE_CVT_ID(IR_U8, IR_U64, FormatReg, FormatReg):
				tmp = *src; tmp._tycode = IR_S8;
				fprintf(ctx->_outfp, "\tmov %s, %s\n", cc_operand_text(dst, 0, szdst), cc_operand_text(&tmp, 0, szsrc)); break;
			case MAKE_CVT_ID(IR_S16, IR_S64, FormatReg, FormatReg):
			case MAKE_CVT_ID(IR_U16, IR_S64, FormatReg, FormatReg):
			case MAKE_CVT_ID(IR_S16, IR_U64, FormatReg, FormatReg):
			case MAKE_CVT_ID(IR_U16, IR_U64, FormatReg, FormatReg):
				tmp = *src; tmp._tycode = IR_S16;
				fprintf(ctx->_outfp, "\tmov %s, %s\n", cc_operand_text(dst, 0, szdst), cc_operand_text(&tmp, 0, szsrc)); break;
			case MAKE_CVT_ID(IR_S32, IR_S64, FormatReg, FormatReg):
			case MAKE_CVT_ID(IR_U32, IR_S64, FormatReg, FormatReg):
			case MAKE_CVT_ID(IR_S32, IR_U64, FormatReg, FormatReg):
			case MAKE_CVT_ID(IR_U32, IR_U64, FormatReg, FormatReg):
				fprintf(ctx->_outfp, "\tmov %s, %s\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc)); break;
			case MAKE_CVT_ID(IR_S64, IR_S64, FormatReg, FormatReg):
			case MAKE_CVT_ID(IR_U64, IR_S64, FormatReg, FormatReg):
			case MAKE_CVT_ID(IR_S64, IR_U64, FormatReg, FormatReg):
			case MAKE_CVT_ID(IR_U64, IR_U64, FormatReg, FormatReg):
				assert(0);
			case MAKE_CVT_ID(IR_F32, IR_S64, FormatReg, FormatReg):
			case MAKE_CVT_ID(IR_F64, IR_S64, FormatReg, FormatReg):
			case MAKE_CVT_ID(IR_F32, IR_U64, FormatReg, FormatReg):
			case MAKE_CVT_ID(IR_F64, IR_U64, FormatReg, FormatReg):
				assert(0);
			case MAKE_CVT_ID(IR_F32, IR_S64, FormatInSIB, FormatReg):
			case MAKE_CVT_ID(IR_F32, IR_U64, FormatInSIB, FormatReg):
				fprintf(ctx->_outfp, "\tpush %s\n\tpush %s\n\tfild qword ptr [esp]\n\tfstp dword ptr [%s]\n\tadd esp, 8\n",
					cc_operand_text(src, 1, szsrc), cc_operand_text(src, 0, szsrc), cc_operand_text(dst, 0, szdst));
				break;
			case MAKE_CVT_ID(IR_F32, IR_S64, FormatInSIB, FormatInSIB):
			case MAKE_CVT_ID(IR_F32, IR_U64, FormatInSIB, FormatInSIB):
				fprintf(ctx->_outfp, "\tfild qword ptr [%s]\n\tfstp dword ptr [%s]\n",
					cc_operand_text(src, 0, szsrc), cc_operand_text(dst, 0, szdst));
				break;
			case MAKE_CVT_ID(IR_F64, IR_S64, FormatInSIB, FormatReg):
			case MAKE_CVT_ID(IR_F64, IR_U64, FormatInSIB, FormatReg):
				fprintf(ctx->_outfp, "\tpush %s\n\tpush %s\n\tfild qword ptr [esp]\n\tfstp qword ptr [%s]\n\tadd esp, 8\n",
					cc_operand_text(src, 1, szsrc), cc_operand_text(src, 0, szsrc), cc_operand_text(dst, 0, szdst));
				break;
			case MAKE_CVT_ID(IR_F64, IR_S64, FormatInSIB, FormatInSIB):
			case MAKE_CVT_ID(IR_F64, IR_U64, FormatInSIB, FormatInSIB):
				fprintf(ctx->_outfp, "\tfild qword ptr [%s]\n\tfstp qword ptr [%s]\n",
					cc_operand_text(src, 0, szsrc), cc_operand_text(dst, 0, szdst));
				break;


			case MAKE_CVT_ID(IR_S8, IR_S64, FormatReg, FormatInSIB):
			case MAKE_CVT_ID(IR_U8, IR_S64, FormatReg, FormatInSIB):
			case MAKE_CVT_ID(IR_S8, IR_U64, FormatReg, FormatInSIB):
			case MAKE_CVT_ID(IR_U8, IR_U64, FormatReg, FormatInSIB):
				fprintf(ctx->_outfp, "\tmov %s, byte ptr[%s]\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc)); break;
			case MAKE_CVT_ID(IR_S16, IR_S64, FormatReg, FormatInSIB):
			case MAKE_CVT_ID(IR_U16, IR_S64, FormatReg, FormatInSIB):
			case MAKE_CVT_ID(IR_S16, IR_U64, FormatReg, FormatInSIB):
			case MAKE_CVT_ID(IR_U16, IR_U64, FormatReg, FormatInSIB):
				fprintf(ctx->_outfp, "\tmov %s, word ptr[%s]\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc)); break;
			case MAKE_CVT_ID(IR_S32, IR_S64, FormatReg, FormatInSIB):
			case MAKE_CVT_ID(IR_U32, IR_S64, FormatReg, FormatInSIB):
			case MAKE_CVT_ID(IR_S32, IR_U64, FormatReg, FormatInSIB):
			case MAKE_CVT_ID(IR_U32, IR_U64, FormatReg, FormatInSIB):
				fprintf(ctx->_outfp, "\tmov %s, dword ptr[%s]\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc)); break;
			case MAKE_CVT_ID(IR_S64, IR_S64, FormatReg, FormatInSIB):
			case MAKE_CVT_ID(IR_U64, IR_S64, FormatReg, FormatInSIB):
			case MAKE_CVT_ID(IR_S64, IR_U64, FormatReg, FormatInSIB):
			case MAKE_CVT_ID(IR_U64, IR_U64, FormatReg, FormatInSIB):
				assert(0);

			case MAKE_CVT_ID(IR_S8, IR_F32, FormatReg, FormatReg):
			case MAKE_CVT_ID(IR_S16, IR_F32, FormatReg, FormatReg):
			case MAKE_CVT_ID(IR_S32, IR_F32, FormatReg, FormatReg):
			case MAKE_CVT_ID(IR_U8, IR_F32, FormatReg, FormatReg):
			case MAKE_CVT_ID(IR_U16, IR_F32, FormatReg, FormatReg):
			case MAKE_CVT_ID(IR_U32, IR_F32, FormatReg, FormatReg):
				fprintf(ctx->_outfp, "\tsub esp, 16\n\tfnstcw word ptr[esp]\n\tmovzx eax, word ptr[esp]\n"
									 "\tor eax, 0c00H\n\tmov 4[esp], eax\n\tfldcw 4[esp]\n\tfistp dword ptr 8[esp]\n"
									 "\tfldcw [esp]\n\tmov eax, 8[esp]\n\tadd esp, 16\n"); 
				break;
			case MAKE_CVT_ID(IR_S64, IR_F32, FormatReg, FormatReg):
			case MAKE_CVT_ID(IR_U64, IR_F32, FormatReg, FormatReg):
				fprintf(ctx->_outfp, "\tsub esp, 16\n\tfnstcw word ptr[esp]\n\tmovzx eax, word ptr[esp]\n"
					"\tor eax, 0c00H\n\tmov 4[esp], eax\n\tfldcw 4[esp]\n\tfistp dword ptr 8[esp]\n"
					"\tfldcw [esp]\n\tmov eax, 8[esp]\n\tmov edx, 12[esp]\n\tadd esp, 16\n");
				break;
			case MAKE_CVT_ID(IR_F32, IR_F32, FormatReg, FormatReg):
			case MAKE_CVT_ID(IR_F64, IR_F32, FormatReg, FormatReg):
				assert(0);
			
			case MAKE_CVT_ID(IR_S8, IR_F32, FormatReg, FormatInSIB):
			case MAKE_CVT_ID(IR_U8, IR_F32, FormatReg, FormatInSIB):
			case MAKE_CVT_ID(IR_S16, IR_F32, FormatReg, FormatInSIB):
			case MAKE_CVT_ID(IR_U16, IR_F32, FormatReg, FormatInSIB):
			case MAKE_CVT_ID(IR_S32, IR_F32, FormatReg, FormatInSIB):
			case MAKE_CVT_ID(IR_U32, IR_F32, FormatReg, FormatInSIB):
				fprintf(ctx->_outfp, "\tfld dword ptr[%s]\n\tsub esp, 16\n\tfnstcw word ptr[esp]\n\tmovzx eax, word ptr[esp]\n"
					"\tor eax, 0c00H\n\tmov 4[esp], eax\n\tfldcw 4[esp]\n\tfistp dword ptr 8[esp]\n"
					"\tfldcw [esp]\n\tmov eax, 8[esp]\n\tadd esp, 16\n", cc_operand_text(src, 0, szsrc));
				break;
			case MAKE_CVT_ID(IR_S64, IR_F32, FormatReg, FormatInSIB):
			case MAKE_CVT_ID(IR_U64, IR_F32, FormatReg, FormatInSIB):
				fprintf(ctx->_outfp, "\tfld dword ptr[%s]\n\tsub esp, 16\n\tfnstcw word ptr[esp]\n\tmovzx eax, word ptr[esp]\n"
					"\tor eax, 0c00H\n\tmov 4[esp], eax\n\tfldcw 4[esp]\n\tfistp dword ptr 8[esp]\n"
					"\tfldcw [esp]\n\tmov eax, 8[esp]\n\tmov edx, 12[esp]\n\tadd esp, 16\n", cc_operand_text(src, 0, szsrc));
				break;
			case MAKE_CVT_ID(IR_F32, IR_F32, FormatReg, FormatInSIB):
			case MAKE_CVT_ID(IR_F64, IR_F32, FormatReg, FormatInSIB):
				fprintf(ctx->_outfp, "\tfld dword ptr[%s]\n", cc_operand_text(src, 0, szsrc));
				break;
			case MAKE_CVT_ID(IR_F64, IR_F32, FormatInSIB, FormatInSIB):
				fprintf(ctx->_outfp, "\tfld dword ptr[%s]\n\tfstp qword ptr[%s]\n", cc_operand_text(src, 0, szsrc), cc_operand_text(dst, 0, szdst));
				break;
			case MAKE_CVT_ID(IR_F64, IR_F32, FormatInSIB, FormatReg):
				fprintf(ctx->_outfp, "\tfst qword ptr[%s]\n", cc_operand_text(dst, 0, szdst));
				break;

			/* double to integer */
			case MAKE_CVT_ID(IR_S8, IR_F64, FormatReg, FormatReg):
			case MAKE_CVT_ID(IR_S16, IR_F64, FormatReg, FormatReg):
			case MAKE_CVT_ID(IR_S32, IR_F64, FormatReg, FormatReg):
			case MAKE_CVT_ID(IR_U8, IR_F64, FormatReg, FormatReg):
			case MAKE_CVT_ID(IR_U16, IR_F64, FormatReg, FormatReg):
			case MAKE_CVT_ID(IR_U32, IR_F64, FormatReg, FormatReg):
				fprintf(ctx->_outfp, "\tsub esp, 16\n\tfnstcw word ptr[esp]\n\tmovzx eax, word ptr[esp]\n"
					"\tor eax, 0c00H\n\tmov 4[esp], eax\n\tfldcw 4[esp]\n\tfistp dword ptr 8[esp]\n"
					"\tfldcw [esp]\n\tmov eax, 8[esp]\n\tadd esp, 16\n");
				break;
			case MAKE_CVT_ID(IR_S64, IR_F64, FormatReg, FormatReg):
			case MAKE_CVT_ID(IR_U64, IR_F64, FormatReg, FormatReg): /* edx:eax */
				fprintf(ctx->_outfp, "\tsub esp, 16\n\tfnstcw word ptr[esp]\n\tmovzx eax, word ptr[esp]\n"
					"\tor eax, 0c00H\n\tmov 4[esp], eax\n\tfldcw 4[esp]\n\tfistp dword ptr 8[esp]\n"
					"\tfldcw [esp]\n\tmov eax, 8[esp]\n\tmov edx, 12[esp]\n\tadd esp, 16\n");
				break;
			case MAKE_CVT_ID(IR_F32, IR_F64, FormatReg, FormatReg):
			case MAKE_CVT_ID(IR_F64, IR_F64, FormatReg, FormatReg):
				assert(0);

			case MAKE_CVT_ID(IR_S8, IR_F64, FormatReg, FormatInSIB):
			case MAKE_CVT_ID(IR_U8, IR_F64, FormatReg, FormatInSIB):
			case MAKE_CVT_ID(IR_S16, IR_F64, FormatReg, FormatInSIB):
			case MAKE_CVT_ID(IR_U16, IR_F64, FormatReg, FormatInSIB):
			case MAKE_CVT_ID(IR_S32, IR_F64, FormatReg, FormatInSIB):
			case MAKE_CVT_ID(IR_U32, IR_F64, FormatReg, FormatInSIB):
				fprintf(ctx->_outfp, "\tfld qword ptr[%s]\n\tsub esp, 16\n\tfnstcw word ptr[esp]\n\tmovzx eax, word ptr[esp]\n"
					"\tor eax, 0c00H\n\tmov 4[esp], eax\n\tfldcw 4[esp]\n\tfistp dword ptr 8[esp]\n"
					"\tfldcw [esp]\n\tmov eax, 8[esp]\n\tadd esp, 16\n", cc_operand_text(src, 0, szsrc));
				break;
			case MAKE_CVT_ID(IR_S64, IR_F64, FormatReg, FormatInSIB):
			case MAKE_CVT_ID(IR_U64, IR_F64, FormatReg, FormatInSIB):
				fprintf(ctx->_outfp, "\tfld qword ptr[%s]\n\tsub esp, 16\n\tfnstcw word ptr[esp]\n\tmovzx eax, word ptr[esp]\n"
					"\tor eax, 0c00H\n\tmov 4[esp], eax\n\tfldcw 4[esp]\n\tfistp dword ptr 8[esp]\n"
					"\tfldcw [esp]\n\tmov eax, 8[esp]\n\tmov edx, 12[esp]\n\tadd esp, 16\n", cc_operand_text(src, 0, szsrc));
				break;
			case MAKE_CVT_ID(IR_F32, IR_F64, FormatReg, FormatInSIB):
			case MAKE_CVT_ID(IR_F64, IR_F64, FormatReg, FormatInSIB):
				fprintf(ctx->_outfp, "\tfld qword ptr[%s]\n", cc_operand_text(src, 0, szsrc));
				break;
			case MAKE_CVT_ID(IR_F32, IR_F64, FormatInSIB, FormatInSIB):
				fprintf(ctx->_outfp, "\tfld qword ptr[%s]\n\tfstp dword ptr[%s]\n", cc_operand_text(src, 0, szsrc), cc_operand_text(dst, 0, szdst));
				break;
			case MAKE_CVT_ID(IR_F32, IR_F64, FormatInSIB, FormatReg):
				fprintf(ctx->_outfp, "\tfst dword ptr[%s]\n", cc_operand_text(dst, 0, szdst));
				break;
			default:
				assert(0); break;
			}
		}
			break;
		case X86_MOV:
		{
			switch (src->_tycode)
			{
			case IR_S8:
			case IR_U8:
			{
				if (dst->_format == FormatSIB)
				{
					fprintf(ctx->_outfp, "\tmov byte ptr [%s], %s\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc));
				}
				else
				{
					if (src->_format == FormatInSIB) {
						fprintf(ctx->_outfp, "\tmov %s, byte ptr [%s]\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc));
					}
					else if (src->_format == FormatSIB) {
						fprintf(ctx->_outfp, "\tlea %s, byte ptr [%s]\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc));
					}
					else {
						fprintf(ctx->_outfp, "\tmov %s, %s\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc));
					}
				}
			}
				break;
			case IR_S16:
			case IR_U16:
			{
				if (dst->_format == FormatSIB)
				{
					fprintf(ctx->_outfp, "\tmov word ptr [%s], %s\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc));
				}
				else
				{
					if (src->_format == FormatInSIB) {
						fprintf(ctx->_outfp, "\tmov %s, word ptr [%s]\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc));
					}
					else if (src->_format == FormatSIB) {
						fprintf(ctx->_outfp, "\tlea %s, word ptr [%s]\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc));
					}
					else {
						fprintf(ctx->_outfp, "\tmov %s, %s\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc));
					}
				}
			}
				break;
			case IR_S32:
			case IR_U32:
			case IR_PTR:
			{
				if (dst->_format == FormatSIB)
				{
					fprintf(ctx->_outfp, "\tmov dword ptr [%s], %s\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc));
				}
				else
				{
					if (src->_format == FormatInSIB) {
						fprintf(ctx->_outfp, "\tmov %s, dword ptr [%s]\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc));
					}
					else if (src->_format == FormatSIB) {
						fprintf(ctx->_outfp, "\tlea %s, dword ptr [%s]\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc));
					}
					else {
						fprintf(ctx->_outfp, "\tmov %s, %s\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc));
					}
				}
			}
				break;
			case IR_S64:
			case IR_U64:
			{
				if (dst->_format == FormatSIB)
				{
					fprintf(ctx->_outfp, "\tmov dword ptr [%s], %s\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc));
					fprintf(ctx->_outfp, "\tmov dword ptr [%s], %s\n", cc_operand_text(dst, 1, szdst), cc_operand_text(src, 1, szsrc));
				}
				else
				{
					if (src->_format == FormatInSIB) {
						fprintf(ctx->_outfp, "\tmov %s, dword ptr [%s]\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc));
						fprintf(ctx->_outfp, "\tmov %s, dword ptr [%s]\n", cc_operand_text(dst, 1, szdst), cc_operand_text(src, 1, szsrc));
					}
					else {
						fprintf(ctx->_outfp, "\tmov %s, %s\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc));
						fprintf(ctx->_outfp, "\tmov %s, %s\n", cc_operand_text(dst, 1, szdst), cc_operand_text(src, 1, szsrc));
					}
				}
			}
				break;
			case IR_F32:
			{
				if (dst->_format == FormatSIB && src->_format == FormatReg)
				{
					assert(src->_u._regs[0] == X86_ST0);
					fprintf(ctx->_outfp, "\tfst dword ptr [%s]\n", cc_operand_text(dst, 0, szdst));
				}
				else if (dst->_format == FormatReg && src->_format == FormatInSIB)
				{
					assert(dst->_u._regs[0] == X86_ST0);
					fprintf(ctx->_outfp, "\tfld dword ptr [%s]\n", cc_operand_text(src, 0, szsrc));
				}
				else if (dst->_format == FormatSIB && src->_format == FormatInSIB)
				{
					fprintf(ctx->_outfp, "\tfld dword ptr [%s]\n\tfst dword ptr [%s]\n", cc_operand_text(src, 0, szdst), cc_operand_text(dst, 0, szdst));
				}
			}
				break;
			case IR_F64:
			{
				if (dst->_format == FormatSIB && src->_format == FormatReg)
				{
					assert(src->_u._regs[0] == X86_ST0);
					fprintf(ctx->_outfp, "\tfst qword ptr [%s]\n", cc_operand_text(dst, 0, szdst));
				}
				else if (dst->_format == FormatReg && src->_format == FormatInSIB)
				{
					assert(dst->_u._regs[0] == X86_ST0);
					fprintf(ctx->_outfp, "\tfld qword ptr [%s]\n", cc_operand_text(src, 0, szsrc));
				}
				else if (dst->_format == FormatSIB && src->_format == FormatInSIB)
				{
					fprintf(ctx->_outfp, "\tfld qword ptr [%s]\n\tfst qword ptr [%s]\n", cc_operand_text(src, 0, szsrc), cc_operand_text(dst, 0, szdst));
				}
			}
				break;
			case IR_BLK:
			{
				assert(dst->_format == FormatSIB && src->_format == FormatInSIB);
				fprintf(ctx->_outfp, "\tlea edi, [%s]\n\tlea esi, [%s]\n\tmov ecx, %d\n\trep movsb\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc), as->_count);
			}
				break;
			default:
				assert(0); break;
			}
		}
			break;
		case X86_PUSH:
		{
			switch (dst->_tycode)
			{
			case IR_S32:
			case IR_U32:
			case IR_PTR:
			{
				if (dst->_format == FormatInSIB)
				{
					fprintf(ctx->_outfp, "\tpush dword ptr [%s]\n", cc_operand_text(dst, 0, szdst));
				}
				else
				{
					fprintf(ctx->_outfp, "\tpush %s\n", cc_operand_text(dst, 0, szdst));
				}
			}
				break;
			case IR_S64:
			case IR_U64:
			{
				if (dst->_format == FormatInSIB)
				{
					fprintf(ctx->_outfp, "\tpush dword ptr [%s]\n", cc_operand_text(dst, 1, szdst));
					fprintf(ctx->_outfp, "\tpush dword ptr [%s]\n", cc_operand_text(dst, 0, szdst));
				}
				else
				{
					fprintf(ctx->_outfp, "\tpush %s\n", cc_operand_text(dst, 1, szdst));
					fprintf(ctx->_outfp, "\tpush %s\n", cc_operand_text(dst, 0, szdst));
				}
			}
				break;
			case IR_F32:
			{
				if (dst->_format == FormatInSIB)
				{
					fprintf(ctx->_outfp, "\tpush dword ptr [%s]\n", cc_operand_text(dst, 0, szdst));
				}
				else
				{
					assert(dst->_format == FormatReg && dst->_u._regs[0] == X86_ST0);
					fprintf(ctx->_outfp, "\tpush ecx\n\tfst dword ptr [esp]\n");
				}
			}
			break;
			case IR_F64:
			{
				if (dst->_format == FormatInSIB)
				{
					fprintf(ctx->_outfp, "\tpush dword ptr [%s]\n", cc_operand_text(dst, 1, szdst));
					fprintf(ctx->_outfp, "\tpush dword ptr [%s]\n", cc_operand_text(dst, 0, szdst));
				}
				else
				{
					assert(dst->_format == FormatReg && dst->_u._regs[0] == X86_ST0);
					fprintf(ctx->_outfp, "\tsub esp, 8\n\tfst qword ptr [esp]\n");
				}
			}
			break;
			case IR_BLK:
			{
				assert(dst->_format == FormatSIB && src->_format == FormatInSIB);
				fprintf(ctx->_outfp, "\tlea edi, [%s]\n\tlea esi, [%s]\n\tmov ecx, %d\n\trep movsb\n", cc_operand_text(dst, 0, szdst), cc_operand_text(src, 0, szsrc), as->_count);
			}
			break;
			default:
				assert(0); break;
			}
		}
			break;
		case X86_CALL:
		{
			if (dst->_format == FormatInSIB) {
				fprintf(ctx->_outfp, "\tcall dword ptr [%s]\n", cc_operand_text(dst, 0, szdst));
			}
			else {
				fprintf(ctx->_outfp, "\tcall %s\n", cc_operand_text(dst, 0, szdst));
			}
			if (as->_count > 0) {
				fprintf(ctx->_outfp, "\tadd esp, %d\n", as->_count);
			}
		}
			break;
		case X86_PROLOGUE:
		{
			fprintf(ctx->_outfp, "\tpush ebp\n\tmov ebp, esp\n");
			if (genctx->_maxlocalbytes > 0) {
				fprintf(ctx->_outfp, "\tsub esp, %d\n", genctx->_maxlocalbytes);
			}
			fprintf(ctx->_outfp, "\tpush ebx\n\tpush esi\n\tpush edi\n");
		}
			break;
		case X86_EPILOGUE:
		{
			fprintf(ctx->_outfp, "\tpop edi\n\tpop esi\n\tpop ebx\n");
			fprintf(ctx->_outfp, "\tmov esp, ebp\n\tpop ebp\n\tret\n");
		}
			break;
		case X86_ZERO_M: /* set memory to zero */
		{
			assert(dst->_format == FormatSIB);
			fprintf(ctx->_outfp, "\tlea edi, [%s]\n\tmov al, 0\n\tmov ecx, %d\n\trep stosb\n", cc_operand_text(dst, 0, szdst), as->_count);
		}
			break;
		default:
			assert(0); break;
		}

	} /* end for as */

	return TRUE;
}

BOOL cc_gen_dumpfunction(struct tagCCContext* ctx, struct tagCCSymbol* func, FArray* caller, FArray* callee, struct tagCCIRBasicBlock* body)
{
	struct tagCCTripleCodeContext triplectx;
	struct tagCCGenCodeContext genctx;

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

	if (!cc_output_asmcodes(ctx, func, &genctx)) {
		logger_output_s("failed to dump asm text for function '%s'\n", func->_name);
		return FALSE;
	}

	return TRUE;
}
