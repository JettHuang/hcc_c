/* \brief
 *		canon.h
 */

#include "canon.h"
#include "parser/expr.h"


BOOL cc_canon_expr_linearize(FCCIRCodeList* list, FCCIRTree* expr, FCCSymbol* tlab, FCCSymbol* flab, FCCIRTree** outexpr, enum EMMArea where)
{
    FCCIRTree* result, *lhs, *rhs;

    if (expr->_isvisited)
    {
        if (outexpr) { *outexpr = cc_expr_right(expr); }
        return TRUE;
    }

    expr->_isvisited = 1;
    result = lhs = rhs = NULL;
	switch (IR_OP(expr->_op))
	{
	case IR_CONST: 
        result = expr; 
        break;
    case IR_ASSIGN:
    {
        if (!cc_canon_expr_linearize(list, expr->_u._kids[0], NULL, NULL, &lhs, where)) { return FALSE; }
        if (!cc_canon_expr_linearize(list, expr->_u._kids[1], NULL, NULL, &rhs, where)) { return FALSE; }
        
        assert(lhs && rhs);
        expr->_u._kids[0] = lhs;
        expr->_u._kids[1] = rhs;
		cc_ir_codelist_append(list, cc_ir_newcode_expr(expr, where));
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
		if (!cc_canon_expr_linearize(list, expr->_u._kids[0], NULL, NULL, &lhs, where)) { return FALSE; }
		if (!cc_canon_expr_linearize(list, expr->_u._kids[1], NULL, NULL, &rhs, where)) { return FALSE; }

		assert(lhs && rhs);
		expr->_u._kids[0] = lhs;
		expr->_u._kids[1] = rhs;
        result = expr;
	}
	    break;
    case IR_LOGAND:
    {
        FCCSymbol* tlab0;

        assert(tlab && flab);
        tlab0 = cc_symbol_label(NULL, &expr->_loc, where);
        if (!cc_canon_expr_linearize(list, expr->_u._kids[0], tlab0, flab, NULL, where)) { return FALSE; }
        cc_ir_codelist_append(list, cc_ir_newcode_label(tlab0, where));
        if (!cc_canon_expr_linearize(list, expr->_u._kids[1], tlab, flab, NULL, where)) { return FALSE; }
    }
        break;
    case IR_LOGOR:
    {
		FCCSymbol* flab0;

        assert(tlab && flab);
		flab0 = cc_symbol_label(NULL, &expr->_loc, where);
        if (!cc_canon_expr_linearize(list, expr->_u._kids[0], tlab, flab0, NULL, where)) { return FALSE; }
		cc_ir_codelist_append(list, cc_ir_newcode_label(flab0, where));
        if (!cc_canon_expr_linearize(list, expr->_u._kids[1], tlab, flab, NULL, where)) { return FALSE; }
    }
        break;
    case IR_EQUAL:
    case IR_UNEQ:
    case IR_LESS:
    case IR_GREAT:
    case IR_LEQ:
    case IR_GEQ:
	{
        int invop = IR_OP(expr->_op);

        if (flab) { /* if flab != NULL, invert op */
            FCCSymbol* tmp;

            tmp = tlab; tlab = flab; flab = tmp;
            switch (IR_OP(expr->_op))
            {
            case IR_EQUAL: invop = IR_UNEQ; break;
            case IR_UNEQ: invop = IR_EQUAL; break;
            case IR_LESS: invop = IR_GEQ; break;
            case IR_GREAT: invop = IR_LEQ; break;
            case IR_LEQ: invop = IR_GREAT; break;
            case IR_GEQ: invop = IR_LESS; break;
            default: assert(0); break;
            }
        }

		if (!cc_canon_expr_linearize(list, expr->_u._kids[0], NULL, NULL, &lhs, where)) { return FALSE; }
		if (!cc_canon_expr_linearize(list, expr->_u._kids[1], NULL, NULL, &rhs, where)) { return FALSE; }

		assert(lhs && rhs);
		expr->_u._kids[0] = lhs;
		expr->_u._kids[1] = rhs;
		expr->_op = IR_MODOP(expr->_op, invop);
		cc_ir_codelist_append(list, cc_ir_newcode_jump(expr, tlab, flab, where));
	}
	    break;
	case IR_NOT:
	{
        assert(tlab && flab);
        if (!cc_canon_expr_linearize(list, expr->_u._kids[0], flab, tlab, NULL, where)) { return FALSE; }
	}
        break;
    case IR_NEG:
    case IR_BCOM:
    case IR_CVT:
    case IR_INDIR:
	{
		if (!cc_canon_expr_linearize(list, expr->_u._kids[0], NULL, NULL, &lhs, where)) { return FALSE; }

		assert(lhs);
		expr->_u._kids[0] = lhs;
		result = expr;
	}
	    break;
    
    case IR_ADDRG:
    case IR_ADDRF:
    case IR_ADDRL:
        result = expr;
        break;
    case IR_CALL:
    {
        int n;
        FCCIRTree* param, ** args, *ret;

        ret = expr->_u._f._ret;
        args = expr->_u._f._args;
        for (n = 0; *args; ++args, ++n); /* get count of args */

        /* evaluate arguments */
        for (--args; n > 0; --n, --args)
        {
            if (!cc_canon_expr_linearize(list, *args, NULL, NULL, &param, where)) { return FALSE; }
            assert(param);
            cc_ir_codelist_append(list, cc_ir_newcode_arg(param, where));
        }
        
		/* evaluate function designator */
		if (!cc_canon_expr_linearize(list, expr->_u._f._lhs, NULL, NULL, &lhs, where)) { return FALSE; }

        /* evaluate side-effect address */
		if (ret && !cc_canon_expr_linearize(list, ret, NULL, NULL, &ret, where)) {
			return FALSE;
		}

        expr->_u._f._lhs = lhs;
        expr->_u._f._ret = ret;
        cc_ir_codelist_append(list, cc_ir_newcode_expr(expr, where));

        result = expr;
        if (ret && outexpr) {
            result = cc_expr_indir(ret, &expr->_loc, where);
            if (!cc_canon_expr_linearize(list, ret, NULL, NULL, &result, where)) { return FALSE; }
        }
    }
        break;
    case IR_SEQ:
    {
        if (!cc_canon_expr_linearize(list, expr->_u._kids[0], NULL, NULL, &lhs, where)) { return FALSE; }
        if (!cc_canon_expr_linearize(list, expr->_u._kids[1], NULL, NULL, &rhs, where)) { return FALSE; }
        result = rhs;
    }
        break;
    case IR_COND:
    {
        FCCSymbol* tlab, * flab, * elab;

        tlab = cc_symbol_label(NULL, &expr->_loc, where);
        flab = cc_symbol_label(NULL, &expr->_loc, where);
        elab = cc_symbol_label(NULL, &expr->_loc, where);

        if (!cc_canon_expr_linearize(list, expr->_u._kids[0], tlab, flab, NULL, where)) { return FALSE; }

        cc_ir_codelist_append(list, cc_ir_newcode_label(tlab, where));
        if (!cc_canon_expr_linearize(list, expr->_u._kids[1], NULL, NULL, NULL, where)) { return FALSE; }
        cc_ir_codelist_append(list, cc_ir_newcode_jump(NULL, elab, NULL, where));

        cc_ir_codelist_append(list, cc_ir_newcode_label(flab, where));
        if (!cc_canon_expr_linearize(list, expr->_u._kids[2], NULL, NULL, NULL, where));
        cc_ir_codelist_append(list, cc_ir_newcode_label(elab, where));
    }
        break;
    default:
        assert(0); break;
	}

    if (outexpr) { *outexpr = result; }
    return TRUE;
}

/* 1. merge nearest labels
 * 2. remove unnecessary labels
 * 3. remove unnecessary jump code
 * 4. convert c-jump to jump if possible
 */
BOOL cc_canon_codelist_simplify(FCCIRCodeList* list, enum EMMArea where)
{
    //TODO:
    return FALSE;
}
