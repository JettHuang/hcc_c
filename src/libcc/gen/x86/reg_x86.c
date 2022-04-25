/* \brief
 *		register allocator
 */

#include "reg_x86.h"
#include "ir/ir.h"
#include "mm.h"


extern void alloc_temporary_space(struct tagCCGenCodeContext* ctx, struct tagCCDagNode* dag, int curseqid);
extern BOOL emit_store_r2staticmem(struct tagCCGenCodeContext* ctx, struct tagCCDagNode* dag);

typedef struct tagRegisterUser {
	struct tagCCDagNode* _dag;
	int _part;

	struct tagRegisterUser* _prev, * _next;
} FRegisterUser;

typedef struct tagRegisterDescriptor {
	int _id;
	int _isreserved : 1;
	int _isusing : 1;
	int _lastref;
	struct tagRegisterUser* _link;
} FRegisterDescriptor;

static struct tagRegisterDescriptor gregisters[] =
{
	{ X86_NIL, 1, 0, 0, NULL },
	{ X86_EAX, 0, 0, 0, NULL },
	{ X86_EBX, 0, 0, 0, NULL },
	{ X86_ECX, 0, 0, 0, NULL },
	{ X86_EDX, 0, 0, 0, NULL },
	{ X86_ESI, 0, 0, 0, NULL },
	{ X86_EDI, 0, 0, 0, NULL },
	{ X86_EBP, 1, 0, 0, NULL },
	{ X86_ESP, 1, 0, 0, NULL },
	{ X86_ST0, 0, 0, 0, NULL },
	{ X86_ST1, 1, 0, 0, NULL }
};

/* get a free user entry */
static struct tagRegisterUser* gfreelist = NULL;
static struct tagRegisterUser* get_userentry()
{
	struct tagRegisterUser* head = NULL;

	if (!gfreelist)
	{
		int count = 64;
		
		head = mm_alloc_area(sizeof(struct tagRegisterUser) * count, CC_MM_TEMPPOOL);
		while (count-- > 0)
		{
			if (gfreelist) { gfreelist->_prev = head; }
			head->_next = gfreelist;
			head->_prev = NULL;
			gfreelist = head;

			head++;
		}
	}

	if (gfreelist) 
	{ 
		head = gfreelist;
		if (head->_next) { head->_next->_prev = NULL; }
		gfreelist = head->_next;
		
		head->_next = NULL;
	}

	return head;
}

static void put_userentry(struct tagRegisterUser* e)
{
	if (gfreelist) { gfreelist->_prev = e; }
	e->_next = gfreelist;
	e->_prev = NULL;
	gfreelist = e;
}

static void calc_lastref(struct tagRegisterDescriptor* desc)
{
	struct tagRegisterUser* l;
	int lastref;
	
	lastref = 0;
	for (l = desc->_link; l; l = l->_next)
	{
		if (l->_dag->_lastref > lastref) {
			lastref = l->_dag->_lastref;
		}
	} /* end for l */
	
	desc->_lastref = lastref;
}

void cc_reg_reset()
{
	int i;

	for (i = 0; i < ELEMENTSCNT(gregisters); ++i)
	{
		gregisters[i]._isusing = 0;
		gregisters[i]._lastref = 0;
		gregisters[i]._link = NULL;
	}

	gfreelist = NULL;
}

int cc_reg_alloc(struct tagCCGenCodeContext* ctx, int currseqid, int regflags)
{
	int candidates[X86_MAX];
	struct tagRegisterDescriptor* p, *result;
	int i, count;

	p = result = NULL;
	for (i = count = 0; i < ELEMENTSCNT(gregisters); ++i)
	{
		if (regflags & REG_BIT(i))
		{
			p = &gregisters[i];
			if (!p->_isreserved && !p->_isusing)
			{
				if (!p->_link || p->_lastref < currseqid) {
					result = p;
					break;
				}
				else {
					candidates[count++] = i;
				}
			}
		}
	} /* end for i */

	if (!result)
	{
		int lastref = 0;
		for (i = 0; i < count; ++i)
		{
			p = &gregisters[candidates[i]];
			if (p->_lastref > lastref)
			{
				lastref = p->_lastref;
				result = p;
			}
		}
	}

	if (!result || !cc_reg_free(ctx, result->_id, currseqid)) { return X86_NIL; }

	return result->_id;
}

/* spill data from register to memory */
BOOL cc_reg_spill(struct tagCCGenCodeContext* ctx, struct tagCCDagNode* dag, int curseqid)
{
	assert(dag->_lastref > curseqid);
	assert(!dag->_x._inmemory);
	
	if (!dag->_x._inmemory) {
		alloc_temporary_space(ctx, dag, curseqid);
	}

	return emit_store_r2staticmem(ctx, dag);
}

/* free register */
BOOL cc_reg_free(struct tagCCGenCodeContext* ctx, int regid, int curseqid)
{
	struct tagRegisterDescriptor* desc;
	struct tagRegisterUser* l, *tmp;
	struct tagCCDagNode* dag;

	assert(regid > X86_NIL && regid <= X86_ST1);
	desc = &gregisters[regid];
	
	for (l = desc->_link; l; )
	{
		dag = l->_dag;
		if (dag->_lastref > curseqid && !dag->_x._inmemory) {
			if (!cc_reg_spill(ctx, dag, curseqid)) { return FALSE; }
		}

		if (dag->_x._loc._regs[0] == regid && dag->_x._loc._regs[1] != X86_NIL) {
			cc_reg_make_unassociated(dag->_x._loc._regs[1], dag, 1);
		}
		else if (dag->_x._loc._regs[1] == regid){
			assert(dag->_x._loc._regs[0] != X86_NIL);
			cc_reg_make_unassociated(dag->_x._loc._regs[0], dag, 0);
		}
		dag->_x._inregister = 0;
		dag->_x._loc._regs[0] = X86_NIL;
		dag->_x._loc._regs[1] = X86_NIL;

		tmp = l;
		l = l->_next;
		put_userentry(tmp);
	} /* end for */

	desc->_link = NULL;
	desc->_lastref = 0;
	return TRUE;
}

/* discard register */
void cc_reg_discard(int regid)
{
	struct tagRegisterDescriptor* desc;
	struct tagRegisterUser* l, * tmp;
	struct tagCCDagNode* dag;

	assert(regid > X86_NIL && regid <= X86_ST1);
	desc = &gregisters[regid];

	for (l = desc->_link; l; )
	{
		dag = l->_dag;
		if (dag->_x._loc._regs[0] == regid && dag->_x._loc._regs[1] != X86_NIL) {
			cc_reg_make_unassociated(dag->_x._loc._regs[1], dag, 1);
		}
		else if (dag->_x._loc._regs[1] == regid) {
			assert(dag->_x._loc._regs[0] != X86_NIL);
			cc_reg_make_unassociated(dag->_x._loc._regs[0], dag, 0);
		}
		dag->_x._inregister = 0;
		dag->_x._loc._regs[0] = X86_NIL;
		dag->_x._loc._regs[1] = X86_NIL;

		tmp = l;
		l = l->_next;
		put_userentry(tmp);
	} /* end for */

	desc->_link = NULL;
	desc->_lastref = 0;
}

/* associate dag with register */
BOOL cc_reg_make_associated(int regid, struct tagCCDagNode* dag, int part)
{
	struct tagRegisterDescriptor* desc;
	struct tagRegisterUser* l;
	
	if (regid == X86_NIL) { return TRUE; }

	assert(regid > X86_NIL && regid <= X86_ST1);
	desc = &gregisters[regid];
	if (!(l = get_userentry())) { return FALSE; }
	l->_dag = dag;
	l->_part = part;
	l->_prev = NULL;
	l->_next = desc->_link;
	desc->_link = l;

	dag->_x._inregister = 1;
	dag->_x._loc._regs[part] = regid;
	
	calc_lastref(desc);
	return TRUE;
}

BOOL cc_reg_make_unassociated(int regid, struct tagCCDagNode* dag, int part)
{
	struct tagRegisterDescriptor* desc;
	struct tagRegisterUser* l;

	if (regid == X86_NIL) { return TRUE; }

	assert(regid > X86_NIL && regid <= X86_ST1);
	desc = &gregisters[regid];
	
	for (l = desc->_link; l; l = l->_next)
	{
		if (l->_dag == dag && l->_part == part)
		{
			if (l->_next) { l->_next->_prev = l->_prev; }
			if (l->_prev) { 
				l->_prev->_next = l->_next; 
			}
			else {
				desc->_link = l->_next;
			}
			put_userentry(l);
			break;
		}
	}  /* end for */
	
	assert(dag->_x._loc._regs[part] == regid);
	dag->_x._inregister = 0;
	dag->_x._loc._regs[part] = X86_NIL;

	calc_lastref(desc);
	return TRUE;
}

/* mark register used flags */
void cc_reg_markused(int regid)
{
	assert(regid > X86_NIL && regid <= X86_ST1);
	gregisters[regid]._isusing = 1;
}

void cc_reg_unmarkused(int regid)
{
	assert(regid > X86_NIL && regid <= X86_ST1);
	gregisters[regid]._isusing = 0;
}

const char* cc_reg_name(int regid, int size)
{
	switch (regid)
	{
	case X86_EAX:
	{
		switch (size)
		{
		case 1: return "al";
		case 2: return "ax";
		case 4: 
		case 8: return "eax";
		}
		assert(0);
	}
		break;
	case X86_EBX:
	{
		switch (size)
		{
		case 1: return "bl";
		case 2: return "bx";
		case 4: 
		case 8: return "ebx";
		}
		assert(0);
	}
		break;
	case X86_ECX:
	{
		switch (size)
		{
		case 1: return "cl";
		case 2: return "cx";
		case 4: 
		case 8: return "ecx";
		}
		assert(0);
	}
		break;
	case X86_EDX:
	{
		switch (size)
		{
		case 1: return "dl";
		case 2: return "dx";
		case 4: 
		case 8: return "edx";
		}
		assert(0);
	}
		break;
	case X86_ESI: return "esi";
	case X86_EDI: return "edi";
	case X86_EBP: return "ebp";
	case X86_ESP: return "esp";
	case X86_ST0: return "st(0)";
	case X86_ST1: return "st(1)";
	default:
		break;
	}

	return NULL;
}