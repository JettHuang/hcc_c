/* \brief
 *		dag pool.
 */

#include "dag.h"
#include "parser/types.h"
#include "logger.h"


#define DAGPOOL_HASHSIZE 256
#define DAGPOOL_MAXNODES 500

static struct tagDagPoolEntry {
	struct tagCCDagNode _node;
	struct tagDagPoolEntry* _link;
} *buckets[DAGPOOL_HASHSIZE];
static int gpooleddagscnt = 0;


static struct tagDagPoolEntry* cc_dag_entry(unsigned int op, int valsize, FCCIRDagNode* lhs, FCCIRDagNode* rhs, FCCSymbol* sym, enum EMMArea where)
{
	struct tagDagPoolEntry* dag = mm_alloc_area(sizeof(struct tagDagPoolEntry), where);
	if (!dag) {
		logger_output_s("error: out of memory at %s:%d\n", __FILE__, __LINE__);
		return NULL;
	}

	util_memset(dag, 0, sizeof(*dag));
	dag->_node._op = op;
	dag->_node._typesize = valsize;
	if ((dag->_node._kids[0] = lhs) != NULL) {
		lhs->_refcnt++;
	}
	if ((dag->_node._kids[1] = rhs) != NULL) {
		rhs->_refcnt++;
	}

	dag->_node._symbol = sym;
	return dag;
}

FCCIRDagNode* cc_dag_newnode(unsigned int op, int valsize, FCCIRDagNode* lhs, FCCIRDagNode* rhs, FCCSymbol* sym, enum EMMArea where)
{
	struct tagDagPoolEntry* dag = cc_dag_entry(op, valsize, lhs, rhs, sym, where);
	return dag ? &dag->_node : NULL;
}

FCCIRDagNode* cc_dag_newnode_inpool(unsigned int op, int valsize, FCCIRDagNode* lhs, FCCIRDagNode* rhs, FCCSymbol* sym, enum EMMArea where)
{
	struct tagDagPoolEntry* p;
	int h;

	h = (op ^ ((unsigned long)sym >> 2)) & (DAGPOOL_HASHSIZE - 1);
	for (p = buckets[h]; p; p = p->_link)
	{
		if (p->_node._op == op && p->_node._symbol == sym
			&& p->_node._kids[0] == lhs && p->_node._kids[1] == rhs)
		{
			return &p->_node;
		}
	} /* end for p */

	p = cc_dag_entry(op, valsize, lhs, rhs, sym, where);
	p->_link = buckets[h];
	buckets[h] = p;
	gpooleddagscnt++;

	return &p->_node;
}

void cc_dag_rmnodes_inpool(FCCSymbol* p)
{
	struct tagDagPoolEntry** q;
	int i, op;

	for (i=0; i<DAGPOOL_HASHSIZE; i++)
	{
		for (q = &buckets[i]; *q; )
		{
			op = (*q)->_node._op;
			if (IR_OP(op) == IR_INDIR)
			{
				op = (*q)->_node._kids[0]->_op;
				if (!IsAddrOp(op) || (*q)->_node._kids[0]->_symbol == p)
				{
					*q = (*q)->_link; /* remove it */
					--gpooleddagscnt;
					continue;
				}
			}

			q = &(*q)->_link;
		} /* end for q */
	} /* end for i */
}

void cc_dag_reset_pool()
{
	if (gpooleddagscnt > 0) {
		util_memset(&buckets, 0, sizeof(buckets));
	}
	gpooleddagscnt = 0;
}

/* generate DAG nodes for expression */
static BOOL cc_dag_translate_expr(FCCIRTree* expr, enum EMMArea where)
{
	FCCIRTree* lhs, *rhs;

	if (expr->_dagnode) {
		return TRUE;
	}

	if (gpooleddagscnt > DAGPOOL_MAXNODES) {
		cc_dag_reset_pool();
	}

	switch (IR_OP(expr->_op))
	{
	case IR_CONST:
	{
		FCCSymbol* p;

		if (!(p = cc_symbol_constant(expr->_ty, expr->_u._val)))
		{
			logger_output_s("error install constant symbol failed at %s:%d\n", __FILE__, __LINE__);
			return FALSE;
		}

		if (!(expr->_dagnode = cc_dag_newnode_inpool(expr->_op, expr->_ty->_size, NULL, NULL, p, where)))
		{
			return FALSE;
		}
	}
		break;
	case IR_ASSIGN:
	{
		lhs = expr->_u._kids[0];
		rhs = expr->_u._kids[1];
		if (cc_dag_translate_expr(lhs, where) &&
			cc_dag_translate_expr(rhs, where))
		{
			if (!(expr->_dagnode = cc_dag_newnode_inpool(expr->_op, expr->_ty->_size, lhs->_dagnode, rhs->_dagnode, NULL, where)))
			{
				return FALSE;
			}
		}

		if (IsAddrOp(lhs->_op) && !lhs->_symbol->_temporary) {
			cc_dag_rmnodes_inpool(lhs->_symbol);
		}
		else {
			cc_dag_reset_pool();
		}
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
		lhs = expr->_u._kids[0];
		rhs = expr->_u._kids[1];
		if (cc_dag_translate_expr(lhs, where) &&
			cc_dag_translate_expr(rhs, where))
		{
			if (!(expr->_dagnode = cc_dag_newnode_inpool(expr->_op, expr->_ty->_size, lhs->_dagnode, rhs->_dagnode, NULL, where)))
			{
				return FALSE;
			}
		}
	}
		break;
	case IR_LOGAND:
	case IR_LOGOR:
		assert(0); break;
	case IR_EQUAL:
	case IR_UNEQ:
	case IR_LESS:
	case IR_GREAT:
	case IR_LEQ:
	case IR_GEQ:
	{
		lhs = expr->_u._kids[0];
		rhs = expr->_u._kids[1];
		if (cc_dag_translate_expr(lhs, where) &&
			cc_dag_translate_expr(rhs, where))
		{
			if (!(expr->_dagnode = cc_dag_newnode_inpool(expr->_op, expr->_ty->_size, lhs->_dagnode, rhs->_dagnode, NULL, where)))
			{
				return FALSE;
			}
		}
	}
		break;
	case IR_NOT:
		assert(0); break;
	case IR_NEG:
	case IR_BCOM:
	case IR_CVT:
	case IR_INDIR:
	{
		if (cc_dag_translate_expr(expr->_u._kids[0], where))
		{
			if (!(expr->_dagnode = cc_dag_newnode_inpool(expr->_op, expr->_ty->_size, expr->_u._kids[0]->_dagnode, NULL, NULL, where)))
			{
				return FALSE;
			}
		}
	}
		break;
	case IR_ADDRG:
	case IR_ADDRF:
	case IR_ADDRL:
	{
		if (!(expr->_dagnode = cc_dag_newnode_inpool(expr->_op, expr->_ty->_size, NULL, NULL, expr->_symbol, where)))
		{
			return FALSE;
		}
	}
		break;
	case IR_CALL:
	{
		FCCIRTree *ret;

		ret = expr->_u._f._ret;
		if (ret && !cc_dag_translate_expr(ret, where)) {
			return FALSE;
		}
		if (!cc_dag_translate_expr(expr->_u._f._lhs, where)) {
			return FALSE;
		}
		
		if (!(expr->_dagnode = cc_dag_newnode_inpool(expr->_op, expr->_ty->_size, expr->_u._f._lhs->_dagnode, ret ? ret->_dagnode : NULL, NULL, where)))
		{
			return FALSE;
		}

		cc_dag_reset_pool();
	}
		break;
	case IR_SEQ:
	case IR_COND:
		assert(0); break;
	default:
		assert(0); break;
	}

	return TRUE;
}

/* generate DAG nodes for basic blocks */
BOOL cc_dag_translate_basicblocks(FCCIRBasicBlock* first, enum EMMArea where)
{
	FCCIRBasicBlock* bb;

	cc_dag_reset_pool();
	for (bb = first; bb; bb = bb->_next)
	{
		if (!cc_dag_translate_codelist(&bb->_codes, where)) { 
			return FALSE; 
		}
	} /* end for bb */

	return TRUE;
}

BOOL cc_dag_translate_codelist(FCCIRCodeList* list, enum EMMArea where)
{
	FCCIRCode* code;

	for (code = list->_head; code; code = code->_next)
	{
		switch (code->_op)
		{
		case IR_ARG:
		case IR_EXP:
			if (!cc_dag_translate_expr(code->_u._expr, where)) {
				return FALSE;
			}
			break;
		case IR_CJMP:
			if (!cc_dag_translate_expr(code->_u._jmp._cond, where)) {
				return FALSE;
			}
			break;
		case IR_JMP:
			break;
		case IR_LABEL:
			cc_dag_reset_pool();
			break;
		case IR_BLKBEG:
		case IR_BLKEND:
		case IR_LOCVAR:
			break;
		case IR_RET:
			if (code->_u._ret._expr && !cc_dag_translate_expr(code->_u._ret._expr, where)) {
				return FALSE;
			}
			break;
		case IR_ZERO:
			if (!cc_dag_translate_expr(code->_u._zero._addr, where)) {
				return FALSE;
			}
			break;
		case IR_FENTER:
		case IR_FEXIT:
			break;
		default:
			assert(0); break;
		}

	} /* end for code */

	return TRUE;
}
