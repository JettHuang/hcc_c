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


static BOOL cc_expr_primary(struct tagCCContext* ctx, FCCExprTree** outexpr, enum EMMArea where);
static BOOL cc_expr_postfix(struct tagCCContext* ctx, FCCExprTree** outexpr, enum EMMArea where);
static BOOL cc_expr_arguments(struct tagCCContext* ctx, FArray* args, enum EMMArea where);
static BOOL cc_expr_unary(struct tagCCContext* ctx, FCCExprTree** outexpr, enum EMMArea where);
static BOOL cc_expr_cast(struct tagCCContext* ctx, FCCExprTree** outexpr, enum EMMArea where);
static BOOL cc_expr_multiplicative(struct tagCCContext* ctx, FCCExprTree** outexpr, enum EMMArea where);
static BOOL cc_expr_additive(struct tagCCContext* ctx, FCCExprTree** outexpr, enum EMMArea where);
static BOOL cc_expr_shift(struct tagCCContext* ctx, FCCExprTree** outexpr, enum EMMArea where);
static BOOL cc_expr_relational(struct tagCCContext* ctx, FCCExprTree** outexpr, enum EMMArea where);
static BOOL cc_expr_equality(struct tagCCContext* ctx, FCCExprTree** outexpr, enum EMMArea where);
static BOOL cc_expr_bitand(struct tagCCContext* ctx, FCCExprTree** outexpr, enum EMMArea where);
static BOOL cc_expr_bitxor(struct tagCCContext* ctx, FCCExprTree** outexpr, enum EMMArea where);
static BOOL cc_expr_bitor(struct tagCCContext* ctx, FCCExprTree** outexpr, enum EMMArea where);
static BOOL cc_expr_logicand(struct tagCCContext* ctx, FCCExprTree** outexpr, enum EMMArea where);
static BOOL cc_expr_logicor(struct tagCCContext* ctx, FCCExprTree** outexpr, enum EMMArea where);
static BOOL cc_expr_conditional(struct tagCCContext* ctx, FCCExprTree** outexpr, enum EMMArea where);

/* ------------------------------------------------------------------------------------------ */

static BOOL cc_expr_primary(struct tagCCContext* ctx, FCCExprTree** outexpr, enum EMMArea where)
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

		if (!(tree = cc_expr_new(where))) {
			return FALSE;
		}
		tree->_op = EXPR_ID;
		tree->_loc = ctx->_currtk._loc;
		tree->_ty = p->_type;
		tree->_u._symbol = p;

		cc_read_token(ctx, &ctx->_currtk);
	}
		break;
	case TK_CONSTANT_INT:
		cnstty = gBuiltinTypes._sinttype; 
		cnstval._sint = ctx->_currtk._val._int; 
		break;
	case TK_CONSTANT_UINT:
		cnstty = gBuiltinTypes._uinttype; 
		cnstval._uint = ctx->_currtk._val._uint;
		break;
	case TK_CONSTANT_LONG:
		cnstty = gBuiltinTypes._slongtype; 
		cnstval._sint = ctx->_currtk._val._long;
		break;
	case TK_CONSTANT_ULONG:
		cnstty = gBuiltinTypes._ulongtype; 
		cnstval._uint = ctx->_currtk._val._ulong;
		break;
	case TK_CONSTANT_LLONG:
		cnstty = gBuiltinTypes._sllongtype; 
		cnstval._sint = ctx->_currtk._val._llong;
		break;
	case TK_CONSTANT_ULLONG:
		cnstty = gBuiltinTypes._ullongtype; 
		cnstval._uint = ctx->_currtk._val._ullong;
		break;
	case TK_CONSTANT_FLOAT:
		cnstty = gBuiltinTypes._floattype; 
		cnstval._float = ctx->_currtk._val._float;
		break;
	case TK_CONSTANT_DOUBLE:
		cnstty = gBuiltinTypes._doubletype; 
		cnstval._float = ctx->_currtk._val._double;
		break;
	case TK_CONSTANT_LDOUBLE:
		cnstty = gBuiltinTypes._ldoubletype; 
		cnstval._float = ctx->_currtk._val._ldouble;
		break;
	case TK_CONSTANT_CHAR:
		cnstty = gBuiltinTypes._chartype; 
		cnstval._sint = ctx->_currtk._val._ch;
		break;
	case TK_CONSTANT_WCHAR:
		cnstty = gBuiltinTypes._wchartype;
		cnstval._sint = ctx->_currtk._val._wch;
		break;
	case TK_CONSTANT_STR:
		cnstty = cc_type_newarray(gBuiltinTypes._chartype, ctx->_currtk._val._astr._chcnt, 0); 
		cnstval._ptr = ctx->_currtk._val._astr._str;
		break;
	case TK_CONSTANT_WSTR:
		cnstty = cc_type_newarray(gBuiltinTypes._wchartype, ctx->_currtk._val._wstr._chcnt / gBuiltinTypes._wchartype->_size, 0); 
		cnstval._ptr = ctx->_currtk._val._wstr._str;
		break;
	case TK_LPAREN: /* '(' */
	{
		cc_read_token(ctx, &ctx->_currtk);
		if (!cc_expr_expression(ctx, &tree, where)) {
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
		cc_read_token(ctx, &ctx->_currtk);

		if (!(p = cc_symbol_constant(cnstty, cnstval)))
		{
			logger_output_s("error: install constant failed at %w\n", &ctx->_currtk._loc);
			return FALSE;
		}
		if (!(tree = cc_expr_new(where))) {
			return FALSE;
		}
		tree->_op = EXPR_CONSTANT;
		tree->_loc = ctx->_currtk._loc;
		tree->_ty = p->_type;
		tree->_u._symbol = p;
	}

	*outexpr = tree;
	return TRUE;
}

static BOOL cc_expr_postfix(struct tagCCContext* ctx, FCCExprTree** outexpr, enum EMMArea where)
{
	FCCExprTree* expr, * tree;
	FLocation loc;

	if (!cc_expr_primary(ctx, &expr, where)) {
		return FALSE;
	}

	for (;;)
	{
		if (ctx->_currtk._type == TK_LBRACKET) /* '[' */
		{
			FCCExprTree* subscript;

			loc = ctx->_currtk._loc;
			cc_read_token(ctx, &ctx->_currtk);
			if (!cc_expr_expression(ctx, &subscript, where))
			{
				return FALSE;
			}

			if (!cc_parser_expect(ctx, TK_RBRACKET)) /* ']' */
			{
				return FALSE;
			}

			if (!(tree = cc_expr_new(where))) {
				return FALSE;
			}

			tree->_op = EXPR_ARRAYSUB;
			tree->_loc = loc;
			tree->_u._kids[0] = expr;
			tree->_u._kids[1] = subscript;
			expr = tree;
		}
		else if (ctx->_currtk._type == TK_LPAREN) /* '(' */
		{
			FArray args;

			loc = ctx->_currtk._loc;
			cc_read_token(ctx, &ctx->_currtk);
			array_init(&args, 32, sizeof(FCCExprTree*), where);
			if (!cc_expr_arguments(ctx, &args, where)) {
				return FALSE;
			}

			array_append_zeroed(&args);
			if (!cc_parser_expect(ctx, TK_RPAREN)) /* ')' */
			{
				return FALSE;
			}

			if (!(tree = cc_expr_new(where))) {
				return FALSE;
			}

			tree->_op = EXPR_CALL;
			tree->_loc = loc;
			tree->_u._f._lhs = expr;
			tree->_u._f._args = args._data;
			expr = tree;
		}
		else if (ctx->_currtk._type == TK_DOT) /* '.' */
		{
			const char* id;
			FCCField* field;

			loc = ctx->_currtk._loc;
			cc_read_token(ctx, &ctx->_currtk);
			if (ctx->_currtk._type != TK_ID)
			{
				logger_output_s("error: illegal field syntax, require an identifier, at %w\n", &ctx->_currtk._loc);
				return FALSE;
			}

			if (!IsStruct(expr->_ty)) {
				logger_output_s("error: l-value is not a structure or union. at %w\n", &ctx->_currtk._loc);
				return FALSE;
			}

			id = ctx->_currtk._val._astr._str;
			if (!(field = cc_type_findfield(id, expr->_ty)))
			{
				logger_output_s("error: can't find field of %t. at %w\n", expr->_ty, &ctx->_currtk._loc);
				return FALSE;
			}

			if (!(tree = cc_expr_new(where))) {
				return FALSE;
			}

			tree->_op = EXPR_DOTFIELD;
			tree->_loc = loc;
			tree->_u._s._lhs = expr;
			tree->_u._s._field = field;
			expr = tree;

			cc_read_token(ctx, &ctx->_currtk);
		}
		else if (ctx->_currtk._type == TK_POINTER) /* -> */
		{
			const char* id;
			FCCType* sty;
			FCCField* field;

			loc = ctx->_currtk._loc;
			cc_read_token(ctx, &ctx->_currtk);
			if (ctx->_currtk._type != TK_ID)
			{
				logger_output_s("error: illegal field syntax, require an identifier, at %w\n", &ctx->_currtk._loc);
				return FALSE;
			}

			if (!IsPtr(expr->_ty) || !IsStruct(cc_type_deref(expr->_ty))) {
				logger_output_s("error: l-value is not a ptr to structure or union. at %w\n", &ctx->_currtk._loc);
				return FALSE;
			}

			id = ctx->_currtk._val._astr._str;
			sty = UnQual(cc_type_deref(expr->_ty));
			if (!(field = cc_type_findfield(id, sty)))
			{
				logger_output_s("error: can't find field of %t. at %w\n", sty, &ctx->_currtk._loc);
				return FALSE;
			}

			if (!(tree = cc_expr_new(where))) {
				return FALSE;
			}

			tree->_op = EXPR_PTRFIELD;
			tree->_loc = loc;
			tree->_u._s._lhs = expr;
			tree->_u._s._field = field;
			expr = tree;

			cc_read_token(ctx, &ctx->_currtk);
		}
		else if (ctx->_currtk._type == TK_INC) /* ++ */
		{
			loc = ctx->_currtk._loc;
			cc_read_token(ctx, &ctx->_currtk);
			if (!(tree = cc_expr_new(where))) {
				return FALSE;
			}

			tree->_op = EXPR_POSINC;
			tree->_loc = loc;
			tree->_u._kids[0] = expr;
			expr = tree;
		}
		else if (ctx->_currtk._type == TK_DEC) /* -- */
		{
			loc = ctx->_currtk._loc;
			cc_read_token(ctx, &ctx->_currtk);
			if (!(tree = cc_expr_new(where))) {
				return FALSE;
			}

			tree->_op = EXPR_POSDEC;
			tree->_loc = loc;
			tree->_u._kids[0] = expr;
			expr = tree;
		}
		else
		{
			break;
		}
	} /* end for ;; */

	*outexpr = expr;
	return TRUE;
}

static BOOL cc_expr_arguments(struct tagCCContext* ctx, FArray* args, enum EMMArea where)
{
	while (ctx->_currtk._type != TK_RPAREN)
	{
		FCCExprTree* expr = NULL;
		if (!cc_expr_assignment(ctx, &expr, where))
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

static BOOL cc_expr_unary(struct tagCCContext* ctx, FCCExprTree** outexpr, enum EMMArea where)
{
	FCCExprTree* tree;
	FLocation loc;

	if (ctx->_currtk._type == TK_INC) /* ++ */
	{
		loc = ctx->_currtk._loc;
		cc_read_token(ctx, &ctx->_currtk);
		if (!(tree = cc_expr_new(where))) {
			return FALSE;
		}

		tree->_op = EXPR_PREINC;
		tree->_loc = loc;
		if (!cc_expr_unary(ctx, &tree->_u._kids[0], where))
		{
			return FALSE;
		}

		*outexpr = tree;
		return TRUE;
	}
	else if (ctx->_currtk._type == TK_DEC) /* -- */
	{
		loc = ctx->_currtk._loc;
		cc_read_token(ctx, &ctx->_currtk);
		if (!(tree = cc_expr_new(where))) {
			return FALSE;
		}

		tree->_op = EXPR_PREDEC;
		tree->_loc = loc;
		if (!cc_expr_unary(ctx, &tree->_u._kids[0], where))
		{
			return FALSE;
		}

		*outexpr = tree;
		return TRUE;
	}
	else if (ctx->_currtk._type == TK_BITAND) /* '&' */
	{
		loc = ctx->_currtk._loc;
		cc_read_token(ctx, &ctx->_currtk);
		if (!(tree = cc_expr_new(where))) {
			return FALSE;
		}

		tree->_op = EXPR_ADDR;
		tree->_loc = loc;
		if (!cc_expr_cast(ctx, &tree->_u._kids[0], where))
		{
			return FALSE;
		}

		*outexpr = tree;
		return TRUE;
	}
	else if (ctx->_currtk._type == TK_MUL) /* '*' */
	{
		loc = ctx->_currtk._loc;
		cc_read_token(ctx, &ctx->_currtk);
		if (!(tree = cc_expr_new(where))) {
			return FALSE;
		}

		tree->_op = EXPR_DEREF;
		tree->_loc = loc;
		if (!cc_expr_cast(ctx, &tree->_u._kids[0], where))
		{
			return FALSE;
		}

		*outexpr = tree;
		return TRUE;
	}
	else if (ctx->_currtk._type == TK_ADD) /* '+' */
	{
		loc = ctx->_currtk._loc;
		cc_read_token(ctx, &ctx->_currtk);
		if (!(tree = cc_expr_new(where))) {
			return FALSE;
		}

		tree->_op = EXPR_POS;
		tree->_loc = loc;
		if (!cc_expr_cast(ctx, &tree->_u._kids[0],where))
		{
			return FALSE;
		}

		*outexpr = tree;
		return TRUE;
	}
	else if (ctx->_currtk._type == TK_SUB) /* '-' */
	{
		loc = ctx->_currtk._loc;
		cc_read_token(ctx, &ctx->_currtk);
		if (!(tree = cc_expr_new(where))) {
			return FALSE;
		}

		tree->_op = EXPR_NEG;
		tree->_loc = loc;
		if (!cc_expr_cast(ctx, &tree->_u._kids[0], where))
		{
			return FALSE;
		}

		*outexpr = tree;
		return TRUE;
	}
	else if (ctx->_currtk._type == TK_COMP) /* '~' */
	{
		loc = ctx->_currtk._loc;
		cc_read_token(ctx, &ctx->_currtk);
		if (!(tree = cc_expr_new(where))) {
			return FALSE;
		}

		tree->_op = EXPR_COMPLEMENT;
		tree->_loc = loc;
		if (!cc_expr_cast(ctx, &tree->_u._kids[0], where))
		{
			return FALSE;
		}

		*outexpr = tree;
		return TRUE;
	}
	else if (ctx->_currtk._type == TK_NOT) /* '!' */
	{
		loc = ctx->_currtk._loc;
		cc_read_token(ctx, &ctx->_currtk);
		if (!(tree = cc_expr_new(where))) {
			return FALSE;
		}

		tree->_op = EXPR_NOT;
		tree->_loc = loc;
		if (!cc_expr_cast(ctx, &tree->_u._kids[0], where))
		{
			return FALSE;
		}

		*outexpr = tree;
		return TRUE;
	}
	else if (ctx->_currtk._type == TK_SIZEOF)
	{
		loc = ctx->_currtk._loc;
		cc_read_token(ctx, &ctx->_currtk);
		if (!(tree = cc_expr_new(where))) {
			return FALSE;
		}

		tree->_op = EXPR_SIZEOF;
		tree->_loc = loc;
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

				tree->_ty = ty;
				tree->_u._kids[0] = NULL;

				*outexpr = tree;
				return TRUE;
			}
		}

		/* go through */
		tree->_ty = NULL;
		if (!cc_expr_unary(ctx, &tree->_u._kids[0], where))
		{
			return FALSE;
		}

		*outexpr = tree;
		return TRUE;
	}
	else
	{
		return cc_expr_postfix(ctx, outexpr, where);
	}

	return FALSE;
}

static BOOL cc_expr_cast(struct tagCCContext* ctx, FCCExprTree** outexpr, enum EMMArea where)
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

			if (!(tree = cc_expr_new(where))) {
				return FALSE;
			}
			tree->_op = EXPR_TYPECAST;
			tree->_loc = ctx->_currtk._loc;
			tree->_ty = ty;
			if (!cc_expr_cast(ctx, &tree->_u._kids[0], where)) {
				return FALSE;
			}

			*outexpr = tree;
			return TRUE;
		}
	}

	return cc_expr_unary(ctx, outexpr, where);
}

static BOOL cc_expr_multiplicative(struct tagCCContext* ctx, FCCExprTree** outexpr, enum EMMArea where)
{
	FCCExprTree* lhs, *rhs, *tree;
	FLocation loc;


	if (!cc_expr_cast(ctx, &lhs, where))
	{
		return FALSE;
	}

	tree = lhs;
	while (ctx->_currtk._type == TK_MUL || ctx->_currtk._type == TK_DIV || ctx->_currtk._type == TK_MOD)
	{
		enum ECCToken tktype = ctx->_currtk._type;

		loc = ctx->_currtk._loc;
		cc_read_token(ctx, &ctx->_currtk);
		if (!cc_expr_cast(ctx, &rhs, where)) {
			return FALSE;
		}

		if (!(tree = cc_expr_new(where))) {
			return FALSE;
		}

		tree->_op = (tktype == TK_MUL) ? EXPR_MUL : (tktype == TK_DIV ? EXPR_DIV : EXPR_MOD);
		tree->_loc = loc;
		tree->_u._kids[0] = lhs;
		tree->_u._kids[1] = rhs;
		
		lhs = tree;
	} /* end while */
	
	*outexpr = tree;
	return TRUE;
}

static BOOL cc_expr_additive(struct tagCCContext* ctx, FCCExprTree** outexpr, enum EMMArea where)
{
	FCCExprTree* lhs, * rhs, * tree;
	FLocation loc;


	if (!cc_expr_multiplicative(ctx, &lhs, where))
	{
		return FALSE;
	}

	tree = lhs;
	while (ctx->_currtk._type == TK_ADD || ctx->_currtk._type == TK_SUB)
	{
		enum ECCToken tktype = ctx->_currtk._type;

		loc = ctx->_currtk._loc;
		cc_read_token(ctx, &ctx->_currtk);
		if (!cc_expr_multiplicative(ctx, &rhs, where)) {
			return FALSE;
		}

		if (!(tree = cc_expr_new(where))) {
			return FALSE;
		}

		tree->_op = (tktype == TK_ADD) ? EXPR_ADD : EXPR_SUB;
		tree->_loc = loc;
		tree->_u._kids[0] = lhs;
		tree->_u._kids[1] = rhs;

		lhs = tree;
	} /* end while */

	*outexpr = tree;
	return TRUE;
}

static BOOL cc_expr_shift(struct tagCCContext* ctx, FCCExprTree** outexpr, enum EMMArea where)
{
	FCCExprTree* lhs, * rhs, * tree;
	FLocation loc;


	if (!cc_expr_additive(ctx, &lhs, where))
	{
		return FALSE;
	}

	tree = lhs;
	while (ctx->_currtk._type == TK_LSHIFT || ctx->_currtk._type == TK_RSHIFT)
	{

		enum ECCToken tktype = ctx->_currtk._type;

		loc = ctx->_currtk._loc;
		cc_read_token(ctx, &ctx->_currtk);
		if (!cc_expr_additive(ctx, &rhs, where)) {
			return FALSE;
		}

		if (!(tree = cc_expr_new(where))) {
			return FALSE;
		}

		tree->_op = (tktype == TK_LSHIFT) ? EXPR_LSHFIT : EXPR_RSHFIT;
		tree->_loc = loc;
		tree->_u._kids[0] = lhs;
		tree->_u._kids[1] = rhs;

		lhs = tree;
	} /* end while */

	*outexpr = tree;
	return TRUE;
}

static BOOL cc_expr_relational(struct tagCCContext* ctx, FCCExprTree** outexpr, enum EMMArea where)
{
	FCCExprTree* lhs, * rhs, * tree;
	FLocation loc;

	if (!cc_expr_shift(ctx, &lhs, where))
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
		if (!cc_expr_shift(ctx, &rhs, where)) {
			return FALSE;
		}

		if (!(tree = cc_expr_new(where))) {
			return FALSE;
		}

		switch (tktype)
		{
		case TK_LESS: op = EXPR_LESS; break;
		case TK_LESS_EQ: op = EXPR_LESSEQ; break;
		case TK_GREAT: op = EXPR_GREAT; break;
		case TK_GREAT_EQ: op = EXPR_GREATEQ; break;
		}

		tree->_op = op;
		tree->_loc = loc;
		tree->_u._kids[0] = lhs;
		tree->_u._kids[1] = rhs;

		lhs = tree;
	} /* end while */

	*outexpr = tree;
	return TRUE;
}

static BOOL cc_expr_equality(struct tagCCContext* ctx, FCCExprTree** outexpr, enum EMMArea where)
{
	FCCExprTree* lhs, * rhs, * tree;
	FLocation loc;


	if (!cc_expr_relational(ctx, &lhs, where))
	{
		return FALSE;
	}

	tree = lhs;
	while (ctx->_currtk._type == TK_EQUAL || ctx->_currtk._type == TK_UNEQUAL)
	{

		enum ECCToken tktype = ctx->_currtk._type;

		loc = ctx->_currtk._loc;
		cc_read_token(ctx, &ctx->_currtk);
		if (!cc_expr_relational(ctx, &rhs, where)) {
			return FALSE;
		}

		if (!(tree = cc_expr_new(where))) {
			return FALSE;
		}

		tree->_op = (tktype == TK_EQUAL) ? EXPR_EQ : EXPR_UNEQ;
		tree->_loc = loc;
		tree->_u._kids[0] = lhs;
		tree->_u._kids[1] = rhs;

		lhs = tree;
	} /* end while */

	*outexpr = tree;
	return TRUE;
}

static BOOL cc_expr_bitand(struct tagCCContext* ctx, FCCExprTree** outexpr, enum EMMArea where)
{
	FCCExprTree* lhs, * rhs, * tree;
	FLocation loc;

	if (!cc_expr_equality(ctx, &lhs, where))
	{
		return FALSE;
	}

	tree = lhs;
	while (ctx->_currtk._type == TK_BITAND)
	{
		loc = ctx->_currtk._loc;
		cc_read_token(ctx, &ctx->_currtk);
		if (!cc_expr_equality(ctx, &rhs, where)) {
			return FALSE;
		}

		if (!(tree = cc_expr_new(where))) {
			return FALSE;
		}

		tree->_op = EXPR_BITAND;
		tree->_loc = loc;
		tree->_u._kids[0] = lhs;
		tree->_u._kids[1] = rhs;

		lhs = tree;
	} /* end while */

	*outexpr = tree;
	return TRUE;
}

static BOOL cc_expr_bitxor(struct tagCCContext* ctx, FCCExprTree** outexpr, enum EMMArea where)
{
	FCCExprTree* lhs, * rhs, * tree;
	FLocation loc;

	if (!cc_expr_bitand(ctx, &lhs, where))
	{
		return FALSE;
	}

	tree = lhs;
	while (ctx->_currtk._type == TK_BITXOR)
	{
		loc = ctx->_currtk._loc;
		cc_read_token(ctx, &ctx->_currtk);
		if (!cc_expr_bitand(ctx, &rhs, where)) {
			return FALSE;
		}

		if (!(tree = cc_expr_new(where))) {
			return FALSE;
		}

		tree->_op = EXPR_BITXOR;
		tree->_loc = loc;
		tree->_u._kids[0] = lhs;
		tree->_u._kids[1] = rhs;

		lhs = tree;
	} /* end while */

	*outexpr = tree;
	return TRUE;
}

static BOOL cc_expr_bitor(struct tagCCContext* ctx, FCCExprTree** outexpr, enum EMMArea where)
{
	FCCExprTree* lhs, * rhs, * tree;
	FLocation loc;

	if (!cc_expr_bitxor(ctx, &lhs, where))
	{
		return FALSE;
	}

	tree = lhs;
	while (ctx->_currtk._type == TK_BITOR)
	{
		loc = ctx->_currtk._loc;
		cc_read_token(ctx, &ctx->_currtk);
		if (!cc_expr_bitxor(ctx, &rhs, where)) {
			return FALSE;
		}

		if (!(tree = cc_expr_new(where))) {
			return FALSE;
		}

		tree->_op = EXPR_BITOR;
		tree->_loc = loc;
		tree->_u._kids[0] = lhs;
		tree->_u._kids[1] = rhs;

		lhs = tree;
	} /* end while */

	*outexpr = tree;
	return TRUE;
}

static BOOL cc_expr_logicand(struct tagCCContext* ctx, FCCExprTree** outexpr, enum EMMArea where)
{
	FCCExprTree* lhs, * rhs, * tree;
	FLocation loc;

	if (!cc_expr_bitor(ctx, &lhs, where))
	{
		return FALSE;
	}

	tree = lhs;
	while (ctx->_currtk._type == TK_AND)
	{
		loc = ctx->_currtk._loc;
		cc_read_token(ctx, &ctx->_currtk);
		if (!cc_expr_bitor(ctx, &rhs, where)) {
			return FALSE;
		}

		if (!(tree = cc_expr_new(where))) {
			return FALSE;
		}

		tree->_op = EXPR_LOGAND;
		tree->_loc = loc;
		tree->_u._kids[0] = lhs;
		tree->_u._kids[1] = rhs;

		lhs = tree;
	} /* end while */

	*outexpr = tree;
	return TRUE;
}

static BOOL cc_expr_logicor(struct tagCCContext* ctx, FCCExprTree** outexpr, enum EMMArea where)
{
	FCCExprTree* lhs, * rhs, * tree;
	FLocation loc;

	if (!cc_expr_logicand(ctx, &lhs, where))
	{
		return FALSE;
	}

	tree = lhs;
	while (ctx->_currtk._type == TK_OR)
	{
		loc = ctx->_currtk._loc;
		cc_read_token(ctx, &ctx->_currtk);
		if (!cc_expr_logicand(ctx, &rhs, where)) {
			return FALSE;
		}

		if (!(tree = cc_expr_new(where))) {
			return FALSE;
		}

		tree->_op = EXPR_LOGOR;
		tree->_loc = loc;
		tree->_u._kids[0] = lhs;
		tree->_u._kids[1] = rhs;

		lhs = tree;
	} /* end while */

	*outexpr = tree;
	return TRUE;
}

static BOOL cc_expr_conditional(struct tagCCContext* ctx, FCCExprTree** outexpr, enum EMMArea where)
{
	FCCExprTree* tree, *expr0, * expr1, * expr2;
	FLocation loc;

	if (!cc_expr_logicor(ctx, &expr0, where))
	{
		return FALSE;
	}
	tree = expr0;

	if (ctx->_currtk._type == TK_QUESTION) /* '?' */
	{
		loc = ctx->_currtk._loc;
		cc_read_token(ctx, &ctx->_currtk);

		if (!cc_expr_expression(ctx, &expr1, where))
		{
			return FALSE;
		}

		if (!cc_parser_expect(ctx, TK_COLON)) /* ':' */
		{
			return FALSE;
		}

		if (!cc_expr_conditional(ctx, &expr2, where))
		{
			return FALSE;
		}

		if (!(tree = cc_expr_new(where))) {
			return FALSE;
		}

		tree->_op = EXPR_COND;
		tree->_loc = loc;
		tree->_u._kids[0] = expr0;
		tree->_u._kids[1] = expr1;
		tree->_u._kids[2] = expr2;
	}

	*outexpr = tree;
	return TRUE;
}

BOOL cc_expr_assignment(struct tagCCContext* ctx, FCCExprTree** outexpr, enum EMMArea where)
{
	FCCExprTree *tree, *lhs, *rhs;
	FLocation loc;

	if (!cc_expr_conditional(ctx, &lhs, where)) {
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

		if (!cc_expr_assignment(ctx, &rhs, where)) {
			return FALSE;
		}

		// TODO: check if l-value
		if (!(tree = cc_expr_new(where))) {
			return FALSE;
		}

		tree->_op = op;
		tree->_loc = loc;
		tree->_u._kids[0] = lhs;
		tree->_u._kids[1] = rhs;
	}

	*outexpr = tree;
	return TRUE;
}

BOOL cc_expr_expression(struct tagCCContext* ctx, FCCExprTree** outexpr, enum EMMArea where)
{
	FCCExprTree* lhs, * rhs, * tree;
	FLocation loc;

	if (!cc_expr_assignment(ctx, &lhs, where))
	{
		return FALSE;
	}

	tree = lhs;
	while (ctx->_currtk._type == TK_COMMA) /* ',' */
	{
		loc = ctx->_currtk._loc;
		cc_read_token(ctx, &ctx->_currtk);
		if (!cc_expr_assignment(ctx, &rhs, where)) {
			return FALSE;
		}

		if (!(tree = cc_expr_new(where))) {
			return FALSE;
		}

		tree->_op = EXPR_COMMA;
		tree->_loc = loc;
		tree->_u._kids[0] = lhs;
		tree->_u._kids[1] = rhs;

		lhs = tree;
	} /* end while */

	*outexpr = tree;
	return TRUE;
}

BOOL cc_expr_constant_expression(struct tagCCContext* ctx, FCCExprTree** outexpr, enum EMMArea where)
{
	return cc_expr_conditional(ctx, outexpr, where);
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

BOOL cc_expr_constant_int(struct tagCCContext* ctx, int* val)
{
	FCCExprTree* expr;

	*val = 0;
	if (!cc_expr_constant_expression(ctx, &expr, CC_MM_TEMPPOOL) || expr==NULL) {
		return FALSE;
	}

	if (expr->_op == EXPR_CONSTANT)
	{
		if (expr->_ty->_op == Type_SInteger)
		{
			*val = (int)expr->_u._symbol->_u._cnstval._sint;
			return TRUE;
		}
		if (expr->_ty->_op == Type_UInteger)
		{
			*val = (int)expr->_u._symbol->_u._cnstval._uint;
			return TRUE;
		}
	}

	return FALSE;
}
