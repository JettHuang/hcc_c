/* \brief
 *		expression parsing.
 */

#include <string.h>
#include <stdarg.h>

#include "expr.h"
#include "lexer/token.h"
#include "logger.h"
#include "parser.h"


static BOOL cc_expr_parse_primary(FCCContext* ctx, FCCIRTree** outexpr, enum EMMArea where);
static BOOL cc_expr_parse_postfix(FCCContext* ctx, FCCIRTree** outexpr, enum EMMArea where);
static BOOL cc_expr_parse_arguments(FCCContext* ctx, FArray* args, enum EMMArea where);
static BOOL cc_expr_parse_unary(FCCContext* ctx, FCCIRTree** outexpr, enum EMMArea where);
static BOOL cc_expr_parse_cast(FCCContext* ctx, FCCIRTree** outexpr, enum EMMArea where);
static BOOL cc_expr_parse_multiplicative(FCCContext* ctx, FCCIRTree** outexpr, enum EMMArea where);
static BOOL cc_expr_parse_additive(FCCContext* ctx, FCCIRTree** outexpr, enum EMMArea where);
static BOOL cc_expr_parse_shift(FCCContext* ctx, FCCIRTree** outexpr, enum EMMArea where);
static BOOL cc_expr_parse_relational(FCCContext* ctx, FCCIRTree** outexpr, enum EMMArea where);
static BOOL cc_expr_parse_equality(FCCContext* ctx, FCCIRTree** outexpr, enum EMMArea where);
static BOOL cc_expr_parse_bitand(FCCContext* ctx, FCCIRTree** outexpr, enum EMMArea where);
static BOOL cc_expr_parse_bitxor(FCCContext* ctx, FCCIRTree** outexpr, enum EMMArea where);
static BOOL cc_expr_parse_bitor(FCCContext* ctx, FCCIRTree** outexpr, enum EMMArea where);
static BOOL cc_expr_parse_logicand(FCCContext* ctx, FCCIRTree** outexpr, enum EMMArea where);
static BOOL cc_expr_parse_logicor(FCCContext* ctx, FCCIRTree** outexpr, enum EMMArea where);
static BOOL cc_expr_parse_conditional(FCCContext* ctx, FCCIRTree** outexpr, enum EMMArea where);

/* ------------------------------------------------------------------------------------------ */

static FCCIRTree* cc_expr_new(enum EMMArea where)
{
	FCCIRTree* tree = mm_alloc_area(sizeof(FCCIRTree), where);
	if (tree) {
		memset(tree, 0, sizeof(FCCIRTree));
	}
	else {
		logger_output_s("error: out of memory at %s:%d\n", __FILE__, __LINE__);
	}

	return tree;
}


static BOOL cc_expr_parse_primary(FCCContext* ctx, FCCIRTree** outexpr, enum EMMArea where)
{
	FCCIRTree* tree;
	FCCSymbol* p;
	FCCType* ty;

	switch (ctx->_currtk._type)
	{
	case TK_ID:
	{
		const char* id = ctx->_currtk._val._astr._str;
		if (!(p = cc_symbol_lookup(id, gIdentifiers))) {
			logger_output_s("error: undeclared identifier %s at %w\n", id, &ctx->_currtk._loc);
			return FALSE;
		}

		if (IsEnum(p->_type))
		{
			tree = cc_expr_constant(ty, cc_ir_typecode(ty), &ctx->_currtk._loc, where, (int)p->_u._cnstval._sint);
		}
		else
		{
			if (!(tree = cc_expr_id(ctx, p, &ctx->_currtk._loc, where))) {
				return FALSE;
			}
		}
		cc_read_token(ctx, &ctx->_currtk);
	}
	break;
	case TK_CONSTANT_INT:
		tree = cc_expr_constant(gbuiltintypes._sinttype, cc_ir_typecode(gbuiltintypes._sinttype), &ctx->_currtk._loc, where, ctx->_currtk._val._int);
		cc_read_token(ctx, &ctx->_currtk);
		break;
	case TK_CONSTANT_UINT:
		tree = cc_expr_constant(gbuiltintypes._uinttype, cc_ir_typecode(gbuiltintypes._uinttype), &ctx->_currtk._loc, where, ctx->_currtk._val._uint);
		cc_read_token(ctx, &ctx->_currtk);
		break;
	case TK_CONSTANT_LONG:
		tree = cc_expr_constant(gbuiltintypes._slongtype, cc_ir_typecode(gbuiltintypes._slongtype), &ctx->_currtk._loc, where, ctx->_currtk._val._long);
		cc_read_token(ctx, &ctx->_currtk);
		break;
	case TK_CONSTANT_ULONG:
		tree = cc_expr_constant(gbuiltintypes._ulongtype, cc_ir_typecode(gbuiltintypes._ulongtype), &ctx->_currtk._loc, where, ctx->_currtk._val._ulong);
		cc_read_token(ctx, &ctx->_currtk);
		break;
	case TK_CONSTANT_LLONG:
		tree = cc_expr_constant(gbuiltintypes._sllongtype, cc_ir_typecode(gbuiltintypes._sllongtype), &ctx->_currtk._loc, where, ctx->_currtk._val._llong);
		cc_read_token(ctx, &ctx->_currtk);
		break;
	case TK_CONSTANT_ULLONG:
		tree = cc_expr_constant(gbuiltintypes._ullongtype, cc_ir_typecode(gbuiltintypes._ullongtype), &ctx->_currtk._loc, where, ctx->_currtk._val._ullong);
		cc_read_token(ctx, &ctx->_currtk);
		break;
	case TK_CONSTANT_FLOAT:
		tree = cc_expr_constant(gbuiltintypes._floattype, cc_ir_typecode(gbuiltintypes._floattype), &ctx->_currtk._loc, where, ctx->_currtk._val._float);
		cc_read_token(ctx, &ctx->_currtk);
		break;
	case TK_CONSTANT_DOUBLE:
		tree = cc_expr_constant(gbuiltintypes._doubletype, cc_ir_typecode(gbuiltintypes._doubletype), &ctx->_currtk._loc, where, ctx->_currtk._val._double);
		cc_read_token(ctx, &ctx->_currtk);
		break;
	case TK_CONSTANT_LDOUBLE:
		tree = cc_expr_constant(gbuiltintypes._ldoubletype, cc_ir_typecode(gbuiltintypes._ldoubletype), &ctx->_currtk._loc, where, ctx->_currtk._val._ldouble);
		cc_read_token(ctx, &ctx->_currtk);
		break;
	case TK_CONSTANT_CHAR:
		tree = cc_expr_constant(gbuiltintypes._chartype, cc_ir_typecode(gbuiltintypes._chartype), &ctx->_currtk._loc, where, ctx->_currtk._val._ch);
		cc_read_token(ctx, &ctx->_currtk);
		break;
	case TK_CONSTANT_WCHAR:
		tree = cc_expr_constant(gbuiltintypes._wchartype, cc_ir_typecode(gbuiltintypes._wchartype), &ctx->_currtk._loc, where, ctx->_currtk._val._wch);
		cc_read_token(ctx, &ctx->_currtk);
		break;
	case TK_CONSTANT_STR:
		ty = cc_type_newarray(gbuiltintypes._chartype, ctx->_currtk._val._astr._chcnt, 0);
		tree = cc_expr_constant(ty, IR_STRA, &ctx->_currtk._loc, where, ctx->_currtk._val._astr._str);
		cc_read_token(ctx, &ctx->_currtk);
		break;
	case TK_CONSTANT_WSTR:
		ty = cc_type_newarray(gbuiltintypes._wchartype, ctx->_currtk._val._wstr._chcnt / gbuiltintypes._wchartype->_size, 0);
		tree = cc_expr_constant(ty, IR_STRW, &ctx->_currtk._loc, where, ctx->_currtk._val._wstr._str);
		cc_read_token(ctx, &ctx->_currtk);
		break;
	case TK_LPAREN: /* '(' */
	{
		cc_read_token(ctx, &ctx->_currtk);
		if (!cc_expr_parse_expression(ctx, &tree, where)) {
			return FALSE;
		}
		if (!cc_parser_expect(ctx, TK_RPAREN)) { /* ')' */
			return FALSE;
		}
	}
		break;
	default:
		logger_output_s("error: illegal expression at %w\n", &ctx->_currtk._loc);
		return FALSE;
	}

	*outexpr = tree;
	return TRUE;
}



static BOOL cc_expr_parse_postfix(FCCContext* ctx, FCCIRTree** outexpr, enum EMMArea where)
{
	FCCIRTree* expr, * tree;
	FLocation loc;

	if (!cc_expr_parse_primary(ctx, &expr, where)) {
		return FALSE;
	}

	for (;;)
	{
		if (ctx->_currtk._type == TK_LBRACKET) /* '[' */
		{
			FCCIRTree* subscript, *offset;
			FCCType* ty;

			loc = ctx->_currtk._loc;
			cc_read_token(ctx, &ctx->_currtk);
			if (!cc_expr_parse_expression(ctx, &subscript, where)) {
				return FALSE;
			}

			if (!cc_parser_expect(ctx, TK_RBRACKET)) /* ']' */ {
				return FALSE;
			}

			expr = cc_expr_adjust(expr, where);
			ty = UnQual(expr->_ty);
			if (!IsPtr(expr->_ty)) {
				logger_output_s("error: pointer or array is expected at %w\n", &loc);
				return FALSE;
			}
			if (ty->_type->_size <= 0) {
				logger_output_s("error: invalid type %t for left operand of '[]' at %w\n", ty, &expr->_loc);
				return FALSE;
			}
			if (!IsInt(subscript->_ty)) {
				logger_output_s("error, integer is expected at %w\n", &subscript->_loc);
				return FALSE;
			}
			
			offset = cc_expr_mul(gbuiltintypes._sinttype, subscript, cc_expr_constant(gbuiltintypes._sinttype, cc_ir_typecode(gbuiltintypes._sinttype), &loc, where, ty->_type->_size), &loc, where);
			tree = cc_expr_add(ty, expr, offset, &loc, where);
			if (IsPtr(tree->_ty) && IsArray(tree->_ty->_type)) {
				tree = cc_expr_change_rettype(tree, tree->_ty->_type, where);
			}
			else {
				tree = cc_expr_indir(expr, &loc, where);
			}
			tree->_islvalue = expr->_islvalue;

			expr = tree;
		}
		else if (ctx->_currtk._type == TK_LPAREN) /* '(' */
		{
			FArray args;
			FCCType* ty, *rty;

			loc = ctx->_currtk._loc;
			cc_read_token(ctx, &ctx->_currtk);
			array_init(&args, 32, sizeof(FCCIRTree*), where);
			if (!cc_expr_parse_arguments(ctx, &args, where)) {
				return FALSE;
			}

			array_append_zeroed(&args);
			if (!cc_parser_expect(ctx, TK_RPAREN)) /* ')' */
			{
				return FALSE;
			}

			expr = cc_expr_adjust(expr, where);
			ty = UnQual(expr->_ty);
			if (IsPtr(ty) && IsFunction(ty->_type))
				rty = ty->_type->_type;
			else {
				logger_output_s("error: found `%t' expected a function at %w\n", ty, &loc);
				return FALSE;
			}
			
			if (!(tree = cc_expr_call(ty->_type, expr, &args, &loc, where))) {
				return FALSE;
			}

			expr = tree;
		}
		else if (ctx->_currtk._type == TK_DOT) /* '.' */
		{
			const char* fieldname;
			FCCIRTree* base, *offset;
			FCCField* field;
			FCCType* ty;

			loc = ctx->_currtk._loc;
			cc_read_token(ctx, &ctx->_currtk);
			if (ctx->_currtk._type != TK_ID)
			{
				logger_output_s("error: illegal field syntax, require an identifier, at %w\n", &ctx->_currtk._loc);
				return FALSE;
			}
			fieldname = ctx->_currtk._val._astr._str;

			ty = UnQual(expr->_ty);
			if (!IsStruct(ty) || !expr->_islvalue) {
				logger_output_s("error: l-value structure is required at %w\n", &expr->_loc);
				return FALSE;
			}

			if (!(field = cc_type_findfield(fieldname, ty)))
			{
				logger_output_s("error: can't find field '%s' of %t at %w\n", fieldname, ty, &loc);
				return FALSE;
			}

			offset = cc_expr_constant(gbuiltintypes._sinttype, cc_ir_typecode(gbuiltintypes._sinttype), &loc, where, field->_offset);
			base = cc_expr_addr(expr, &expr->_loc, where);
			if (IsArray(field->_type)) {
				ty = field->_type;
			}
			else {
				ty = cc_type_ptr(field->_type);
			}
			tree = cc_expr_add(ty, base, offset, &loc, where);
			if (IsPtr(ty)) {
				tree = cc_expr_indir(tree, &loc, where);
			}
			tree->_u._field = field;
			tree->_isbitfield = field->_lsb > 0;
			tree->_islvalue = expr->_islvalue;

			expr = tree;
			cc_read_token(ctx, &ctx->_currtk);
		}
		else if (ctx->_currtk._type == TK_POINTER) /* -> */
		{
			const char* fieldname;
			FCCIRTree* base, * offset;
			FCCField* field;
			FCCType* ty;

			loc = ctx->_currtk._loc;
			cc_read_token(ctx, &ctx->_currtk);
			if (ctx->_currtk._type != TK_ID)
			{
				logger_output_s("error: illegal field syntax, require an identifier, at %w\n", &ctx->_currtk._loc);
				return FALSE;
			}
			fieldname = ctx->_currtk._val._astr._str;

			if (!IsPtr(expr->_ty) || ((ty = cc_type_deref(expr->_ty)) && !IsStruct(ty))) {
				logger_output_s("error: a pointer to structure is expected. at %w\n", &expr->_loc);
				return FALSE;
			}
			if (!(field = cc_type_findfield(fieldname, ty)))
			{
				logger_output_s("error: can't find field '%s' of %t at %w\n", fieldname, ty, &loc);
				return FALSE;
			}

			offset = cc_expr_constant(gbuiltintypes._sinttype, cc_ir_typecode(gbuiltintypes._sinttype), &loc, where, field->_offset);
			base = expr;
			if (IsArray(field->_type)) {
				ty = field->_type;
			}
			else {
				ty = cc_type_ptr(field->_type);
			}
			tree = cc_expr_add(ty, base, offset, &loc, where);
			if (IsPtr(ty)) {
				tree = cc_expr_indir(tree, &loc, where);
			}
			tree->_u._field = field;
			tree->_isbitfield = field->_lsb > 0;
			tree->_islvalue = 1;

			expr = tree;
			cc_read_token(ctx, &ctx->_currtk);
		}
		else if (ctx->_currtk._type == TK_INC) /* ++ */
		{
			FCCType* ty;
			int v;

			loc = ctx->_currtk._loc;
			if (!cc_expr_is_modifiable(expr)) {
				logger_output_s("error: modifiable l-value is expected at %w.\n", &expr->_loc);
				return FALSE;
			}

			ty = UnQual(expr->_ty);
			if (!IsScalar(ty)) {
				logger_output_s("error: %t is not a scalar type at %w\n", ty, &expr->_loc);
			}
			if (IsPtr(ty)) {
				v = IsVoidptr(ty) ? 1 : ty->_type->_size; /* surport void *p; p++ */
			}
			else {
				v = 1;
			}

			if (v <= 0)
			{
				logger_output_s("error pointer to an incomplete type %t at %w.\n", ty->_type, &expr->_loc);
				return FALSE;
			}

			tree = cc_expr_constant(gbuiltintypes._sinttype, cc_ir_typecode(gbuiltintypes._sinttype), &loc, where, v);
			tree = cc_expr_seq(ty, expr, cc_expr_assign(ty, expr, cc_expr_add(ty, expr, tree, &loc, where), &loc, where), &loc, where);
			tree = cc_expr_seq(ty, tree, expr, &loc, where);

			expr = tree;
			cc_read_token(ctx, &ctx->_currtk);
		}
		else if (ctx->_currtk._type == TK_DEC) /* -- */
		{
			FCCType* ty;
			int v;

			loc = ctx->_currtk._loc;
			if (!cc_expr_is_modifiable(expr)) {
				logger_output_s("error: modifiable l-value is expected at %w.\n", &expr->_loc);
				return FALSE;
			}

			ty = UnQual(expr->_ty);
			if (!IsScalar(ty)) {
				logger_output_s("error: %t is not a scalar type at %w\n", ty, &expr->_loc);
			}
			if (IsPtr(ty)) {
				v = IsVoidptr(ty) ? 1 : ty->_type->_size; /* surport void *p; p++ */
			}
			else {
				v = 1;
			}

			if (v <= 0)
			{
				logger_output_s("error pointer to an incomplete type %t at %w.\n", ty->_type, &expr->_loc);
				return FALSE;
			}

			tree = cc_expr_constant(gbuiltintypes._sinttype, cc_ir_typecode(gbuiltintypes._sinttype), &loc, where, v);
			tree = cc_expr_seq(ty, expr, cc_expr_assign(ty, expr, cc_expr_sub(ty, expr, tree, &loc, where), &loc, where), &loc, where);
			tree = cc_expr_seq(ty, tree, expr, &loc, where);

			expr = tree;
			cc_read_token(ctx, &ctx->_currtk);
		}
		else
		{
			break;
		}
	} /* end for ;; */

	*outexpr = expr;
	return TRUE;
}

static BOOL cc_expr_parse_arguments(FCCContext* ctx, FArray* args, enum EMMArea where)
{
	while (ctx->_currtk._type != TK_RPAREN)
	{
		FCCIRTree* expr = NULL;
		if (!cc_expr_parse_assignment(ctx, &expr, where))
		{
			return FALSE;
		}

		array_append(args, &expr);

		if (ctx->_currtk._type != TK_COMMA) /* ',' */
		{
			break;
		}

		cc_read_token(ctx, &ctx->_currtk);
	} /* end while */

	return TRUE;
}

static BOOL cc_expr_parse_unary(FCCContext* ctx, FCCIRTree** outexpr, enum EMMArea where)
{
	FCCIRTree* tree, * expr;
	FLocation loc;

	if (ctx->_currtk._type == TK_INC) /* ++ */
	{
		FCCType* ty;
		int v;

		loc = ctx->_currtk._loc;
		cc_read_token(ctx, &ctx->_currtk);
		if (!cc_expr_parse_unary(ctx, &expr, where))
		{
			return FALSE;
		}

		if (!cc_expr_is_modifiable(expr)) {
			logger_output_s("error: modifiable l-value is expected at %w.\n", &expr->_loc);
			return FALSE;
		}

		ty = UnQual(expr->_ty);
		if (!IsScalar(ty)) {
			logger_output_s("error: %t is not a scalar type at %w\n", ty, &expr->_loc);
		}
		if (IsPtr(ty)) {
			v = IsVoidptr(ty) ? 1 : ty->_type->_size; /* surport void *p; p++ */
		}
		else {
			v = 1;
		}

		if (v <= 0)
		{
			logger_output_s("error pointer to an incomplete type %t at %w.\n", ty->_type, &expr->_loc);
			return FALSE;
		}

		tree = cc_expr_constant(gbuiltintypes._sinttype, cc_ir_typecode(gbuiltintypes._sinttype), &loc, where, v);
		tree = cc_expr_assign(ty, expr, cc_expr_add(ty, expr, tree, &loc, where), &loc, where);

		*outexpr = tree;
		return TRUE;
	}
	else if (ctx->_currtk._type == TK_DEC) /* -- */
	{
		FCCType* ty;
		int v;

		loc = ctx->_currtk._loc;
		cc_read_token(ctx, &ctx->_currtk);
		if (!cc_expr_parse_unary(ctx, &expr, where))
		{
			return FALSE;
		}

		if (!cc_expr_is_modifiable(expr)) {
			logger_output_s("error: modifiable l-value is expected at %w.\n", &expr->_loc);
			return FALSE;
		}

		ty = UnQual(expr->_ty);
		if (!IsScalar(ty)) {
			logger_output_s("error: %t is not a scalar type at %w\n", ty, &expr->_loc);
		}
		if (IsPtr(ty)) {
			v = IsVoidptr(ty) ? 1 : ty->_type->_size; /* surport void *p; p++ */
		}
		else {
			v = 1;
		}

		if (v <= 0)
		{
			logger_output_s("error pointer to an incomplete type %t at %w.\n", ty->_type, &expr->_loc);
			return FALSE;
		}

		tree = cc_expr_constant(gbuiltintypes._sinttype, cc_ir_typecode(gbuiltintypes._sinttype), &loc, where, v);
		tree = cc_expr_assign(ty, expr, cc_expr_sub(ty, expr, tree, &loc, where), &loc, where);

		*outexpr = tree;
		return TRUE;
	}
	else if (ctx->_currtk._type == TK_BITAND) /* '&' */
	{
		loc = ctx->_currtk._loc;
		cc_read_token(ctx, &ctx->_currtk);
		if (!cc_expr_parse_cast(ctx, &expr, where))
		{
			return FALSE;
		}

		if (!expr->_islvalue) {
			logger_output_s("error: l-value is required for '&' at %w\n", &loc);
			return FALSE;
		}

		if (IsArray(expr->_ty) || IsFunction(expr->_ty)) {
			tree = cc_expr_change_rettype(expr, cc_type_ptr(expr->_ty), where);
		}
		else {
			tree = cc_expr_addr(expr, &loc, where);
		}
		if (IsAddrOp(tree->_op) && tree->_u._symbol->_sclass == SC_Register) {
			logger_output_s("error: invalid operand of unary '&', '%s' is declared register at %w\n", tree->_u._symbol->_name, &loc);
			return FALSE;
		}
		else if (IsAddrOp(tree->_op)) {
			tree->_u._symbol->_addressed = 1;
		}

		*outexpr = tree;
		return TRUE;
	}
	else if (ctx->_currtk._type == TK_MUL) /* '*' */
	{
		loc = ctx->_currtk._loc;
		cc_read_token(ctx, &ctx->_currtk);
		if (!cc_expr_parse_cast(ctx, &expr, where))
		{
			return FALSE;
		}

		expr = cc_expr_adjust(expr, where);
		if (!IsPtr(expr->_ty)) {
			logger_output_s("error: pointer is expected at %w\n", &loc);
			return FALSE;
		}
		if (IsFunction(expr->_ty->_type) || IsArray(expr->_ty->_type)) {
			expr = cc_expr_change_rettype(expr, expr->_ty->_type, where);
		}
		else {
			expr = cc_expr_indir(expr, &loc, where);
		}

		*outexpr = tree;
		return TRUE;
	}
	else if (ctx->_currtk._type == TK_ADD) /* '+' */
	{
		loc = ctx->_currtk._loc;
		cc_read_token(ctx, &ctx->_currtk);
		if (!cc_expr_parse_cast(ctx, &expr, where))
		{
			return FALSE;
		}

		expr = cc_expr_adjust(expr, where);
		if (!IsArith(expr->_ty)) {
			logger_output_s("error arithmetic type is expected for '+' at %w.\n", &loc);
			return FALSE;
		}
		tree = cc_expr_cast(cc_type_promote(expr->_ty), expr, &loc, where);

		*outexpr = tree;
		return TRUE;
	}
	else if (ctx->_currtk._type == TK_SUB) /* '-' */
	{
		loc = ctx->_currtk._loc;
		cc_read_token(ctx, &ctx->_currtk);
		if (!cc_expr_parse_cast(ctx, &expr, where))
		{
			return FALSE;
		}

		expr = cc_expr_adjust(expr, where);
		if (!IsArith(expr->_ty)) {
			logger_output_s("error arithmetic type is expected for '-' at %w.\n", &loc);
			return FALSE;
		}
		tree = cc_expr_cast(cc_type_promote(expr->_ty), expr, &loc, where);
		if (tree->_ty->_op == Type_UInteger) {
			logger_output_s("warning unsigned operand of unary '-' at %w\n", &loc);
			tree = cc_expr_add(tree->_ty, cc_expr_bitcom(tree->_ty, tree, &loc, where),
				cc_expr_constant(tree->_ty, cc_ir_typecode(tree->_ty), &loc, where, 1u), &loc, where);
		}
		else {
			tree = cc_expr_neg(tree->_ty, expr, &loc, where);
		}

		*outexpr = tree;
		return TRUE;
	}
	else if (ctx->_currtk._type == TK_COMP) /* '~' */
	{
		loc = ctx->_currtk._loc;
		cc_read_token(ctx, &ctx->_currtk);
		if (!cc_expr_parse_cast(ctx, &expr, where))
		{
			return FALSE;
		}

		expr = cc_expr_adjust(expr, where);
		if (!IsInt(expr->_ty)) {
			logger_output_s("error integer type is expected for '~' at %w.\n", &loc);
			return FALSE;
		}
		tree = cc_expr_cast(cc_type_promote(expr->_ty), expr, &loc, where);
		tree = cc_expr_neg(tree->_ty, tree, &loc, where);

		*outexpr = tree;
		return TRUE;
	}
	else if (ctx->_currtk._type == TK_NOT) /* '!' */
	{
		loc = ctx->_currtk._loc;
		cc_read_token(ctx, &ctx->_currtk);
		if (!cc_expr_parse_cast(ctx, &expr, where))
		{
			return FALSE;
		}

		expr = cc_expr_adjust(expr, where);
		if (!IsScalar(expr->_ty)) {
			logger_output_s("error scalar type is expected for '!' at %w.\n", &loc);
			return FALSE;
		}
		
		tree = cc_expr_not(gbuiltintypes._sinttype, expr, &loc, where);
		*outexpr = tree;
		return TRUE;
	}
	else if (ctx->_currtk._type == TK_SIZEOF)
	{
		FCCType* exprty;

		loc = ctx->_currtk._loc;
		cc_read_token(ctx, &ctx->_currtk);
		/*
		  sizeof (typename)
		  sizeof unary-expression
		 */
		exprty = NULL;
		if (ctx->_currtk._type == TK_LPAREN) /* '(' */
		{
			FCCToken aheadtk;
			cc_lookahead_token(ctx, &aheadtk);
			if (cc_parser_is_typename(&aheadtk))
			{
				int sclass;
				const char* id;
				FLocation loc;
				FCCType* ty;

				cc_read_token(ctx, &ctx->_currtk);
				if (!cc_parser_declarator(ctx, cc_parser_declspecifier(ctx, &sclass), &id, &loc, NULL, &ty) || sclass || id) {
					logger_output_s("error: illegal sizeof(type) at %w.\n", &ctx->_currtk._loc);
					return FALSE;
				}

				if (UnQual(ty)->_size <= 0) {
					logger_output_s("error: unknown sizeof(%t) at %w.\n", ty, &ctx->_currtk._loc);
					return FALSE;
				}

				if (!cc_parser_expect(ctx, TK_RPAREN)) /* ')' */
				{
					return FALSE;
				}

				exprty = ty;
			}
		}

		if (!exprty) /* handle 'sizeof unary-expression' */
		{
			FCCIRTree* subexpr;
			if (!cc_expr_parse_unary(ctx, &subexpr, where)) {
				return FALSE;
			}

			exprty = subexpr->_ty;
		}

		if (!(tree = cc_expr_constant(ctx, gbuiltintypes._sinttype, cc_ir_typecode(gbuiltintypes._sinttype), &loc, where, exprty->_size))) {
			return FALSE;
		}

		*outexpr = tree;
		return TRUE;
	}
	else
	{
		return cc_expr_parse_postfix(ctx, outexpr, where);
	}

	return FALSE;
}

static BOOL cc_expr_parse_cast(FCCContext* ctx, FCCIRTree** outexpr, enum EMMArea where)
{
	FCCIRTree* tree;

	if (ctx->_currtk._type == TK_LPAREN) /* '(' */
	{
		FCCToken aheadtk;

		cc_lookahead_token(ctx, &aheadtk);
		if (cc_parser_is_typename(&aheadtk))
		{
			int sclass;
			const char* id;
			FLocation loc;
			FCCType* ty;

			cc_read_token(ctx, &ctx->_currtk);
			if (!cc_parser_declarator(ctx, cc_parser_declspecifier(ctx, &sclass), &id, &loc, NULL, &ty) || sclass || id) {
				logger_output_s("error: illegal type-cast at %w.\n", &ctx->_currtk._loc);
				return FALSE;
			}

			if (UnQual(ty)->_size <= 0) {
				logger_output_s("error: unknown sizeof(%t) at %w.\n", ty, &ctx->_currtk._loc);
				return FALSE;
			}

			if (!cc_parser_expect(ctx, TK_RPAREN)) /* ')' */
			{
				return FALSE;
			}

			loc = ctx->_currtk._loc;
			if (!cc_expr_parse_cast(ctx, &tree, where)) {
				return FALSE;
			}

			if (!(tree = cc_expr_cast(ty, tree, &loc, where))) {
				return FALSE;
			}

			*outexpr = tree;
			return TRUE;
		}
	}

	return cc_expr_parse_unary(ctx, outexpr, where);
}

static BOOL cc_expr_parse_multiplicative(FCCContext* ctx, FCCIRTree** outexpr, enum EMMArea where)
{
	FCCIRTree* lhs, * rhs, * tree;
	FLocation loc;


	if (!cc_expr_parse_cast(ctx, &lhs, where))
	{
		return FALSE;
	}

	tree = lhs;
	while (ctx->_currtk._type == TK_MUL || ctx->_currtk._type == TK_DIV || ctx->_currtk._type == TK_MOD)
	{
		FCCType* ty;
		enum ECCToken tktype = ctx->_currtk._type;

		loc = ctx->_currtk._loc;
		cc_read_token(ctx, &ctx->_currtk);
		if (!cc_expr_parse_cast(ctx, &rhs, where)) {
			return FALSE;
		}

		ty = cc_type_select(lhs->_ty, rhs->_ty);
		switch (tktype)
		{
		case TK_MUL:
			tree = cc_expr_mul(ty, lhs, rhs, &loc, where); break;
		case TK_DIV:
			tree = cc_expr_div(ty, lhs, rhs, &loc, where); break;
		case TK_MOD:
			tree = cc_expr_mod(ty, lhs, rhs, &loc, where); break;
		default:
			tree = NULL; break;
		}
		
		if (!tree) {
			return FALSE;
		}

		lhs = tree;
	} /* end while */

	*outexpr = tree;
	return TRUE;
}

static BOOL cc_expr_parse_additive(FCCContext* ctx, FCCIRTree** outexpr, enum EMMArea where)
{
	FCCIRTree* lhs, * rhs, * tree;
	FLocation loc;


	if (!cc_expr_parse_multiplicative(ctx, &lhs, where))
	{
		return FALSE;
	}

	tree = lhs;
	while (ctx->_currtk._type == TK_ADD || ctx->_currtk._type == TK_SUB)
	{
		enum ECCToken tktype = ctx->_currtk._type;

		loc = ctx->_currtk._loc;
		cc_read_token(ctx, &ctx->_currtk);
		if (!cc_expr_parse_multiplicative(ctx, &rhs, where)) {
			return FALSE;
		}

		switch (tktype)
		{
		case TK_ADD:

		case TK_SUB:
		default:
			tree = NULL; break;
		}

		if (!(tree = cc_expr_additive(ctx, (tktype == TK_ADD) ? EXPR_ADD : EXPR_SUB, lhs, rhs, loc, where))) {
			return FALSE;
		}

		lhs = tree;
	} /* end while */

	*outexpr = tree;
	return TRUE;
}

static FCCIRTree* cc_expr_shift(FCCContext* ctx, int op, FCCIRTree* lhs, FCCIRTree* rhs, FLocation loc, enum EMMArea where)
{
	FCCIRTree* tree;

	if (!IsInt(lhs->_ty) || !IsInt(rhs->_ty)) {
		logger_output_s("error: integer operand is expected for '%s' at %w\n", cc_expr_opname(op), &loc);
		return NULL;
	}
	lhs = cc_expr_cast(ctx, cc_type_promote(lhs->_ty), lhs, where);
	rhs = cc_expr_cast(ctx, cc_type_promote(rhs->_ty), rhs, where);

	/* constant folding */
	if (lhs->_op == EXPR_CONSTANT && rhs->_op == EXPR_CONSTANT)
	{
		FCCConstVal cnstval;

		if (UnQual(lhs->_ty)->_op == Type_SInteger)
		{
			switch (op)
			{
			case EXPR_LSHFIT:
				cnstval._sint = lhs->_u._symbol->_u._cnstval._sint << rhs->_u._symbol->_u._cnstval._sint; break;
			case EXPR_RSHFIT:
				cnstval._sint = lhs->_u._symbol->_u._cnstval._sint >> rhs->_u._symbol->_u._cnstval._sint; break;
			default:
				break;
			}
		}
		else if (UnQual(lhs->_ty)->_op == Type_UInteger)
		{
			switch (op)
			{
			case EXPR_LSHFIT:
				cnstval._uint = lhs->_u._symbol->_u._cnstval._uint << rhs->_u._symbol->_u._cnstval._sint; break;
			case EXPR_RSHFIT:
				cnstval._uint = lhs->_u._symbol->_u._cnstval._uint >> rhs->_u._symbol->_u._cnstval._sint; break;
			default:
				break;
			}
		}

		return cc_expr_constant(ctx, lhs->_ty, cnstval, loc, where);
	}

	if (!(tree = cc_expr_new(where))) {
		return FALSE;
	}

	tree->_op = op;
	tree->_loc = loc;
	tree->_ty = lhs->_ty;
	tree->_u._kids[0] = lhs;
	tree->_u._kids[1] = rhs;

	return tree;
}

static BOOL cc_expr_parse_shift(FCCContext* ctx, FCCIRTree** outexpr, enum EMMArea where)
{
	FCCIRTree* lhs, * rhs, * tree;
	FLocation loc;


	if (!cc_expr_parse_additive(ctx, &lhs, where))
	{
		return FALSE;
	}

	tree = lhs;
	while (ctx->_currtk._type == TK_LSHIFT || ctx->_currtk._type == TK_RSHIFT)
	{

		enum ECCToken tktype = ctx->_currtk._type;

		loc = ctx->_currtk._loc;
		cc_read_token(ctx, &ctx->_currtk);
		if (!cc_expr_parse_additive(ctx, &rhs, where)) {
			return FALSE;
		}

		if (!(tree = cc_expr_shift(ctx, (tktype == TK_LSHIFT) ? EXPR_LSHFIT : EXPR_RSHFIT, lhs, rhs, loc, where))) {
			return FALSE;
		}

		lhs = tree;
	} /* end while */

	*outexpr = tree;
	return TRUE;
}

static FCCIRTree* cc_expr_relational(FCCContext* ctx, int op, FCCIRTree* lhs, FCCIRTree* rhs, FLocation loc, enum EMMArea where)
{
	FCCIRTree* tree;

	if (IsArith(lhs->_ty) && IsArith(rhs->_ty))
	{
		FCCType* ty;

		ty = cc_type_select(lhs->_ty, rhs->_ty);
		lhs = cc_expr_cast(ctx, ty, lhs, where);
		rhs = cc_expr_cast(ctx, ty, rhs, where);

		/* do constant fold */
		if (lhs->_op == EXPR_CONSTANT && rhs->_op == EXPR_CONSTANT)
		{
			FCCConstVal cnstval;

			if (UnQual(ty)->_op == Type_SInteger) {
				switch (op)
				{
				case EXPR_LESS:
					cnstval._sint = lhs->_u._symbol->_u._cnstval._sint < rhs->_u._symbol->_u._cnstval._sint; break;
				case EXPR_LESSEQ:
					cnstval._sint = lhs->_u._symbol->_u._cnstval._sint <= rhs->_u._symbol->_u._cnstval._sint; break;
				case EXPR_GREAT:
					cnstval._sint = lhs->_u._symbol->_u._cnstval._sint > rhs->_u._symbol->_u._cnstval._sint; break;
				case EXPR_GREATEQ:
					cnstval._sint = lhs->_u._symbol->_u._cnstval._sint >= rhs->_u._symbol->_u._cnstval._sint; break;
				default:
					assert(0);
					break;
				}
			}
			else if (UnQual(ty)->_op == Type_UInteger) {
				switch (op)
				{
				case EXPR_LESS:
					cnstval._sint = lhs->_u._symbol->_u._cnstval._uint < rhs->_u._symbol->_u._cnstval._uint; break;
				case EXPR_LESSEQ:
					cnstval._sint = lhs->_u._symbol->_u._cnstval._uint <= rhs->_u._symbol->_u._cnstval._uint; break;
				case EXPR_GREAT:
					cnstval._sint = lhs->_u._symbol->_u._cnstval._uint > rhs->_u._symbol->_u._cnstval._uint; break;
				case EXPR_GREATEQ:
					cnstval._sint = lhs->_u._symbol->_u._cnstval._uint >= rhs->_u._symbol->_u._cnstval._uint; break;
				default:
					assert(0);
					break;
				}
			}
			else if (UnQual(ty)->_op == Type_Float) {
				switch (op)
				{
				case EXPR_LESS:
					cnstval._sint = lhs->_u._symbol->_u._cnstval._float < rhs->_u._symbol->_u._cnstval._float; break;
				case EXPR_LESSEQ:
					cnstval._sint = lhs->_u._symbol->_u._cnstval._float <= rhs->_u._symbol->_u._cnstval._float; break;
				case EXPR_GREAT:
					cnstval._sint = lhs->_u._symbol->_u._cnstval._float > rhs->_u._symbol->_u._cnstval._float; break;
				case EXPR_GREATEQ:
					cnstval._sint = lhs->_u._symbol->_u._cnstval._float >= rhs->_u._symbol->_u._cnstval._float; break;
				default:
					assert(0);
					break;
				}
			}

			return cc_expr_constant(ctx, gbuiltintypes._sinttype, cnstval, loc, where);
		}

	}
	else if ((IsPtr(lhs->_ty) || IsArray(lhs->_ty))
		&& (IsPtr(rhs->_ty) || IsArray(rhs->_ty))
		&& cc_type_isequal(lhs->_ty->_type, rhs->_ty->_type, TRUE))
	{
		/* constant folding */
		if (lhs->_op == EXPR_CONSTANT && !lhs->_islvalue && rhs->_op == EXPR_CONSTANT && !rhs->_islvalue)
		{
			FCCConstVal cnstval;

			switch (op)
			{
			case EXPR_LESS:
				cnstval._sint = lhs->_u._symbol->_u._cnstval._pointer < rhs->_u._symbol->_u._cnstval._pointer; break;
			case EXPR_LESSEQ:
				cnstval._sint = lhs->_u._symbol->_u._cnstval._pointer <= rhs->_u._symbol->_u._cnstval._pointer; break;
			case EXPR_GREAT:
				cnstval._sint = lhs->_u._symbol->_u._cnstval._pointer > rhs->_u._symbol->_u._cnstval._pointer; break;
			case EXPR_GREATEQ:
				cnstval._sint = lhs->_u._symbol->_u._cnstval._pointer >= rhs->_u._symbol->_u._cnstval._pointer; break;
			default:
				assert(0);
				break;
			}

			return cc_expr_constant(ctx, gbuiltintypes._sinttype, cnstval, loc, where);
		}
	}
	else {
		logger_output_s("error: invalid operand for '%s' at %w\n", cc_expr_opname(op), &loc);
		return NULL;
	}

	if (!(tree = cc_expr_new(where))) {
		return FALSE;
	}

	tree->_op = op;
	tree->_loc = loc;
	tree->_ty = gbuiltintypes._sinttype;
	tree->_u._kids[0] = lhs;
	tree->_u._kids[1] = rhs;

	return tree;
}

static BOOL cc_expr_parse_relational(FCCContext* ctx, FCCIRTree** outexpr, enum EMMArea where)
{
	FCCIRTree* lhs, * rhs, * tree;
	FLocation loc;

	if (!cc_expr_parse_shift(ctx, &lhs, where))
	{
		return FALSE;
	}

	tree = lhs;
	while (ctx->_currtk._type == TK_LESS || ctx->_currtk._type == TK_LESS_EQ
		|| ctx->_currtk._type == TK_GREAT || ctx->_currtk._type == TK_GREAT_EQ)
	{
		enum ECCToken tktype = ctx->_currtk._type;
		enum EExprOp op;

		loc = ctx->_currtk._loc;
		cc_read_token(ctx, &ctx->_currtk);
		if (!cc_expr_parse_shift(ctx, &rhs, where)) {
			return FALSE;
		}

		switch (tktype)
		{
		case TK_LESS: op = EXPR_LESS; break;
		case TK_LESS_EQ: op = EXPR_LESSEQ; break;
		case TK_GREAT: op = EXPR_GREAT; break;
		case TK_GREAT_EQ: op = EXPR_GREATEQ; break;
		}

		if (!(tree = cc_expr_relational(ctx, op, lhs, rhs, loc, where))) {
			return FALSE;
		}

		lhs = tree;
	} /* end while */

	*outexpr = tree;
	return TRUE;
}

static FCCIRTree* cc_expr_equality(FCCContext* ctx, int op, FCCIRTree* lhs, FCCIRTree* rhs, FLocation loc, enum EMMArea where)
{
	FCCIRTree* tree;

	if (IsArith(lhs->_ty) && IsArith(rhs->_ty))
	{
		FCCType* ty;

		ty = cc_type_select(lhs->_ty, rhs->_ty);
		lhs = cc_expr_cast(ctx, ty, lhs, where);
		rhs = cc_expr_cast(ctx, ty, rhs, where);

		/* do constant fold */
		if (lhs->_op == EXPR_CONSTANT && rhs->_op == EXPR_CONSTANT)
		{
			FCCConstVal cnstval;

			if (UnQual(ty)->_op == Type_SInteger) {
				switch (op)
				{
				case EXPR_EQ:
					cnstval._sint = lhs->_u._symbol->_u._cnstval._sint == rhs->_u._symbol->_u._cnstval._sint; break;
				case EXPR_UNEQ:
					cnstval._sint = lhs->_u._symbol->_u._cnstval._sint != rhs->_u._symbol->_u._cnstval._sint; break;
				default:
					assert(0);
					break;
				}
			}
			else if (UnQual(ty)->_op == Type_UInteger) {
				switch (op)
				{
				case EXPR_EQ:
					cnstval._sint = lhs->_u._symbol->_u._cnstval._uint == rhs->_u._symbol->_u._cnstval._uint; break;
				case EXPR_UNEQ:
					cnstval._sint = lhs->_u._symbol->_u._cnstval._uint != rhs->_u._symbol->_u._cnstval._uint; break;
				default:
					assert(0);
					break;
				}
			}
			else if (UnQual(ty)->_op == Type_Float) {
				switch (op)
				{
				case EXPR_EQ:
					cnstval._sint = lhs->_u._symbol->_u._cnstval._float == rhs->_u._symbol->_u._cnstval._float; break;
				case EXPR_UNEQ:
					cnstval._sint = lhs->_u._symbol->_u._cnstval._float != rhs->_u._symbol->_u._cnstval._float; break;
				default:
					assert(0);
					break;
				}
			}

			return cc_expr_constant(ctx, gbuiltintypes._sinttype, cnstval, loc, where);
		}

	}
	else if ((IsPtr(lhs->_ty) || IsArray(lhs->_ty) || cc_expr_is_nullptr(lhs))
		&& (IsPtr(rhs->_ty) || IsArray(rhs->_ty) || cc_expr_is_nullptr(lhs))
		&& (cc_type_isequal(lhs->_ty->_type, rhs->_ty->_type, TRUE) || IsVoidptr(lhs->_ty) || IsVoidptr(rhs->_ty)))
	{
		/* constant folding */
		if (lhs->_op == EXPR_CONSTANT && !lhs->_islvalue && rhs->_op == EXPR_CONSTANT && !rhs->_islvalue)
		{
			FCCConstVal cnstval;

			switch (op)
			{
			case EXPR_EQ:
				cnstval._sint = lhs->_u._symbol->_u._cnstval._pointer == rhs->_u._symbol->_u._cnstval._pointer; break;
			case EXPR_UNEQ:
				cnstval._sint = lhs->_u._symbol->_u._cnstval._pointer != rhs->_u._symbol->_u._cnstval._pointer; break;
			default:
				assert(0);
				break;
			}

			return cc_expr_constant(ctx, gbuiltintypes._sinttype, cnstval, loc, where);
		}
	}
	else {
		logger_output_s("error: invalid operand for '%s' at %w\n", cc_expr_opname(op), &loc);
		return NULL;
	}

	if (!(tree = cc_expr_new(where))) {
		return FALSE;
	}

	tree->_op = op;
	tree->_loc = loc;
	tree->_ty = gbuiltintypes._sinttype;
	tree->_u._kids[0] = lhs;
	tree->_u._kids[1] = rhs;

	return tree;
}

static BOOL cc_expr_parse_equality(FCCContext* ctx, FCCIRTree** outexpr, enum EMMArea where)
{
	FCCIRTree* lhs, * rhs, * tree;
	FLocation loc;


	if (!cc_expr_parse_relational(ctx, &lhs, where))
	{
		return FALSE;
	}

	tree = lhs;
	while (ctx->_currtk._type == TK_EQUAL || ctx->_currtk._type == TK_UNEQUAL)
	{

		enum ECCToken tktype = ctx->_currtk._type;

		loc = ctx->_currtk._loc;
		cc_read_token(ctx, &ctx->_currtk);
		if (!cc_expr_parse_relational(ctx, &rhs, where)) {
			return FALSE;
		}

		if (!(tree = cc_expr_equality(ctx, (tktype == TK_EQUAL) ? EXPR_EQ : EXPR_UNEQ, lhs, rhs, loc, where))) {
			return FALSE;
		}

		lhs = tree;
	} /* end while */

	*outexpr = tree;
	return TRUE;
}

static FCCIRTree* cc_expr_bitand(FCCContext* ctx, int op, FCCIRTree* lhs, FCCIRTree* rhs, FLocation loc, enum EMMArea where)
{
	FCCIRTree* tree;
	FCCType* ty;

	if (!IsInt(lhs->_ty) || !IsInt(rhs->_ty)) {
		logger_output_s("error: integer operand is expected for '%s' at %w\n", cc_expr_opname(op), &loc);
		return NULL;
	}

	ty = cc_type_promote(cc_type_select(lhs->_ty, rhs->_ty));
	lhs = cc_expr_cast(ctx, ty, lhs, where);
	rhs = cc_expr_cast(ctx, ty, rhs, where);

	/* do constant fold */
	if (lhs->_op == EXPR_CONSTANT && rhs->_op == EXPR_CONSTANT)
	{
		FCCConstVal cnstval;

		if (UnQual(ty)->_op == Type_SInteger) {
			cnstval._sint = lhs->_u._symbol->_u._cnstval._sint & rhs->_u._symbol->_u._cnstval._sint;
		}
		else if (UnQual(ty)->_op == Type_UInteger) {
			cnstval._uint = lhs->_u._symbol->_u._cnstval._uint & rhs->_u._symbol->_u._cnstval._uint;
		}

		return cc_expr_constant(ctx, ty, cnstval, loc, where);
	}

	if (!(tree = cc_expr_new(where))) {
		return FALSE;
	}

	tree->_op = op;
	tree->_loc = loc;
	tree->_ty = ty;
	tree->_u._kids[0] = lhs;
	tree->_u._kids[1] = rhs;

	return tree;
}

static BOOL cc_expr_parse_bitand(FCCContext* ctx, FCCIRTree** outexpr, enum EMMArea where)
{
	FCCIRTree* lhs, * rhs, * tree;
	FLocation loc;

	if (!cc_expr_parse_equality(ctx, &lhs, where))
	{
		return FALSE;
	}

	tree = lhs;
	while (ctx->_currtk._type == TK_BITAND)
	{
		loc = ctx->_currtk._loc;
		cc_read_token(ctx, &ctx->_currtk);
		if (!cc_expr_parse_equality(ctx, &rhs, where)) {
			return FALSE;
		}

		if (!(tree = cc_expr_bitand(ctx, EXPR_BITAND, lhs, rhs, loc, where))) {
			return FALSE;
		}

		lhs = tree;
	} /* end while */

	*outexpr = tree;
	return TRUE;
}

static FCCIRTree* cc_expr_bitxor(FCCContext* ctx, int op, FCCIRTree* lhs, FCCIRTree* rhs, FLocation loc, enum EMMArea where)
{
	FCCIRTree* tree;
	FCCType* ty;

	if (!IsInt(lhs->_ty) || !IsInt(rhs->_ty)) {
		logger_output_s("error: integer operand is expected for '%s' at %w\n", cc_expr_opname(op), &loc);
		return NULL;
	}

	ty = cc_type_promote(cc_type_select(lhs->_ty, rhs->_ty));
	lhs = cc_expr_cast(ctx, ty, lhs, where);
	rhs = cc_expr_cast(ctx, ty, rhs, where);

	/* do constant fold */
	if (lhs->_op == EXPR_CONSTANT && rhs->_op == EXPR_CONSTANT)
	{
		FCCConstVal cnstval;

		if (UnQual(ty)->_op == Type_SInteger) {
			cnstval._sint = lhs->_u._symbol->_u._cnstval._sint ^ rhs->_u._symbol->_u._cnstval._sint;
		}
		else if (UnQual(ty)->_op == Type_UInteger) {
			cnstval._uint = lhs->_u._symbol->_u._cnstval._uint ^ rhs->_u._symbol->_u._cnstval._uint;
		}

		return cc_expr_constant(ctx, ty, cnstval, loc, where);
	}

	if (!(tree = cc_expr_new(where))) {
		return FALSE;
	}

	tree->_op = op;
	tree->_loc = loc;
	tree->_ty = ty;
	tree->_u._kids[0] = lhs;
	tree->_u._kids[1] = rhs;

	return tree;
}

static BOOL cc_expr_parse_bitxor(FCCContext* ctx, FCCIRTree** outexpr, enum EMMArea where)
{
	FCCIRTree* lhs, * rhs, * tree;
	FLocation loc;

	if (!cc_expr_parse_bitand(ctx, &lhs, where))
	{
		return FALSE;
	}

	tree = lhs;
	while (ctx->_currtk._type == TK_BITXOR)
	{
		loc = ctx->_currtk._loc;
		cc_read_token(ctx, &ctx->_currtk);
		if (!cc_expr_parse_bitand(ctx, &rhs, where)) {
			return FALSE;
		}

		if (!(tree = cc_expr_bitxor(ctx, EXPR_BITXOR, lhs, rhs, loc, where))) {
			return FALSE;
		}

		lhs = tree;
	} /* end while */

	*outexpr = tree;
	return TRUE;
}

static FCCIRTree* cc_expr_bitor(FCCContext* ctx, int op, FCCIRTree* lhs, FCCIRTree* rhs, FLocation loc, enum EMMArea where)
{
	FCCIRTree* tree;
	FCCType* ty;

	if (!IsInt(lhs->_ty) || !IsInt(rhs->_ty)) {
		logger_output_s("error: integer operand is expected for '%s' at %w\n", cc_expr_opname(op), &loc);
		return NULL;
	}

	ty = cc_type_promote(cc_type_select(lhs->_ty, rhs->_ty));
	lhs = cc_expr_cast(ctx, ty, lhs, where);
	rhs = cc_expr_cast(ctx, ty, rhs, where);

	/* do constant fold */
	if (lhs->_op == EXPR_CONSTANT && rhs->_op == EXPR_CONSTANT)
	{
		FCCConstVal cnstval;

		if (UnQual(ty)->_op == Type_SInteger) {
			cnstval._sint = lhs->_u._symbol->_u._cnstval._sint | rhs->_u._symbol->_u._cnstval._sint;
		}
		else if (UnQual(ty)->_op == Type_UInteger) {
			cnstval._uint = lhs->_u._symbol->_u._cnstval._uint | rhs->_u._symbol->_u._cnstval._uint;
		}

		return cc_expr_constant(ctx, ty, cnstval, loc, where);
	}

	if (!(tree = cc_expr_new(where))) {
		return FALSE;
	}

	tree->_op = op;
	tree->_loc = loc;
	tree->_ty = ty;
	tree->_u._kids[0] = lhs;
	tree->_u._kids[1] = rhs;

	return tree;
}

static BOOL cc_expr_parse_bitor(FCCContext* ctx, FCCIRTree** outexpr, enum EMMArea where)
{
	FCCIRTree* lhs, * rhs, * tree;
	FLocation loc;

	if (!cc_expr_parse_bitxor(ctx, &lhs, where))
	{
		return FALSE;
	}

	tree = lhs;
	while (ctx->_currtk._type == TK_BITOR)
	{
		loc = ctx->_currtk._loc;
		cc_read_token(ctx, &ctx->_currtk);
		if (!cc_expr_parse_bitxor(ctx, &rhs, where)) {
			return FALSE;
		}

		if (!(tree = cc_expr_bitor(ctx, EXPR_BITOR, lhs, rhs, loc, where))) {
			return FALSE;
		}

		lhs = tree;
	} /* end while */

	*outexpr = tree;
	return TRUE;
}

static FCCIRTree* cc_expr_logicand(FCCContext* ctx, int op, FCCIRTree* lhs, FCCIRTree* rhs, FLocation loc, enum EMMArea where)
{
	FCCIRTree* tree;

	if (!IsScalar(lhs->_ty) || !IsScalar(rhs->_ty))
	{
		logger_output_s("error scalar type is expected for '&&' at %w.\n", &loc);
		return NULL;
	}

	if (lhs->_op == EXPR_CONSTANT && rhs->_op == EXPR_CONSTANT)
	{
		FCCConstVal cnstval;
		int lv = 0, rv = 0;

		if (UnQual(lhs->_ty)->_op == Type_SInteger) {
			lv = lhs->_u._symbol->_u._cnstval._sint != 0;
		}
		else if (UnQual(lhs->_ty)->_op == Type_UInteger) {
			lv = lhs->_u._symbol->_u._cnstval._uint != 0;
		}
		else if (UnQual(lhs->_ty)->_op == Type_Float) {
			lv = lhs->_u._symbol->_u._cnstval._float != 0.0;
		}
		else if (UnQual(lhs->_ty)->_op == Type_Pointer) {
			lv = lhs->_u._symbol->_u._cnstval._pointer == 0;
		}

		if (UnQual(rhs->_ty)->_op == Type_SInteger) {
			rv = rhs->_u._symbol->_u._cnstval._sint != 0;
		}
		else if (UnQual(rhs->_ty)->_op == Type_UInteger) {
			rv = rhs->_u._symbol->_u._cnstval._uint != 0;
		}
		else if (UnQual(rhs->_ty)->_op == Type_Float) {
			rv = rhs->_u._symbol->_u._cnstval._float != 0.0;
		}
		else if (UnQual(rhs->_ty)->_op == Type_Pointer) {
			rv = rhs->_u._symbol->_u._cnstval._pointer == 0;
		}

		cnstval._sint = lv && rv;
		return cc_expr_constant(ctx, gbuiltintypes._sinttype, cnstval, loc, where);
	}


	if (!(tree = cc_expr_new(where))) {
		return FALSE;
	}

	tree->_op = op;
	tree->_loc = loc;
	tree->_ty = gbuiltintypes._sinttype;
	tree->_u._kids[0] = lhs;
	tree->_u._kids[1] = rhs;

	return tree;
}

static BOOL cc_expr_parse_logicand(FCCContext* ctx, FCCIRTree** outexpr, enum EMMArea where)
{
	FCCIRTree* lhs, * rhs, * tree;
	FLocation loc;

	if (!cc_expr_parse_bitor(ctx, &lhs, where))
	{
		return FALSE;
	}

	tree = lhs;
	while (ctx->_currtk._type == TK_AND)
	{
		loc = ctx->_currtk._loc;
		cc_read_token(ctx, &ctx->_currtk);
		if (!cc_expr_parse_bitor(ctx, &rhs, where)) {
			return FALSE;
		}

		if (!(tree = cc_expr_logicand(ctx, EXPR_LOGAND, lhs, rhs, loc, where))) {
			return FALSE;
		}

		lhs = tree;
	} /* end while */

	*outexpr = tree;
	return TRUE;
}

static FCCIRTree* cc_expr_logicor(FCCContext* ctx, int op, FCCIRTree* lhs, FCCIRTree* rhs, FLocation loc, enum EMMArea where)
{
	FCCIRTree* tree;

	if (!IsScalar(lhs->_ty) || !IsScalar(rhs->_ty))
	{
		logger_output_s("error scalar type is expected for '||' at %w.\n", &loc);
		return NULL;
	}

	if (lhs->_op == EXPR_CONSTANT && rhs->_op == EXPR_CONSTANT)
	{
		FCCConstVal cnstval;
		int lv = 0, rv = 0;

		if (UnQual(lhs->_ty)->_op == Type_SInteger) {
			lv = lhs->_u._symbol->_u._cnstval._sint != 0;
		}
		else if (UnQual(lhs->_ty)->_op == Type_UInteger) {
			lv = lhs->_u._symbol->_u._cnstval._uint != 0;
		}
		else if (UnQual(lhs->_ty)->_op == Type_Float) {
			lv = lhs->_u._symbol->_u._cnstval._float != 0.0;
		}
		else if (UnQual(lhs->_ty)->_op == Type_Pointer) {
			lv = lhs->_u._symbol->_u._cnstval._pointer == 0;
		}

		if (UnQual(rhs->_ty)->_op == Type_SInteger) {
			rv = rhs->_u._symbol->_u._cnstval._sint != 0;
		}
		else if (UnQual(rhs->_ty)->_op == Type_UInteger) {
			rv = rhs->_u._symbol->_u._cnstval._uint != 0;
		}
		else if (UnQual(rhs->_ty)->_op == Type_Float) {
			rv = rhs->_u._symbol->_u._cnstval._float != 0.0;
		}
		else if (UnQual(rhs->_ty)->_op == Type_Pointer) {
			rv = rhs->_u._symbol->_u._cnstval._pointer == 0;
		}

		cnstval._sint = lv || rv;
		return cc_expr_constant(ctx, gbuiltintypes._sinttype, cnstval, loc, where);
	}

	if (!(tree = cc_expr_new(where))) {
		return FALSE;
	}

	tree->_op = op;
	tree->_loc = loc;
	tree->_ty = gbuiltintypes._sinttype;
	tree->_u._kids[0] = lhs;
	tree->_u._kids[1] = rhs;

	return tree;
}

static BOOL cc_expr_parse_logicor(FCCContext* ctx, FCCIRTree** outexpr, enum EMMArea where)
{
	FCCIRTree* lhs, * rhs, * tree;
	FLocation loc;

	if (!cc_expr_parse_logicand(ctx, &lhs, where))
	{
		return FALSE;
	}

	tree = lhs;
	while (ctx->_currtk._type == TK_OR)
	{
		loc = ctx->_currtk._loc;
		cc_read_token(ctx, &ctx->_currtk);
		if (!cc_expr_parse_logicand(ctx, &rhs, where)) {
			return FALSE;
		}

		if (!(tree = cc_expr_logicor(ctx, EXPR_LOGOR, lhs, rhs, loc, where))) {
			return FALSE;
		}

		lhs = tree;
	} /* end while */

	*outexpr = tree;
	return TRUE;
}

static FCCIRTree* cc_expr_conditional(FCCContext* ctx, int op, FCCIRTree* expr0, FCCIRTree* expr1, FCCIRTree* expr2, FLocation loc, enum EMMArea where)
{
	FCCIRTree* tree;
	FCCType* ty;

	if (!IsScalar(expr0->_ty))
	{
		logger_output_s("error scalar type is expected for '? : ' first expression at %w.\n", &loc);
		return NULL;
	}

	if (cc_type_isequal(expr1->_ty, expr2->_ty, TRUE)) {
		ty = expr1->_ty;
	}
	else if ((IsPtr(expr1->_ty) || IsArray(expr1->_ty)) && cc_expr_is_nullptr(expr2))
	{
		ty = expr1->_ty;
	}
	else if (cc_expr_is_nullptr(expr1) && (IsPtr(expr2->_ty) || IsArray(expr2->_ty)))
	{
		ty = expr2->_ty;
	}
	else
	{
		logger_output_s("error same type is expected for '? : ' 2rd & 3rd expression at %w.\n", &loc);
		return NULL;
	}

	if (expr0->_op == EXPR_CONSTANT)
	{
		int val = 1;

		if (UnQual(expr0->_ty)->_op == Type_SInteger) {
			val = expr0->_u._symbol->_u._cnstval._sint != 0;
		}
		else if (UnQual(expr0->_ty)->_op == Type_UInteger) {
			val = expr0->_u._symbol->_u._cnstval._uint != 0;
		}
		else if (UnQual(expr0->_ty)->_op == Type_Float) {
			val = expr0->_u._symbol->_u._cnstval._float != 0.0;
		}
		else if (UnQual(expr0->_ty)->_op == Type_Pointer) {
			val = expr0->_u._symbol->_u._cnstval._pointer == 0;
		}

		return val ? expr1 : expr2;
	}

	if (!(tree = cc_expr_new(where))) {
		return FALSE;
	}

	tree->_op = op;
	tree->_loc = loc;
	tree->_u._kids[0] = expr0;
	tree->_u._kids[1] = expr1;
	tree->_u._kids[2] = expr2;

	return tree;
}

static BOOL cc_expr_parse_conditional(FCCContext* ctx, FCCIRTree** outexpr, enum EMMArea where)
{
	FCCIRTree* tree, * expr0, * expr1, * expr2;
	FLocation loc;

	if (!cc_expr_parse_logicor(ctx, &expr0, where))
	{
		return FALSE;
	}
	tree = expr0;

	if (ctx->_currtk._type == TK_QUESTION) /* '?' */
	{
		loc = ctx->_currtk._loc;
		cc_read_token(ctx, &ctx->_currtk);

		if (!cc_expr_parse_expression(ctx, &expr1, where))
		{
			return FALSE;
		}

		if (!cc_parser_expect(ctx, TK_COLON)) /* ':' */
		{
			return FALSE;
		}

		if (!cc_expr_parse_conditional(ctx, &expr2, where))
		{
			return FALSE;
		}

		if (!(tree = cc_expr_conditional(ctx, EXPR_COND, expr0, expr1, expr2, loc, where))) {
			return FALSE;
		}
	}

	*outexpr = tree;
	return TRUE;
}

static FCCIRTree* cc_expr_assignment(FCCContext* ctx, int op, FCCIRTree* lhs, FCCIRTree* rhs, FLocation loc, enum EMMArea where)
{
	FCCIRTree* tree;

	if (!cc_expr_is_modifiable(lhs)) {
		logger_output_s("error l-value is expected for operator '%s' at %w\n", cc_expr_opname(op), &loc);
		return NULL;
	}

	switch (op)
	{
	case EXPR_ASSIGN:
	{
		if (IsArith(lhs->_ty) && IsArith(rhs->_ty))
		{
			rhs = cc_expr_cast(ctx, UnQual(rhs->_ty), rhs, where);
		}
		else if (IsPtr(lhs->_ty)) {
			if (IsPtr(rhs->_ty)) {
				if (!cc_type_isequal(lhs->_ty, rhs->_ty, TRUE) && (!IsVoidptr(lhs->_ty) && !IsVoidptr(rhs->_ty))) {
					logger_output_s("error incompatible operands for operator '%s' at %w\n", cc_expr_opname(op), &loc);
					return NULL;
				}
			}
			else if (!cc_expr_is_nullptr(rhs)) {
				logger_output_s("error incompatible operands for operator '%s' at %w\n", cc_expr_opname(op), &loc);
				return NULL;
			}
		}
		else if (!cc_type_isequal(UnQual(lhs->_ty), UnQual(rhs->_ty), TRUE)) {
			logger_output_s("error incompatible operands for operator '%s' at %w\n", cc_expr_opname(op), &loc);
			return NULL;
		}
	}
	break;
	case EXPR_ASSIGN_MUL:
	case EXPR_ASSIGN_DIV:
	{
		if (!IsArith(lhs->_ty) || !IsArith(rhs->_ty))
		{
			logger_output_s("error arithmetic operands is expected for '%s', at %w\n", cc_expr_opname(op), &loc);
			return NULL;
		}

		rhs = cc_expr_cast(ctx, lhs->_ty, rhs, where);
	}
	break;
	case EXPR_ASSIGN_MOD:
	{
		if (!IsInt(lhs->_ty) || !IsInt(rhs->_ty))
		{
			logger_output_s("error arithmetic operands is expected for '%s', at %w\n", cc_expr_opname(op), &loc);
			return NULL;
		}

		rhs = cc_expr_cast(ctx, lhs->_ty, rhs, where);
	}
	break;
	case EXPR_ASSIGN_ADD:
	case EXPR_ASSIGN_SUB:
	{
		if (IsArith(lhs->_ty) && IsArith(rhs->_ty))
		{
			rhs = cc_expr_cast(ctx, lhs->_ty, rhs, where);
		}
		else if (IsPtr(lhs->_ty) && IsInt(rhs->_ty))
		{
			if (lhs->_ty->_type->_size == 0 && !IsVoid(lhs->_ty->_type)) {
				logger_output_s("error: unknown %t size at %w\n", lhs->_ty->_type, &loc);
				return NULL;
			}

			rhs = cc_expr_cast(ctx, gbuiltintypes._sinttype, rhs, where);
		}
		else
		{
			logger_output_s("error: invalid operand for '%s' at %w\n", cc_expr_opname(op), &loc);
			return NULL;
		}
	}
	break;
	case EXPR_ASSIGN_LSHIFT:
	case EXPR_ASSIGN_RSHIFT:
	case EXPR_ASSIGN_BITAND:
	case EXPR_ASSIGN_BITXOR:
	case EXPR_ASSIGN_BITOR:
	{
		if (IsInt(lhs->_ty) && IsInt(rhs->_ty))
		{
			rhs = cc_expr_cast(ctx, gbuiltintypes._sinttype, rhs, where);
		}
		else
		{
			logger_output_s("error: invalid operand for '%s' at %w\n", cc_expr_opname(op), &loc);
			return NULL;
		}
	}
	break;
	default:
		assert(0); break;
	}

	if (!(tree = cc_expr_new(where))) {
		return FALSE;
	}

	tree->_op = op;
	tree->_loc = loc;
	tree->_ty = lhs->_ty;
	tree->_u._kids[0] = lhs;
	tree->_u._kids[1] = rhs;

	return tree;
}

BOOL cc_expr_parse_assignment(FCCContext* ctx, FCCIRTree** outexpr, enum EMMArea where)
{
	FCCIRTree* tree, * lhs, * rhs;
	FLocation loc;

	if (!cc_expr_parse_conditional(ctx, &lhs, where)) {
		return FALSE;
	}

	tree = lhs;
	if (cc_parser_is_assign(ctx->_currtk._type))
	{
		enum EExprOp op;

		loc = ctx->_currtk._loc;
		switch (ctx->_currtk._type)
		{
		case TK_ASSIGN: op = EXPR_ASSIGN; break;
		case TK_MUL_ASSIGN: op = EXPR_ASSIGN_MUL; break;
		case TK_DIV_ASSIGN: op = EXPR_ASSIGN_DIV; break;
		case TK_MOD_ASSIGN: op = EXPR_ASSIGN_MOD; break;
		case TK_ADD_ASSIGN: op = EXPR_ASSIGN_ADD; break;
		case TK_SUB_ASSIGN: op = EXPR_ASSIGN_SUB; break;
		case TK_LSHIFT_ASSIGN: op = EXPR_ASSIGN_LSHIFT; break;
		case TK_RSHIFT_ASSIGN: op = EXPR_ASSIGN_RSHIFT; break;
		case TK_BITAND_ASSIGN: op = EXPR_ASSIGN_BITAND; break;
		case TK_BITXOR_ASSIGN: op = EXPR_ASSIGN_BITXOR; break;
		case TK_BITOR_ASSIGN: op = EXPR_ASSIGN_BITOR; break;
		default: assert(0); break;
		}

		if (!cc_expr_parse_assignment(ctx, &rhs, where)) {
			return FALSE;
		}

		if (!(tree = cc_expr_assignment(ctx, op, lhs, rhs, loc, where))) {
			return FALSE;
		}

	}

	*outexpr = tree;
	return TRUE;
}

static FCCIRTree* cc_expr_comma(FCCContext* ctx, int op, FCCIRTree* lhs, FCCIRTree* rhs, FLocation loc, enum EMMArea where)
{
	FCCIRTree* tree;

	if (lhs->_op == EXPR_CONSTANT && rhs->_op == EXPR_CONSTANT)
	{
		return rhs;
	}

	if (!(tree = cc_expr_new(where))) {
		return FALSE;
	}

	tree->_op = op;
	tree->_loc = loc;
	tree->_ty = rhs->_ty;
	tree->_u._kids[0] = lhs;
	tree->_u._kids[1] = rhs;

	return tree;
}

BOOL cc_expr_parse_expression(FCCContext* ctx, FCCIRTree** outexpr, enum EMMArea where)
{
	FCCIRTree* lhs, * rhs, * tree;
	FLocation loc;

	if (!cc_expr_parse_assignment(ctx, &lhs, where))
	{
		return FALSE;
	}

	tree = lhs;
	while (ctx->_currtk._type == TK_COMMA) /* ',' */
	{
		loc = ctx->_currtk._loc;
		cc_read_token(ctx, &ctx->_currtk);
		if (!cc_expr_parse_assignment(ctx, &rhs, where)) {
			return FALSE;
		}

		if (!(tree = cc_expr_comma(ctx, EXPR_COMMA, lhs, rhs, loc, where))) {
			return FALSE;
		}

		lhs = tree;
	} /* end while */

	*outexpr = tree;
	return TRUE;
}

BOOL cc_expr_parse_constant_expression(FCCContext* ctx, FCCIRTree** outexpr, enum EMMArea where)
{
	if (cc_expr_parse_conditional(ctx, outexpr, where) && (*outexpr)->_isconstant)
	{
		return TRUE;
	}

	logger_output_s("error: constant expression is expected at %w\n", &((*outexpr)->_loc));
	return FALSE;
}

BOOL cc_expr_parse_constant_int(FCCContext* ctx, int* val)
{
	FCCIRTree* expr;

	*val = 0;
	if (!cc_expr_parse_constant_expression(ctx, &expr, CC_MM_TEMPPOOL) || expr == NULL) {
		return FALSE;
	}

	if (IR_OP(expr->_op) == IR_CONST)
	{
		FCCType* ty = expr->_ty;
		if (ty->_op == Type_SInteger)
		{
			*val = (int)expr->_u._val._sint;
			return TRUE;
		}
		if (ty->_op == Type_UInteger)
		{
			*val = (int)expr->_u._val._uint;
			return TRUE;
		}
	}

	logger_output_s("error: integral constant is expected. at %w.\n", &expr->_loc);
	return FALSE;
}

/* ----------------------------------------------------------------------------------- */
FCCType* cc_expr_assigntype(FCCType* lhs, FCCIRTree* expr)
{
	FCCType* xty, * yty;

	xty = UnQual(lhs);
	yty = UnQual(expr->_ty);

	if (IsEnum(xty)) {
		xty = xty->_type;
	}
	if (IsEnum(yty)) {
		yty = yty->_type;
	}

	if (xty->_size == 0 || yty->_size == 0) {
		return NULL;
	}

	if (IsArith(xty) && IsArith(yty) || IsStruct(xty) && xty == yty) {
		return xty;
	}

	if (IsPtr(xty) && cc_expr_is_nullptr(expr)) {
		return xty;
	}

	if ((IsVoidptr(xty) && IsPtr(yty) || IsPtr(xty) && IsVoidptr(yty)) &&
		((IsConst(xty->_type) || !IsConst(yty->_type)) && (IsVolatile(xty->_type) || !IsVolatile(yty->_type)))
		)
	{
		return xty;
	}

	if ((IsPtr(xty) && IsPtr(yty)
		&& cc_type_isequal(UnQual(xty->_type), UnQual(yty->_type), TRUE))
		&& ((IsConst(xty->_type) || !IsConst(yty->_type))
			&& (IsVolatile(xty->_type) || !IsVolatile(yty->_type))))
	{
		return xty;
	}

	if (IsPtr(xty) && IsPtr(yty)
		&& ((IsConst(xty->_type) || !IsConst(yty->_type))
			&& (IsVolatile(xty->_type) || !IsVolatile(yty->_type))))
	{
		FCCType* lty = UnQual(xty->_type);
		FCCType* rty = UnQual(yty->_type);
		if (IsEnum(lty) && IsInt(rty)
			|| IsEnum(rty) && IsInt(lty))
		{
			logger_output_s("assignment between `%t' and `%t' is compiler-dependent\n", xty, yty);
			return xty;
		}
	}

	return NULL;
}

BOOL cc_expr_is_modifiable(FCCIRTree* expr)
{
	/* modifiable:
	  1. l-value
	  2. not const
	  3. not array type
	  4. not function
	  5. for union/struct, there is not any const field.
	*/
	return expr->_islvalue
		&& !IsConst(expr->_ty)
		&& !IsArray(expr->_ty)
		&& !IsFunction(expr->_ty)
		&& (IsStruct(expr->_ty) ? UnQual(expr->_ty)->_u._symbol->_u._s._cfields : TRUE);
}

BOOL cc_expr_is_nullptr(FCCIRTree* expr)
{
	FCCType* ty = UnQual(expr->_ty);

	return (IR_OP(expr->_op) == IR_CONST)
		&& ((ty->_op == Type_SInteger && expr->_u._val._sint == 0)
			|| (ty->_op == Type_UInteger && expr->_u._val._uint == 0)
			|| (IsVoidptr(ty) && expr->_u._val._pointer == 0));
}

/* ----------------------------------------------------------------------------------- */
FCCIRTree* cc_expr_change_rettype(FCCIRTree* expr, FCCType* newty, enum EMMArea where)
{
	FCCIRTree* tree;
	int tycode;

	if (cc_type_isequal(expr->_ty, newty, TRUE)) {
		return expr;
	}

	tycode = cc_ir_typecode(newty);
	if (!(tree = cc_expr_new(where))) {
		return NULL;
	}

	*tree = *expr;
	tree->_op = IR_MKOP2(IR_OP(tree->_op), tycode, IR_OP1(tree->_op));
	tree->_ty = newty;
	return tree;
}

FCCIRTree* cc_expr_adjust(FCCIRTree* expr, enum EMMArea where)
{
	if (expr->_op == IR_MKOP1(IR_CONST, IR_STRA)
		|| expr->_op == IR_MKOP1(IR_CONST, IR_STRW))
	{
		FCCSymbol* p;
		
		if (!(p = cc_symbol_constant(expr->_ty, expr->_u._val)))
		{
			logger_output_s("error install constant symbol failed at %s:%d\n", __FILE__, __LINE__);
			return NULL;
		}

		if (!(expr = cc_expr_id(p, NULL, where)))
		{
			return NULL;
		}

		expr = cc_expr_addr(expr, NULL, where);
	} else if (IsArray(expr->_ty)) {
		expr = cc_expr_change_rettype(expr, cc_type_arraytoptr(expr->_ty), where);
	} else if (IsFunction(expr->_ty)) {
		expr = cc_expr_change_rettype(expr, cc_type_ptr(expr->_ty), where);
	}

	return expr;
}

FCCIRTree* cc_expr_id(FCCSymbol* p, FLocation* loc, enum EMMArea where)
{
	int op;
	FCCIRTree* tree;
	FCCType* ty = UnQual(p->_type);

	if (p->_scope <= SCOPE_GLOBAL) {
		op = IR_ADDRG;
	}
	else if (p->_scope == SCOPE_PARAM) {
		op = IR_ADDRF;
	}
	else if (p->_sclass == SC_External || p->_sclass == SC_Static) {
		assert(p->_u._alias);
		p = p->_u._alias;
		op = IR_ADDRG;
	}
	else {
		op = IR_ADDRL;
	}

	if (!(tree = cc_expr_new(where))) {
		return NULL;
	}

	tree->_op = IR_MKOP(op);
	if (loc) { tree->_loc = *loc; }
	tree->_u._symbol = p;

	if (IsArray(ty) || IsFunction(ty)) {
		tree->_ty = p->_type;
	} 
	else {
		tree->_ty = cc_type_ptr(p->_type);
		tree = cc_expr_indir(tree, loc, where);
	}

	tree->_islvalue = 1;
	return tree;
}

FCCIRTree* cc_expr_addr(FCCIRTree*expr, FLocation* loc, enum EMMArea where)
{
	if (!expr->_islvalue || IR_OP(expr->_op) != IR_INDIR) {
		logger_output_s("error l-value required at %w\n", &expr->_loc);
		return NULL;
	}

	return expr->_u._kids[0];
}

FCCIRTree* cc_expr_indir(FCCIRTree* expr, FLocation* loc, enum EMMArea where)
{
	FCCIRTree* tree;
	FCCType* ty = cc_type_deref(expr->_ty);

	if (!(tree = cc_expr_new(where))) {
		return NULL;
	}

	tree->_op = IR_MKOP1(IR_INDIR, cc_ir_typecode(ty));
	if (loc) { tree->_loc = *loc; }
	tree->_ty = ty;
	tree->_u._kids[0] = expr;
	tree->_islvalue = 1;

	return tree;
}

FCCIRTree* cc_expr_constant(FCCType* ty, int tycode, FLocation* loc, enum EMMArea where, ...)
{
	FCCIRTree* tree;
	va_list arg;

	if (!(tree = cc_expr_new(where))) {
		return NULL;
	}

	va_start(arg, where);
	tree->_op = IR_MKOP1(IR_CONST, tycode);
	tree->_ty = ty;
	if (loc) { tree->_loc = *loc; }
	switch (tycode)
	{
	case IR_S8:  tree->_u._val._sint = va_arg(arg, int8_t); break;
	case IR_U8:  tree->_u._val._uint = va_arg(arg, uint8_t); break;
	case IR_S16: tree->_u._val._sint = va_arg(arg, int16_t); break;
	case IR_U16: tree->_u._val._uint = va_arg(arg, uint16_t); break;
	case IR_S32: tree->_u._val._sint = va_arg(arg, int32_t); break;
	case IR_U32: tree->_u._val._uint = va_arg(arg, uint32_t); break;
	case IR_S64: tree->_u._val._sint = va_arg(arg, int64_t); break;
	case IR_U64: tree->_u._val._uint = va_arg(arg, uint64_t); break;
	case IR_F32: tree->_u._val._float = va_arg(arg, float); break;
	case IR_F64: tree->_u._val._float = va_arg(arg, double); break;
	case IR_PTR: tree->_u._val._pointer = va_arg(arg, uint64_t); break;
	case IR_STRA:
	case IR_STRW:
		tree->_u._val._payload = va_arg(arg, void*); break;
	default:
		logger_output_s("invalid constant is found at %w, type code %d\n", loc, tycode);
		tree = NULL;
		break;
	}

	va_end(arg);
	return tree;
}

static FCCIRTree* cc_expr_cast_constant(FCCType* ty, FCCIRTree* expr, FLocation* loc, enum EMMArea where)
{
	/* begin define */
#define XX_CAST_CNST(type)		\
		switch (dstty)			\
		{						\
		case IR_S8:				\
		case IR_S16:			\
		case IR_S32:			\
		case IR_S64:			\
			return cc_expr_constant(ty, dstty, loc, where, (type)expr->_u._val._sint);	\
		case IR_U8:				\
		case IR_U16:			\
		case IR_U32:			\
		case IR_U64:			\
			return cc_expr_constant(ty, dstty, loc, where, (type)expr->_u._val._uint);	\
		case IR_F32:			\
		case IR_F64:			\
			return cc_expr_constant(ty, dstty, loc, where, (type)expr->_u._val._float);	\
		case IR_PTR:			\
			return cc_expr_constant(ty, dstty, loc, where, (type)expr->_u._val._pointer);	\
		default:				\
			break;				\
		}
	/* end define */

	int srcty = IR_OPTY0(expr->_op);
	int dstty = cc_ir_typecode(ty);
	
	loc = loc ? loc : &expr->_loc;
	switch (dstty)
	{
	case IR_S8:
		XX_CAST_CNST(int8_t);
		break;
	case IR_U8:
		XX_CAST_CNST(uint8_t);
		break;
	case IR_S16:
		XX_CAST_CNST(int16_t);
		break;
	case IR_U16:
		XX_CAST_CNST(uint16_t);
		break;
	case IR_S32:
		XX_CAST_CNST(int32_t);
		break;
	case IR_U32:
		XX_CAST_CNST(uint32_t);
		break;
	case IR_S64:
		XX_CAST_CNST(int64_t);
		break;
	case IR_U64:
		XX_CAST_CNST(int64_t);
		break;
	case IR_F32:
		XX_CAST_CNST(float);
		break;
	case IR_F64:
		XX_CAST_CNST(double);
		break;
	case IR_PTR:
		XX_CAST_CNST(uint64_t);
		break;
	default:
		break;
	}

	assert(0);
	return NULL;

#undef XX_CAST_CNST
}

FCCIRTree* cc_expr_cast(FCCType* ty, FCCIRTree* expr, FLocation* loc, enum EMMArea where)
{
	FCCIRTree* tree;
	int src, dst;

	if (cc_type_isequal(UnQual(ty), UnQual(expr->_ty), TRUE)) {
		return expr;
	}
	if (!cc_type_cancast(UnQual(ty), UnQual(expr->_ty))) {
		logger_output_s("error typecast failed at %w\n", &expr->_loc);
		return NULL;
	}

	if (IsPtr(expr->_ty) && IsPtr(ty)) {
		return cc_expr_change_rettype(expr, ty, where);
	}

	if (IR_OP(expr->_op) == IR_CONST) {
		return cc_expr_cast_constant(ty, expr, loc, where);
	}

	if (!(tree = cc_expr_new(where))) {
		return NULL;
	}

	dst = cc_ir_typecode(ty);
	src = cc_ir_typecode(expr->_ty);
	tree->_op = IR_MKOP2(IR_CVT, dst, src);
	tree->_loc = loc ? *loc : expr->_loc;
	tree->_ty = ty;
	tree->_u._kids[0] = expr;

	return tree;
}

FCCIRTree* cc_expr_add(FCCType* ty, FCCIRTree* lhs, FCCIRTree* rhs, FLocation* loc, enum EMMArea where)
{
	return NULL;
}

FCCIRTree* cc_expr_sub(FCCType* ty, FCCIRTree* lhs, FCCIRTree* rhs, FLocation* loc, enum EMMArea where)
{
	return NULL;
}

FCCIRTree* cc_expr_mul(FCCType* ty, FCCIRTree* lhs, FCCIRTree* rhs, FLocation* loc, enum EMMArea where)
{
	return NULL;
}

FCCIRTree* cc_expr_div(FCCType* ty, FCCIRTree* lhs, FCCIRTree* rhs, FLocation* loc, enum EMMArea where)
{
	return NULL;
}

FCCIRTree* cc_expr_mod(FCCType* ty, FCCIRTree* lhs, FCCIRTree* rhs, FLocation* loc, enum EMMArea where)
{
	if (!IsInt(ty))
	{
		logger_output_s("error integer operands is expected for '%', at %w\n", loc);
		return NULL;
	}

	return NULL;
}

static BOOL cc_expr_checkarguments(FCCType* functy, FCCIRTree** args, enum EMMArea where)
{
	FCCType** protos;
	int m, n, k;

	protos = functy->_u._f._protos;
	for (m = 0; protos[m]; m++); /* end for m */
	for (n = 0; args[n]; n++); /* end for n */

	if (cc_type_isvariance(functy)) {
		if (m > n) {
			logger_output_s("error: too few parameters in call. \n");
			return FALSE;
		}
	}
	else if (m != n) {
		logger_output_s("error: mismatch parameters count in call. \n");
		return FALSE;
	}

	for (k = 0; k < m; k++)
	{
		FCCType* ty = cc_expr_assigntype(protos[k], args[k]);

		if (!ty) {
			logger_output_s("type error in argument %d; found `%t' expected `%t'\n", k, args[k]->_ty, protos[k]);
			return FALSE;
		}

		args[k] = cc_expr_cast(ty, args[k], NULL, where);
		if (IsInt(args[k]->_ty)
			&& args[k]->_ty->_size != gbuiltintypes._sinttype->_size)
		{
			args[k] = cc_expr_cast(cc_type_promote(args[k]->_ty), args[k], NULL, where);
		}
	} /* end for k */

	for (; k < n; k++)
	{
		args[k] = cc_expr_cast(cc_type_promote(args[k]->_ty), args[k], NULL, where);
	}

	return TRUE;
}

FCCIRTree* cc_expr_call(FCCType* fty, FCCIRTree* expr, FArray* args, FLocation* loc, enum EMMArea where)
{
	/* check arguments */
	if (!cc_expr_checkarguments(fty, args->_data, where)) {
		logger_output_s("error: function parameters checking failed at %w\n", &loc);
		return NULL;
	}

	return NULL;
}

FCCIRTree* cc_expr_seq(FCCType* ty, FCCIRTree* expr0, FCCIRTree* expr1, FLocation* loc, enum EMMArea where)
{
	return NULL;
}

FCCIRTree* cc_expr_assign(FCCType* ty, FCCIRTree* lhs, FCCIRTree* rhs, FLocation* loc, enum EMMArea where)
{
	return NULL;
}

FCCIRTree* cc_expr_neg(FCCType* ty, FCCIRTree* expr, FLocation* loc, enum EMMArea where)
{
	return NULL;
}

FCCIRTree* cc_expr_bitcom(FCCType* ty, FCCIRTree* expr, FLocation* loc, enum EMMArea where)
{
	return NULL;
}
