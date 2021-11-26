/* \brief
 *		expression parsing.
 */

#include "expr.h"
#include "lexer/token.h"
#include "logger.h"
#include "symbols.h"
#include "types.h"
#include "parser.h"
#include <string.h>


static BOOL cc_expr_parse_primary(struct tagCCContext* ctx, FCCExprTree** outexpr, enum EMMArea where);
static BOOL cc_expr_parse_postfix(struct tagCCContext* ctx, FCCExprTree** outexpr, enum EMMArea where);
static BOOL cc_expr_parse_arguments(struct tagCCContext* ctx, FArray* args, enum EMMArea where);
static BOOL cc_expr_parse_unary(struct tagCCContext* ctx, FCCExprTree** outexpr, enum EMMArea where);
static BOOL cc_expr_parse_cast(struct tagCCContext* ctx, FCCExprTree** outexpr, enum EMMArea where);
static BOOL cc_expr_parse_multiplicative(struct tagCCContext* ctx, FCCExprTree** outexpr, enum EMMArea where);
static BOOL cc_expr_parse_additive(struct tagCCContext* ctx, FCCExprTree** outexpr, enum EMMArea where);
static BOOL cc_expr_parse_shift(struct tagCCContext* ctx, FCCExprTree** outexpr, enum EMMArea where);
static BOOL cc_expr_parse_relational(struct tagCCContext* ctx, FCCExprTree** outexpr, enum EMMArea where);
static BOOL cc_expr_parse_equality(struct tagCCContext* ctx, FCCExprTree** outexpr, enum EMMArea where);
static BOOL cc_expr_parse_bitand(struct tagCCContext* ctx, FCCExprTree** outexpr, enum EMMArea where);
static BOOL cc_expr_parse_bitxor(struct tagCCContext* ctx, FCCExprTree** outexpr, enum EMMArea where);
static BOOL cc_expr_parse_bitor(struct tagCCContext* ctx, FCCExprTree** outexpr, enum EMMArea where);
static BOOL cc_expr_parse_logicand(struct tagCCContext* ctx, FCCExprTree** outexpr, enum EMMArea where);
static BOOL cc_expr_parse_logicor(struct tagCCContext* ctx, FCCExprTree** outexpr, enum EMMArea where);
static BOOL cc_expr_parse_conditional(struct tagCCContext* ctx, FCCExprTree** outexpr, enum EMMArea where);


/* ------------------------------------------------------------------------------------------ */
static FCCExprTree* cc_expr_id(struct tagCCContext* ctx, FCCSymbol* p, FLocation loc, enum EMMArea where)
{
	int op;
	FCCExprTree* tree, * tree2;
	FCCType* ty = UnQual(p->_type);

	if (p->_scope == SCOPE_GLOBAL) {
		op = EXPR_ADDRG;
	}
	else if (p->_scope == SCOPE_PARAM) {
		op = EXPR_ADDRF;
	}
	else if (p->_sclass == SC_External || p->_sclass == SC_Static) {
		assert(p->_u._alias);
		p = p->_u._alias;
		op = EXPR_ADDRG;
	}
	else {
		op = EXPR_ADDRL;
	}

	if (IsArray(ty) || IsFunction(ty)) {
		if (!(tree = cc_expr_new(where))) {
			return NULL;
		}
		tree->_op = op;
		tree->_loc = loc;
		tree->_ty = p->_type;
		tree->_u._symbol = p;
		tree->_islvalue = !IsFunction(ty);
		tree->_isconstant = (op == EXPR_ADDRG);
	}
	else {
		if (!(tree = cc_expr_new(where))) {
			return NULL;
		}
		tree->_op = op;
		tree->_loc = loc;
		tree->_ty = cc_type_ptr(p->_type);
		tree->_u._symbol = p;
		tree->_isconstant = (op == EXPR_ADDRG);

		tree = cc_expr_unary_deref(ctx, tree, loc, where);
	}

	return tree;
}

static BOOL cc_expr_parse_primary(struct tagCCContext* ctx, FCCExprTree** outexpr, enum EMMArea where)
{
	FCCExprTree* tree = NULL;
	FCCSymbol* p = NULL;
	FCCType* cnstty = NULL;
	FCCConstVal cnstval;

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
			cnstty = p->_type->_type;
			cnstval._sint = p->_u._cnstval._sint;
		}
		else
		{
			if (!(tree = cc_expr_id(ctx, p, ctx->_currtk._loc, where))) {
				return FALSE;
			}

			cc_read_token(ctx, &ctx->_currtk);
		}
	}
	break;
	case TK_CONSTANT_INT:
		cnstty = gbuiltintypes._sinttype;
		cnstval._sint = ctx->_currtk._val._int;
		break;
	case TK_CONSTANT_UINT:
		cnstty = gbuiltintypes._uinttype;
		cnstval._uint = ctx->_currtk._val._uint;
		break;
	case TK_CONSTANT_LONG:
		cnstty = gbuiltintypes._slongtype;
		cnstval._sint = ctx->_currtk._val._long;
		break;
	case TK_CONSTANT_ULONG:
		cnstty = gbuiltintypes._ulongtype;
		cnstval._uint = ctx->_currtk._val._ulong;
		break;
	case TK_CONSTANT_LLONG:
		cnstty = gbuiltintypes._sllongtype;
		cnstval._sint = ctx->_currtk._val._llong;
		break;
	case TK_CONSTANT_ULLONG:
		cnstty = gbuiltintypes._ullongtype;
		cnstval._uint = ctx->_currtk._val._ullong;
		break;
	case TK_CONSTANT_FLOAT:
		cnstty = gbuiltintypes._floattype;
		cnstval._float = ctx->_currtk._val._float;
		break;
	case TK_CONSTANT_DOUBLE:
		cnstty = gbuiltintypes._doubletype;
		cnstval._float = ctx->_currtk._val._double;
		break;
	case TK_CONSTANT_LDOUBLE:
		cnstty = gbuiltintypes._ldoubletype;
		cnstval._float = ctx->_currtk._val._ldouble;
		break;
	case TK_CONSTANT_CHAR:
		cnstty = gbuiltintypes._chartype;
		cnstval._sint = ctx->_currtk._val._ch;
		break;
	case TK_CONSTANT_WCHAR:
		cnstty = gbuiltintypes._wchartype;
		cnstval._sint = ctx->_currtk._val._wch;
		break;
	case TK_CONSTANT_STR:
		cnstty = cc_type_newarray(gbuiltintypes._chartype, ctx->_currtk._val._astr._chcnt, 0);
		cnstval._payload = ctx->_currtk._val._astr._str;
		break;
	case TK_CONSTANT_WSTR:
		cnstty = cc_type_newarray(gbuiltintypes._wchartype, ctx->_currtk._val._wstr._chcnt / gbuiltintypes._wchartype->_size, 0);
		cnstval._payload = ctx->_currtk._val._wstr._str;
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

	if (cnstty != NULL)
	{
		if (IsArray(cnstty))
		{
			if (!(tree = cc_expr_constant_str(ctx, cnstty, cnstval, ctx->_currtk._loc, where)))
			{
				return FALSE;
			}
		}
		else if (!(tree = cc_expr_constant(ctx, cnstty, cnstval, ctx->_currtk._loc, where)))
		{
			return FALSE;
		}

		cc_read_token(ctx, &ctx->_currtk);
	}

	*outexpr = tree;
	return TRUE;
}

static BOOL cc_expr_checkarguments(struct tagCCContext* ctx, FCCType* functy, FCCExprTree** args, enum EMMArea where)
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

		args[k] = cc_expr_cast(ctx, ty, args[k], where);
		if (IsInt(args[k]->_ty)
			&& args[k]->_ty->_size != gbuiltintypes._sinttype->_size)
		{
			args[k] = cc_expr_cast(ctx, cc_type_promote(args[k]->_ty), args[k], where);
		}
	} /* end for k */

	for (; k < n; k++)
	{
		args[k] = cc_expr_cast(ctx, cc_type_promote(args[k]->_ty), args[k], where);
	}

	return TRUE;
}

static FCCExprTree* cc_expr_postfix_subscript(struct tagCCContext* ctx, int op, FCCExprTree* expr, FCCExprTree* subscript, FLocation loc, enum EMMArea where)
{
	FCCExprTree* tree, *offset;
	FCCType* ty;
	FCCConstVal cnstval;

	/* check semantic */
	if (!IsPtr(expr->_ty) && !IsArray(expr->_ty)) {
		logger_output_s("error, pointer or array is expected at %w\n", &expr->_loc);
		return NULL;
	}

	ty = UnQual(expr->_ty)->_type;
	if (ty->_size == 0 || IsVoid(ty)) {
		logger_output_s("error, in-completable type %t at %w\n", ty, &expr->_loc);
		return NULL;
	}
	if (!IsInt(subscript->_ty)) {
		logger_output_s("error, integer is expected at %w\n", &subscript->_loc);
		return NULL;
	}

	cnstval._sint = ty->_size;
	offset = cc_expr_multiplicative(ctx, EXPR_MUL, subscript, cc_expr_constant(ctx, gbuiltintypes._sinttype, cnstval, loc, where), loc, where);
	offset = cc_expr_cast(ctx, gbuiltintypes._sinttype, offset, where);

	if (!(tree = cc_expr_new(where))) {
		return NULL;
	}

	tree->_op = EXPR_ADD;
	tree->_loc = loc;
	tree->_ty = ty;
	tree->_u._kids[0] = expr;
	tree->_u._kids[1] = offset;
	tree->_islvalue = 1;
	tree->_isconstant = expr->_isconstant;

	return tree;
}

static FCCExprTree* cc_expr_postfix_call(struct tagCCContext* ctx, int op, FCCExprTree* expr, FArray* args, FLocation loc, enum EMMArea where)
{
	FCCExprTree* tree;
	FCCType* functy;

	/* check function type */
	functy = UnQual(expr->_ty);
	if (IsPtr(functy)) { functy = functy->_type; }
	if (!IsFunction(functy)) {
		logger_output_s("error: function is expected at %w\n", &loc);
		return NULL;
	}

	/* check arguments */
	if (!cc_expr_checkarguments(ctx, functy, args->_data, where)) {
		logger_output_s("error occurred of function parameters checking at %w\n", &loc);
		return NULL;
	}

	if (!(tree = cc_expr_new(where))) {
		return NULL;
	}

	tree->_op = op;
	tree->_loc = loc;
	tree->_ty = cc_type_rettype(functy);
	tree->_u._f._lhs = expr;
	tree->_u._f._args = args->_data;

	return tree;
}

static FCCExprTree* cc_expr_postfix_dotfield(struct tagCCContext* ctx, int op, FCCExprTree* expr, const char* fieldname, FLocation loc, enum EMMArea where)
{
	FCCExprTree* tree;
	FCCField* field;


	if (!IsStruct(expr->_ty)) {
		logger_output_s("error: l-value is not a structure or union at %w\n", &expr->_loc);
		return NULL;
	}

	if (!(field = cc_type_findfield(fieldname, expr->_ty)))
	{
		logger_output_s("error: can't find field '%s' of %t at %w\n", fieldname, expr->_ty, &loc);
		return NULL;
	}

	if (!(tree = cc_expr_new(where))) {
		return NULL;
	}

	/**
		typedef struct{ int a; int b; } S;
		S s;
		s.a = 3;			// legal		lvalue is 1
		GetData().a = 3;	// illegal		lvalue is 0
	 */

	tree->_op = op;
	tree->_loc = loc;
	tree->_ty = cc_type_qual(field->_type, cc_type_getqual(expr->_ty));
	tree->_u._s._lhs = expr;
	tree->_u._s._field = field;
	tree->_islvalue = expr->_islvalue;
	tree->_isstaticaddressable = expr->_isstaticaddressable;

	return tree;
}

static FCCExprTree* cc_expr_postfix_ptrfield(struct tagCCContext* ctx, int op, FCCExprTree* expr, const char* fieldname, FLocation loc, enum EMMArea where)
{
	FCCExprTree* tree;
	FCCType* sty;
	FCCField* field;


	if (!IsPtr(expr->_ty) || !IsStruct(cc_type_deref(expr->_ty))) {
		logger_output_s("error: l-value is not a ptr to structure or union. at %w\n", &expr->_loc);
		return NULL;
	}

	sty = cc_type_deref(expr->_ty);
	if (!(field = cc_type_findfield(fieldname, UnQual(sty))))
	{
		logger_output_s("error: can't find field '%s' of %t. at %w\n", fieldname, UnQual(sty), &loc);
		return NULL;
	}

	if (!(tree = cc_expr_new(where))) {
		return NULL;
	}

	tree->_op = op;
	tree->_loc = loc;
	tree->_ty = cc_type_qual(field->_type, cc_type_getqual(sty));
	tree->_u._s._lhs = expr;
	tree->_u._s._field = field;
	tree->_islvalue = 1;

	return tree;
}

static FCCExprTree* cc_expr_postfix_incdec(struct tagCCContext* ctx, int op, FCCExprTree* expr, FLocation loc, enum EMMArea where)
{
	FCCExprTree* tree;
	FCCType* ty;

	if (!cc_expr_is_modifiable(expr)) {
		logger_output_s("error modifiable l-value is expected at %w.\n", &expr->_loc);
		return NULL;
	}

	ty = UnQual(expr->_ty);
	if (IsPtr(ty) && (ty->_type->_size == 0) && !IsVoid(ty->_type)) /* surport void *p; p++ */
	{
		logger_output_s("error pointer to an incomplete type %t at %w.\n", ty->_type, &expr->_loc);
		return NULL;
	}

	if (!(tree = cc_expr_new(where))) {
		return NULL;
	}

	tree->_op = op;
	tree->_loc = loc;
	tree->_ty = ty;
	tree->_u._kids[0] = expr;
	tree->_islvalue = 0;

	return tree;
}

static BOOL cc_expr_parse_postfix(struct tagCCContext* ctx, FCCExprTree** outexpr, enum EMMArea where)
{
	FCCExprTree* expr, * tree;
	FLocation loc;

	if (!cc_expr_parse_primary(ctx, &expr, where)) {
		return FALSE;
	}

	for (;;)
	{
		if (ctx->_currtk._type == TK_LBRACKET) /* '[' */
		{
			FCCExprTree* subscript;

			loc = ctx->_currtk._loc;
			cc_read_token(ctx, &ctx->_currtk);
			if (!cc_expr_parse_expression(ctx, &subscript, where))
			{
				return FALSE;
			}

			if (!cc_parser_expect(ctx, TK_RBRACKET)) /* ']' */
			{
				return FALSE;
			}

			if (!(tree = cc_expr_postfix_subscript(ctx, EXPR_ARRAYINDEX, expr, subscript, loc, where))) {
				return FALSE;
			}

			expr = tree;
		}
		else if (ctx->_currtk._type == TK_LPAREN) /* '(' */
		{
			FArray args;

			loc = ctx->_currtk._loc;
			cc_read_token(ctx, &ctx->_currtk);
			array_init(&args, 32, sizeof(FCCExprTree*), where);
			if (!cc_expr_parse_arguments(ctx, &args, where)) {
				return FALSE;
			}

			array_append_zeroed(&args);
			if (!cc_parser_expect(ctx, TK_RPAREN)) /* ')' */
			{
				return FALSE;
			}

			if (!(tree = cc_expr_postfix_call(ctx, EXPR_CALL, expr, &args, loc, where))) {
				return FALSE;
			}

			expr = tree;
		}
		else if (ctx->_currtk._type == TK_DOT) /* '.' */
		{
			const char* fieldname;

			loc = ctx->_currtk._loc;
			cc_read_token(ctx, &ctx->_currtk);
			if (ctx->_currtk._type != TK_ID)
			{
				logger_output_s("error: illegal field syntax, require an identifier, at %w\n", &ctx->_currtk._loc);
				return FALSE;
			}
			fieldname = ctx->_currtk._val._astr._str;

			if (!(tree = cc_expr_postfix_dotfield(ctx, EXPR_DOTFIELD, expr, fieldname, loc, where))) {
				return FALSE;
			}

			expr = tree;
			cc_read_token(ctx, &ctx->_currtk);
		}
		else if (ctx->_currtk._type == TK_POINTER) /* -> */
		{
			const char* fieldname;

			loc = ctx->_currtk._loc;
			cc_read_token(ctx, &ctx->_currtk);
			if (ctx->_currtk._type != TK_ID)
			{
				logger_output_s("error: illegal field syntax, require an identifier, at %w\n", &ctx->_currtk._loc);
				return FALSE;
			}
			fieldname = ctx->_currtk._val._astr._str;

			if (!(tree = cc_expr_postfix_ptrfield(ctx, EXPR_PTRFIELD, expr, fieldname, loc, where))) {
				return FALSE;
			}

			expr = tree;
			cc_read_token(ctx, &ctx->_currtk);
		}
		else if (ctx->_currtk._type == TK_INC) /* ++ */
		{
			loc = ctx->_currtk._loc;
			if (!(tree = cc_expr_postfix_incdec(ctx, EXPR_POSINC, expr, loc, where))) {
				return FALSE;
			}

			expr = tree;
			cc_read_token(ctx, &ctx->_currtk);
		}
		else if (ctx->_currtk._type == TK_DEC) /* -- */
		{
			loc = ctx->_currtk._loc;
			if (!(tree = cc_expr_postfix_incdec(ctx, EXPR_POSDEC, expr, loc, where))) {
				return FALSE;
			}

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

static BOOL cc_expr_parse_arguments(struct tagCCContext* ctx, FArray* args, enum EMMArea where)
{
	while (ctx->_currtk._type != TK_RPAREN)
	{
		FCCExprTree* expr = NULL;
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

static FCCExprTree* cc_expr_unary_incdec(struct tagCCContext* ctx, int op, FCCExprTree* expr, FLocation loc, enum EMMArea where)
{
	FCCExprTree* tree;
	FCCType* ty;


	if (!cc_expr_is_modifiable(expr)) {
		logger_output_s("error modifiable l-value is expected at %w.\n", &loc);
		return NULL;
	}

	ty = UnQual(expr->_ty);
	if (IsPtr(ty) && (ty->_type->_size == 0) && !IsVoid(ty->_type)) /* surport void *p; --p */
	{
		logger_output_s("error pointer to an incomplete type %t at %w.\n", ty->_type, &expr->_loc);
		return NULL;
	}

	if (!(tree = cc_expr_new(where))) {
		return NULL;
	}

	tree->_op = op;
	tree->_loc = loc;
	tree->_ty = ty;
	tree->_u._kids[0] = expr;
	tree->_islvalue = 0;

	return tree;
}

static FCCExprTree* cc_expr_unary_address(struct tagCCContext* ctx, int op, FCCExprTree* expr, FLocation loc, enum EMMArea where)
{
	FCCExprTree* tree;

	if (expr->_op == EXPR_DEREF) { /* &(*ptr) */
		return expr;
	}

	if (!(IsFunction(expr->_ty) || expr->_islvalue))
	{
		logger_output_s("error l-value or function designator for '&' is expected at %w.\n", &loc);
		return NULL;
	}
	if ((expr->_op == EXPR_PTRFIELD || expr->_op == EXPR_DOTFIELD) && expr->_u._s._field->_lsb > 0)
	{
		logger_output_s("error can't address a bits value at %w.\n", &loc);
		return NULL;
	}

	if (!(tree = cc_expr_new(where))) {
		return NULL;
	}

	tree->_op = op;
	tree->_loc = loc;
	tree->_ty = cc_type_ptr(expr->_ty);
	tree->_u._kids[0] = expr;
	tree->_islvalue = 0;
	tree->_isconstant = expr->_isstaticaddressable;

	return tree;
}

static FCCExprTree* cc_expr_unary_deref(struct tagCCContext* ctx, FCCExprTree* expr, FLocation loc, enum EMMArea where)
{
	FCCExprTree* tree;

	if (!IsPtr(expr->_ty)) {
		logger_output_s("error pointer is expected for dereference '*' at %w.\n", &loc);
		return NULL;
	}

	if (!(tree = cc_expr_new(where))) {
		return NULL;
	}

	tree->_op = op;
	tree->_loc = loc;
	tree->_ty = cc_type_deref(expr->_ty);
	tree->_u._kids[0] = expr;
	tree->_islvalue = !IsFunction(tree->_ty);

	return tree;
}

static FCCExprTree* cc_expr_unary_positive(struct tagCCContext* ctx, int op, FCCExprTree* expr, FLocation loc, enum EMMArea where)
{
	FCCExprTree* tree;

	if (!IsArith(expr->_ty))
	{
		logger_output_s("error arithmetic type is expected for '+' at %w.\n", &loc);
		return NULL;
	}

	if (expr->_op == EXPR_CONSTANT) {
		return expr;
	}

	if (!(tree = cc_expr_new(where))) {
		return NULL;
	}

	tree->_op = op;
	tree->_loc = loc;
	tree->_ty = cc_type_promote(expr->_ty);
	tree->_u._kids[0] = cc_expr_cast(ctx, tree->_ty, expr, where);
	tree->_islvalue = 0;

	return tree;
}

static FCCExprTree* cc_expr_unary_negtive(struct tagCCContext* ctx, int op, FCCExprTree* expr, FLocation loc, enum EMMArea where)
{
	FCCExprTree* tree;

	if (!IsArith(expr->_ty))
	{
		logger_output_s("error arithmetic type is expected for '%s' at %w.\n", cc_expr_opname(op), &loc);
		return NULL;
	}

	if (expr->_op == EXPR_CONSTANT)
	{
		FCCType* cnstty;
		FCCConstVal cnstval;

		cnstty = cc_type_promote(expr->_ty);
		if (UnQual(expr->_ty)->_op == Type_SInteger) {
			cnstval._sint = -expr->_u._symbol->_u._cnstval._sint;
		}
		else if (UnQual(expr->_ty)->_op == Type_UInteger) {
			cnstval._uint = -expr->_u._symbol->_u._cnstval._uint;
		}
		else if (UnQual(expr->_ty)->_op == Type_Float) {
			cnstval._float = -expr->_u._symbol->_u._cnstval._float;
		}

		return cc_expr_constant(ctx, cnstty, cnstval, loc, where);
	}

	if (!(tree = cc_expr_new(where))) {
		return NULL;
	}

	tree->_op = op;
	tree->_loc = loc;
	tree->_ty = cc_type_promote(expr->_ty);
	tree->_u._kids[0] = cc_expr_cast(ctx, tree->_ty, expr, where);
	tree->_islvalue = 0;

	return tree;
}

static FCCExprTree* cc_expr_unary_complement(struct tagCCContext* ctx, int op, FCCExprTree* expr, FLocation loc, enum EMMArea where)
{
	FCCExprTree* tree;

	if (!IsInt(expr->_ty))
	{
		logger_output_s("error integer type is expected for '%s' at %w.\n", cc_expr_opname(op), &loc);
		return NULL;
	}

	if (expr->_op == EXPR_CONSTANT)
	{
		FCCType* cnstty;
		FCCConstVal cnstval;

		cnstty = cc_type_promote(expr->_ty);
		if (UnQual(expr->_ty)->_op == Type_SInteger) {
			cnstval._sint = ~expr->_u._symbol->_u._cnstval._sint;
		}
		else if (UnQual(expr->_ty)->_op == Type_UInteger) {
			cnstval._uint = ~expr->_u._symbol->_u._cnstval._uint;
		}

		return cc_expr_constant(ctx, cnstty, cnstval, loc, where);
	}

	if (!(tree = cc_expr_new(where))) {
		return NULL;
	}

	tree->_op = op;
	tree->_loc = loc;
	tree->_ty = cc_type_promote(expr->_ty);
	tree->_u._kids[0] = cc_expr_cast(ctx, tree->_ty, expr, where);
	tree->_islvalue = 0;

	return tree;
}

static FCCExprTree* cc_expr_unary_not(struct tagCCContext* ctx, int op, FCCExprTree* expr, FLocation loc, enum EMMArea where)
{
	FCCExprTree* tree;

	if (!IsScalar(expr->_ty))
	{
		logger_output_s("error scalar type is expected for '!' at %w.\n", &loc);
		return NULL;
	}

	if (expr->_op == EXPR_CONSTANT)
	{
		FCCConstVal cnstval;

		if (UnQual(expr->_ty)->_op == Type_SInteger) {
			cnstval._sint = expr->_u._symbol->_u._cnstval._sint == 0;
		}
		else if (UnQual(expr->_ty)->_op == Type_UInteger) {
			cnstval._sint = expr->_u._symbol->_u._cnstval._uint == 0;
		}
		else if (UnQual(expr->_ty)->_op == Type_Float) {
			cnstval._sint = expr->_u._symbol->_u._cnstval._float == 0.0;
		}
		else if (UnQual(expr->_ty)->_op == Type_Pointer) {
			cnstval._sint = expr->_u._symbol->_u._cnstval._pointer == 0;
		}

		return cc_expr_constant(ctx, gbuiltintypes._sinttype, cnstval, loc, where);
	}

	if (!(tree = cc_expr_new(where))) {
		return NULL;
	}

	tree->_op = op;
	tree->_loc = loc;
	tree->_ty = gbuiltintypes._sinttype;
	tree->_u._kids[0] = expr;
	tree->_islvalue = 0;

	return tree;
}

static BOOL cc_expr_parse_unary(struct tagCCContext* ctx, FCCExprTree** outexpr, enum EMMArea where)
{
	FCCExprTree* tree, * expr;
	FLocation loc;

	if (ctx->_currtk._type == TK_INC) /* ++ */
	{
		loc = ctx->_currtk._loc;
		cc_read_token(ctx, &ctx->_currtk);
		if (!cc_expr_parse_unary(ctx, &expr, where))
		{
			return FALSE;
		}

		if (!(tree = cc_expr_unary_incdec(ctx, EXPR_PREINC, expr, loc, where))) {
			return FALSE;
		}

		*outexpr = tree;
		return TRUE;
	}
	else if (ctx->_currtk._type == TK_DEC) /* -- */
	{
		loc = ctx->_currtk._loc;
		cc_read_token(ctx, &ctx->_currtk);
		if (!cc_expr_parse_unary(ctx, &expr, where))
		{
			return FALSE;
		}

		if (!(tree = cc_expr_unary_incdec(ctx, EXPR_PREDEC, expr, loc, where))) {
			return FALSE;
		}

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

		if (!(tree = cc_expr_unary_address(ctx, EXPR_ADDR, expr, loc, where))) {
			return FALSE;
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

		if (!(tree = cc_expr_unary_deref(ctx, EXPR_DEREF, expr, loc, where))) {
			return FALSE;
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

		if (!(tree = cc_expr_unary_positive(ctx, EXPR_POS, expr, loc, where))) {
			return FALSE;
		}

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

		if (!(tree = cc_expr_unary_negtive(ctx, EXPR_NEG, expr, loc, where))) {
			return FALSE;
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

		if (!(tree = cc_expr_unary_complement(ctx, EXPR_COMPLEMENT, expr, loc, where))) {
			return FALSE;
		}

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

		if (!(tree = cc_expr_unary_not(ctx, EXPR_NOT, expr, loc, where))) {
			return FALSE;
		}

		*outexpr = tree;
		return TRUE;
	}
	else if (ctx->_currtk._type == TK_SIZEOF)
	{
		FCCType* exprty;
		FCCConstVal cnstval;

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
			FCCExprTree* subexpr;
			if (!cc_expr_parse_unary(ctx, &subexpr, where)) {
				return FALSE;
			}

			exprty = subexpr->_ty;
		}

		cnstval._sint = exprty->_size;
		if (!(tree = cc_expr_constant(ctx, gbuiltintypes._sinttype, cnstval, loc, where))) {
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

static BOOL cc_expr_parse_cast(struct tagCCContext* ctx, FCCExprTree** outexpr, enum EMMArea where)
{
	FCCExprTree* tree;

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

			if (!(tree = cc_expr_cast(ctx, ty, tree, where))) {
				return FALSE;
			}

			tree->_loc = loc;
			*outexpr = tree;
			return TRUE;
		}
	}

	return cc_expr_parse_unary(ctx, outexpr, where);
}

static FCCExprTree* cc_expr_multiplicative(struct tagCCContext* ctx, int op, FCCExprTree* lhs, FCCExprTree* rhs, FLocation loc, enum EMMArea where)
{
	FCCExprTree* tree;
	FCCType* ty;

	if (!IsArith(lhs->_ty) || !IsArith(rhs->_ty))
	{
		logger_output_s("error arithmetic operands is expected for '%s', at %w\n", cc_expr_opname(op), &loc);
		return NULL;
	}

	ty = cc_type_select(lhs->_ty, rhs->_ty);
	lhs = cc_expr_cast(ctx, ty, lhs, where);
	rhs = cc_expr_cast(ctx, ty, rhs, where);

	if (op == EXPR_MOD && !IsInt(ty))
	{
		logger_output_s("error integer operands is expected for '%s', at %w\n", cc_expr_opname(op), &loc);
		return NULL;
	}

	/* do constant fold */
	if (lhs->_op == EXPR_CONSTANT && rhs->_op == EXPR_CONSTANT)
	{
		FCCConstVal cnstval;

		if (UnQual(ty)->_op == Type_SInteger) {
			switch (op)
			{
			case EXPR_MUL:
				cnstval._sint = lhs->_u._symbol->_u._cnstval._sint * rhs->_u._symbol->_u._cnstval._sint; break;
			case EXPR_DIV:
				if (rhs->_u._symbol->_u._cnstval._sint == 0) {
					logger_output_s("error divide zero at %w\n", &loc);
					return NULL;
				}
				cnstval._sint = lhs->_u._symbol->_u._cnstval._sint / rhs->_u._symbol->_u._cnstval._sint; break;
			case EXPR_MOD:
				if (rhs->_u._symbol->_u._cnstval._sint == 0) {
					logger_output_s("error divide zero at %w\n", &loc);
					return NULL;
				}
				cnstval._sint = lhs->_u._symbol->_u._cnstval._sint % rhs->_u._symbol->_u._cnstval._sint; break;
			default:
				assert(0);
				break;
			}
		}
		else if (UnQual(ty)->_op == Type_UInteger) {
			switch (op)
			{
			case EXPR_MUL:
				cnstval._uint = lhs->_u._symbol->_u._cnstval._uint * rhs->_u._symbol->_u._cnstval._uint; break;
			case EXPR_DIV:
				if (rhs->_u._symbol->_u._cnstval._uint == 0) {
					logger_output_s("error divide zero at %w\n", &loc);
					return NULL;
				}
				cnstval._uint = lhs->_u._symbol->_u._cnstval._uint / rhs->_u._symbol->_u._cnstval._uint; break;
			case EXPR_MOD:
				if (rhs->_u._symbol->_u._cnstval._uint == 0) {
					logger_output_s("error divide zero at %w\n", &loc);
					return NULL;
				}
				cnstval._uint = lhs->_u._symbol->_u._cnstval._uint % rhs->_u._symbol->_u._cnstval._uint; break;
			default:
				assert(0);
				break;
			}
		}
		else if (UnQual(ty)->_op == Type_Float) {
			switch (op)
			{
			case EXPR_MUL:
				cnstval._float = lhs->_u._symbol->_u._cnstval._float * rhs->_u._symbol->_u._cnstval._float; break;
			case EXPR_DIV:
				if (rhs->_u._symbol->_u._cnstval._float == 0.0) {
					logger_output_s("error divide zero at %w\n", &loc);
					return NULL;
				}
				cnstval._float = lhs->_u._symbol->_u._cnstval._float / rhs->_u._symbol->_u._cnstval._float; break;
			default:
				assert(0);
				break;
			}
		}

		return cc_expr_constant(ctx, ty, cnstval, loc, where);
	}

	if (!(tree = cc_expr_new(where))) {
		return NULL;
	}

	tree->_op = op;
	tree->_ty = ty;
	tree->_loc = loc;
	tree->_u._kids[0] = lhs;
	tree->_u._kids[1] = rhs;

	return tree;
}

static BOOL cc_expr_parse_multiplicative(struct tagCCContext* ctx, FCCExprTree** outexpr, enum EMMArea where)
{
	FCCExprTree* lhs, * rhs, * tree;
	FLocation loc;


	if (!cc_expr_parse_cast(ctx, &lhs, where))
	{
		return FALSE;
	}

	tree = lhs;
	while (ctx->_currtk._type == TK_MUL || ctx->_currtk._type == TK_DIV || ctx->_currtk._type == TK_MOD)
	{
		enum ECCToken tktype = ctx->_currtk._type;

		loc = ctx->_currtk._loc;
		cc_read_token(ctx, &ctx->_currtk);
		if (!cc_expr_parse_cast(ctx, &rhs, where)) {
			return FALSE;
		}

		if (!(tree = cc_expr_multiplicative(ctx, (tktype == TK_MUL) ? EXPR_MUL : (tktype == TK_DIV ? EXPR_DIV : EXPR_MOD), lhs, rhs, loc, where)))
		{
			return FALSE;
		}

		lhs = tree;
	} /* end while */

	*outexpr = tree;
	return TRUE;
}

static FCCExprTree* cc_expr_additive(struct tagCCContext* ctx, int op, FCCExprTree* lhs, FCCExprTree* rhs, FLocation loc, enum EMMArea where)
{
	FCCExprTree* tree;
	FCCType* ty = NULL;
	BOOL isptrdiff = FALSE;

	if (IsArith(lhs->_ty) && IsArith(rhs->_ty))
	{
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
				case EXPR_ADD:
					cnstval._sint = lhs->_u._symbol->_u._cnstval._sint + rhs->_u._symbol->_u._cnstval._sint; break;
				case EXPR_SUB:
					cnstval._sint = lhs->_u._symbol->_u._cnstval._sint - rhs->_u._symbol->_u._cnstval._sint; break;
				default:
					assert(0);
					break;
				}
			}
			else if (UnQual(ty)->_op == Type_UInteger) {
				switch (op)
				{
				case EXPR_ADD:
					cnstval._uint = lhs->_u._symbol->_u._cnstval._uint + rhs->_u._symbol->_u._cnstval._uint; break;
				case EXPR_SUB:
					cnstval._uint = lhs->_u._symbol->_u._cnstval._uint - rhs->_u._symbol->_u._cnstval._uint; break;
				default:
					assert(0);
					break;
				}
			}
			else if (UnQual(ty)->_op == Type_Float) {
				switch (op)
				{
				case EXPR_ADD:
					cnstval._float = lhs->_u._symbol->_u._cnstval._float + rhs->_u._symbol->_u._cnstval._float; break;
				case EXPR_SUB:
					cnstval._float = lhs->_u._symbol->_u._cnstval._float - rhs->_u._symbol->_u._cnstval._float; break;
				default:
					assert(0);
					break;
				}
			}

			return cc_expr_constant(ctx, ty, cnstval, loc, where);
		}

	}
	else if (IsPtr(lhs->_ty) || IsArray(lhs->_ty))
	{
		int size = lhs->_ty->_type->_size;


		if (IsVoid(lhs->_ty->_type)) {
			size = 1;
		}
		if (size <= 0) {
			logger_output_s("error: unknown %t size at %w\n", lhs->_ty->_type, &loc);
			return NULL;
		}

		if (IsInt(rhs->_ty)) {
			ty = lhs->_ty;
			rhs = cc_expr_cast(ctx, gbuiltintypes._sinttype, rhs, where);

			/* constant folding */
			if (lhs->_op == EXPR_CONSTANT && rhs->_op == EXPR_CONSTANT)
			{
				FCCConstVal cnstval;

				switch (op)
				{
				case EXPR_ADD:
					cnstval._pointer = lhs->_u._symbol->_u._cnstval._pointer + rhs->_u._symbol->_u._cnstval._sint * size; break;
				case EXPR_SUB:
					cnstval._pointer = lhs->_u._symbol->_u._cnstval._pointer - rhs->_u._symbol->_u._cnstval._sint * size; break;
				default:
					assert(0);
					break;
				}

				return cc_expr_constant(ctx, ty, cnstval, loc, where);
			}
		}
		else if (IsPtr(rhs->_ty) || IsArray(rhs->_ty)) {
			if (!cc_type_isequal(lhs->_ty->_type, rhs->_ty->_type, 0) || op != EXPR_SUB) {
				logger_output_s("error: invalid pointer additive operate at %w\n", &loc);
				return FALSE;
			}

			ty = gbuiltintypes._sinttype;
			isptrdiff = TRUE;
			/* constant folding */
			if (lhs->_op == EXPR_CONSTANT && rhs->_op == EXPR_CONSTANT)
			{
				FCCConstVal cnstval;

				cnstval._sint = (lhs->_u._symbol->_u._cnstval._pointer - rhs->_u._symbol->_u._cnstval._pointer) / size;
				return cc_expr_constant(ctx, ty, cnstval, loc, where);
			}
		}
	}
	else {
		logger_output_s("error: invalid operand for '%s' at %w\n", cc_expr_opname(op), &loc);
		return NULL;
	}

	if (!(tree = cc_expr_new(where))) {
		return NULL;
	}

	tree->_op = op;
	tree->_loc = loc;
	tree->_ty = ty;
	tree->_u._kids[0] = lhs;
	tree->_u._kids[1] = rhs;
	tree->_isconstant = lhs->_isconstant && rhs->_isconstant && !isptrdiff;

	return tree;
}

static BOOL cc_expr_parse_additive(struct tagCCContext* ctx, FCCExprTree** outexpr, enum EMMArea where)
{
	FCCExprTree* lhs, * rhs, * tree;
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

		if (!(tree = cc_expr_additive(ctx, (tktype == TK_ADD) ? EXPR_ADD : EXPR_SUB, lhs, rhs, loc, where))) {
			return FALSE;
		}

		lhs = tree;
	} /* end while */

	*outexpr = tree;
	return TRUE;
}

static FCCExprTree* cc_expr_shift(struct tagCCContext* ctx, int op, FCCExprTree* lhs, FCCExprTree* rhs, FLocation loc, enum EMMArea where)
{
	FCCExprTree* tree;

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

static BOOL cc_expr_parse_shift(struct tagCCContext* ctx, FCCExprTree** outexpr, enum EMMArea where)
{
	FCCExprTree* lhs, * rhs, * tree;
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

static FCCExprTree* cc_expr_relational(struct tagCCContext* ctx, int op, FCCExprTree* lhs, FCCExprTree* rhs, FLocation loc, enum EMMArea where)
{
	FCCExprTree* tree;

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

static BOOL cc_expr_parse_relational(struct tagCCContext* ctx, FCCExprTree** outexpr, enum EMMArea where)
{
	FCCExprTree* lhs, * rhs, * tree;
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

static FCCExprTree* cc_expr_equality(struct tagCCContext* ctx, int op, FCCExprTree* lhs, FCCExprTree* rhs, FLocation loc, enum EMMArea where)
{
	FCCExprTree* tree;

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

static BOOL cc_expr_parse_equality(struct tagCCContext* ctx, FCCExprTree** outexpr, enum EMMArea where)
{
	FCCExprTree* lhs, * rhs, * tree;
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

static FCCExprTree* cc_expr_bitand(struct tagCCContext* ctx, int op, FCCExprTree* lhs, FCCExprTree* rhs, FLocation loc, enum EMMArea where)
{
	FCCExprTree* tree;
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

static BOOL cc_expr_parse_bitand(struct tagCCContext* ctx, FCCExprTree** outexpr, enum EMMArea where)
{
	FCCExprTree* lhs, * rhs, * tree;
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

static FCCExprTree* cc_expr_bitxor(struct tagCCContext* ctx, int op, FCCExprTree* lhs, FCCExprTree* rhs, FLocation loc, enum EMMArea where)
{
	FCCExprTree* tree;
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

static BOOL cc_expr_parse_bitxor(struct tagCCContext* ctx, FCCExprTree** outexpr, enum EMMArea where)
{
	FCCExprTree* lhs, * rhs, * tree;
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

static FCCExprTree* cc_expr_bitor(struct tagCCContext* ctx, int op, FCCExprTree* lhs, FCCExprTree* rhs, FLocation loc, enum EMMArea where)
{
	FCCExprTree* tree;
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

static BOOL cc_expr_parse_bitor(struct tagCCContext* ctx, FCCExprTree** outexpr, enum EMMArea where)
{
	FCCExprTree* lhs, * rhs, * tree;
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

static FCCExprTree* cc_expr_logicand(struct tagCCContext* ctx, int op, FCCExprTree* lhs, FCCExprTree* rhs, FLocation loc, enum EMMArea where)
{
	FCCExprTree* tree;

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

static BOOL cc_expr_parse_logicand(struct tagCCContext* ctx, FCCExprTree** outexpr, enum EMMArea where)
{
	FCCExprTree* lhs, * rhs, * tree;
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

static FCCExprTree* cc_expr_logicor(struct tagCCContext* ctx, int op, FCCExprTree* lhs, FCCExprTree* rhs, FLocation loc, enum EMMArea where)
{
	FCCExprTree* tree;

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

static BOOL cc_expr_parse_logicor(struct tagCCContext* ctx, FCCExprTree** outexpr, enum EMMArea where)
{
	FCCExprTree* lhs, * rhs, * tree;
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

static FCCExprTree* cc_expr_conditional(struct tagCCContext* ctx, int op, FCCExprTree* expr0, FCCExprTree* expr1, FCCExprTree* expr2, FLocation loc, enum EMMArea where)
{
	FCCExprTree* tree;
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

static BOOL cc_expr_parse_conditional(struct tagCCContext* ctx, FCCExprTree** outexpr, enum EMMArea where)
{
	FCCExprTree* tree, * expr0, * expr1, * expr2;
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

static FCCExprTree* cc_expr_assignment(struct tagCCContext* ctx, int op, FCCExprTree* lhs, FCCExprTree* rhs, FLocation loc, enum EMMArea where)
{
	FCCExprTree* tree;

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

BOOL cc_expr_parse_assignment(struct tagCCContext* ctx, FCCExprTree** outexpr, enum EMMArea where)
{
	FCCExprTree* tree, * lhs, * rhs;
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

static FCCExprTree* cc_expr_comma(struct tagCCContext* ctx, int op, FCCExprTree* lhs, FCCExprTree* rhs, FLocation loc, enum EMMArea where)
{
	FCCExprTree* tree;

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

BOOL cc_expr_parse_expression(struct tagCCContext* ctx, FCCExprTree** outexpr, enum EMMArea where)
{
	FCCExprTree* lhs, * rhs, * tree;
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

BOOL cc_expr_parse_constant_expression(struct tagCCContext* ctx, FCCExprTree** outexpr, enum EMMArea where)
{
	if (cc_expr_parse_conditional(ctx, outexpr, where) && (*outexpr)->_isconstant)
	{
		return TRUE;
	}

	logger_output_s("error: constant expression is expected at %w\n", &((*outexpr)->_loc));
	return FALSE;
}

BOOL cc_expr_parse_constant_int(struct tagCCContext* ctx, int* val)
{
	FCCExprTree* expr;

	*val = 0;
	if (!cc_expr_parse_constant_expression(ctx, &expr, CC_MM_TEMPPOOL) || expr == NULL) {
		return FALSE;
	}

	if (expr->_op == EXPR_CONSTANT)
	{
		FCCType* ty = expr->_ty;

		if (IsEnum(ty)) {
			ty = ty->_type;
		}

		if (ty->_op == Type_SInteger)
		{
			*val = (int)expr->_u._symbol->_u._cnstval._sint;
			return TRUE;
		}
		if (ty->_op == Type_UInteger)
		{
			*val = (int)expr->_u._symbol->_u._cnstval._uint;
			return TRUE;
		}
	}

	logger_output_s("error: integral constant is expected. at %w.\n", &expr->_loc);

	return FALSE;
}

struct tagCCType* cc_expr_assigntype(struct tagCCType* lhs, struct tagCCExprTree* expr)
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

	if ((IsVoidptr(xty) && (IsPtr(yty) || IsArray(yty)) || IsPtr(xty) && IsVoidptr(yty)) &&
		((IsConst(xty->_type) || !IsConst(yty->_type)) && (IsVolatile(xty->_type) || !IsVolatile(yty->_type)))
		)
	{
		return xty;
	}

	if ((IsPtr(xty) && (IsPtr(yty) || IsArray(yty))
		&& cc_type_isequal(UnQual(xty->_type), UnQual(yty->_type), TRUE))
		&& ((IsConst(xty->_type) || !IsConst(yty->_type))
			&& (IsVolatile(xty->_type) || !IsVolatile(yty->_type))))
	{
		return xty;
	}

	if (IsPtr(xty) && (IsPtr(yty) || IsArray(yty))
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

FCCExprTree* cc_expr_cast(struct tagCCContext* ctx, struct tagCCType* castty, FCCExprTree* expr, enum EMMArea where)
{
	FCCExprTree* tree;

	if (!cc_type_cancast(castty, expr->_ty))
	{
		logger_output_s("error typecast failed at %w\n", &expr->_loc);
		return NULL;
	}

	if (expr->_op == EXPR_CONSTANT)
	{
		FCCConstVal cnstval;

		cnstval = expr->_u._symbol->_u._cnstval;
		if (IsFloat(castty) && !IsFloat(expr->_ty)) {
			cnstval._float = (double)expr->_u._symbol->_u._cnstval._sint;
		}
		else if (!IsFloat(castty) && IsFloat(expr->_ty)) {
			cnstval._sint = (int64_t)expr->_u._symbol->_u._cnstval._float;
		}

		return cc_expr_constant(ctx, castty, cnstval, expr->_loc, where);
	}

	if (!(tree = cc_expr_new(where))) {
		return NULL;
	}
	tree->_op = EXPR_TYPECAST;
	tree->_loc = expr->_loc;
	tree->_ty = castty;
	tree->_u._kids[0] = expr;
	tree->_islvalue = IsArray(tree->_ty);
	if ((IsPtr(expr->_ty) || IsInt(expr->_ty)) && expr->_ty->_size == castty->_size)
	{
		tree->_isconstant = expr->_isconstant;
	}

	return tree;
}

FCCExprTree* cc_expr_constant(struct tagCCContext* ctx, struct tagCCType* cnstty, FCCConstVal cnstval, FLocation loc, enum EMMArea where)
{
	FCCExprTree* tree;
	FCCSymbol* p;

	if (!(p = cc_symbol_constant(cnstty, cnstval)))
	{
		logger_output_s("error: install constant failed at %w\n", &loc);
		return NULL;
	}
	if (!(tree = cc_expr_new(where))) {
		return NULL;
	}
	tree->_op = EXPR_CONSTANT;
	tree->_loc = loc;
	tree->_ty = p->_type;
	tree->_u._symbol = p;
	tree->_isconstant = 1;

	return tree;
}

FCCExprTree* cc_expr_constant_str(struct tagCCContext* ctx, struct tagCCType* cnstty, FCCConstVal cnstval, FLocation loc, enum EMMArea where)
{
	FCCExprTree* tree;
	FCCSymbol* p;

	if (!(p = cc_symbol_constant(cnstty, cnstval)))
	{
		logger_output_s("error: install constant failed at %w\n", &loc);
		return NULL;
	}
	if (!(tree = cc_expr_new(where))) {
		return NULL;
	}
	tree->_op = EXPR_CONSTANT_STR;
	tree->_loc = loc;
	tree->_ty = p->_type;
	tree->_u._symbol = p;
	tree->_islvalue = 1;
	tree->_isconstant = 1;
	tree->_isstaticaddressable = 1;

	return tree;
}

BOOL cc_expr_is_modifiable(FCCExprTree* expr)
{
	/* modifiable:
	  1. l-value
	  2. not const
	  3. not array type
	  4. for union/struct, there is not any const field.
	*/
	return expr->_islvalue
		&& !IsConst(expr->_ty)
		&& !IsArray(expr->_ty)
		&& (IsStruct(expr->_ty) ? UnQual(expr->_ty)->_u._symbol->_u._s._cfields : TRUE);
}

BOOL cc_expr_is_nullptr(FCCExprTree* expr)
{
	FCCType* ty = UnQual(expr->_ty);

	return (expr->_op == EXPR_CONSTANT)
		&& ((ty->_op == Type_SInteger && expr->_u._symbol->_u._cnstval._sint == 0)
			|| (ty->_op == Type_UInteger && expr->_u._symbol->_u._cnstval._uint == 0)
			|| (IsVoidptr(ty) && expr->_u._symbol->_u._cnstval._pointer == 0));
}

FCCExprTree* cc_expr_to_pointer(FCCExprTree* expr)
{
	if (IsArray(expr->_ty)) {
		expr->_ty = cc_type_arraytoptr(expr->_ty);
	}
	else if (IsFunction(expr->_ty)) {
		expr->_ty = cc_type_ptr(expr->_ty);
	}

	return expr;
}

FCCExprTree* cc_expr_new(enum EMMArea where)
{
	FCCExprTree* tree = mm_alloc_area(sizeof(FCCExprTree), where);
	if (tree) {
		memset(tree, 0, sizeof(FCCExprTree));
	}
	else {
		logger_output_s("error: out of memory at %s:%d\n", __FILE__, __LINE__);
	}

	return tree;
}

const char* cc_expr_opname(enum EExprOp op)
{
	switch (op)
	{
	case EXPR_COMMA:
		return ",";
	case EXPR_ASSIGN:
		return "=";
	case EXPR_ASSIGN_MUL:
		return "*=";
	case EXPR_ASSIGN_DIV:
		return "/=";
	case EXPR_ASSIGN_MOD:
		return "%=";
	case EXPR_ASSIGN_ADD:
		return "+=";
	case EXPR_ASSIGN_SUB:
		return "-=";
	case EXPR_ASSIGN_LSHIFT:
		return "<<=";
	case EXPR_ASSIGN_RSHIFT:
		return ">>=";
	case EXPR_ASSIGN_BITAND:
		return "&=";
	case EXPR_ASSIGN_BITXOR:
		return "^=";
	case EXPR_ASSIGN_BITOR:
		return "|=";
	case EXPR_COND:
		return "? : ";
	case EXPR_LOGOR:
		return "||";
	case EXPR_LOGAND:
		return "&&";
	case EXPR_BITOR:
		return "|";
	case EXPR_BITXOR:
		return "^";
	case EXPR_BITAND:
		return "&";
	case EXPR_EQ:
		return "=";
	case EXPR_UNEQ:
		return "!=";
	case EXPR_LESS:
		return "<";
	case EXPR_GREAT:
		return ">";
	case EXPR_LESSEQ:
		return "<=";
	case EXPR_GREATEQ:
		return ">=";
	case EXPR_LSHFIT:
		return "<<";
	case EXPR_RSHFIT:
		return ">>";
	case EXPR_ADD:
		return "+";
	case EXPR_SUB:
		return "-";
	case EXPR_MUL:
		return "*";
	case EXPR_DIV:
		return "/";
	case EXPR_MOD:
		return "%";
	case EXPR_DEREF:
		return "*";
	case EXPR_ADDRG:
	case EXPR_ADDRF:
	case EXPR_ADDRL:
		return "&";
	case EXPR_NEG:
		return "-";
	case EXPR_POS:
		return "+";
	case EXPR_NOT:
		return "!";
	case EXPR_COMPLEMENT:
		return "~";
	case EXPR_PREINC:
		return "++";
	case EXPR_PREDEC:
		return "--";
	case EXPR_SIZEOF:
		return "sizeof";
	case EXPR_TYPECAST:
		return "(typecast)";
	case EXPR_POSINC:
		return "++";
	case EXPR_POSDEC:
		return "--";
	case EXPR_CONSTANT:
		return "constant";
	case EXPR_CALL:
		return "function()";
	default:
		break;
	}

	return "Unknown";
}
