/* \brief
 *		intermediate representation.
 */

#include <string.h>

#include "ir.h"
#include "logger.h"
#include "parser/types.h"
#include "parser/symbols.h"
#include "parser/expr.h"


int cc_ir_typecode(const struct tagCCType* ty)
{
	ty = UnQual(ty);

	switch (ty->_op)
	{
	case Type_SInteger:
		switch (ty->_size)
		{
		case 1: return IR_S8;
		case 2: return IR_S16;
		case 4: return IR_S32;
		case 8: return IR_S64;
		}
		break;
	case Type_UInteger:
		switch (ty->_size)
		{
		case 1: return IR_U8;
		case 2: return IR_U16;
		case 4: return IR_U32;
		case 8: return IR_U64;
		}
		break;
	case Type_Float:
		switch (ty->_size)
		{
		case 4: return IR_F32;
		case 8: return IR_F64;
		}
		break;
	case Type_Enum:
		return cc_ir_typecode(ty->_type);
	case Type_Void:
		return IR_VOID;
	case Type_Pointer:
	case Type_Array:
		return IR_PTR;
	case Type_Union:
	case Type_Struct:
		return IR_BLK;
	case Type_Function:
		return IR_PTR;
	}

	return IR_VOID;
}

FCCIRCode* cc_ir_newcode(unsigned int op, enum EMMArea where)
{
	FCCIRCode* code = mm_alloc_area(sizeof(FCCIRCode), where);
	if (!code) {
		logger_output_s("error: out of memory at %s:%d\n", __FILE__, __LINE__);
		return NULL;
	}
	
	memset(code, 0, sizeof(FCCIRCode));
	code->_op = op;
	return code;
}

FCCIRCode* cc_ir_newcode_arg(struct tagCCExprTree* expr, enum EMMArea where)
{
	FCCIRCode* c = cc_ir_newcode(IR_ARG, where);
	if (c) {
		c->_u._expr = expr;
	}

	return c;
}

FCCIRCode* cc_ir_newcode_ret(struct tagCCExprTree* expr, enum EMMArea where)
{
	FCCIRCode* c = cc_ir_newcode(IR_RET, where);
	if (c) {
		c->_u._expr = expr;
	}

	return c;
}

FCCIRCode* cc_ir_newcode_expr(struct tagCCExprTree* expr, enum EMMArea where)
{
	FCCIRCode* c = cc_ir_newcode(IR_EXP, where);
	if (c) {
		c->_u._expr = expr;
	}

	return c;
}

FCCIRCode* cc_ir_newcode_var(struct tagCCSymbol* id, enum EMMArea where)
{
	FCCIRCode* c = cc_ir_newcode(IR_LOCVAR, where);
	if (c) {
		c->_u._id = id;
	}

	return c;
}

FCCIRCode* cc_ir_newcode_label(struct tagCCSymbol* lab, enum EMMArea where)
{
	FCCIRCode* c = cc_ir_newcode(IR_LABEL, where);
	if (c) {
		c->_u._label = lab;
	}

	return c;
}

FCCIRCode* cc_ir_newcode_jump(struct tagCCExprTree* cond, struct tagCCSymbol* tlabel, struct tagCCSymbol* flabel, enum EMMArea where)
{
	FCCIRCode* c = cc_ir_newcode(cond ? IR_CJMP : IR_JMP, where);
	if (c) {
		c->_u._jmp._cond = cond;
		c->_u._jmp._tlabel = tlabel;
		c->_u._jmp._flabel = flabel;
	}

	return c;
}

FCCIRCode* cc_ir_newcode_blk(BOOL isbegin, int level, enum EMMArea where)
{
	FCCIRCode* c = cc_ir_newcode(isbegin ? IR_BLKBEG : IR_BLKEND, where);
	if (c) {
		c->_u._blklevel = level;
	}

	return c;
}

void cc_ir_codelist_append(FCCIRCodeList* l, FCCIRCode* c)
{
	if (l->_tail) {
		c->_prev = l->_tail;
		l->_tail->_next = c;
	}
	else {
		l->_head = c;
	}

	l->_tail = c;
}

void cc_ir_codelist_remove(FCCIRCodeList* l, FCCIRCode* c)
{
	if (c->_prev) {
		c->_prev->_next = c->_next;
	}
	else {
		assert(l->_head == c);
		l->_head = NULL;
	}

	if (c->_next) {
		c->_next->_prev = c->_prev;
	}
	else {
		assert(l->_tail == c);
		l->_tail = c->_prev;
	}

	c->_prev = c->_next = NULL;
}

FCCIRCode* cc_ir_codelist_insert_before(FCCIRCodeList* l, FCCIRCode* t, FCCIRCode* c)
{
	if (t->_prev) {
		t->_prev->_next = c;
		c->_prev = t->_prev;
	}
	else {
		l->_head = c;
	}

	c->_next = t;
	t->_prev = c;

	return c;
}

FCCIRCode* cc_ir_codelist_insert_after(FCCIRCodeList* l, FCCIRCode* t, FCCIRCode* c)
{
	if (t->_next) {
		t->_next->_prev = c;
		c->_next = t->_next;
	}
	else {
		l->_tail = c;
	}

	t->_next = c;
	c->_prev = t;

	return c;
}

FCCIRCode* cc_ir_codelist_insert_list_before(FCCIRCodeList* l, FCCIRCode* t, FCCIRCodeList* c)
{
	FCCIRCode* head, * tail;

	head = c->_head;
	tail = c->_tail;
	if (!head || !tail) { 
		return t; 
	}

	if (t->_prev) {
		t->_prev->_next = head;
		head->_prev = t->_prev;
	}
	else {
		l->_head = head;
	}

	tail->_next = t;
	t->_prev = tail;

	return head;
}

FCCIRCode* cc_ir_codelist_insert_list_after(FCCIRCodeList* l, FCCIRCode* t, FCCIRCodeList* c)
{
	FCCIRCode* head, * tail;

	head = c->_head;
	tail = c->_tail;
	if (!head || !tail) {
		return t;
	}

	if (t->_next) {
		t->_next->_prev = tail;
		tail->_next = t->_next;
	}
	else {
		l->_tail = tail;
	}

	t->_next = head;
	head->_prev = t;

	return tail;
}

void cc_ir_codelist_display(FCCIRCodeList* l, int maxdepth)
{
	FCCIRCode* c = l->_head;

	for (; c; c = c->_next)
	{
		switch (c->_op)
		{
		case IR_ARG:
			logger_output_s("\tIR_ARG ");
			cc_expr_display(c->_u._expr, maxdepth); 
			logger_output_s("\n");
			break;
		case IR_EXP:
			logger_output_s("\tIR_EXP ");
			cc_expr_display(c->_u._expr, maxdepth);
			logger_output_s("\n");
			break;
		case IR_JMP:
			logger_output_s("\tIR_JMP %s\n", c->_u._jmp._tlabel->_name);
			break;
		case IR_CJMP:
			logger_output_s("\tIR_CJMP %s ", c->_u._jmp._tlabel->_name);
			cc_expr_display(c->_u._jmp._cond, maxdepth);
			logger_output_s("\n");
			break;
		case IR_LABEL:
			logger_output_s("IR_LABEL %s:\n", c->_u._label->_name);
			break;
		case IR_BLKBEG:
			logger_output_s("\tIR_BLKBEG %d:\n", c->_u._blklevel);
			break;
		case IR_BLKEND:
			logger_output_s("\tIR_BLKEND %d:\n", c->_u._blklevel);
			break;
		case IR_LOCVAR:
			logger_output_s("\tIR_LOCVAR %s\n", c->_u._id->_name);
			break;
		case IR_RET:
			logger_output_s("\tIR_RET ");
			cc_expr_display(c->_u._expr, maxdepth);
			logger_output_s("\n");
			break;
		case IR_FENTER:
			logger_output_s("\tIR_FENTER\n");
			break;
		case IR_FEXIT:
			logger_output_s("\tIR_FEXIT\n");
			break;
		default:
			logger_output_s("IR_Unkown"); break;
		}
	} /* end for */
}
