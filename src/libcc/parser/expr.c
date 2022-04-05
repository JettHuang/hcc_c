/* \brief
 *		expression parsing.
 */

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
		util_memset(tree, 0, sizeof(FCCIRTree));
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
			ty = p->_type->_type;
			tree = cc_expr_constant(ty, cc_ir_typecode(ty), &ctx->_currtk._loc, where, (int)p->_u._cnstval._sint);
		}
		else
		{
			if (!(tree = cc_expr_id(p, &ctx->_currtk._loc, where))) {
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

			ty = UnQual(expr->_ty);
			if (!IsPtr(ty) && !IsArray(ty)) {
				logger_output_s("error: pointer or array is expected at %w\n", &loc);
				return FALSE;
			}
			if (IsPtr(ty) && IsFunction(ty->_type)) {
				logger_output_s("error: subscripted value is pointer to function at %w\n", &loc);
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

			expr = cc_expr_value(expr, where);
			expr = cc_expr_adjust(expr, where);
			subscript = cc_expr_value(subscript, where);
			offset = cc_expr_mul(gbuiltintypes._sinttype, subscript, cc_expr_constant(gbuiltintypes._sinttype, cc_ir_typecode(gbuiltintypes._sinttype), &loc, where, ty->_type->_size), &loc, where);
			tree = cc_expr_add(expr->_ty, expr, offset, &loc, where);
			if (IsPtr(tree->_ty) && IsArray(tree->_ty->_type)) {
				tree = cc_expr_change_rettype(tree, tree->_ty->_type, where);
			}
			else {
				tree = cc_expr_indir(tree, &loc, where);
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

			expr = cc_expr_value(expr, where);
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

			expr = cc_expr_value(expr, where);
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
			tree->_field = field;
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

			expr = cc_expr_value(expr, where);
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
			tree->_field = field;
			tree->_isbitfield = field->_lsb > 0;
			tree->_islvalue = 1;

			expr = tree;
			cc_read_token(ctx, &ctx->_currtk);
		}
		else if (ctx->_currtk._type == TK_INC) /* ++ */
		{
			FCCType* ty;
			FCCIRTree* saveval, *tmpId;
			int v;

			loc = ctx->_currtk._loc;
			if (!cc_expr_is_modifiable(expr)) {
				logger_output_s("error: modifiable l-value is expected at %w.\n", &loc);
				return FALSE;
			}

			ty = UnQual(expr->_ty);
			if (!IsScalar(ty)) {
				logger_output_s("error: %t is not a scalar type at %w\n", ty, &loc);
			}
			if (IsPtr(ty)) {
				v = IsVoidptr(ty) ? 1 : ty->_type->_size; /* surport void *p; p++ */
			}
			else {
				v = 1;
			}

			if (v <= 0)
			{
				logger_output_s("error pointer to an incomplete type %t at %w.\n", ty->_type, &loc);
				return FALSE;
			}

			tree = cc_expr_constant(gbuiltintypes._sinttype, cc_ir_typecode(gbuiltintypes._sinttype), &loc, where, v);
			tmpId = cc_expr_id(cc_symbol_temporary(ty, SC_Auto), NULL, where);
			saveval = cc_expr_assign(ty, tmpId, expr, &loc, where);
			tree = cc_expr_seq(ty, saveval, cc_expr_assign(ty, expr, cc_expr_add(ty, expr, tree, &loc, where), &loc, where), &loc, where);
			tree = cc_expr_seq(ty, tree, tmpId, &loc, where);

			expr = tree;
			cc_read_token(ctx, &ctx->_currtk);
		}
		else if (ctx->_currtk._type == TK_DEC) /* -- */
		{
			FCCType* ty;
			FCCIRTree* saveval, * tmpId;
			int v;

			loc = ctx->_currtk._loc;
			if (!cc_expr_is_modifiable(expr)) {
				logger_output_s("error: modifiable l-value is expected at %w.\n", &loc);
				return FALSE;
			}

			ty = UnQual(expr->_ty);
			if (!IsScalar(ty)) {
				logger_output_s("error: %t is not a scalar type at %w\n", ty, &loc);
			}
			if (IsPtr(ty)) {
				v = IsVoidptr(ty) ? 1 : ty->_type->_size; /* surport void *p; p++ */
			}
			else {
				v = 1;
			}

			if (v <= 0)
			{
				logger_output_s("error pointer to an incomplete type %t at %w.\n", ty->_type, &loc);
				return FALSE;
			}

			tree = cc_expr_constant(gbuiltintypes._sinttype, cc_ir_typecode(gbuiltintypes._sinttype), &loc, where, v);
			tmpId = cc_expr_id(cc_symbol_temporary(ty, SC_Auto), NULL, where);
			saveval = cc_expr_assign(ty, tmpId, expr, &loc, where);
			tree = cc_expr_seq(ty, saveval, cc_expr_assign(ty, expr, cc_expr_sub(ty, expr, tree, &loc, where), &loc, where), &loc, where);
			tree = cc_expr_seq(ty, tree, tmpId, &loc, where);

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

		if (!(expr = cc_expr_value(expr, where))) {
			return FALSE;
		}
		if (!(expr = cc_expr_adjust(expr, where))) {
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
		if (IsAddrOp(tree->_op) && tree->_symbol->_sclass == SC_Register) {
			logger_output_s("error: invalid operand of unary '&', '%s' is declared register at %w\n", tree->_symbol->_name, &loc);
			return FALSE;
		}
		else if (IsAddrOp(tree->_op)) {
			tree->_symbol->_addressed = 1;
		}

		tree->_islvalue = 0;
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

		expr = cc_expr_value(expr, where);
		expr = cc_expr_adjust(expr, where);
		if (!IsPtr(expr->_ty)) {
			logger_output_s("error: pointer is expected at %w\n", &loc);
			return FALSE;
		}
		if (IsFunction(expr->_ty->_type) || IsArray(expr->_ty->_type)) {
			tree = cc_expr_change_rettype(expr, expr->_ty->_type, where);
		}
		else {
			tree = cc_expr_indir(expr, &loc, where);
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

		if (!IsArith(expr->_ty)) {
			logger_output_s("error arithmetic type is expected for '+' at %w.\n", &loc);
			return FALSE;
		}
		expr = cc_expr_value(expr, where);
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

		if (!IsArith(expr->_ty)) {
			logger_output_s("error arithmetic type is expected for '-' at %w.\n", &loc);
			return FALSE;
		}
		expr = cc_expr_value(expr, where);
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

		if (!IsInt(expr->_ty)) {
			logger_output_s("error integer type is expected for '~' at %w.\n", &loc);
			return FALSE;
		}
		expr = cc_expr_value(expr, where);
		tree = cc_expr_cast(cc_type_promote(expr->_ty), expr, &loc, where);
		tree = cc_expr_bitcom(tree->_ty, tree, &loc, where);

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

		if (!(tree = cc_expr_not(gbuiltintypes._sinttype, expr, &loc, where)))
		{
			return FALSE;
		}

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

		if (!(tree = cc_expr_constant(gbuiltintypes._sinttype, cc_ir_typecode(gbuiltintypes._sinttype), &loc, where, exprty->_size))) {
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

			tree = cc_expr_value(tree, where);
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
		lhs = cc_expr_value(lhs, where);
		lhs = cc_expr_cast(ty, lhs, &loc, where);
		rhs = cc_expr_value(rhs, where);
		rhs = cc_expr_cast(ty, rhs, &loc, where);
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
		FCCType* ty = gbuiltintypes._sinttype;

		loc = ctx->_currtk._loc;
		cc_read_token(ctx, &ctx->_currtk);
		if (!cc_expr_parse_multiplicative(ctx, &rhs, where)) {
			return FALSE;
		}

		lhs = cc_expr_value(lhs, where);
		lhs = cc_expr_adjust(lhs, where);
		rhs = cc_expr_value(rhs, where);
		rhs = cc_expr_adjust(rhs, where);
		if (tktype == TK_ADD)
		{
			if (IsArith(lhs->_ty) && IsArith(rhs->_ty)) {
				ty = cc_type_select(lhs->_ty, rhs->_ty);
				lhs = cc_expr_cast(ty, lhs, &loc, where);
				rhs = cc_expr_cast(ty, rhs, &loc, where);
				tree = cc_expr_add(ty, lhs, rhs, &loc, where);
			}
			else if (IsPtr(lhs->_ty) && !IsFunction(lhs->_ty->_type) && IsInt(rhs->_ty)) {
				FCCType* equalty;
				int n;

				ty = UnQual(lhs->_ty);
				n = ty->_type->_size;
				if (IsVoidptr(ty)) { n = 1; }
				if (n == 0) {
					logger_output_s("error: unknown size for type '%t'\n", ty->_type);
					return FALSE;
				}

				equalty = IsUnsigned(rhs->_ty) ? gbuiltintypes._uinttype : gbuiltintypes._sinttype;
				assert(equalty->_size == gbuiltintypes._ptrvoidtype->_size);
				rhs = cc_expr_cast(equalty, rhs, &loc, where);
				if (n > 1) {
					rhs = cc_expr_mul(equalty, rhs, cc_expr_constant(equalty, cc_ir_typecode(equalty), &loc, where, n), &loc, where);
				}

				tree = cc_expr_add(ty, lhs, rhs, &loc, where);
			}
			else if (IsInt(lhs->_ty) && IsPtr(rhs->_ty) && !IsFunction(rhs->_ty->_type)) {
				FCCType* equalty;
				int n;

				ty = UnQual(rhs->_ty);
				n = ty->_type->_size;
				if (IsVoidptr(ty)) { n = 1; }
				if (n == 0) {
					logger_output_s("error: unknown size for type '%t'\n", ty->_type);
					return FALSE;
				}

				equalty = IsUnsigned(lhs->_ty) ? gbuiltintypes._uinttype : gbuiltintypes._sinttype;
				assert(equalty->_size == gbuiltintypes._ptrvoidtype->_size);
				lhs = cc_expr_cast(equalty, lhs, &loc, where);
				if (n > 1) {
					lhs = cc_expr_mul(equalty, lhs, cc_expr_constant(equalty, cc_ir_typecode(equalty), &loc, where, n), &loc, where);
				}

				tree = cc_expr_add(ty, lhs, rhs, &loc, where);
			}
			else
			{
				logger_output_s("error: illegal operand for '+', %t + %t at %w\n", lhs->_ty, rhs->_ty, &loc);
				return FALSE;
			}
		}
		else /* TK_SUB */
		{
			if (IsArith(lhs->_ty) && IsArith(rhs->_ty)) {
				ty = cc_type_select(lhs->_ty, rhs->_ty);
				lhs = cc_expr_cast(ty, lhs, &loc, where);
				rhs = cc_expr_cast(ty, rhs, &loc, where);
				tree = cc_expr_sub(ty, lhs, rhs, &loc, where);
			}
			else if (IsPtr(lhs->_ty) && !IsFunction(lhs->_ty->_type) && IsInt(rhs->_ty)) {
				FCCType* equalty;
				int n;

				ty = UnQual(lhs->_ty);
				n = ty->_type->_size;
				if (IsVoidptr(ty)) { n = 1; }
				if (n == 0) {
					logger_output_s("error: unknown size for type '%t'\n", ty->_type);
					return FALSE;
				}

				equalty = IsUnsigned(rhs->_ty) ? gbuiltintypes._uinttype : gbuiltintypes._sinttype;
				assert(equalty->_size == gbuiltintypes._ptrvoidtype->_size);
				rhs = cc_expr_cast(equalty, rhs, &loc, where);
				if (n > 1) {
					rhs = cc_expr_mul(equalty, rhs, cc_expr_constant(equalty, cc_ir_typecode(equalty), &loc, where, n), &loc, where);
				}

				tree = cc_expr_sub(ty, lhs, rhs, &loc, where);
			}
			else if (IsPtr(lhs->_ty) && !IsFunction(lhs->_ty->_type) && IsPtr(rhs->_ty) && !IsFunction(rhs->_ty->_type)
				&& cc_type_iscompatible(lhs->_ty, rhs->_ty))
			{
				FCCType* equalty;
				int n;

				ty = UnQual(rhs->_ty);
				n = ty->_type->_size;
				if (IsVoidptr(ty)) { n = 1; }
				if (n == 0) {
					logger_output_s("error: unknown size for type '%t'\n", ty->_type);
					return FALSE;
				}

				equalty = gbuiltintypes._uinttype;
				assert(equalty->_size == gbuiltintypes._ptrvoidtype->_size);
				lhs = cc_expr_cast(equalty, lhs, &loc, where);
				rhs = cc_expr_cast(equalty, rhs, &loc, where);
				tree = cc_expr_sub(equalty, lhs, rhs, &loc, where);
				if (n > 1) {
					tree = cc_expr_div(equalty, tree, cc_expr_constant(equalty, cc_ir_typecode(equalty), &loc, where, n), &loc, where);
				}
			}
			else
			{
				logger_output_s("error: illegal operand for '+', %t + %t at %w\n", lhs->_ty, rhs->_ty, &loc);
				return FALSE;
			}
		}

		lhs = tree;
	} /* end while */

	*outexpr = tree;
	return TRUE;
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
		FCCType* ty;

		loc = ctx->_currtk._loc;
		cc_read_token(ctx, &ctx->_currtk);
		if (!cc_expr_parse_additive(ctx, &rhs, where)) {
			return FALSE;
		}

		if (!IsInt(lhs->_ty) || !IsInt(rhs->_ty)) {
			logger_output_s("error: integer operand is expected for '%s' at %w\n", (tktype == TK_LSHIFT) ? "<<" : ">>", &loc);
			return FALSE;
		}
		ty = cc_type_promote(lhs->_ty);
		lhs = cc_expr_value(lhs, where);
		lhs = cc_expr_cast(ty, lhs, &loc, where);
		rhs = cc_expr_value(rhs, where);
		rhs = cc_expr_cast(gbuiltintypes._sinttype, rhs, &loc, where);
		tree = (tktype == TK_LSHIFT) ? cc_expr_lshift(ty, lhs, rhs, &loc, where) : cc_expr_rshift(ty, lhs, rhs, &loc, where);

		lhs = tree;
	} /* end while */

	*outexpr = tree;
	return TRUE;
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
		FCCType* ty;

		loc = ctx->_currtk._loc;
		cc_read_token(ctx, &ctx->_currtk);
		if (!cc_expr_parse_shift(ctx, &rhs, where)) {
			return FALSE;
		}

		lhs = cc_expr_value(lhs, where);
		lhs = cc_expr_adjust(lhs, where);
		rhs = cc_expr_value(rhs, where);
		rhs = cc_expr_adjust(rhs, where);
		if (IsArith(lhs->_ty) && IsArith(rhs->_ty)) {
			ty = cc_type_select(lhs->_ty, rhs->_ty);
			lhs = cc_expr_cast(ty, lhs, &loc, where);
			rhs = cc_expr_cast(ty, rhs, &loc, where);
		}
		else if (cc_type_iscompatible(lhs->_ty, rhs->_ty)) {
			ty = gbuiltintypes._uinttype;
			lhs = cc_expr_cast(ty, lhs, &loc, where);
			rhs = cc_expr_cast(ty, rhs, &loc, where);
		}
		else {
			logger_output_s("error: illegal operands for relational operator at %w", &loc);
			return FALSE;
		}

		switch (tktype)
		{
		case TK_LESS: tree = cc_expr_less(gbuiltintypes._sinttype, lhs, rhs, &loc, where); break;
		case TK_LESS_EQ: tree = cc_expr_lequal(gbuiltintypes._sinttype, lhs, rhs, &loc, where); break;
		case TK_GREAT: tree = cc_expr_great(gbuiltintypes._sinttype, lhs, rhs, &loc, where); break;
		case TK_GREAT_EQ: tree = cc_expr_gequal(gbuiltintypes._sinttype, lhs, rhs, &loc, where); break;
		}

		if (!tree) {
			return FALSE;
		}

		lhs = tree;
	} /* end while */

	*outexpr = tree;
	return TRUE;
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
		FCCType* ty, *xty, *yty;

		loc = ctx->_currtk._loc;
		cc_read_token(ctx, &ctx->_currtk);
		if (!cc_expr_parse_relational(ctx, &rhs, where)) {
			return FALSE;
		}

		lhs = cc_expr_value(lhs, where);
		lhs = cc_expr_adjust(lhs, where);
		rhs = cc_expr_value(rhs, where);
		rhs = cc_expr_adjust(rhs, where);
		xty = UnQual(lhs->_ty);
		yty = UnQual(rhs->_ty);
		if (IsPtr(xty) && cc_expr_is_nullptr(rhs)
			|| IsPtr(xty) && !IsFunction(xty->_type) && IsVoidptr(yty)
			|| (IsPtr(xty) && IsPtr(yty) && cc_type_isequal(UnQual(xty->_type), UnQual(yty->_type), TRUE))
			)
		{
			ty = gbuiltintypes._uinttype;
			lhs = cc_expr_cast(ty, lhs, &loc, where);
			rhs = cc_expr_cast(ty, rhs, &loc, where);
		}
		else if (IsPtr(yty) && cc_expr_is_nullptr(lhs)
			|| IsPtr(yty) && !IsFunction(yty->_type) && IsVoidptr(xty))
		{
			ty = gbuiltintypes._uinttype;
			lhs = cc_expr_cast(ty, lhs, &loc, where);
			rhs = cc_expr_cast(ty, rhs, &loc, where);
		}
		else if (IsArith(lhs->_ty) && IsArith(rhs->_ty)) {
			ty = cc_type_select(lhs->_ty, rhs->_ty);
			lhs = cc_expr_cast(ty, lhs, &loc, where);
			rhs = cc_expr_cast(ty, rhs, &loc, where);
		}
		else if (cc_type_iscompatible(lhs->_ty, rhs->_ty)) {
			ty = gbuiltintypes._uinttype;
			lhs = cc_expr_cast(ty, lhs, &loc, where);
			rhs = cc_expr_cast(ty, rhs, &loc, where);
		}
		else {
			logger_output_s("error: illegal operands for equality operator at %w", &loc);
			return FALSE;
		}

		tree = (tktype == TK_EQUAL) ? cc_expr_equal(gbuiltintypes._sinttype, lhs, rhs, &loc, where) : cc_expr_unequal(gbuiltintypes._sinttype, lhs, rhs, &loc, where);
		lhs = tree;
	} /* end while */

	*outexpr = tree;
	return TRUE;
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
		FCCType* ty;

		loc = ctx->_currtk._loc;
		cc_read_token(ctx, &ctx->_currtk);
		if (!cc_expr_parse_equality(ctx, &rhs, where)) {
			return FALSE;
		}

		if (!IsInt(lhs->_ty) || !IsInt(rhs->_ty)) {
			logger_output_s("error: integer operand is expected for '&' at %w\n", &loc);
			return FALSE;
		}

		ty = cc_type_select(lhs->_ty, rhs->_ty);
		lhs = cc_expr_value(lhs, where);
		lhs = cc_expr_cast(ty, lhs, &loc, where);
		rhs = cc_expr_value(rhs, where);
		rhs = cc_expr_cast(ty, rhs, &loc, where);
		if (!(tree = cc_expr_bitand(ty, lhs, rhs, &loc, where))) {
			return FALSE;
		}
		lhs = tree;
	} /* end while */

	*outexpr = tree;
	return TRUE;
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
		FCCType* ty;

		loc = ctx->_currtk._loc;
		cc_read_token(ctx, &ctx->_currtk);
		if (!cc_expr_parse_bitand(ctx, &rhs, where)) {
			return FALSE;
		}

		if (!IsInt(lhs->_ty) || !IsInt(rhs->_ty)) {
			logger_output_s("error: integer operand is expected for '^' at %w\n", &loc);
			return FALSE;
		}

		ty = cc_type_select(lhs->_ty, rhs->_ty);
		lhs = cc_expr_value(lhs, where);
		lhs = cc_expr_cast(ty, lhs, &loc, where);
		rhs = cc_expr_value(rhs, where);
		rhs = cc_expr_cast(ty, rhs, &loc, where);
		if (!(tree = cc_expr_bitxor(ty, lhs, rhs, &loc, where))) {
			return FALSE;
		}
		lhs = tree;
	} /* end while */

	*outexpr = tree;
	return TRUE;
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
		FCCType* ty;

		loc = ctx->_currtk._loc;
		cc_read_token(ctx, &ctx->_currtk);
		if (!cc_expr_parse_bitxor(ctx, &rhs, where)) {
			return FALSE;
		}

		if (!IsInt(lhs->_ty) || !IsInt(rhs->_ty)) {
			logger_output_s("error: integer operand is expected for '^' at %w\n", &loc);
			return FALSE;
		}

		ty = cc_type_select(lhs->_ty, rhs->_ty);
		lhs = cc_expr_value(lhs, where);
		lhs = cc_expr_cast(ty, lhs, &loc, where);
		rhs = cc_expr_value(rhs, where);
		rhs = cc_expr_cast(ty, rhs, &loc, where);
		if (!(tree = cc_expr_bitor(ty, lhs, rhs, &loc, where))) {
			return FALSE;
		}
		lhs = tree;
	} /* end while */

	*outexpr = tree;
	return TRUE;
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

		if (!(tree = cc_expr_logicand(gbuiltintypes._sinttype, lhs, rhs, &loc, where))) {
			return FALSE;
		}

		lhs = tree;
	} /* end while */

	*outexpr = tree;
	return TRUE;
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

		if (!(tree = cc_expr_logicor(gbuiltintypes._sinttype, lhs, rhs, &loc, where))) {
			return FALSE;
		}

		lhs = tree;
	} /* end while */

	*outexpr = tree;
	return TRUE;
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
		if (!cc_expr_parse_expression(ctx, &expr1, where)) {
			return FALSE;
		}
		if (!cc_parser_expect(ctx, TK_COLON)) /* ':' */ {
			return FALSE;
		}
		if (!cc_expr_parse_conditional(ctx, &expr2, where)) {
			return FALSE;
		}

		expr1 = cc_expr_value(expr1, where);
		expr1 = cc_expr_adjust(expr1, where);
		expr2 = cc_expr_value(expr2, where);
		expr2 = cc_expr_adjust(expr2, where);

		if (!(tree = cc_expr_condition(expr0, expr1, expr2, &loc, where))) {
			return FALSE;
		}
	}

	*outexpr = tree;
	return TRUE;
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
		enum ECCToken assigntk = ctx->_currtk._type;
		FCCType *ty;

		loc = ctx->_currtk._loc;
		cc_read_token(ctx, &ctx->_currtk);
		if (!cc_expr_parse_assignment(ctx, &rhs, where)) {
			return FALSE;
		}

		if (!cc_expr_is_modifiable(lhs)) {
			logger_output_s("error: assign to an un-modifiable object at %w\n", &loc);
			return FALSE;
		}

		if (!(IsInt(lhs->_ty) && IsInt(rhs->_ty)) && assigntk == TK_MOD_ASSIGN) {
			logger_output_s("error: illegal operands for '%%=' '%t'%='%t' at %w\n", lhs->_ty, rhs->_ty, &loc);
			return FALSE;
		}

		rhs = cc_expr_value(rhs, where);
		rhs = cc_expr_adjust(rhs, where);
		ty = cc_expr_assigntype(lhs->_ty, rhs);
		if (ty) {
			rhs = cc_expr_cast(ty, rhs, &loc, where);
		}
		else {
			logger_output_s("error: can't assign to different type operands '%t' = '%t' at %w\n", lhs->_ty, rhs->_ty, &loc);
			return FALSE;
		}

		switch (assigntk)
		{
		case TK_ASSIGN:
			tree = cc_expr_assign(ty, lhs, rhs, &loc, where);
			break;
		case TK_MUL_ASSIGN:
			tree = cc_expr_mul(ty, lhs, rhs, &loc, where);
			tree = cc_expr_seq(ty, tree, cc_expr_assign(ty, lhs, tree, &loc, where), &loc, where);
			break;
		case TK_DIV_ASSIGN:
			tree = cc_expr_div(ty, lhs, rhs, &loc, where);
			tree = cc_expr_seq(ty, tree, cc_expr_assign(ty, lhs, tree, &loc, where), &loc, where);
			break;
		case TK_MOD_ASSIGN:
			tree = cc_expr_mod(ty, lhs, rhs, &loc, where);
			tree = cc_expr_seq(ty, tree, cc_expr_assign(ty, lhs, tree, &loc, where), &loc, where);
			break;
		case TK_ADD_ASSIGN:
			tree = cc_expr_add(ty, lhs, rhs, &loc, where);
			tree = cc_expr_seq(ty, tree, cc_expr_assign(ty, lhs, tree, &loc, where), &loc, where);
			break;
		case TK_SUB_ASSIGN:
			tree = cc_expr_sub(ty, lhs, rhs, &loc, where);
			tree = cc_expr_seq(ty, tree, cc_expr_assign(ty, lhs, tree, &loc, where), &loc, where);
			break;
		case TK_LSHIFT_ASSIGN:
			tree = cc_expr_lshift(ty, lhs, rhs, &loc, where);
			tree = cc_expr_seq(ty, tree, cc_expr_assign(ty, lhs, tree, &loc, where), &loc, where);
			break;
		case TK_RSHIFT_ASSIGN:
			tree = cc_expr_rshift(ty, lhs, rhs, &loc, where);
			tree = cc_expr_seq(ty, tree, cc_expr_assign(ty, lhs, tree, &loc, where), &loc, where);
			break;
		case TK_BITAND_ASSIGN:
			tree = cc_expr_bitand(ty, lhs, rhs, &loc, where);
			tree = cc_expr_seq(ty, tree, cc_expr_assign(ty, lhs, tree, &loc, where), &loc, where);
			break;
		case TK_BITXOR_ASSIGN:
			tree = cc_expr_bitxor(ty, lhs, rhs, &loc, where);
			tree = cc_expr_seq(ty, tree, cc_expr_assign(ty, lhs, tree, &loc, where), &loc, where);
			break;
		case TK_BITOR_ASSIGN:
			tree = cc_expr_bitor(ty, lhs, rhs, &loc, where);
			tree = cc_expr_seq(ty, tree, cc_expr_assign(ty, lhs, tree, &loc, where), &loc, where);
			break;
		}

		if (!tree) { return FALSE; }
	} /* end if */

	*outexpr = tree;
	return TRUE;
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

		if (!(tree = cc_expr_seq(rhs->_ty, lhs, rhs, &loc, where))) {
			return FALSE;
		}

		lhs = tree;
	} /* end while */

	*outexpr = tree;
	return TRUE;
}

BOOL cc_expr_parse_constant_expression(FCCContext* ctx, FCCIRTree** outexpr, enum EMMArea where)
{
	if (cc_expr_parse_conditional(ctx, outexpr, where) && cc_expr_is_constant(*outexpr))
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
	BOOL cfields = FALSE;

	if (IsStruct(expr->_ty)) {
		cfields = cc_type_has_cfields(UnQual(expr->_ty));
	}

	return expr->_islvalue
		&& !IsConst(expr->_ty)
		&& !IsArray(expr->_ty)
		&& !IsFunction(expr->_ty)
		&& !cfields;
}

BOOL cc_expr_is_nullptr(FCCIRTree* expr)
{
	FCCType* ty = UnQual(expr->_ty);

	return (IR_OP(expr->_op) == IR_CONST)
		&& ((ty->_op == Type_SInteger && expr->_u._val._sint == 0)
			|| (ty->_op == Type_UInteger && expr->_u._val._uint == 0)
			|| (IsVoidptr(ty) && expr->_u._val._pointer == 0));
}

BOOL cc_expr_is_constant(FCCIRTree* expr)
{
	/* 1. address of static storage object's
	 * 2. constant value
	 * 3. no type cast with size-change
	 * 4. + -
	 */

	int ircode = IR_OP(expr->_op);
	switch (ircode)
	{
	case IR_ADDRG:
	case IR_CONST:
		return TRUE;
	case IR_ADD:
	case IR_SUB:
		return cc_expr_is_constant(expr->_u._kids[0]) && cc_expr_is_constant(expr->_u._kids[1]);
	case IR_CVT:
		if (IsPtr(expr->_u._kids[0]->_ty) && IsInt(expr->_ty) && expr->_ty->_size == gbuiltintypes._ptrvoidtype->_size
			|| IsPtr(expr->_ty) && IsInt(expr->_u._kids[0]->_ty) && expr->_u._kids[0]->_ty->_size == gbuiltintypes._ptrvoidtype->_size
			|| IsInt(expr->_ty) && IsInt(expr->_u._kids[0]->_ty) && expr->_u._kids[0]->_ty->_size == expr->_ty->_size)
		{
			return TRUE;
		}
		break;
	default:
		break;
	}

	return FALSE;
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
	tree->_op = IR_MKOP2(IR_OP(tree->_op), tycode, IR_OPTY1(tree->_op));
	tree->_ty = newty;
	return tree;
}

FCCIRTree* cc_expr_adjust(FCCIRTree* expr, enum EMMArea where)
{
	FCCIRTree* right, ** replace;

	right = expr;
	replace = NULL;
	while (IR_OP(right->_op) == IR_SEQ)
	{
		replace = &(right->_u._kids[1]);
		right = *replace;
	}

	if (right->_op == IR_MKOP1(IR_CONST, IR_STRA)
		|| right->_op == IR_MKOP1(IR_CONST, IR_STRW))
	{
		FCCSymbol* p;
		
		if (!(p = cc_symbol_constant(right->_ty, right->_u._val)))
		{
			logger_output_s("error install constant symbol failed at %s:%d\n", __FILE__, __LINE__);
			return NULL;
		}

		if (!(right = cc_expr_id(p, NULL, where)))
		{
			return NULL;
		}
	}
	
	if (IsArray(right->_ty)) {
		right = cc_expr_change_rettype(right, cc_type_arraytoptr(right->_ty), where);
	} else if (IsFunction(right->_ty)) {
		right = cc_expr_change_rettype(right, cc_type_ptr(right->_ty), where);
	}

	if (replace) {
		*replace = right;
		expr = cc_expr_change_rettype(expr, right->_ty, where);
	}
	else {
		expr = right;
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

	tree->_op = IR_MKOP1(op, IR_PTR);
	if (loc) { tree->_loc = *loc; }
	tree->_symbol = p;

	if (IsArray(ty) || IsFunction(ty)) {
		tree->_ty = p->_type;
	} 
	else {
		tree->_ty = cc_type_ptr(p->_type);
		tree = cc_expr_indir(tree, loc, where);
	}

	tree->_islvalue = !p->_temporary;
	return tree;
}

FCCIRTree* cc_expr_addr(FCCIRTree*expr, FLocation* loc, enum EMMArea where)
{
	if (IR_OP(expr->_op) != IR_INDIR) {
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
	case IR_F32: tree->_u._val._float = va_arg(arg, double); break;
	case IR_F64: tree->_u._val._float = va_arg(arg, double); break;
	case IR_PTR: tree->_u._val._pointer = va_arg(arg, uint32_t); break;
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
		switch (srcty)			\
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
		XX_CAST_CNST(uint32_t);
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

	if (IR_OP(expr->_op) == IR_CONST) {
		return cc_expr_cast_constant(ty, expr, loc, where);
	}
	if (IsPtr(expr->_ty) && IsPtr(ty)) {
		return cc_expr_change_rettype(expr, ty, where);
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

static BOOL cc_expr_iszero(FCCIRTree* lhs)
{
	int tycode = cc_ir_typecode(lhs->_ty);

	switch (tycode)	
	{
	case IR_S8:
	case IR_S16:
	case IR_S32:
	case IR_S64:
		return lhs->_u._val._sint == 0;
	case IR_U8:	
	case IR_U16:
	case IR_U32:
	case IR_U64:
		return lhs->_u._val._uint == 0u;
	case IR_F32:
	case IR_F64:
		return lhs->_u._val._float == 0.0;
	case IR_PTR:			\
		return lhs->_u._val._pointer == 0;
	default:
		assert(0); break;
	}

	return FALSE;
}

static FCCIRTree* cc_fold_constant(int op, FCCType* ty, FCCIRTree* lhs, FCCIRTree* rhs, FLocation* loc, enum EMMArea where)
{
	/* begin define */
	#define XX_CALC_CNST(tycode, op)\
		switch (tycode)			\
		{						\
		case IR_S8:				\
		case IR_S16:			\
		case IR_S32:			\
		case IR_S64:			\
			ivalue = lhs->_u._val._sint op rhs->_u._val._sint;	break;\
		case IR_U8:				\
		case IR_U16:			\
		case IR_U32:			\
		case IR_U64:			\
			uvalue = lhs->_u._val._uint op rhs->_u._val._uint;	break;\
		case IR_F32:			\
		case IR_F64:			\
			fvalue = lhs->_u._val._float op rhs->_u._val._float; break;\
		case IR_PTR:			\
			pvalue = lhs->_u._val._pointer op (uint32_t)(rhs->_u._val._uint); break;\
		default:				\
			assert(0); break;	\
		}

	#define XX_CALC_CNST_MOD(tycode)\
		switch (tycode)			\
		{						\
		case IR_S8:				\
		case IR_S16:			\
		case IR_S32:			\
		case IR_S64:			\
			ivalue = lhs->_u._val._sint % rhs->_u._val._sint;	break;\
		case IR_U8:				\
		case IR_U16:			\
		case IR_U32:			\
		case IR_U64:			\
			uvalue = lhs->_u._val._uint % rhs->_u._val._uint;	break;\
		default:	\
			assert(0); break;	\
		}
	/* end define */

	int64_t ivalue;
	uint64_t uvalue;
	double fvalue;
	uint32_t pvalue;

	int tycode = cc_ir_typecode(ty);
	switch (op)
	{
	case '+':
		XX_CALC_CNST(tycode, +);
		break;
	case '-':
		XX_CALC_CNST(tycode, -);
		break;
	case '*':
		XX_CALC_CNST(tycode, *);
		break;
	case '/':
		XX_CALC_CNST(tycode, /);
		break;
	case '%':
		XX_CALC_CNST_MOD(tycode);
		break;
	}

	switch (tycode)	
	{
	case IR_S8:	
		return cc_expr_constant(ty, tycode, loc, where, (int8_t)ivalue);
	case IR_S16:
		return cc_expr_constant(ty, tycode, loc, where, (int16_t)ivalue);
	case IR_S32:
		return cc_expr_constant(ty, tycode, loc, where, (int32_t)ivalue);
	case IR_S64:
		return cc_expr_constant(ty, tycode, loc, where, (int64_t)ivalue);
	case IR_U8:
		return cc_expr_constant(ty, tycode, loc, where, (uint8_t)uvalue);
	case IR_U16:
		return cc_expr_constant(ty, tycode, loc, where, (uint16_t)uvalue);
	case IR_U32:
		return cc_expr_constant(ty, tycode, loc, where, (uint32_t)uvalue);
	case IR_U64:
		return cc_expr_constant(ty, tycode, loc, where, (uint64_t)uvalue);
	case IR_F32:
		return cc_expr_constant(ty, tycode, loc, where, (float)fvalue);
	case IR_F64:
		return cc_expr_constant(ty, tycode, loc, where, (double)fvalue);
	case IR_PTR:
		return cc_expr_constant(ty, tycode, loc, where, (uint32_t)pvalue);
	default:
		assert(0); break;
	}

	return NULL;

#undef XX_CALC_CNST
#undef XX_CALC_CNST_MOD
}

FCCIRTree* cc_expr_add(FCCType* ty, FCCIRTree* lhs, FCCIRTree* rhs, FLocation* loc, enum EMMArea where)
{
	FCCIRTree* tree;
	int code0, code1;

	if (IR_OP(lhs->_op) == IR_CONST && IR_OP(rhs->_op) == IR_CONST) {
		return cc_fold_constant('+', ty, lhs, rhs, loc, where);
	}
	if (IR_OP(rhs->_op) == IR_CONST)
	{
		if (IR_OP(lhs->_op) == IR_ADD) {
			FCCIRTree* llhs = lhs->_u._kids[0];
			FCCIRTree* lrhs = lhs->_u._kids[1];
			if (IR_OP(lrhs->_op) == IR_CONST) {
				lrhs = cc_expr_add(lrhs->_ty, lrhs, cc_expr_cast(lrhs->_ty, rhs, loc, where), loc, where);
				return cc_expr_add(ty, llhs, lrhs, loc, where);
			}
		}
		else if (IR_OP(lhs->_op) == IR_SUB) {
			FCCIRTree* llhs = lhs->_u._kids[0];
			FCCIRTree* lrhs = lhs->_u._kids[1];
			if (IR_OP(lrhs->_op) == IR_CONST) {
				lrhs = cc_expr_sub(lrhs->_ty, cc_expr_cast(lrhs->_ty, rhs, loc, where), lrhs, loc, where);
				return cc_expr_add(ty, llhs, lrhs, loc, where);
			}
		}
	}

	if (!(tree = cc_expr_new(where))) { return NULL; }
	
	code0 = cc_ir_typecode(ty);
	code1 = cc_ir_typecode(rhs->_ty);
	tree->_op = IR_MKOP2(IR_ADD, code0, code1);
	tree->_ty = ty;
	tree->_loc = loc ? *loc : lhs->_loc;
	tree->_u._kids[0] = lhs;
	tree->_u._kids[1] = rhs;

	return tree;
}

FCCIRTree* cc_expr_sub(FCCType* ty, FCCIRTree* lhs, FCCIRTree* rhs, FLocation* loc, enum EMMArea where)
{
	FCCIRTree* tree;
	int code0, code1;

	if (IR_OP(lhs->_op) == IR_CONST && IR_OP(rhs->_op) == IR_CONST) {
		return cc_fold_constant('-', ty, lhs, rhs, loc, where);
	}
	if (IR_OP(rhs->_op) == IR_CONST)
	{
		if (IR_OP(lhs->_op) == IR_ADD) {
			FCCIRTree* llhs = lhs->_u._kids[0];
			FCCIRTree* lrhs = lhs->_u._kids[1];
			if (IR_OP(lrhs->_op) == IR_CONST) {
				lrhs = cc_expr_sub(lrhs->_ty, lrhs, cc_expr_cast(lrhs->_ty, rhs, loc, where), loc, where);
				return cc_expr_add(ty, llhs, lrhs, loc, where);
			}
		}
		else if (IR_OP(lhs->_op) == IR_SUB) {
			FCCIRTree* llhs = lhs->_u._kids[0];
			FCCIRTree* lrhs = lhs->_u._kids[1];
			if (IR_OP(lrhs->_op) == IR_CONST) {
				lrhs = cc_expr_add(lrhs->_ty, lrhs, cc_expr_cast(lrhs->_ty, rhs, loc, where), loc, where);
				return cc_expr_sub(ty, llhs, lrhs, loc, where);
			}
		}
	}

	if (!(tree = cc_expr_new(where))) { return NULL; }

	code0 = cc_ir_typecode(ty);
	code1 = cc_ir_typecode(rhs->_ty);
	tree->_op = IR_MKOP2(IR_SUB, code0, code1);
	tree->_ty = ty;
	tree->_loc = loc ? *loc : lhs->_loc;
	tree->_u._kids[0] = lhs;
	tree->_u._kids[1] = rhs;

	return tree;
}

FCCIRTree* cc_expr_mul(FCCType* ty, FCCIRTree* lhs, FCCIRTree* rhs, FLocation* loc, enum EMMArea where)
{
	FCCIRTree* tree;
	int code0, code1;

	if (IR_OP(lhs->_op) == IR_CONST && IR_OP(rhs->_op) == IR_CONST) {
		return cc_fold_constant('*', ty, lhs, rhs, loc, where);
	}

	if (!(tree = cc_expr_new(where))) { return NULL; }

	code0 = code1 = cc_ir_typecode(ty);
	tree->_op = IR_MKOP2(IR_MUL, code0, code0);
	tree->_ty = ty;
	tree->_loc = loc ? *loc : lhs->_loc;
	tree->_u._kids[0] = lhs;
	tree->_u._kids[1] = rhs;

	return tree;
}

FCCIRTree* cc_expr_div(FCCType* ty, FCCIRTree* lhs, FCCIRTree* rhs, FLocation* loc, enum EMMArea where)
{
	FCCIRTree* tree;
	int code0, code1;

	if (IR_OP(lhs->_op) == IR_CONST && IR_OP(rhs->_op) == IR_CONST) {
		if (cc_expr_iszero(rhs)) {
			logger_output_s("error: divisor is 0 at %w\n", loc);
			return NULL;
		}
		return cc_fold_constant('/', ty, lhs, rhs, loc, where);
	}

	if (!(tree = cc_expr_new(where))) { return NULL; }

	code0 = code1 = cc_ir_typecode(ty);
	tree->_op = IR_MKOP2(IR_DIV, code0, code0);
	tree->_ty = ty;
	tree->_loc = loc ? *loc : lhs->_loc;
	tree->_u._kids[0] = lhs;
	tree->_u._kids[1] = rhs;

	return tree;
}

FCCIRTree* cc_expr_mod(FCCType* ty, FCCIRTree* lhs, FCCIRTree* rhs, FLocation* loc, enum EMMArea where)
{
	FCCIRTree* tree;
	int code0, code1;

	if (!IsInt(ty))
	{
		logger_output_s("error integer operands is expected for '%', at %w\n", loc);
		return NULL;
	}
	if (IR_OP(lhs->_op) == IR_CONST && IR_OP(rhs->_op) == IR_CONST) {
		if (cc_expr_iszero(rhs)) {
			logger_output_s("error: divisor is 0 at %w\n", loc);
			return NULL;
		}
		return cc_fold_constant('%', ty, lhs, rhs, loc, where);
	}

	if (!(tree = cc_expr_new(where))) { return NULL; }

	code0 = code1 = cc_ir_typecode(ty);
	tree->_op = IR_MKOP2(IR_MOD, code0, code0);
	tree->_ty = ty;
	tree->_loc = loc ? *loc : lhs->_loc;
	tree->_u._kids[0] = lhs;
	tree->_u._kids[1] = rhs;

	return tree;
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

		if (IsInt(ty) && ty->_size != gbuiltintypes._sinttype->_size)
		{
			args[k] = cc_expr_cast(cc_type_promote(ty), args[k], NULL, where);
		} else {
			args[k] = cc_expr_cast(ty, args[k], NULL, where);
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
	FCCIRTree* tree;
	FCCType* rty;
	int code0;

	/* check arguments */
	if (!cc_expr_checkarguments(fty, args->_data, where)) {
		logger_output_s("error: function parameters checking failed at %w\n", loc);
		return NULL;
	}

	if (!(tree = cc_expr_new(where))) {
		return NULL;
	}

	rty = fty->_type;
	code0 = cc_ir_typecode(rty);
	tree->_op = IR_MKOP1(IR_CALL, code0);
	tree->_ty = rty;
	tree->_loc = loc ? *loc : expr->_loc;
	tree->_u._f._lhs = expr;
	tree->_u._f._args = args->_data;

	if (code0 == IR_BLK) {
		FCCSymbol* rettmp = cc_symbol_temporary(rty, SC_Auto);
		tree->_u._f._ret = cc_expr_addr(cc_expr_id(rettmp, &tree->_loc, where), &tree->_loc, where);
	}

	return tree;
}

FCCIRTree* cc_expr_seq(FCCType* ty, FCCIRTree* expr0, FCCIRTree* expr1, FLocation* loc, enum EMMArea where)
{
	FCCIRTree* tree;
	int code0;

	if (!(tree = cc_expr_new(where))) {
		return NULL;
	}

	code0 = cc_ir_typecode(ty);
	tree->_op = IR_MKOP1(IR_SEQ, code0);
	tree->_ty = ty;
	tree->_loc = loc ? *loc : expr1->_loc;
	tree->_u._kids[0] = expr0;
	tree->_u._kids[1] = expr1;

	tree->_field = expr1->_field;
	tree->_isbitfield = expr1->_isbitfield;
	return tree;
}

FCCIRTree* cc_expr_neg(FCCType* ty, FCCIRTree* expr, FLocation* loc, enum EMMArea where)
{
	FCCIRTree* tree;
	int tycode;

	tycode = cc_ir_typecode(ty);
	if (IR_OP(expr->_op) == IR_CONST)
	{
		switch (tycode)
		{
		case IR_S8:
			return cc_expr_constant(ty, tycode, loc, where, (int8_t)(-expr->_u._val._sint));
		case IR_S16:
			return cc_expr_constant(ty, tycode, loc, where, (int16_t)(-expr->_u._val._sint));
		case IR_S32:
			return cc_expr_constant(ty, tycode, loc, where, (int32_t)(-expr->_u._val._sint));
		case IR_S64:
			return cc_expr_constant(ty, tycode, loc, where, (int64_t)(-expr->_u._val._sint));
		case IR_U8:
			return cc_expr_constant(ty, tycode, loc, where, (uint8_t)(-(int64_t)expr->_u._val._uint));
		case IR_U16:
			return cc_expr_constant(ty, tycode, loc, where, (uint16_t)(-(int64_t)expr->_u._val._uint));
		case IR_U32:
			return cc_expr_constant(ty, tycode, loc, where, (uint32_t)(-(int64_t)expr->_u._val._uint));
		case IR_U64:
			return cc_expr_constant(ty, tycode, loc, where, (uint64_t)(-(int64_t)expr->_u._val._uint));
		case IR_F32:
			return cc_expr_constant(ty, tycode, loc, where, (float)(-expr->_u._val._float));
		case IR_F64:
			return cc_expr_constant(ty, tycode, loc, where, (double)(-expr->_u._val._float));
		case IR_PTR:
			return cc_expr_constant(ty, tycode, loc, where, (uint32_t)(-(int32_t)expr->_u._val._pointer));
		default:
			assert(0); break;
		}
	}

	if (!(tree = cc_expr_new(where))) { return NULL; }

	tree->_op = IR_MKOP1(IR_NEG, tycode);
	tree->_ty = ty;
	tree->_loc = loc ? *loc : expr->_loc;
	tree->_u._kids[0] = expr;

	return tree;
}

FCCIRTree* cc_expr_bitcom(FCCType* ty, FCCIRTree* expr, FLocation* loc, enum EMMArea where)
{
	FCCIRTree* tree;
	int tycode;

	tycode = cc_ir_typecode(ty);
	if (IR_OP(expr->_op) == IR_CONST)
	{
		switch (tycode)
		{
		case IR_S8:
			return cc_expr_constant(ty, tycode, loc, where, (int8_t)(~expr->_u._val._sint));
		case IR_S16:
			return cc_expr_constant(ty, tycode, loc, where, (int16_t)(~expr->_u._val._sint));
		case IR_S32:
			return cc_expr_constant(ty, tycode, loc, where, (int32_t)(~expr->_u._val._sint));
		case IR_S64:
			return cc_expr_constant(ty, tycode, loc, where, (int64_t)(~expr->_u._val._sint));
		case IR_U8:
			return cc_expr_constant(ty, tycode, loc, where, (uint8_t)(~expr->_u._val._uint));
		case IR_U16:
			return cc_expr_constant(ty, tycode, loc, where, (uint16_t)(~expr->_u._val._uint));
		case IR_U32:
			return cc_expr_constant(ty, tycode, loc, where, (uint32_t)(~expr->_u._val._uint));
		case IR_U64:
			return cc_expr_constant(ty, tycode, loc, where, (uint64_t)(~expr->_u._val._uint));
		default:
			assert(0); break;
		}
	}

	if (!(tree = cc_expr_new(where))) { return NULL; }

	tree->_op = IR_MKOP1(IR_BCOM, tycode);
	tree->_ty = ty;
	tree->_loc = loc ? *loc : expr->_loc;
	tree->_u._kids[0] = expr;

	return tree;
}

FCCIRTree* cc_expr_not(FCCType* ty, FCCIRTree* expr, FLocation* loc, enum EMMArea where)
{
	FCCIRTree* tree;
	int tycode;

	if (!(expr = cc_expr_bool(gbuiltintypes._sinttype, expr, &expr->_loc, where))) {
		return NULL;
	}

	if (!(tree = cc_expr_new(where))) { return NULL; }

	tycode = cc_ir_typecode(ty);
	tree->_op = IR_MKOP1(IR_NOT, tycode);
	tree->_ty = ty;
	tree->_loc = loc ? *loc : expr->_loc;
	tree->_u._kids[0] = expr;

	return tree;
}

FCCIRTree* cc_expr_lshift(FCCType* ty, FCCIRTree* lhs, FCCIRTree* rhs, FLocation* loc, enum EMMArea where)
{
	FCCIRTree* tree;
	int code0, code1;

	code0 = cc_ir_typecode(ty);
	code1 = cc_ir_typecode(rhs->_ty);
	if (IR_OP(lhs->_op) == IR_CONST && IR_OP(rhs->_op) == IR_CONST)
	{
		switch (code0)
		{
		case IR_S8:
			return cc_expr_constant(ty, code0, loc, where, (int8_t)(lhs->_u._val._sint << rhs->_u._val._sint));
		case IR_S16:
			return cc_expr_constant(ty, code0, loc, where, (int16_t)(lhs->_u._val._sint << rhs->_u._val._sint));
		case IR_S32:
			return cc_expr_constant(ty, code0, loc, where, (int32_t)(lhs->_u._val._sint << rhs->_u._val._sint));
		case IR_S64:
			return cc_expr_constant(ty, code0, loc, where, (int64_t)(lhs->_u._val._sint << rhs->_u._val._sint));
		case IR_U8:
			return cc_expr_constant(ty, code0, loc, where, (uint8_t)(lhs->_u._val._uint << rhs->_u._val._sint));
		case IR_U16:
			return cc_expr_constant(ty, code0, loc, where, (uint16_t)(lhs->_u._val._uint << rhs->_u._val._sint));
		case IR_U32:
			return cc_expr_constant(ty, code0, loc, where, (uint32_t)(lhs->_u._val._uint << rhs->_u._val._sint));
		case IR_U64:
			return cc_expr_constant(ty, code0, loc, where, (uint64_t)(lhs->_u._val._uint << rhs->_u._val._sint));
		default:
			assert(0); break;
		}
	}

	if (!(tree = cc_expr_new(where))) { return NULL; }

	tree->_op = IR_MKOP2(IR_LSHIFT, code0, code1);
	tree->_ty = ty;
	tree->_loc = loc ? *loc : lhs->_loc;
	tree->_u._kids[0] = lhs;
	tree->_u._kids[1] = rhs;

	return tree;
}

FCCIRTree* cc_expr_rshift(FCCType* ty, FCCIRTree* lhs, FCCIRTree* rhs, FLocation* loc, enum EMMArea where)
{
	FCCIRTree* tree;
	int code0, code1;

	code0 = cc_ir_typecode(ty);
	code1 = cc_ir_typecode(rhs->_ty);
	if (IR_OP(lhs->_op) == IR_CONST && IR_OP(rhs->_op) == IR_CONST)
	{
		switch (code0)
		{
		case IR_S8:
			return cc_expr_constant(ty, code0, loc, where, (int8_t)(lhs->_u._val._sint >> rhs->_u._val._sint));
		case IR_S16:
			return cc_expr_constant(ty, code0, loc, where, (int16_t)(lhs->_u._val._sint >> rhs->_u._val._sint));
		case IR_S32:
			return cc_expr_constant(ty, code0, loc, where, (int32_t)(lhs->_u._val._sint >> rhs->_u._val._sint));
		case IR_S64:
			return cc_expr_constant(ty, code0, loc, where, (int64_t)(lhs->_u._val._sint >> rhs->_u._val._sint));
		case IR_U8:
			return cc_expr_constant(ty, code0, loc, where, (uint8_t)(lhs->_u._val._uint >> rhs->_u._val._sint));
		case IR_U16:
			return cc_expr_constant(ty, code0, loc, where, (uint16_t)(lhs->_u._val._uint >> rhs->_u._val._sint));
		case IR_U32:
			return cc_expr_constant(ty, code0, loc, where, (uint32_t)(lhs->_u._val._uint >> rhs->_u._val._sint));
		case IR_U64:
			return cc_expr_constant(ty, code0, loc, where, (uint64_t)(lhs->_u._val._uint >> rhs->_u._val._sint));
		default:
			assert(0); break;
		}
	}

	if (!(tree = cc_expr_new(where))) { return NULL; }

	tree->_op = IR_MKOP2(IR_RSHIFT, code0, code1);
	tree->_ty = ty;
	tree->_loc = loc ? *loc : lhs->_loc;
	tree->_u._kids[0] = lhs;
	tree->_u._kids[1] = rhs;

	return tree;
}

static FCCIRTree* cc_fold_constant2(int op, FCCType* ty, FCCIRTree* lhs, FCCIRTree* rhs, FLocation* loc, enum EMMArea where)
{
	/* begin define */
	#define XX_CALC_CNST(op)\
		switch (code1)			\
		{						\
		case IR_S8:				\
		case IR_S16:			\
		case IR_S32:			\
		case IR_S64:			\
			return cc_expr_constant(ty, code0, loc, where, (int32_t)(lhs->_u._val._sint op rhs->_u._val._sint)); \
		case IR_U8:				\
		case IR_U16:			\
		case IR_U32:			\
		case IR_U64:			\
			return cc_expr_constant(ty, code0, loc, where, (int32_t)(lhs->_u._val._uint op rhs->_u._val._uint)); \
		case IR_F32:			\
		case IR_F64:			\
			return cc_expr_constant(ty, code0, loc, where, (int32_t)(lhs->_u._val._float op rhs->_u._val._float)); \
		case IR_PTR:			\
			return cc_expr_constant(ty, code0, loc, where, (int32_t)(lhs->_u._val._pointer op rhs->_u._val._pointer)); \
		default:				\
			assert(0); break;	\
		}
	/* end define */

	int code0, code1;

	code0 = cc_ir_typecode(ty);
	code1 = cc_ir_typecode(lhs->_ty);
	
	switch (op)
	{
	case '<':
		XX_CALC_CNST(<);
		break;
	case '<=':
		XX_CALC_CNST(<=);
		break;
	case '>':
		XX_CALC_CNST(>);
		break;
	case '>=':
		XX_CALC_CNST(>=);
		break;
	case '==':
		XX_CALC_CNST(==);
		break;
	case '!=':
		XX_CALC_CNST(!=);
		break;
	}

	assert(0);
	return NULL;

#undef XX_CALC_CNST
}

FCCIRTree* cc_expr_less(FCCType* ty, FCCIRTree* lhs, FCCIRTree* rhs, FLocation* loc, enum EMMArea where)
{
	FCCIRTree* tree;
	int code0, code1;

	code0 = cc_ir_typecode(ty);
	code1 = cc_ir_typecode(lhs->_ty);
	if (IR_OP(lhs->_op) == IR_CONST && IR_OP(rhs->_op) == IR_CONST)
	{
		return cc_fold_constant2('<', ty, lhs, rhs, loc, where);
	}

	if (!(tree = cc_expr_new(where))) { return NULL; }

	tree->_op = IR_MKOP2(IR_LESS, code0, code1);
	tree->_ty = ty;
	tree->_loc = loc ? *loc : lhs->_loc;
	tree->_u._kids[0] = lhs;
	tree->_u._kids[1] = rhs;

	return tree;
}

FCCIRTree* cc_expr_lequal(FCCType* ty, FCCIRTree* lhs, FCCIRTree* rhs, FLocation* loc, enum EMMArea where)
{
	FCCIRTree* tree;
	int code0, code1;

	code0 = cc_ir_typecode(ty);
	code1 = cc_ir_typecode(lhs->_ty);
	if (IR_OP(lhs->_op) == IR_CONST && IR_OP(rhs->_op) == IR_CONST)
	{
		return cc_fold_constant2('<=', ty, lhs, rhs, loc, where);
	}

	if (!(tree = cc_expr_new(where))) { return NULL; }

	tree->_op = IR_MKOP2(IR_LEQ, code0, code1);
	tree->_ty = ty;
	tree->_loc = loc ? *loc : lhs->_loc;
	tree->_u._kids[0] = lhs;
	tree->_u._kids[1] = rhs;

	return tree;
}

FCCIRTree* cc_expr_great(FCCType* ty, FCCIRTree* lhs, FCCIRTree* rhs, FLocation* loc, enum EMMArea where)
{
	FCCIRTree* tree;
	int code0, code1;

	code0 = cc_ir_typecode(ty);
	code1 = cc_ir_typecode(lhs->_ty);
	if (IR_OP(lhs->_op) == IR_CONST && IR_OP(rhs->_op) == IR_CONST)
	{
		return cc_fold_constant2('>', ty, lhs, rhs, loc, where);
	}

	if (!(tree = cc_expr_new(where))) { return NULL; }

	tree->_op = IR_MKOP2(IR_GREAT, code0, code1);
	tree->_ty = ty;
	tree->_loc = loc ? *loc : lhs->_loc;
	tree->_u._kids[0] = lhs;
	tree->_u._kids[1] = rhs;

	return tree;
}

FCCIRTree* cc_expr_gequal(FCCType* ty, FCCIRTree* lhs, FCCIRTree* rhs, FLocation* loc, enum EMMArea where)
{
	FCCIRTree* tree;
	int code0, code1;

	code0 = cc_ir_typecode(ty);
	code1 = cc_ir_typecode(lhs->_ty);
	if (IR_OP(lhs->_op) == IR_CONST && IR_OP(rhs->_op) == IR_CONST)
	{
		return cc_fold_constant2('>=', ty, lhs, rhs, loc, where);
	}

	if (!(tree = cc_expr_new(where))) { return NULL; }

	tree->_op = IR_MKOP2(IR_GEQ, code0, code1);
	tree->_ty = ty;
	tree->_loc = loc ? *loc : lhs->_loc;
	tree->_u._kids[0] = lhs;
	tree->_u._kids[1] = rhs;

	return tree;
}

FCCIRTree* cc_expr_equal(FCCType* ty, FCCIRTree* lhs, FCCIRTree* rhs, FLocation* loc, enum EMMArea where)
{
	FCCIRTree* tree;
	int code0, code1;

	code0 = cc_ir_typecode(ty);
	code1 = cc_ir_typecode(lhs->_ty);
	if (IR_OP(lhs->_op) == IR_CONST && IR_OP(rhs->_op) == IR_CONST)
	{
		return cc_fold_constant2('==', ty, lhs, rhs, loc, where);
	}

	if (!(tree = cc_expr_new(where))) { return NULL; }

	tree->_op = IR_MKOP2(IR_EQUAL, code0, code1);
	tree->_ty = ty;
	tree->_loc = loc ? *loc : lhs->_loc;
	tree->_u._kids[0] = lhs;
	tree->_u._kids[1] = rhs;

	return tree;
}

FCCIRTree* cc_expr_unequal(FCCType* ty, FCCIRTree* lhs, FCCIRTree* rhs, FLocation* loc, enum EMMArea where)
{
	FCCIRTree* tree;
	int code0, code1;

	code0 = cc_ir_typecode(ty);
	code1 = cc_ir_typecode(lhs->_ty);
	if (IR_OP(lhs->_op) == IR_CONST && IR_OP(rhs->_op) == IR_CONST)
	{
		return cc_fold_constant2('!=', ty, lhs, rhs, loc, where);
	}

	if (!(tree = cc_expr_new(where))) { return NULL; }

	tree->_op = IR_MKOP2(IR_UNEQ, code0, code1);
	tree->_ty = ty;
	tree->_loc = loc ? *loc : lhs->_loc;
	tree->_u._kids[0] = lhs;
	tree->_u._kids[1] = rhs;

	return tree;
}

static FCCIRTree* cc_fold_constant3(int op, FCCType* ty, FCCIRTree* lhs, FCCIRTree* rhs, FLocation* loc, enum EMMArea where)
{
	/* begin define */
#define XX_CALC_CNST(tycode, op)\
		switch (tycode)			\
		{						\
		case IR_S8:				\
		case IR_S16:			\
		case IR_S32:			\
		case IR_S64:			\
			ivalue = lhs->_u._val._sint op rhs->_u._val._sint;	break;\
		case IR_U8:				\
		case IR_U16:			\
		case IR_U32:			\
		case IR_U64:			\
			uvalue = lhs->_u._val._uint op rhs->_u._val._uint;	break;\
		default:				\
			assert(0); break;	\
		}
	/* end define */

	int64_t ivalue;
	uint64_t uvalue;

	int tycode = cc_ir_typecode(ty);
	switch (op)
	{
	case '&':
		XX_CALC_CNST(tycode, &);
		break;
	case '|':
		XX_CALC_CNST(tycode, |);
		break;
	case '^':
		XX_CALC_CNST(tycode, ^);
		break;
	}

	switch (tycode)
	{
	case IR_S8:
		return cc_expr_constant(ty, tycode, loc, where, (int8_t)ivalue);
	case IR_S16:
		return cc_expr_constant(ty, tycode, loc, where, (int16_t)ivalue);
	case IR_S32:
		return cc_expr_constant(ty, tycode, loc, where, (int32_t)ivalue);
	case IR_S64:
		return cc_expr_constant(ty, tycode, loc, where, (int64_t)ivalue);
	case IR_U8:
		return cc_expr_constant(ty, tycode, loc, where, (uint8_t)uvalue);
	case IR_U16:
		return cc_expr_constant(ty, tycode, loc, where, (uint16_t)uvalue);
	case IR_U32:
		return cc_expr_constant(ty, tycode, loc, where, (uint32_t)uvalue);
	case IR_U64:
		return cc_expr_constant(ty, tycode, loc, where, (uint64_t)uvalue);
	default:
		assert(0); break;
	}

	return NULL;

#undef XX_CAST_CNST
}

FCCIRTree* cc_expr_bitand(FCCType* ty, FCCIRTree* lhs, FCCIRTree* rhs, FLocation* loc, enum EMMArea where)
{
	FCCIRTree* tree;
	int code0;

	code0 = cc_ir_typecode(ty);
	if (IR_OP(lhs->_op) == IR_CONST && IR_OP(rhs->_op) == IR_CONST)
	{
		return cc_fold_constant3('&', ty, lhs, rhs, loc, where);
	}

	if (!(tree = cc_expr_new(where))) { return NULL; }

	tree->_op = IR_MKOP2(IR_BITAND, code0, code0);
	tree->_ty = ty;
	tree->_loc = loc ? *loc : lhs->_loc;
	tree->_u._kids[0] = lhs;
	tree->_u._kids[1] = rhs;

	return tree;
}

FCCIRTree* cc_expr_bitxor(FCCType* ty, FCCIRTree* lhs, FCCIRTree* rhs, FLocation* loc, enum EMMArea where)
{
	FCCIRTree* tree;
	int code0;

	code0 = cc_ir_typecode(ty);
	if (IR_OP(lhs->_op) == IR_CONST && IR_OP(rhs->_op) == IR_CONST)
	{
		return cc_fold_constant3('^', ty, lhs, rhs, loc, where);
	}

	if (!(tree = cc_expr_new(where))) { return NULL; }

	tree->_op = IR_MKOP2(IR_BITXOR, code0, code0);
	tree->_ty = ty;
	tree->_loc = loc ? *loc : lhs->_loc;
	tree->_u._kids[0] = lhs;
	tree->_u._kids[1] = rhs;

	return tree;
}

FCCIRTree* cc_expr_bitor(FCCType* ty, FCCIRTree* lhs, FCCIRTree* rhs, FLocation* loc, enum EMMArea where)
{
	FCCIRTree* tree;
	int code0;

	code0 = cc_ir_typecode(ty);
	if (IR_OP(lhs->_op) == IR_CONST && IR_OP(rhs->_op) == IR_CONST)
	{
		return cc_fold_constant3('|', ty, lhs, rhs, loc, where);
	}

	if (!(tree = cc_expr_new(where))) { return NULL; }

	tree->_op = IR_MKOP2(IR_BITOR, code0, code0);
	tree->_ty = ty;
	tree->_loc = loc ? *loc : lhs->_loc;
	tree->_u._kids[0] = lhs;
	tree->_u._kids[1] = rhs;

	return tree;
}

FCCIRTree* cc_expr_logicand(FCCType* ty, FCCIRTree* lhs, FCCIRTree* rhs, FLocation* loc, enum EMMArea where)
{
	FCCIRTree* tree;
	int code0;

	code0 = cc_ir_typecode(ty);
	if (!(lhs = cc_expr_bool(gbuiltintypes._sinttype, lhs, &lhs->_loc, where))) {
		return NULL;
	}
	if (!(rhs = cc_expr_bool(gbuiltintypes._sinttype, rhs, &lhs->_loc, where))) {
		return NULL;
	}
	if (IR_OP(lhs->_op) == IR_CONST && IR_OP(rhs->_op) == IR_CONST)
	{
		int r = lhs->_u._val._sint && rhs->_u._val._sint;
		return cc_expr_constant(ty, code0, loc, where, r);
	}

	if (!(tree = cc_expr_new(where))) {
		return NULL;
	}

	tree->_op = IR_MKOP2(IR_LOGAND, code0, cc_ir_typecode(gbuiltintypes._sinttype));
	tree->_ty = ty;
	tree->_loc = loc ? *loc : lhs->_loc;
	tree->_u._kids[0] = lhs;
	tree->_u._kids[1] = rhs;

	return tree;
}

FCCIRTree* cc_expr_logicor(FCCType* ty, FCCIRTree* lhs, FCCIRTree* rhs, FLocation* loc, enum EMMArea where)
{
	FCCIRTree* tree;
	int code0;

	code0 = cc_ir_typecode(ty);
	if (!(lhs = cc_expr_bool(gbuiltintypes._sinttype, lhs, &lhs->_loc, where))) {
		return NULL;
	}
	if (!(rhs = cc_expr_bool(gbuiltintypes._sinttype, rhs, &lhs->_loc, where))) {
		return NULL;
	}

	if (IR_OP(lhs->_op) == IR_CONST && IR_OP(rhs->_op) == IR_CONST)
	{
		int r = lhs->_u._val._sint || rhs->_u._val._sint;
		return cc_expr_constant(ty, code0, loc, where, r);
	}

	if (!(tree = cc_expr_new(where))) {
		return NULL;
	}

	tree->_op = IR_MKOP2(IR_LOGOR, code0, cc_ir_typecode(gbuiltintypes._sinttype));
	tree->_ty = ty;
	tree->_loc = loc ? *loc : lhs->_loc;
	tree->_u._kids[0] = lhs;
	tree->_u._kids[1] = rhs;

	return tree;
}

FCCIRTree* cc_expr_condition(FCCIRTree* expr0, FCCIRTree* expr1, FCCIRTree* expr2, FLocation *loc, enum EMMArea where)
{
	FCCType* ty, *xty = expr1->_ty, *yty = expr2->_ty;
	FCCIRTree* tree;
	FCCSymbol* tmp;
	int code0;

	expr0 = cc_expr_bool(gbuiltintypes._sinttype, expr0, &expr0->_loc, where);
	if (!expr0) { return NULL; }

	xty = UnQual(xty);
	yty = UnQual(yty);
	if (IsArith(xty) && IsArith(yty))
		ty = cc_type_select(xty, yty);
	else if (cc_type_isequal(xty, yty, TRUE))
		ty = UnQual(xty);
	else if ((IsPtr(xty) && IsPtr(yty)
		&& cc_type_isequal(UnQual(xty->_type), UnQual(yty->_type), TRUE)))
		ty = xty;
	else if (IsPtr(xty) && cc_expr_is_nullptr(expr2))
		ty = xty;
	else if (cc_expr_is_nullptr(expr1) && IsPtr(yty))
		ty = yty;
	else if (IsPtr(xty) && !IsFunction(xty->_type) && IsVoidptr(yty)
		|| IsPtr(yty) && !IsFunction(yty->_type) && IsVoidptr(xty))
		ty = gbuiltintypes._voidtype;
	else {
		logger_output_s("error: invalid operands for '?:' '%t' : '%t' at %w\n", xty, yty, loc);
		return NULL;
	}

	if (IsPtr(ty)) {
		ty = UnQual(UnQual(ty)->_type);
		if (IsPtr(xty) && IsConst(UnQual(xty)->_type)
			|| IsPtr(yty) && IsConst(UnQual(yty)->_type))
			ty = cc_type_qual(ty, Type_Const);
		if (IsPtr(xty) && IsVolatile(UnQual(xty)->_type)
			|| IsPtr(yty) && IsVolatile(UnQual(yty)->_type))
			ty = cc_type_qual(ty, Type_Volatile);
		ty = cc_type_ptr(ty);
	}

	tmp = NULL;
	if (!IsVoid(ty) && ty->_size > 0)
	{
		if (!(tmp = cc_symbol_temporary(ty, SC_Auto))) {
			return NULL;
		}

		expr1 = cc_expr_assign(ty, cc_expr_id(tmp, &expr1->_loc, where), expr1, &expr1->_loc, where);
		expr2 = cc_expr_assign(ty, cc_expr_id(tmp, &expr2->_loc, where), expr2, &expr2->_loc, where);
	}
	if (!(tree = cc_expr_new(where))) {
		return NULL;
	}

	code0 = cc_ir_typecode(ty);
	tree->_op = IR_MKOP1(IR_COND, code0);
	tree->_ty = ty;
	tree->_loc = loc ? *loc : expr0->_loc;
	tree->_u._kids[0] = expr0;
	tree->_u._kids[1] = expr1;
	tree->_u._kids[2] = expr2;

	if (tmp) {
		tree = cc_expr_seq(tree->_ty, tree, cc_expr_id(tmp, NULL, where), &tree->_loc, where);
	}
	return tree;
}

FCCIRTree* cc_expr_bool(FCCType* ty, FCCIRTree* expr, FLocation* loc, enum EMMArea where)
{
	FCCIRTree *zero, *right;
	int op;

	right = cc_expr_right(expr);
	op = IR_OP(right->_op);
	if (op == IR_LOGAND || op == IR_LOGOR
		|| op == IR_EQUAL || op == IR_UNEQ
		|| op == IR_LESS || op == IR_GREAT
		|| op == IR_LEQ || op == IR_GEQ
		|| op == IR_NOT)
	{
		return expr;
	}

	expr = cc_expr_value(expr, where);
	expr = cc_expr_adjust(expr, where);
	if (!IsScalar(expr->_ty)) {
		logger_output_s("error: can't convert to bool expression '%t' at %w\n", expr->_ty, loc);
		return NULL;
	}

	zero = cc_expr_constant(gbuiltintypes._sinttype, cc_ir_typecode(gbuiltintypes._sinttype), loc, where, 0);
	zero = cc_expr_cast_constant(expr->_ty, zero, loc, where);
	return cc_expr_unequal(ty, expr, zero, loc, where);
}

FCCIRTree* cc_expr_right(FCCIRTree* expr)
{
	FCCIRTree* right = expr;

	while(IR_OP(right->_op) == IR_SEQ)
	{
		right = right->_u._kids[1];
	}

	return right;
}

static FCCIRTree* cc_expr_erase_bitfield(FCCIRTree* expr, enum EMMArea where)
{
	FCCIRTree* tree;

	if (!(tree = cc_expr_new(where))) {
		return NULL;
	}

	*tree = *expr;
	tree->_field = NULL;
	tree->_isbitfield = 0;
	return tree;
}

static FCCIRTree* cc_expr_read_bitvalue(FCCIRTree* expr, enum EMMArea where)
{
	FCCIRTree* t0, *t1, *t2;
	FCCField* field;
	FCCType* ty;
	int bitoffset, shlcnt, shrcnt;
	
	assert(expr->_isbitfield);
	field = expr->_field;
	bitoffset = field->_lsb - 1;

	/* 1. t1 = t0 << shlcnt
	 * 2. t2 = t1 >> shrcnt
	 */
	if (!(t0 = cc_expr_erase_bitfield(expr, where))) {
		return NULL;
	}
	ty = cc_type_promote(t0->_ty);
	shlcnt = ty->_size * 8 - (field->_bitsize + bitoffset);
	shrcnt = ty->_size * 8 - field->_bitsize;

	t1 = t0 = cc_expr_cast(ty, t0, &t0->_loc, where);
	if (shlcnt > 0 && !(t1 = cc_expr_lshift(ty, t0,
		cc_expr_constant(gbuiltintypes._sinttype, cc_ir_typecode(gbuiltintypes._sinttype), NULL, where, shlcnt), NULL, where)))
	{
		return NULL;
	}
	if (!(t2 = cc_expr_rshift(ty, t1,
		cc_expr_constant(gbuiltintypes._sinttype, cc_ir_typecode(gbuiltintypes._sinttype), NULL, where, shrcnt), NULL, where)))
	{
		return NULL;
	}
	
	return t2;
}

FCCIRTree* cc_expr_value(FCCIRTree* expr, enum EMMArea where)
{
	FCCIRTree* right, ** replace;
	int op;

	right = expr;
	replace = NULL;
	while (IR_OP(right->_op) == IR_SEQ)
	{
		replace = &(right->_u._kids[1]);
		right = *replace;
	}

	op = IR_OP(right->_op);
	if (op == IR_ASSIGN)
	{
		FCCIRTree *val; 

		if (!(val = cc_expr_indir(right->_u._kids[0], NULL, where)))
		{
			return NULL;
		}
		val->_field = right->_field;
		val->_isbitfield = right->_isbitfield;
		if (!(expr = cc_expr_seq(val->_ty, expr, val, &right->_loc, where)))
		{
			return NULL;
		}
	}
	else if (op == IR_LOGAND || op == IR_LOGOR || op == IR_EQUAL
		|| op == IR_UNEQ || op == IR_LESS || op == IR_GREAT || op == IR_LEQ
		|| op == IR_GEQ || op == IR_NOT)
	{
		FCCIRTree* cond;
		int tycode;

		tycode = cc_ir_typecode(gbuiltintypes._sinttype);
		cond = cc_expr_condition(right, cc_expr_constant(gbuiltintypes._sinttype, tycode, &right->_loc, where, 1),
					cc_expr_constant(gbuiltintypes._sinttype, tycode, &right->_loc, where, 0), &right->_loc, where);
		if (replace) {
			*replace = cond;
		}
		else {
			expr = cond;
		}
	}

	if (expr->_isbitfield) {
		return cc_expr_read_bitvalue(expr, where);
	}

	return expr;
}

FCCIRTree* cc_expr_assign(FCCType* ty, FCCIRTree* lhs, FCCIRTree* rhs, FLocation* loc, enum EMMArea where)
{
	FCCIRTree* asgntree, *addr, *right;
	FCCField* field;
	int isbitfield, code0;


	if (!(addr = cc_expr_addr(lhs, loc, where))) {
		return NULL;
	}

	/* check if assign block value from call() */
	right = cc_expr_right(rhs);
	if (IR_OP(right->_op) == IR_CALL && IR_OPTY0(right->_op) == IR_BLK)
	{
		FCCIRTree* retaddr = right->_u._f._ret;
		
		if (!retaddr || (IsAddrOp(retaddr->_op) && retaddr->_symbol->_temporary))
		{
			right->_u._f._ret = addr;
			return rhs;
		}
	}

	field = lhs->_field;
	isbitfield = lhs->_isbitfield;
	/* check if bit-field */
	if (isbitfield)
	{
		FCCIRTree *t, *t0, *t1, *t2, *t3;
		int32_t bitmask0, bitmask1, bitoffset;
		FCCType* promotety;

		bitoffset = field->_lsb - 1;
		bitmask0 = (1 << field->_bitsize) - 1;
		bitmask1 = ~(bitmask0 << bitoffset);

		if (!(t = cc_expr_erase_bitfield(lhs, where))) {
			return NULL;
		}
		promotety = cc_type_promote(t->_ty);
		t = cc_expr_cast(promotety, t, loc, where);
		rhs = cc_expr_cast(promotety, rhs, loc, where);

		/* 1. t0 = rhs & bitmask0
		 * 2. t1 = t0 << bitoffset
		 * 3. t2 = t & bitmask1
		 * 4. t3 = t1 | t2
		 */
		if (!(t0 = cc_expr_bitand(promotety, rhs,
			cc_expr_constant(gbuiltintypes._sinttype, cc_ir_typecode(gbuiltintypes._sinttype), NULL, where, bitmask0), loc, where)))
		{
			return NULL;
		}
		if (!(t1 = cc_expr_lshift(promotety, t0,
			cc_expr_constant(gbuiltintypes._sinttype, cc_ir_typecode(gbuiltintypes._sinttype), NULL, where, bitoffset), loc, where)))
		{
			return NULL;
		}
		if (!(t2 = cc_expr_bitand(promotety, t,
			cc_expr_constant(gbuiltintypes._sinttype, cc_ir_typecode(gbuiltintypes._sinttype), NULL, where, bitmask1), loc, where)))
		{
			return NULL;
		}
		if (!(t3 = cc_expr_bitor(promotety, t1, t2, loc, where)) || !(rhs = cc_expr_cast(lhs->_ty, rhs, loc, where)))
		{
			return NULL;
		}

		rhs = t3;
	}

	if (!(asgntree = cc_expr_new(where))) {
		return NULL;
	}
	
	rhs = cc_expr_cast(ty, rhs, loc, where);
	code0 = cc_ir_typecode(ty);
	asgntree->_op = IR_MKOP1(IR_ASSIGN, code0);
	asgntree->_ty = ty;
	asgntree->_loc = loc ? *loc : lhs->_loc;
	asgntree->_u._kids[0] = addr;
	asgntree->_u._kids[1] = rhs;
	asgntree->_field = field;
	asgntree->_isbitfield = isbitfield;

	return asgntree;
}

static void cc_expr_internaldisplay(FCCIRTree* expr, int depth, int maxdepth);
void cc_expr_display(FCCIRTree* expr, int maxdepth)
{
	cc_expr_internaldisplay(expr, 0, maxdepth);
}

static void cc_expr_internaldisplay(FCCIRTree* expr, int depth, int maxdepth)
{
	int op;
	
	if (!expr || (maxdepth >= 0 && depth > maxdepth))
	{
		return;
	}

	if (expr->_dagnode) 
	{
		logger_output_s("DAG(0x%p)_", expr->_dagnode);
	}

	depth++;
	op = IR_OP(expr->_op);
	switch (op)
	{
	case IR_CONST:
		logger_output_s(" CONST %d", expr->_u._val._sint); break;
	case IR_ASSIGN:
		logger_output_s("(");
		cc_expr_internaldisplay(expr->_u._kids[0], depth, maxdepth);
		logger_output_s(")");
		logger_output_s("="); 
		logger_output_s("(");
		cc_expr_internaldisplay(expr->_u._kids[1], depth, maxdepth);
		logger_output_s(")");
		break;
	case IR_ADD:
		logger_output_s("(");
		cc_expr_internaldisplay(expr->_u._kids[0], depth, maxdepth);
		logger_output_s(")");
		logger_output_s("+");
		logger_output_s("(");
		cc_expr_internaldisplay(expr->_u._kids[1], depth, maxdepth);
		logger_output_s(")");
		break;
	case IR_SUB:
		logger_output_s("(");
		cc_expr_internaldisplay(expr->_u._kids[0], depth, maxdepth);
		logger_output_s(")");
		logger_output_s("-");
		logger_output_s("(");
		cc_expr_internaldisplay(expr->_u._kids[1], depth, maxdepth);
		logger_output_s(")");
		break;
	case IR_MUL:
		logger_output_s("(");
		cc_expr_internaldisplay(expr->_u._kids[0], depth, maxdepth);
		logger_output_s(")");
		logger_output_s("*");
		logger_output_s("(");
		cc_expr_internaldisplay(expr->_u._kids[1], depth, maxdepth);
		logger_output_s(")");
		break;
	case IR_DIV:
		logger_output_s("(");
		cc_expr_internaldisplay(expr->_u._kids[0], depth, maxdepth);
		logger_output_s(")");
		logger_output_s("/");
		logger_output_s("(");
		cc_expr_internaldisplay(expr->_u._kids[1], depth, maxdepth);
		logger_output_s(")");
		break;
	case IR_MOD:
		logger_output_s("(");
		cc_expr_internaldisplay(expr->_u._kids[0], depth, maxdepth);
		logger_output_s(")");
		logger_output_s("%");
		logger_output_s("(");
		cc_expr_internaldisplay(expr->_u._kids[1], depth, maxdepth);
		logger_output_s(")");
		break;
	case IR_LSHIFT:
		logger_output_s("(");
		cc_expr_internaldisplay(expr->_u._kids[0], depth, maxdepth);
		logger_output_s(")");
		logger_output_s("<<");
		logger_output_s("(");
		cc_expr_internaldisplay(expr->_u._kids[1], depth, maxdepth);
		logger_output_s(")");
		break;
	case IR_RSHIFT:
		logger_output_s("(");
		cc_expr_internaldisplay(expr->_u._kids[0], depth, maxdepth);
		logger_output_s(")");
		logger_output_s(">>");
		logger_output_s("(");
		cc_expr_internaldisplay(expr->_u._kids[1], depth, maxdepth);
		logger_output_s(")");
		break;
	case IR_BITAND:
		logger_output_s("(");
		cc_expr_internaldisplay(expr->_u._kids[0], depth, maxdepth);
		logger_output_s(")");
		logger_output_s("&");
		logger_output_s("(");
		cc_expr_internaldisplay(expr->_u._kids[1], depth, maxdepth);
		logger_output_s(")");
		break;
	case IR_BITOR:
		logger_output_s("(");
		cc_expr_internaldisplay(expr->_u._kids[0], depth, maxdepth);
		logger_output_s(")");
		logger_output_s("|");
		logger_output_s("(");
		cc_expr_internaldisplay(expr->_u._kids[1], depth, maxdepth);
		logger_output_s(")");
		break;
	case IR_BITXOR:
		logger_output_s("(");
		cc_expr_internaldisplay(expr->_u._kids[0], depth, maxdepth);
		logger_output_s(")");
		logger_output_s("^");
		logger_output_s("(");
		cc_expr_internaldisplay(expr->_u._kids[1], depth, maxdepth);
		logger_output_s(")");
		break;
	case IR_LOGAND:
		logger_output_s("(");
		cc_expr_internaldisplay(expr->_u._kids[0], depth, maxdepth);
		logger_output_s(")");
		logger_output_s("&&");
		logger_output_s("(");
		cc_expr_internaldisplay(expr->_u._kids[1], depth, maxdepth);
		logger_output_s(")");
		break;
	case IR_LOGOR:
		logger_output_s("(");
		cc_expr_internaldisplay(expr->_u._kids[0], depth, maxdepth);
		logger_output_s(")");
		logger_output_s("||");
		logger_output_s("(");
		cc_expr_internaldisplay(expr->_u._kids[1], depth, maxdepth);
		logger_output_s(")");
		break;
	case IR_EQUAL:
		logger_output_s("(");
		cc_expr_internaldisplay(expr->_u._kids[0], depth, maxdepth);
		logger_output_s(")");
		logger_output_s("==");
		logger_output_s("(");
		cc_expr_internaldisplay(expr->_u._kids[1], depth, maxdepth);
		logger_output_s(")");
		break;
	case IR_UNEQ:
		logger_output_s("(");
		cc_expr_internaldisplay(expr->_u._kids[0], depth, maxdepth);
		logger_output_s(")");
		logger_output_s("!=");
		logger_output_s("(");
		cc_expr_internaldisplay(expr->_u._kids[1], depth, maxdepth);
		logger_output_s(")");
		break;
	case IR_LESS:
		logger_output_s("(");
		cc_expr_internaldisplay(expr->_u._kids[0], depth, maxdepth);
		logger_output_s(")");
		logger_output_s("<");
		logger_output_s("(");
		cc_expr_internaldisplay(expr->_u._kids[1], depth, maxdepth);
		logger_output_s(")");
		break;
	case IR_GREAT:
		logger_output_s("(");
		cc_expr_internaldisplay(expr->_u._kids[0], depth, maxdepth);
		logger_output_s(")");
		logger_output_s(">");
		logger_output_s("(");
		cc_expr_internaldisplay(expr->_u._kids[1], depth, maxdepth);
		logger_output_s(")");
		break;
	case IR_LEQ:
		logger_output_s("(");
		cc_expr_internaldisplay(expr->_u._kids[0], depth, maxdepth);
		logger_output_s(")");
		logger_output_s("<=");
		logger_output_s("(");
		cc_expr_internaldisplay(expr->_u._kids[1], depth, maxdepth);
		logger_output_s(")");
		break;
	case IR_GEQ:
		logger_output_s("(");
		cc_expr_internaldisplay(expr->_u._kids[0], depth, maxdepth);
		logger_output_s(")");
		logger_output_s(">=");
		logger_output_s("(");
		cc_expr_internaldisplay(expr->_u._kids[1], depth, maxdepth);
		logger_output_s(")");
		break;
	case IR_NOT:
		logger_output_s("!");
		logger_output_s("(");
		cc_expr_internaldisplay(expr->_u._kids[0], depth, maxdepth);
		logger_output_s(")");
		break;
	case IR_NEG:
		logger_output_s("-");
		logger_output_s("(");
		cc_expr_internaldisplay(expr->_u._kids[0], depth, maxdepth);
		logger_output_s(")");
		break;
	case IR_BCOM:
		logger_output_s("~");
		logger_output_s("(");
		cc_expr_internaldisplay(expr->_u._kids[0], depth, maxdepth);
		logger_output_s(")");
		break;
	case IR_CVT:
		logger_output_s("CVT");
		logger_output_s("(");
		cc_expr_internaldisplay(expr->_u._kids[0], depth, maxdepth);
		logger_output_s(")");
		break;
	case IR_INDIR:
		logger_output_s("INDIR");
		logger_output_s("(");
		cc_expr_internaldisplay(expr->_u._kids[0], depth, maxdepth);
		logger_output_s(")");
		break;
	case IR_ADDRG:
		logger_output_s("ADDRG %s", expr->_symbol->_name);
		break;
	case IR_ADDRF:
		logger_output_s("ADDRF %s", expr->_symbol->_name);
		break;
	case IR_ADDRL:
		logger_output_s("ADDRL %s", expr->_symbol->_name);
		break;
	case IR_CALL:
		if (expr->_u._f._ret) {
			cc_expr_internaldisplay(expr->_u._f._ret, depth, maxdepth);
			logger_output_s("=");
		}
		logger_output_s("CALL");
		logger_output_s("(");
		cc_expr_internaldisplay(expr->_u._f._lhs, depth, maxdepth);
		logger_output_s(")");
		break;
	case IR_SEQ:
		logger_output_s("(");
		cc_expr_internaldisplay(expr->_u._kids[0], depth, maxdepth);
		logger_output_s(")");
		logger_output_s(", ");
		logger_output_s("(");
		cc_expr_internaldisplay(expr->_u._kids[1], depth, maxdepth);
		logger_output_s(")");
		logger_output_s(", ");
		break;
	case IR_COND:
		logger_output_s("(");
		cc_expr_internaldisplay(expr->_u._kids[0], depth, maxdepth);
		logger_output_s(")");
		logger_output_s("?");
		logger_output_s("(");
		cc_expr_internaldisplay(expr->_u._kids[1], depth, maxdepth);
		logger_output_s(")");
		logger_output_s(":");
		logger_output_s("(");
		cc_expr_internaldisplay(expr->_u._kids[2], depth, maxdepth);
		logger_output_s(")");
		break;
	default:
		logger_output_s("Unkown");
		break;
	}
}
