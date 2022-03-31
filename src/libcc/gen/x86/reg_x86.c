/* \brief
 *		register allocator
 */

#include "reg_x86.h"
#include "ir/ir.h"


extern int gen_get_tempspace(struct tagCCGenCodeContext* ctx, struct tagCCDagNode* dag);
extern BOOL emit_store_regvalue(struct tagCCGenCodeContext* ctx, struct tagCCDagNode* dag, int tmpoffset);

typedef struct tagRegisterEntry
{
	int _id;
	int _isreserved : 1;
	
	struct tagCCDagNode* _dag;
	int _part;
} FRegisterEntry;

static struct tagRegisterEntry gRegisters[] = 
{
	{ X86_NIL, 1, NULL, 0 },
	{ X86_EAX, 0, NULL, 0 },
	{ X86_EBX, 0, NULL, 0 },
	{ X86_ECX, 0, NULL, 0 },
	{ X86_EDX, 0, NULL, 0 },
	{ X86_ESI, 0, NULL, 0 },
	{ X86_EDI, 0, NULL, 0 },
	{ X86_EBP, 1, NULL, 0 },
	{ X86_ESP, 1, NULL, 0 },
	{ X86_ST0, 1, NULL, 0 },
	{ X86_ST1, 1, NULL, 0 }
};

static BOOL cc_reg_spill(struct tagCCGenCodeContext* ctx, int regid)
{
	struct tagCCDagNode* dag;
	int tmpoffset;

	dag = gRegisters[regid]._dag;
	assert(dag);
	assert(dag->_x._inregister);

	tmpoffset = gen_get_tempspace(ctx, dag);
	if (!emit_store_regvalue(ctx, dag, tmpoffset))
	{
		return FALSE;
	}

	cc_reg_put(dag->_x._loc._registers[0]);
	cc_reg_put(dag->_x._loc._registers[1]);
	dag->_x._inregister = 0;
	dag->_x._intemporary = 1;
	dag->_x._loc._temp_offset = tmpoffset;

	return TRUE;
}

void cc_reg_reset()
{
	int i;

	for (i = 0; i < ELEMENTSCNT(gRegisters); ++i)
	{
		gRegisters[i]._dag = NULL;
		gRegisters[i]._part = 0;
	}
}

BOOL cc_reg_get(struct tagCCGenCodeContext* ctx, int requires, struct tagCCDagNode* dag, int part)
{
	int candidates[X86_MAX];
	int i, cnt;
	FRegisterEntry* p, *result;

	p = result = NULL;
	for (i = cnt = 0; i < ELEMENTSCNT(gRegisters); ++i)
	{
		if (requires & REG_BIT(i))
		{
			p = &gRegisters[i];
			if (!p->_dag || p->_dag->_refcnt <= 0) {
				result = p;
				break;
			}
			else {
				candidates[cnt++] = i;
			}
		}
	} /* end for i */

	if (!result)
	{
		int maxref = 0;
		for (i = 0; i < cnt; ++i)
		{
			p = &gRegisters[candidates[i]];
			assert(p->_dag);
			if (p->_dag->_refcnt > maxref)
			{
				maxref = p->_dag->_refcnt;
				result = p;
			}
		}
	}

	if (!result) { return FALSE; }
	if (result->_dag && result->_dag->_refcnt > 0)
	{
		if (!cc_reg_spill(ctx, result->_id)) {
			return FALSE;
		}
	}

	result->_dag = dag;
	result->_part = part;
	return TRUE;
}

BOOL cc_reg_put(int regid)
{
	assert(regid >= X86_NIL && regid <= X86_ST1);
	gRegisters[regid]._dag = NULL;
	gRegisters[regid]._part = 0;

	return TRUE;
}
