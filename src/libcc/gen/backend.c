/* \brief
 *		generator
 */

#include "cc.h"
#include "logger.h"
#include "backend.h"
#include "parser/symbols.h"
#include "parser/init.h"
#include "parser/types.h"
#include "parser/expr.h"


static void doglobal(struct tagCCContext* ctx, struct tagCCSymbol* p);
static void doexternal(struct tagCCContext* ctx, struct tagCCSymbol* p);
static void doconstant(struct tagCCContext* ctx, struct tagCCSymbol* p);
static void doexport(struct tagCCContext* ctx, struct tagCCSymbol* p);

static void cc_gen_dumpinitvalues(struct tagCCContext* ctx, struct tagCCSymbol* p);

void cc_gen_internalname(struct tagCCSymbol* sym)
{
	if (sym->_type && IsFunction(sym->_type))
	{
		int convention = sym->_type->_u._f._convention;

		assert(convention != Type_Defcall);
		if (convention == Type_Cdecl)
		{
			sym->_x._name = hs_hashstr(util_stringf("_%s", sym->_name));
		}
		else
		{
			FCCType** protos, *ty;
			int bytes;

			protos = sym->_type->_u._f._protos;
			for (bytes = 0; *protos; ++protos)
			{
				ty = cc_type_promote(*protos);
				bytes += ty->_size;
			} /* end for */

			if (convention == Type_Stdcall) {
				sym->_x._name = hs_hashstr(util_stringf("_%s@%d", sym->_name, bytes));
			}
			else {
				sym->_x._name = hs_hashstr(util_stringf("@%s@%d", sym->_name, bytes));
			}
		}
	} 
	else if (sym->_generated) {
		sym->_x._name = hs_hashstr(util_stringf("$%s_", sym->_name));
	}
	else if (sym->_scope == SCOPE_GLOBAL || sym->_sclass == SC_External) {
		sym->_x._name = hs_hashstr(util_stringf("_%s", sym->_name));
	}
	else {
		sym->_x._name = hs_hashstr(util_stringf("%s$", sym->_name));
	}
}

void cc_gen_dumpsymbols(struct tagCCContext* ctx)
{
	cc_symbol_foreach(ctx, gGlobals, SCOPE_GLOBAL, &doglobal);
	cc_symbol_foreach(ctx, gConstants, SCOPE_CONST, &doconstant);

	ctx->_backend->_comment(ctx, "--------export & import-----------");
	cc_symbol_foreach(ctx, gGlobals, SCOPE_GLOBAL, &doexport);
	cc_symbol_foreach(ctx, gExternals, SCOPE_GLOBAL, &doexternal);
}

static void doglobal(struct tagCCContext* ctx, struct tagCCSymbol* p)
{
	if (!IsFunction(p->_type)) 
	{
		FCCType* ty;

		for (ty = p->_type; IsArray(ty); ty = ty->_type) { /* do nothing */ }

		if (p->_defined) {
			ctx->_backend->_defglobal_begin(ctx, p, IsConst(ty) ? SEG_CONST : SEG_DATA);
			cc_gen_dumpinitvalues(ctx, p);
			ctx->_backend->_defglobal_end(ctx, p);
		}
		else if (p->_sclass != SC_External && p->_sclass != SC_Typedef) {
			ctx->_backend->_defglobal_begin(ctx, p, IsConst(ty) ? SEG_CONST : SEG_BSS);
			ctx->_backend->_defconst_signed(ctx, 1, 0, p->_type->_size);
			ctx->_backend->_defglobal_end(ctx, p);
		}
	}
}

static void doexport(struct tagCCContext* ctx, struct tagCCSymbol* p)
{
	if (!p->_defined && (p->_sclass == SC_External
		|| (IsFunction(p->_type) && p->_sclass == SC_Auto)))
	{
		if (p->_x._refcnt > 0) {
			ctx->_backend->_importsymbol(ctx, p);
		}
	}
	else if (p->_sclass != SC_Static && p->_sclass != SC_Typedef) {
		ctx->_backend->_exportsymbol(ctx, p);
	}
}

static void doexternal(struct tagCCContext* ctx, struct tagCCSymbol* p)
{
	ctx->_backend->_importsymbol(ctx, p);
}

static void doconstant(struct tagCCContext* ctx, struct tagCCSymbol* p)
{
	FCCType* ty = UnQual(p->_type);

	/* don't export integers to constant-table */
	if (ty->_op == Type_SInteger 
		|| ty->_op == Type_UInteger 
		|| ty->_op == Type_Pointer)
	{
		return;
	}

	ctx->_backend->_defglobal_begin(ctx, p, SEG_CONST);

	switch (ty->_op)
	{
	case Type_SInteger:
		ctx->_backend->_defconst_signed(ctx, ty->_size, p->_u._cnstval._sint, 1);
		break;
	case Type_UInteger:
		ctx->_backend->_defconst_unsigned(ctx, ty->_size, p->_u._cnstval._uint, 1);
		break;
	case Type_Float:
		ctx->_backend->_defconst_real(ctx, ty->_size, p->_u._cnstval._float, 1);
		break;
	case Type_Pointer:
		ctx->_backend->_defconst_unsigned(ctx, ty->_size, p->_u._cnstval._pointer, 1);
		break;
	case Type_Array:
		ctx->_backend->_defconst_string(ctx, p->_u._cnstval._payload, ty->_size, ty->_type->_size);
		break;
	default:
		assert(0);
		break;
	}
	ctx->_backend->_defglobal_end(ctx, p);
}


/*-------------------------------------------------------------
 *  initializer dump
 *-------------------------------------------------------------*/
static BOOL cc_gen_dumpinitvalues_inner(struct tagCCContext* ctx, struct tagCCType* ty, FVarInitializer* init, int* outerindex, BOOL bUsingOuterBlock);

static BOOL cc_varinit_dump_scalar(struct tagCCContext* ctx, struct tagCCType* ty, FVarInitializer* init, int* outerindex, BOOL bUsingOuterBlock)
{
	FVarInitializer* thisinit;
	FCCIRTree* expr;

	assert(!bUsingOuterBlock || init->_isblock);
	if (bUsingOuterBlock) {
		thisinit = *(init->_u._kids._kids + *outerindex);
		(*outerindex)++;
	}
	else {
		thisinit = init;
	}

	if (thisinit->_isblock) {
		if (thisinit->_u._kids._cnt > 1) {
			logger_output_s("error: too many initializers at %w.\n", &init->_loc);
			return FALSE;
		}

		thisinit = *(thisinit->_u._kids._kids + 0);
	}

	assert(!thisinit->_isblock);
	expr = thisinit->_u._expr;

	if (UnQual(ty)->_op == Type_Enum) {
		ty = UnQual(ty)->_type;
	}

	switch (UnQual(ty)->_op)
	{
	case Type_SInteger:
		if (IR_OP(expr->_op) != IR_CONST) {
			logger_output_s("error: integer constant expected. at %w\n", &expr->_loc);
			return FALSE;
		}
		ctx->_backend->_defconst_signed(ctx, ty->_size, expr->_u._val._sint, 1);
		break;
	case Type_UInteger:
		if (IR_OP(expr->_op) != IR_CONST) {
			logger_output_s("error: integer constant expected. at %w\n", &expr->_loc);
			return FALSE;
		}
		ctx->_backend->_defconst_unsigned(ctx, ty->_size, expr->_u._val._uint, 1);
		break;
	case Type_Float:
		if (IR_OP(expr->_op) != IR_CONST) {
			logger_output_s("error: float constant expected. at %w\n", &expr->_loc);
			return FALSE;
		}
		ctx->_backend->_defconst_real(ctx, ty->_size, expr->_u._val._float, 1);
		break;
	case Type_Pointer:
		ctx->_backend->_defconst_address(ctx, expr);
		break;
	default:
		assert(0);
		break;
	}

	return TRUE;
}

static BOOL cc_varinit_get_scalar_int(struct tagCCContext* ctx, struct tagCCType* ty, FVarInitializer* init, int* outerindex, BOOL bUsingOuterBlock, int *outval)
{
	FVarInitializer* thisinit;
	FCCIRTree* expr;

	assert(!bUsingOuterBlock || init->_isblock);
	if (bUsingOuterBlock) {
		thisinit = *(init->_u._kids._kids + *outerindex);
		(*outerindex)++;
	}
	else {
		thisinit = init;
	}

	if (thisinit->_isblock) {
		if (thisinit->_u._kids._cnt > 1) {
			logger_output_s("error: too many initializers at %w.\n", &init->_loc);
			return FALSE;
		}

		thisinit = *(thisinit->_u._kids._kids + 0);
	}

	assert(!thisinit->_isblock);
	expr = thisinit->_u._expr;

	if (UnQual(ty)->_op == Type_Enum) {
		ty = UnQual(ty)->_type;
	}
	switch (UnQual(ty)->_op)
	{
	case Type_SInteger:
		*outval = (int32_t)expr->_u._val._sint;
		break;
	case Type_UInteger:
		*outval = (int32_t)expr->_u._val._uint;
		break;
	default:
		assert(0);
		break;
	}

	return TRUE;
}

static BOOL cc_varinit_dump_union(struct tagCCContext* ctx, struct tagCCType* ty, FVarInitializer* init, int* outerindex, BOOL bUsingOuterBlock)
{
	FVarInitializer* thisinit, * tmpinit;
	FCCField* first;
	BOOL bnewblock = TRUE;

	assert(!bUsingOuterBlock || init->_isblock);
	thisinit = init;
	if (bUsingOuterBlock) {
		tmpinit = *(init->_u._kids._kids + *outerindex);
		if (tmpinit->_isblock) {
			(*outerindex)++;
			thisinit = tmpinit;
		}
		else {
			bnewblock = FALSE;
		}
	}
	if (!thisinit->_isblock) {
		logger_output_s("error: initializer for union need brace '{', at %w\n", &thisinit->_loc);
		return FALSE;
	}

	first = cc_type_fields(ty);
	if (bnewblock) {
		int innerindex = 0;

		if (first->_lsb > 0) {
			int bitsval;
			if (!cc_varinit_get_scalar_int(ctx, first->_type, thisinit, &innerindex, TRUE, &bitsval))
			{
				return FALSE;
			}

			bitsval = bitsval << (first->_lsb - 1);
			ctx->_backend->_defconst_unsigned(ctx, 4, (uint64_t)bitsval, 1);
		} else {
			if (!cc_gen_dumpinitvalues_inner(ctx, first->_type, thisinit, &innerindex, TRUE))
			{
				return FALSE;
			}
		}

		if (innerindex < thisinit->_u._kids._cnt) {
			logger_output_s("error: too many initializers in block %w\n", &thisinit->_loc);
			return FALSE;
		}
	}
	else {
		if (first->_lsb > 0) {
			int bitsval;
			if (!cc_varinit_get_scalar_int(ctx, first->_type, thisinit, outerindex, TRUE, &bitsval))
			{
				return FALSE;
			}

			bitsval = bitsval << (first->_lsb - 1);
			ctx->_backend->_defconst_unsigned(ctx, 4, (uint64_t)bitsval, 1);
		}
		else {
			if (!cc_gen_dumpinitvalues_inner(ctx, first->_type, thisinit, outerindex, TRUE))
			{
				return FALSE;
			}
		}
	}

	return TRUE;
}

static BOOL cc_varinit_dump_struct(struct tagCCContext* ctx, struct tagCCType* ty, FVarInitializer* init, int* outerindex, BOOL bUsingOuterBlock)
{
	FVarInitializer* thisinit, * tmpinit;
	FCCField* field;
	BOOL bnewblock = TRUE;

	assert(!bUsingOuterBlock || init->_isblock);
	thisinit = init;
	if (bUsingOuterBlock) {
		tmpinit = *(init->_u._kids._kids + *outerindex);
		if (tmpinit->_isblock) {
			(*outerindex)++;
			thisinit = tmpinit;
		}
		else {
			bnewblock = FALSE;
		}
	}
	
	assert(thisinit->_isblock);

	if (bnewblock) {
		int offset = 0, innerindex = 0;

		for (field = cc_type_fields(ty); field; ) 
		{
			if (offset < field->_offset) {
				ctx->_backend->_defconst_signed(ctx, 1, 0, field->_offset - offset);
				offset = field->_offset;
			}

			if (field->_lsb > 0) {
				int bitsval = 0, val = 0;

				assert(field->_type->_size == 4);
				for (; field; field = field->_next) {
					if (field->_offset != offset) {
						break;
					}
					if (innerindex < thisinit->_u._kids._cnt) {
						if (!cc_varinit_get_scalar_int(ctx, field->_type, thisinit, &innerindex, TRUE, &val))
						{
							return FALSE;
						}
						bitsval |= val << (field->_lsb - 1);
					}
				}

				ctx->_backend->_defconst_unsigned(ctx, 4, bitsval, 1);
				offset += 4;
			}
			else {
				if (innerindex < thisinit->_u._kids._cnt) {
					if (!cc_gen_dumpinitvalues_inner(ctx, field->_type, thisinit, &innerindex, TRUE))
					{
						return FALSE;
					}
				}
				else {
					ctx->_backend->_defconst_signed(ctx, 1, 0, field->_type->_size);
				}

				offset += field->_type->_size;
				field = field->_next;
			}
		} /* end for */
	}
	else {
		int offset = 0;

		for (field = cc_type_fields(ty); field; ) 
		{
			if (offset < field->_offset) {
				ctx->_backend->_defconst_signed(ctx, 1, 0, field->_offset - offset);
				offset = field->_offset;
			}

			if (field->_lsb > 0) {
				int bitsval = 0, val = 0;

				assert(field->_type->_size == 4);
				for (; field; field = field->_next) {
					if (field->_offset != offset) {
						break;
					}
					if (*outerindex < thisinit->_u._kids._cnt) {
						if (!cc_varinit_get_scalar_int(ctx, field->_type, thisinit, outerindex, TRUE, &val))
						{
							return FALSE;
						}
						bitsval |= val << (field->_lsb - 1);
					}
				}

				ctx->_backend->_defconst_unsigned(ctx, 4, bitsval, 1);
				offset += 4;
			}
			else {
				if (*outerindex < thisinit->_u._kids._cnt) {
					if (!cc_gen_dumpinitvalues_inner(ctx, field->_type, thisinit, outerindex, TRUE))
					{
						return FALSE;
					}
				}
				else {
					ctx->_backend->_defconst_signed(ctx, 1, 0, field->_type->_size);
				}

				offset += field->_type->_size;
				field = field->_next;
			}
		} /* end for */
	}

	return TRUE;
}

static BOOL cc_varinit_dump_array(struct tagCCContext* ctx, struct tagCCType* ty, FVarInitializer* init, int* outerindex, BOOL bUsingOuterBlock)
{
	FVarInitializer* thisinit, * tmpinit;
	BOOL bnewblock = TRUE;

	thisinit = init;
	if (bUsingOuterBlock) {
		tmpinit = *(init->_u._kids._kids + *outerindex);
		if (tmpinit->_isblock) {
			(*outerindex)++;
			thisinit = tmpinit;
		}
		else {
			bnewblock = FALSE;
		}
	}

	/* check if new block */
	if (bnewblock)
	{
		/*  ie:
		 *    int a[] = { 0, 1, 2}
		 *    char s0[] = { "abc" }
		 *    char s1[] = { 'a', 'b', "cd" }
		 *
		 */

		FCCType* elety;
		BOOL ischararray, iscnststr;
		int arraysize, innerindex;

		elety = UnQual(ty->_type);
		ischararray = (elety == gbuiltintypes._chartype) || (elety == gbuiltintypes._wchartype);

		if (thisinit->_isblock)
		{
			assert(ty->_size > 0);
			for (innerindex = arraysize = 0; ty->_size > arraysize && innerindex < thisinit->_u._kids._cnt; arraysize += elety->_size)
			{
				tmpinit = *(thisinit->_u._kids._kids + innerindex);
				iscnststr = !tmpinit->_isblock && (tmpinit->_u._expr->_op == IR_MKOP1(IR_CONST, IR_STRA) || tmpinit->_u._expr->_op == IR_MKOP1(IR_CONST, IR_STRW));

				if (ischararray && iscnststr)
				{
					int n, chcnt;
					const void* str;

					str = tmpinit->_u._expr->_u._val._payload;
					chcnt = (tmpinit->_u._expr->_ty->_size) / (tmpinit->_u._expr->_ty->_type->_size);
					if (elety == gbuiltintypes._chartype) {
						for (n = 0; ty->_size > arraysize; n++, arraysize += elety->_size)
						{
							ctx->_backend->_defconst_unsigned(ctx, 1, (n < chcnt) ? ((const uint8_t*)str)[n] : 0, 1);
						}
					}
					else {
						for (n = 0; ty->_size > arraysize; n++, arraysize += elety->_size)
						{
							ctx->_backend->_defconst_unsigned(ctx, 2, (n < chcnt) ? ((const uint16_t*)str)[n] : 0, 1);
						}
					}

					innerindex++;
					break; /* don't continue */
				}
				else if (!cc_gen_dumpinitvalues_inner(ctx, elety, thisinit, &innerindex, TRUE))
				{
					return FALSE;
				}
			} /* end for */

			if (ty->_size > arraysize) {
				ctx->_backend->_defconst_unsigned(ctx, 1, 0, ty->_size - arraysize); /* un-initialized data */
			}
		}
		else
		{
			tmpinit = thisinit;
			iscnststr = !tmpinit->_isblock && (tmpinit->_u._expr->_op == IR_MKOP1(IR_CONST, IR_STRA) || tmpinit->_u._expr->_op == IR_MKOP1(IR_CONST, IR_STRW));
			if (ischararray && iscnststr)
			{
				int n, chcnt;
				const void* str;

				str = tmpinit->_u._expr->_u._val._payload;
				chcnt = (tmpinit->_u._expr->_ty->_size) / (tmpinit->_u._expr->_ty->_type->_size);
				if (elety == gbuiltintypes._chartype) {
					for (n = 0; n < (ty->_size / ty->_type->_size); n++)
					{
						ctx->_backend->_defconst_unsigned(ctx, 1, (n < chcnt) ? ((const uint8_t*)str)[n] : 0, 1);
					}
				}
				else {
					for (n = 0; n < (ty->_size / ty->_type->_size); n++)
					{
						ctx->_backend->_defconst_unsigned(ctx, 2, (n < chcnt) ? ((const uint16_t*)str)[n] : 0, 1);
					}
				}
			}
			else
			{
				logger_output_s("error: initializer for array need brace '{', at %w\n", &tmpinit->_loc);
				return FALSE;
			}
		}
	}
	else /* !bnewblock */
	{
		FCCType* elety;
		BOOL ischararray, iscnststr;
		int arraysize;

		assert(thisinit->_isblock);
		elety = UnQual(ty->_type);
		ischararray = (elety == gbuiltintypes._chartype) || (elety == gbuiltintypes._wchartype);

		assert(ty->_size > 0);
		for (arraysize = 0; ty->_size > arraysize && *outerindex < thisinit->_u._kids._cnt; arraysize += elety->_size)
		{
			tmpinit = *(thisinit->_u._kids._kids + *outerindex);
			iscnststr = !tmpinit->_isblock && (tmpinit->_u._expr->_op == IR_MKOP1(IR_CONST, IR_STRA) || tmpinit->_u._expr->_op == IR_MKOP1(IR_CONST, IR_STRW));

			if (ischararray && iscnststr)
			{
				/* ie: "ABC" will be ignore for str[4].
					struct Struct {
						int a;
						int b;
						char str[4];
					};

					struct Struct s = { 1, 2, 'a', 'b', "ABC" };
				*/

				int n, chcnt;
				const void* str;

				if (arraysize > 0) {
					break; /* "ABC" will be ignore for str[4]. */
				}

				str = tmpinit->_u._expr->_u._val._payload;
				chcnt = (tmpinit->_u._expr->_ty->_size) / (tmpinit->_u._expr->_ty->_type->_size);
				if (elety == gbuiltintypes._chartype) {
					for (n = 0; ty->_size > arraysize; n++, arraysize += elety->_size)
					{
						ctx->_backend->_defconst_unsigned(ctx, 1, (n < chcnt) ? ((const uint8_t*)str)[n] : 0, 1);
					}
				}
				else {
					for (n = 0; ty->_size > arraysize; n++, arraysize += elety->_size)
					{
						ctx->_backend->_defconst_unsigned(ctx, 2, (n < chcnt) ? ((const uint16_t*)str)[n] : 0, 1);
					}
				}

				(*outerindex)++;
				break; /* don't continue */
			}
			else if (!cc_gen_dumpinitvalues_inner(ctx, elety, thisinit, outerindex, TRUE))
			{
				return FALSE;
			}
		} /* end for */

		if (ty->_size > arraysize)
		{
			ctx->_backend->_defconst_unsigned(ctx, 1, 0, ty->_size - arraysize); /* un-initialized data */
		}
	}

	return TRUE;
}

static BOOL cc_gen_dumpinitvalues_inner(struct tagCCContext* ctx, struct tagCCType* ty, FVarInitializer* init, int* outerindex, BOOL bUsingOuterBlock)
{
	if (IsScalar(ty)) {
		return cc_varinit_dump_scalar(ctx, ty, init, outerindex, bUsingOuterBlock);
	}
	else if (IsUnion(ty)) {
		return cc_varinit_dump_union(ctx, ty, init, outerindex, bUsingOuterBlock);
	}
	else if (IsStruct(ty)) {
		return cc_varinit_dump_struct(ctx, ty, init, outerindex, bUsingOuterBlock);
	}
	else if (IsArray(ty)) {
		return cc_varinit_dump_array(ctx, ty, init, outerindex, bUsingOuterBlock);
	}
	else {
		assert(0);
	}

	return TRUE;
}

static void cc_gen_dumpinitvalues(struct tagCCContext* ctx, struct tagCCSymbol* p)
{
	assert(p->_u._initializer);

	cc_gen_dumpinitvalues_inner(ctx, p->_type, p->_u._initializer, NULL, FALSE);
}
