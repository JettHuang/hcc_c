/* \brief
 *		canon.h
 */

#include "canon.h"
#include "logger.h"
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
        if (tlab || flab)
        {
            tlab = expr->_u._val._sint ? tlab : flab;
            if (tlab) {
                cc_ir_codelist_append(list, cc_ir_newcode_jump(NULL, tlab, NULL, where));
            }
        }
        else if (UnQual(expr->_ty)->_u._symbol->_addressed && outexpr)
        {
			FCCSymbol* p;

			if (!(p = cc_symbol_constant(expr->_ty, expr->_u._val)))
			{
				logger_output_s("error install constant symbol failed at %s:%d\n", __FILE__, __LINE__);
				return FALSE;
			}

			if (!(expr = cc_expr_id(p, NULL, where)))
			{
				return FALSE;
			}
        }

        result = expr; 
        break;
    case IR_ASSIGN:
    {
        if (!cc_canon_expr_linearize(list, expr->_u._kids[1], NULL, NULL, &rhs, where)) { return FALSE; }
        if (!cc_canon_expr_linearize(list, expr->_u._kids[0], NULL, NULL, &lhs, where)) { return FALSE; }
        
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
        int n, argsbytes;
        FCCIRTree* param, ** args, *ret;

        ret = expr->_u._f._ret;
        args = expr->_u._f._args;
        for (n = 0; *args; ++args, ++n); /* get count of args */

		/* evaluate side-effect address */
		if (ret && !cc_canon_expr_linearize(list, ret, NULL, NULL, &ret, where)) {
			return FALSE;
		}

        /* evaluate arguments */
        argsbytes = 0;
        for (--args; n > 0; --n, --args)
        {
            if (!cc_canon_expr_linearize(list, *args, NULL, NULL, &param, where)) { return FALSE; }
            assert(param);
            cc_ir_codelist_append(list, cc_ir_newcode_arg(param, where));

            argsbytes += param->_ty->_size;
        }
        
        if (ret) { /* push receiver address as argument 0 */
            cc_ir_codelist_append(list, cc_ir_newcode_arg(ret, where));
            argsbytes += ret->_ty->_size;
        }
        
		/* evaluate function designator */
		if (!cc_canon_expr_linearize(list, expr->_u._f._lhs, NULL, NULL, &lhs, where)) { return FALSE; }

        assert(IsPtr(lhs->_ty) && IsFunction(lhs->_ty->_type));
        if (lhs->_ty->_type->_u._f._convention == Type_Fastcall)
        {
			logger_output_s("error: call '__fastcall' function is not support in this version, at %w\n", &lhs->_loc);
			return FALSE;
        }

        expr->_u._f._lhs = lhs;
        expr->_u._f._ret = ret;
        expr->_u._f._argsbytes = argsbytes;
        expr->_u._f._convention = lhs->_ty->_type->_u._f._convention;
        cc_ir_codelist_append(list, cc_ir_newcode_expr(expr, where));

        result = expr;
        if (ret && outexpr) {
            ret = cc_expr_indir(ret, &expr->_loc, where);
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
/* return true if need re-scan the code list */
static BOOL cc_canon_codelist_simplify_inner(FCCIRCodeList* list, enum EMMArea where)
{
    FArray adjacentlabs;
    FCCIRCode* code, *tmp;
    FCCSymbol* p, *q;
    BOOL iscontnext,  ismodified;

    ismodified = FALSE;
	for (code = list->_head; code; code = code->_next)
	{
		if (code->_op == IR_LABEL)
		{
            code->_u._label->_x._u._redirect = NULL;
            code->_u._label->_x._refcnt = 0;
		}
	} /* end for */

    /* pass 1: */
    array_init(&adjacentlabs, 32, sizeof(FCCSymbol*), CC_MM_TEMPPOOL);
    for (code = list->_head; code; code = iscontnext ? code->_next : code)
    {
        iscontnext = TRUE;
        switch (code->_op)
        {
        case IR_CJMP:
        {
            /* ie:
             *      cjmp constant  label_0   label_1
             *      xxx
             *  label_0:
             *      yyy
             *  label_1:
             *      zzz
             */
            if (IR_OP(code->_u._jmp._cond->_op) != IR_CONST)
            {
                if (code->_u._jmp._tlabel) {
                    code->_u._jmp._tlabel->_x._refcnt++;
                }
                if (code->_u._jmp._flabel) 
                {
                    p = code->_u._jmp._flabel;
                    p->_x._refcnt++;
					for (tmp = code->_next; tmp; tmp = tmp->_next)
					{
						if (tmp->_op != IR_LABEL
							&& tmp->_op != IR_BLKBEG
							&& tmp->_op != IR_BLKEND)
						{
							break;
						}

						if (tmp->_op == IR_LABEL && tmp->_u._label == p)
						{
                            p->_x._refcnt--;
                            code->_u._jmp._flabel = NULL;
							ismodified = TRUE;
							break;
						}
					} /* end for */
                }

                if (code->_u._jmp._tlabel && !code->_u._jmp._flabel)
                {
					p = code->_u._jmp._tlabel;
					for (tmp = code->_next; tmp; tmp = tmp->_next)
					{
						if (tmp->_op != IR_LABEL
							&& tmp->_op != IR_BLKBEG
							&& tmp->_op != IR_BLKEND)
						{
							break;
						}

						if (tmp->_op == IR_LABEL && tmp->_u._label == p)
						{
							p->_x._refcnt--;
							code = cc_ir_codelist_remove(list, code);
							iscontnext = FALSE;
							ismodified = TRUE;
							break;
						}
					} /* end for */
                }

                break;
            }

            p = (code->_u._jmp._cond->_u._val._sint) ? code->_u._jmp._tlabel : code->_u._jmp._flabel;
            code->_op = IR_JMP;
            code->_u._jmp._cond = NULL;
            code->_u._jmp._tlabel = p;
            code->_u._jmp._flabel = NULL;

            ismodified = TRUE;
        }
            /* go through */
        case IR_JMP:
        {
            /* ie:
             *       jmp  label_0
             *   label_x:
             *   label_0:
             *       xxx
             */
            p = code->_u._jmp._tlabel;
            if (p) {
				p->_x._refcnt++;
				for (tmp = code->_next; tmp; tmp = tmp->_next)
				{
					if (tmp->_op != IR_LABEL
						&& tmp->_op != IR_BLKBEG
						&& tmp->_op != IR_BLKEND)
					{
						break;
					}

					if (tmp->_op == IR_LABEL && tmp->_u._label == p)
					{
						p->_x._refcnt--;
						code = cc_ir_codelist_remove(list, code);
						iscontnext = FALSE;
                        ismodified = TRUE;
						break;
					}
				} /* end for */
            }
            else {
				code = cc_ir_codelist_remove(list, code);
				iscontnext = FALSE;
                ismodified = TRUE;
            }
        }
            break;
        case IR_LABEL:
        {
            /* ie:
			 *   label_0:
			 *   label_1:
			 *   label_2:
			 *       xxx
			 */
            array_clear(&adjacentlabs);
            for (; code; code = code->_next)
            {
				if (code->_op != IR_LABEL
					&& code->_op != IR_BLKBEG
					&& code->_op != IR_BLKEND)
				{
					break;
				}

                if (code->_op == IR_LABEL)
                {
                    array_append(&adjacentlabs, &(code->_u._label));
                }
            } /* end for */

            if (adjacentlabs._elecount >= 2) {
                int n;

                p = *(FCCSymbol**)array_element(&adjacentlabs, adjacentlabs._elecount - 1);
                for (n = 0; n<adjacentlabs._elecount - 1; ++n)
                {
                    q = *(FCCSymbol**)array_element(&adjacentlabs, n);
                    q->_x._u._redirect = p;
                }

                ismodified = TRUE;
            }
            
            iscontnext = FALSE;
        }
            break;
        case IR_RET:
        {
            /* ie:
             *       ret  exit_0
             *   label_x:
             *   label_0:
             *    exit_0:
             */
            p = code->_u._ret._exitlab;
            if (p) {
                p->_x._refcnt++;
                for (tmp = code->_next; tmp; tmp = tmp->_next)
                {
                    if (tmp->_op != IR_LABEL
                        && tmp->_op != IR_BLKBEG
                        && tmp->_op != IR_BLKEND)
                    {
                        break;
                    }

                    if (tmp->_op == IR_LABEL && tmp->_u._label == p)
                    {
                        p->_x._refcnt--;
                        code->_u._ret._exitlab = NULL;
                        ismodified = TRUE;
                        break;
                    }
                } /* end for */
            }
        }
            break;
        default:
            break;
        }
    } /* end for */

    if (!ismodified) { return FALSE; }
    /* pass 2: adjust to redirect label */
    for (code = list->_head; code; code = code->_next)
    {
        switch (code->_op)
        {
        case IR_CJMP:
        {
            p = code->_u._jmp._tlabel;
			if (p && p->_x._u._redirect) {
				code->_u._jmp._tlabel = p->_x._u._redirect;
			}
			p = code->_u._jmp._flabel;
			if (p && p->_x._u._redirect) {
				code->_u._jmp._flabel = p->_x._u._redirect;
			}

            if (code->_u._jmp._tlabel == code->_u._jmp._flabel)
            {
                code->_u._jmp._flabel->_x._refcnt--;
				code->_op = IR_JMP;
				code->_u._jmp._cond = NULL;
				code->_u._jmp._flabel = NULL;
            }
        }
            break;
        case IR_JMP:
        {
            p = code->_u._jmp._tlabel;
            if (p->_x._u._redirect) {
                code->_u._jmp._tlabel = p->_x._u._redirect;
            }
        }
            break;
		case IR_LABEL:
		{
			p = code->_u._label->_x._u._redirect;
			if (p) {
				p->_x._refcnt += code->_u._label->_x._refcnt;
				code->_u._label->_x._refcnt = 0;
			}
		}
		    break;
        case IR_RET:
        {
            p = code->_u._ret._exitlab;
            if (p && p->_x._u._redirect) {
                code->_u._ret._exitlab = p->_x._u._redirect;
            }
        }
            break;
        default:
            break;
        }
    } /* end for */

    /* pass 3: remove unnecessary label */
    for (code = list->_head; code; )
    {
        if (code->_op == IR_LABEL && code->_u._label->_x._refcnt == 0)
        {
            code = cc_ir_codelist_remove(list, code);
            continue;
        }

        code = code->_next;
    } /* end for */

    return TRUE;
}

BOOL cc_canon_codelist_simplify(FCCIRCodeList* list, enum EMMArea where)
{
    int loopcnt = 1;
    while (cc_canon_codelist_simplify_inner(list, where))
    {
#if 0 /* for debug */
        logger_output_s("pass %d:\n", loopcnt);
        cc_ir_codelist_display(list, 5);
#endif
        loopcnt++;
        /* do nothing */
    }

    return TRUE;
}

static void cc_canon_reset_visitedflags(FCCIRBasicBlock* bb)
{
    for (; bb; bb = bb->_next)
    {
        bb->_visited = 0;
    }
}

static void cc_canon_checkreachable(FCCIRBasicBlock* bb)
{
    if (bb->_visited) { return; }
    bb->_visited = 1;
    bb->_reachable = 1;

    if (bb->_tjmp) {
        cc_canon_checkreachable(bb->_tjmp);
    }
    if (bb->_fjmp) {
        cc_canon_checkreachable(bb->_fjmp);
    }
    if (!bb->_tjmp && !bb->_fjmp && bb->_next) {
        cc_canon_checkreachable(bb->_next);
    }
}

FCCIRBasicBlock* cc_canon_gen_basicblocks(FCCIRCodeList* list, enum EMMArea where)
{
    FCCIRBasicBlock* first, *last, *bb;
    FCCIRCode* code, *next;

    /* generate basic blocks */
    first = last = cc_ir_newbasicblock(where);
    for (code = list->_head; code; code = next)
    {
        if (code->_op != IR_LABEL && !last->_name)
        {
            last->_name = hs_hashstr(util_itoa(cc_symbol_genlabel(1)));
        }

        if (code->_op == IR_CJMP || code->_op == IR_JMP)
        {
			next = cc_ir_codelist_remove(list, code);
			cc_ir_codelist_append(&last->_codes, code);
            
            bb = cc_ir_newbasicblock(where);
            last->_next = bb;
            bb->_prev = last;
            last = bb;
        }
        else if (code->_op == IR_LABEL)
        {
            if (last->_codes._head != NULL) 
            {
				bb = cc_ir_newbasicblock(where);
				last->_next = bb;
				bb->_prev = last;
				last = bb;
            }
            code->_u._label->_x._u._basicblock = last;
            last->_name = code->_u._label->_name;
            last->_withlabel = 1;
            
			next = cc_ir_codelist_remove(list, code);
			cc_ir_codelist_append(&last->_codes, code);
        }
        else if (code->_op == IR_RET)
        {
			next = cc_ir_codelist_remove(list, code);
			cc_ir_codelist_append(&last->_codes, code);

            if (code->_u._ret._exitlab) {
                bb = cc_ir_newbasicblock(where);
                last->_next = bb;
                bb->_prev = last;
                last = bb;
            }
        }
        else
        {
			next = cc_ir_codelist_remove(list, code);
			cc_ir_codelist_append(&last->_codes, code);
        }
    } /* end for code */

    for (bb = first; bb; bb = bb->_next)
    {
        code = bb->_codes._tail;
        if (code)
        {
            switch (code->_op)
            {
            case IR_JMP:
                bb->_tjmp = code->_u._jmp._tlabel->_x._u._basicblock;
                break;
            case IR_CJMP:
                bb->_tjmp = code->_u._jmp._tlabel->_x._u._basicblock;
                bb->_fjmp = code->_u._jmp._flabel ? code->_u._jmp._flabel->_x._u._basicblock : bb->_next;
                break;
            case IR_RET:
                bb->_tjmp = code->_u._ret._exitlab ? code->_u._ret._exitlab->_x._u._basicblock : NULL;
                break;
            default:
                break;
            }
        }
    } /* end for bb */

    /* set reachable */
    cc_canon_checkreachable(first);

    return first;
}

FCCIRBasicBlock* cc_canon_erease_deadbasicblocks(FCCIRBasicBlock* first, enum EMMArea where)
{
    FCCIRBasicBlock * bb;

    for (bb = first; bb;)
    {
        FCCIRCode* code;

        if (bb->_reachable) 
        {
            bb = bb->_next;
            continue;
        }

        for (code = bb->_codes._head; code;)
        {
            if (code->_op != IR_BLKBEG && code->_op != IR_BLKEND)
            {
                code = cc_ir_codelist_remove(&bb->_codes, code);
            }
            else
            {
                code = code->_next;
            }
        }

        if (bb->_codes._head == NULL)
        {
            /* remove this bb */
            if (bb->_prev == NULL) {
                first = bb->_next;
            }
            else {
                bb->_prev->_next = bb->_next;
            }

            if (bb->_next) {
                bb->_next->_prev = bb->_prev;
            }
        }

        bb = bb->_next;
    } /* end for bb */

    return first;
}


static int retpaths, noretpaths;
static void cc_canon_check_return_inner(FCCIRBasicBlock* bb)
{
    FCCIRCode* code;

	if (bb->_visited) { return; }
	bb->_visited = 1;

    for (code = bb->_codes._head; code; code = code->_next)
    {
		if (code->_op == IR_RET)
		{
			retpaths++;
			return;
		}
    }

	if (bb->_tjmp) {
        cc_canon_check_return_inner(bb->_tjmp);
	}
	if (bb->_fjmp) {
        cc_canon_check_return_inner(bb->_fjmp);
	}
	if (!bb->_tjmp && !bb->_fjmp) {
        if (bb->_next) {
            cc_canon_check_return_inner(bb->_next);
        }
        else {
            noretpaths++;
        }
	}
}

BOOL cc_canon_check_return(FCCSymbol* func, FCCIRBasicBlock* first)
{
    FCCType* rty = cc_type_rettype(func->_type);
   
    if (IsVoid(rty)) {
        return TRUE;
    }

    retpaths = noretpaths = 0;
    cc_canon_reset_visitedflags(first);
    cc_canon_check_return_inner(first);

    if (retpaths > 0 && noretpaths > 0)
    {
        logger_output_s("warning: not all control paths return a value in function %s\n", func->_name);
    }
    else if (noretpaths > 0) {
        if (gccconfig._must_retvalue) {
            logger_output_s("error: must return a value in function %s\n", func->_name);
            return FALSE;
        }

        logger_output_s("warning: no return value in function %s\n", func->_name);
    }

    return TRUE;
}

FCCIRBasicBlock* cc_canon_uber(FCCIRCodeList* list, enum EMMArea where)
{
	FCCIRBasicBlock* first;

	cc_canon_codelist_simplify(list, where);

	/* for debug */
#if 0
	logger_output_s("After Simplify:\n");
	cc_ir_codelist_display(list, 5);
	logger_output_s("\n");
#endif

	first = cc_canon_gen_basicblocks(list, where);
#if 0
    logger_output_s("Generate Basic Blocks:\n");
	cc_ir_basicblock_display(first, 5);
	logger_output_s("\n");
#endif

	first = cc_canon_erease_deadbasicblocks(first, where);
#if 0	
    logger_output_s("Erase Dead Blocks:\n");
	cc_ir_basicblock_display(first, 5);
	logger_output_s("\n");
#endif

    list = cc_ir_codeblocks_to_codelist(first, where);
    if (!list) { return NULL; }

    /* simplify again */
    cc_canon_codelist_simplify(list, where);

	first = cc_canon_gen_basicblocks(list, where);
#if 0
    logger_output_s("Final Canon Result:\n");
	cc_ir_basicblock_display(first, 5);
	logger_output_s("\n");
#endif

    return first;
}