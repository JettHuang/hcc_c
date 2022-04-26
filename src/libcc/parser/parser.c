/* \brief
 *		c parser
 */

#include "cc.h"
#include "lexer/token.h"
#include "parser.h"
#include "logger.h"
#include "init.h"
#include "expr.h"
#include "stmt.h"
#include "ir/ir.h"
#include "ir/canon.h"
#include "ir/dag.h"
#include "gen/backend.h"


const char* cc_sclass_displayname(int sclass);

static void cc_ext_types()
{
	FCCSymbol* p;

	p = cc_symbol_install(hs_hashstr("_Bool"), &gIdentifiers, SCOPE_GLOBAL, CC_MM_PERMPOOL);
	p->_type = gbuiltintypes._chartype;
	p->_sclass = SC_Typedef;
}

BOOL cc_parser_program(FCCContext* ctx)
{
	gCurrentLevel = SCOPE_GLOBAL;

	/* add extension types */
	cc_ext_types();

	if (!cc_read_token(ctx, &ctx->_currtk)) {
		return FALSE;
	}

	ctx->_backend->_program_begin(ctx);
	while (ctx->_currtk._type != TK_EOF)
	{
		if (cc_parser_is_typename(&ctx->_currtk))
		{
			if (!cc_parser_declaration(ctx, &cc_parser_declglobal))
			{
				return FALSE;
			}
		}
		else
		{
			if (ctx->_currtk._type == TK_SEMICOLON)
			{
				logger_output_s("empty declaration at %w.\n", &ctx->_currtk._loc);
			}
			else
			{
				logger_output_s("error: unrecognized declaration at %w.\n", &ctx->_currtk._loc);
			}

			if (!cc_read_token(ctx, &ctx->_currtk)) {
				logger_output_s("error: read token failed. %s:%d\n", __FILE__, __LINE__);
				return FALSE;
			}
		}
	} /* end while */

	/* dump data */
	cc_gen_dumpsymbols(ctx);

	ctx->_backend->_program_end(ctx);
	return TRUE;
}

BOOL cc_parser_declaration(FCCContext* ctx, FDeclCallback callback)
{
	int storage;
	FCCType* basety;
	enum ECCToken tktype;

	if (!(basety = cc_parser_declspecifier(ctx, &storage)))
	{
		return FALSE;
	}

	tktype = ctx->_currtk._type;
	if (tktype == TK_ID || tktype == TK_MUL || tktype == TK_LPAREN || tktype == TK_LBRACKET ||
		tktype == TK_STDCALL || tktype == TK_CDECL || tktype == TK_FASTCALL ) /* id * ( [ */
	{
		const char* id = NULL;
		FCCType* ty = NULL;
		FLocation loc = { NULL, 0, 0 };

		if (gCurrentLevel == SCOPE_GLOBAL)
		{
			FArray params;
			array_init(&params, 32, sizeof(FCCSymbol*), CC_MM_TEMPPOOL);
			if (!cc_parser_declarator(ctx, basety, &id, &loc, &params, &ty)) {
				logger_output_s("error: parser declarator failed. at %w\n", &ctx->_currtk._loc);
				return FALSE;
			}

			
			if (IsFunction(ty) && ctx->_currtk._type == TK_LBRACE) /* '{' check if a function definition */
			{
				if (storage == SC_Typedef)
				{
					logger_output_s("error: invalid use of typedef for function at %w\n", &ctx->_currtk._loc);
					storage = SC_External;
				}

				return cc_parser_funcdefinition(ctx, storage, id, ty, &loc, &params);
			}

			if (gCurrentLevel != SCOPE_GLOBAL) {
				cc_symbol_exitscope();
			}
			assert(gCurrentLevel == SCOPE_GLOBAL);
		}
		else
		{
			if (!cc_parser_declarator(ctx, basety, &id, &loc, NULL, &ty)) {
				logger_output_s("error: parser declarator failed. at %w\n", &ctx->_currtk._loc);
				return FALSE;
			}
		}

		
		/* loop for other declarators */
		for (;;)
		{
			if (!id) 
			{
				logger_output_s("syntax error: missing identifier before %w\n", &ctx->_currtk._loc);
				return FALSE;
			}
			else if (storage == SC_Typedef)
			{
				FCCSymbol* p = cc_symbol_lookup(id, gIdentifiers);
				if (p && p->_scope == gCurrentLevel && !cc_type_isequal(p->_type, ty, TRUE))
				{
					logger_output_s("error: redefinition of %s, previous definition is at %w\n", id, &p->_loc);
					return FALSE;
				}

				p = cc_symbol_install(id, &gIdentifiers, gCurrentLevel, gCurrentLevel <= SCOPE_GLOBAL ? CC_MM_PERMPOOL : CC_MM_TEMPPOOL);
				p->_type = ty;
				p->_sclass = storage;
				p->_loc = loc;
			}
			else 
			{
				FCCSymbol* p = (*callback)(ctx, storage, id, &loc, ty);
				if (!p) {
					return FALSE;
				}

				/*
					logger_output_s("debug: symbol: %s, %s, %w, %t\n", cc_sclass_displayname(p->_sclass), p->_name, &p->_loc, p->_type);
				*/
			}

			if (ctx->_currtk._type != TK_COMMA) /* ',' */
			{
				break;
			}

			if (!cc_read_token(ctx, &ctx->_currtk)) {
				return FALSE;
			}

			id = NULL;
			if (!cc_parser_declarator(ctx, basety, &id, &loc, NULL, &ty))
			{
				logger_output_s("error: parser declarator failed. at %w\n", &ctx->_currtk._loc);
				return FALSE;
			}
		} /* end for ;; */
	}
	else
	{
		BOOL bStructDecl = IsStruct(basety) && !cc_symbol_isgenerated(UnQual(basety)->_u._symbol->_name);
		if (!IsEnum(basety) && !bStructDecl)
		{
			logger_output_s("warning: empty declaration. at %w\n", &ctx->_currtk._loc);
		}
	}

	return cc_parser_expect(ctx, TK_SEMICOLON); /* ';' */
}

BOOL cc_parser_is_specifier(enum ECCToken tk)
{
	return gCCTokenMetas[tk]._flags & TK_IS_DECL_SPECIFIER;
}

BOOL cc_parser_is_constant(enum ECCToken tk)
{
	return gCCTokenMetas[tk]._flags & TK_IS_CONSTEXPR;
}

BOOL cc_parser_is_assign(enum ECCToken tk)
{
	return gCCTokenMetas[tk]._flags & TK_IS_ASSIGN;
}

BOOL cc_parser_is_stmtspecifier(enum ECCToken tk)
{
	return gCCTokenMetas[tk]._flags & (TK_IS_STATEMENT | TK_IS_CONSTEXPR);
}

BOOL cc_parser_is_typename(FCCToken* tk)
{
	FCCSymbol* p;

	return (gCCTokenMetas[tk->_type]._flags & TK_IS_DECL_SPECIFIER)
		|| (tk->_type == TK_ID && (p = cc_symbol_lookup(tk->_val._astr._str, gIdentifiers)) && p->_sclass == SC_Typedef);
}

BOOL cc_parser_expect(FCCContext* ctx, enum ECCToken tk)
{
	if (ctx->_currtk._type == tk)
	{
		if (!cc_read_token(ctx, &ctx->_currtk)) {
			return FALSE;
		}

		return TRUE;
	}

	logger_output_s("error: expect '%k' at %w\n", tk, &ctx->_currtk._loc);
	return FALSE;
}

FCCType* cc_parser_declspecifier(FCCContext* ctx, int* storage)
{
	FCCType* ty = NULL;
	int sclass = SC_Unknown;
	int typespec = TK_NONE;
	int typequal = Type_Unknown;
	int funcspec = FS_Unknown;
	int sign = Sign_Unknown;
	int size = Size_Unknown;
	BOOL  bQuitLoop = FALSE;

	while (!bQuitLoop)
	{
		switch (ctx->_currtk._type)
		{
			// storage classes
		case TK_AUTO:
			if (gCurrentLevel <= SCOPE_GLOBAL || sclass != SC_Unknown)
			{
				logger_output_s("error: invalid use of 'auto', at %w\n", &ctx->_currtk._loc);
				return NULL;
			}
			sclass = SC_Auto;
			cc_read_token(ctx, &ctx->_currtk);
			break;
		case TK_REGISTER:
			if (gCurrentLevel <= SCOPE_GLOBAL || sclass != SC_Unknown)
			{
				logger_output_s("error: invalid use of 'register', at %w\n", &ctx->_currtk._loc);
				return NULL;
			}
			sclass = SC_Register;
			cc_read_token(ctx, &ctx->_currtk);
			break;
		case TK_STATIC:
			if (sclass != SC_Unknown)
			{
				logger_output_s("error: more than one storage specifier, at %w\n", &ctx->_currtk._loc);
				return NULL;
			}
			sclass = SC_Static;
			cc_read_token(ctx, &ctx->_currtk);
			break;
		case TK_EXTERN:
			if (sclass != SC_Unknown)
			{
				logger_output_s("error: more than one storage specifier, at %w\n", &ctx->_currtk._loc);
				return NULL;
			}
			sclass = SC_External;
			cc_read_token(ctx, &ctx->_currtk);
			break;
		case TK_TYPEDEF:
			if (sclass != SC_Unknown)
			{
				logger_output_s("error: invalid use 'typedef', at %w\n", &ctx->_currtk._loc);
				return NULL;
			}
			sclass = SC_Typedef;
			cc_read_token(ctx, &ctx->_currtk);
			break;
		case TK_INLINE:
			if (funcspec != FS_Unknown)
			{
				logger_output_s("error: invalid use 'typedef', at %w\n", &ctx->_currtk._loc);
				return NULL;
			}
			funcspec = FS_INLINE;
			cc_read_token(ctx, &ctx->_currtk);
			break;

			// qualifier
		case TK_RESTRICT:
			typequal |= Type_Restrict;
			cc_read_token(ctx, &ctx->_currtk);
			break;
		case TK_VOLATILE:
			typequal |= Type_Volatile;
			cc_read_token(ctx, &ctx->_currtk);
			break;
		case TK_CONST:
			typequal |= Type_Const;
			cc_read_token(ctx, &ctx->_currtk);
			break;

			// sign
		case TK_SIGNED:
			if (sign != Sign_Unknown)
			{
				logger_output_s("error: invalid use 'signed', at %w\n", &ctx->_currtk._loc);
				return NULL;
			}
			sign = Sign_Signed;
			cc_read_token(ctx, &ctx->_currtk);
			break;
		case TK_UNSIGNED:
			if (sign != Sign_Unknown)
			{
				logger_output_s("error: invalid use 'unsigned', at %w\n", &ctx->_currtk._loc);
				return NULL;
			}
			sign = Sign_Unsigned;
			cc_read_token(ctx, &ctx->_currtk);
			break;

		case TK_SHORT:
			if (size != Size_Unknown)
			{
				logger_output_s("error: invalid use 'short', at %w\n", &ctx->_currtk._loc);
				return NULL;
			}
			size = Size_Short;
			cc_read_token(ctx, &ctx->_currtk);
			break;
		case TK_LONG:
			if (size == Size_Short || size == Size_Longlong)
			{
				logger_output_s("error: invalid use 'long', at %w\n", &ctx->_currtk._loc);
				return NULL;
			}
			else if (size == Size_Long)
			{
				size = Size_Longlong;
			}
			else
			{
				size = Size_Long;
			}
			cc_read_token(ctx, &ctx->_currtk);
			break;

		case TK_VOID:
			if (typespec != Type_Unknown)
			{
				logger_output_s("error: invalid use 'void', at %w\n", &ctx->_currtk._loc);
				return NULL;
			}
			typespec = Type_Void;
			ty = gbuiltintypes._voidtype;
			cc_read_token(ctx, &ctx->_currtk);
			break;
		case TK_CHAR:
			if (typespec != TK_NONE)
			{
				logger_output_s("error: invalid use 'char', at %w\n", &ctx->_currtk._loc);
				return NULL;
			}
			typespec = TK_CHAR;
			cc_read_token(ctx, &ctx->_currtk);
			break;
		case TK_INT:
			if (typespec != TK_NONE)
			{
				logger_output_s("error: invalid use 'int', at %w\n", &ctx->_currtk._loc);
				return NULL;
			}
			typespec = TK_INT;
			cc_read_token(ctx, &ctx->_currtk);
			break;
		case TK_FLOAT:
			if (typespec != TK_NONE)
			{
				logger_output_s("error: invalid use 'float', at %w\n", &ctx->_currtk._loc);
				return NULL;
			}
			typespec = TK_FLOAT;
			ty = gbuiltintypes._floattype;
			cc_read_token(ctx, &ctx->_currtk);
			break;
		case TK_DOUBLE:
			if (typespec != TK_NONE)
			{
				logger_output_s("error: invalid use 'double', at %w\n", &ctx->_currtk._loc);
				return NULL;
			}
			typespec = TK_DOUBLE;
			ty = gbuiltintypes._doubletype;
			cc_read_token(ctx, &ctx->_currtk);
			break;
		case TK_ENUM:
			if (typespec != TK_NONE)
			{
				logger_output_s("error: invalid use 'enum', at %w\n", &ctx->_currtk._loc);
				return NULL;
			}
			typespec = TK_ENUM;
			ty = cc_parser_declenum(ctx);
			break;
		case TK_STRUCT:
			if (typespec != TK_NONE)
			{
				logger_output_s("error: invalid use 'struct', at %w\n", &ctx->_currtk._loc);
				return NULL;
			}
			typespec = TK_STRUCT;
			ty = cc_parser_declstruct(ctx, Type_Struct);
			break;
		case TK_UNION:
			if (typespec != TK_NONE)
			{
				logger_output_s("error: invalid use 'union', at %w\n", &ctx->_currtk._loc);
				return NULL;
			}
			typespec = TK_UNION;
			ty = cc_parser_declstruct(ctx, Type_Union);
			break;
		case TK_ID:
		{
			FCCSymbol* p = NULL;

			if (typespec == TK_NONE && size == Size_Unknown && sign == Sign_Unknown
				&& (p = cc_symbol_lookup(ctx->_currtk._val._astr._str, gIdentifiers)) && p->_sclass == SC_Typedef)
			{
				typespec = TK_TYPEDEF;
				ty = p->_type;
				cc_read_token(ctx, &ctx->_currtk);
			}
			else
			{
				bQuitLoop = TRUE;
			}
		}
		break;
		default:
			bQuitLoop = TRUE;
			break;
		}
	} /* end while */

	if (typespec == Type_Unknown)
	{
		if (size != Size_Unknown || sign != Sign_Unknown)
		{
			typespec = TK_INT;
		}
		else
		{
			logger_output_s("syntax error: no type specifier, at %w\n", &ctx->_currtk._loc);
			return NULL;
		}
	}

	if (typespec == TK_CHAR)
	{
		if (size != Size_Unknown)
		{
			logger_output_s("syntax error: invalid size specifier for char, at %w\n", &ctx->_currtk._loc);
			return NULL;
		}

		ty = gbuiltintypes._chartype;
		if (sign == Sign_Signed)
		{
			ty = gbuiltintypes._schartype;
		}
		else if (sign == Sign_Unsigned)
		{
			ty = gbuiltintypes._uchartype;
		}
	}
	else if (typespec == TK_INT)
	{
		if (sign == Sign_Signed || sign == Sign_Unknown)
		{
			ty = gbuiltintypes._sinttype;
			if (size == Size_Short)
			{
				ty = gbuiltintypes._sshorttype;
			}
			else if (size == Size_Long)
			{
				ty = gbuiltintypes._slongtype;
			}
			else if (size == Size_Longlong)
			{
				ty = gbuiltintypes._sllongtype;
			}
		}
		else
		{
			ty = gbuiltintypes._uinttype;
			if (size == Size_Short)
			{
				ty = gbuiltintypes._ushorttype;
			}
			else if (size == Size_Long)
			{
				ty = gbuiltintypes._ulongtype;
			}
			else if (size == Size_Longlong)
			{
				ty = gbuiltintypes._ullongtype;
			}
		}
	}
	else if (typespec == TK_DOUBLE)
	{
		if (sign != Sign_Unknown || (size != Size_Unknown && size != Size_Long))
		{
			logger_output_s("syntax error: invalid type specifier, at %w\n", &ctx->_currtk._loc);
			return NULL;
		}
		if (size == Size_Long)
		{
			ty = gbuiltintypes._ldoubletype;
		}
	}
	else
	{
		if (sign != Sign_Unknown || size != Size_Unknown)
		{
			logger_output_s("syntax error: invalid type specifier, at %w\n", &ctx->_currtk._loc);
			return NULL;
		}
	}

	/* qualifier */
	if (typequal != 0) 
	{
		ty = cc_type_qual(ty, typequal);
	}
	
	if (storage)
	{
		if ((sclass == SC_Unknown || sclass == SC_Static) && funcspec == FS_INLINE)
		{
			sclass = SC_Static;
		}

		*storage = sclass;
	}

	return ty;
}

BOOL cc_parser_declarator(FCCContext* ctx, FCCType* basety, const char** id, FLocation* loc, FArray* params, FCCType** outty)
{
	BOOL bgetparams = TRUE;
	FCCType* ty = NULL;

	*id = NULL;
	if (!cc_parser_declarator1(ctx, id, loc, params, &bgetparams, &ty))
	{
		return FALSE;
	}
	for (; ty; ty = ty->_type)
	{
		if (ty->_op != Type_Function && (basety->_op == Type_Cdecl || basety->_op == Type_Stdcall || basety->_op == Type_Fastcall))
		{
			logger_output_s("syntax error: unexpected function calling convention at %w\n", ctx->_currtk._loc);
			return FALSE;
		}

		switch (ty->_op)
		{
		case Type_Array:
			basety = cc_type_newarray(basety, ty->_size, 0);
			break;
		case Type_Function:
		{
			int callconv = Type_Defcall;
			if (basety->_op == Type_Cdecl || basety->_op == Type_Stdcall || basety->_op == Type_Fastcall)
			{
				callconv = basety->_op;
				basety = basety->_type;
			}
			basety = cc_type_func(basety, callconv, ty->_u._f._protos, ty->_u._f._has_ellipsis);
			if (cc_type_isvariance(basety))
			{
				basety->_u._f._convention = Type_Cdecl;
			}
		}
			break;
		case Type_Pointer:
			basety = cc_type_ptr(basety);
			break;
		case Type_Cdecl:
		case Type_Stdcall:
		case Type_Fastcall:
		{
			if (basety->_op == Type_Function)
			{
				if (basety->_u._f._convention != Type_Defcall)
				{
					logger_output_s("syntax error: too much function calling convention at %w\n", ctx->_currtk._loc);
					return FALSE;
				}

				if (cc_type_isvariance(basety)) {
					basety->_u._f._convention = Type_Cdecl;
				}
				else {
					basety->_u._f._convention = ty->_op;
				}
			}
			else
			{
				basety = cc_type_tmp(ty->_op, basety);
			}
		}
			break;
		default:
			if (IsQual(ty)) {
				basety = cc_type_qual(basety, ty->_op);
			}
			else {
				assert(0);
			}
			break;
		}

		if (!basety) {
			return FALSE;
		}

	} /* end for */

	if (basety->_op == Type_Cdecl || basety->_op == Type_Stdcall || basety->_op == Type_Fastcall)
	{
		logger_output_s("syntax error: unexpected function calling convention at %w\n", ctx->_currtk._loc);
		return FALSE;
	}

	if (basety->_size > 32767) {
		logger_output_s("warning: more than 32767 bytes in `%t'\n", basety);
	}

	for (ty = basety; ty; ty = ty->_type)
	{
		if (ty->_op == Type_Function && ty->_u._f._convention == Type_Defcall)
		{
			ty->_u._f._convention = gdefcall;
		}
	} /* end for */
	
	*outty = basety;
	return TRUE;
}

BOOL cc_parser_declarator1(FCCContext* ctx, const char** id, FLocation* loc, FArray* params, BOOL* bgetparams, FCCType** outty)
{
	FCCType* ty = NULL;

	switch (ctx->_currtk._type)
	{
	case TK_ID:
	{
		if (*id) {
			logger_output_s("error: extraneous identifier '%s' at %w.\n", &ctx->_currtk._val._astr, &ctx->_currtk._loc);
			return FALSE;
		}

		*id = ctx->_currtk._val._astr._str;
		*loc = ctx->_currtk._loc;
		cc_read_token(ctx, &ctx->_currtk);
	}
	break;
	case TK_CDECL:
	case TK_STDCALL:
	case TK_FASTCALL:
	{
		int callty = Type_Defcall;

		switch (ctx->_currtk._type)
		{
		case TK_CDECL: callty = Type_Cdecl; break;
		case TK_STDCALL: callty = Type_Stdcall; break;
		case TK_FASTCALL: callty = Type_Fastcall; break;
		default: break;
		}

		cc_read_token(ctx, &ctx->_currtk);
		ty = cc_type_tmp(callty, NULL);
		if (!cc_parser_declarator1(ctx, id, loc, params, bgetparams, &ty->_type))
		{
			return FALSE;
		}
	}
	break;
	case TK_CONST:
	case TK_VOLATILE:
	case TK_RESTRICT:
	{
		int qual = 0;

		while (ctx->_currtk._type == TK_CONST || ctx->_currtk._type == TK_VOLATILE || ctx->_currtk._type == TK_RESTRICT)
		{
			switch (ctx->_currtk._type)
			{
			case TK_CONST: qual |= Type_Const; break;
			case TK_VOLATILE: qual |= Type_Volatile; break;
			default: qual |= Type_Restrict; break;
			}

			cc_read_token(ctx, &ctx->_currtk);
		} /* end while */

		ty = cc_type_tmp(qual, NULL);
		if (!cc_parser_declarator1(ctx, id, loc, params, bgetparams, &ty->_type))
		{
			return FALSE;
		}
	}
	break;
	case TK_MUL: /* '*' */
	{
		ty = cc_type_tmp(Type_Pointer, NULL);
		cc_read_token(ctx, &ctx->_currtk);

		if (!cc_parser_declarator1(ctx, id, loc, params, bgetparams, &ty->_type))
		{
			return FALSE;
		}
	}
	break;
	case TK_LPAREN: /* '(' */
	{
		cc_read_token(ctx, &ctx->_currtk);
		if (ctx->_currtk._type == TK_RPAREN || cc_parser_is_typename(&ctx->_currtk))  /* ie. int (int),  int () */
		{
			FArray* names = NULL;
			if (*bgetparams) {
				names = params;
				*bgetparams = FALSE;
			}

			cc_read_token(ctx, &ctx->_currtk);
			ty = cc_type_tmp(Type_Function, ty);
			cc_symbol_enterscope();
			if (!cc_parser_parameters(ctx, ty, names))
			{
				logger_output_s("error: parsing parameter failed. %w\n", &ctx->_currtk);
				return FALSE;
			}
			if (gCurrentLevel > SCOPE_PARAM)
			{
				cc_symbol_exitscope(); 
			}
		}
		else 
		{
			if (!cc_parser_declarator1(ctx, id, loc, params, bgetparams, &ty))
			{
				return FALSE;
			}
			cc_parser_expect(ctx, TK_RPAREN); /* ')' */
		}
	}
	break;
	case TK_LBRACKET: /* '[' */
		break;
	default:
		*outty = ty;
		return TRUE;
	}

	while (ctx->_currtk._type == TK_LPAREN || ctx->_currtk._type == TK_LBRACKET) /* '(' '[' */
	{
		if (ctx->_currtk._type == TK_LPAREN) /* ( */
		{
			FArray* names = NULL;
			if (*bgetparams) {
				names = params;
				*bgetparams = FALSE;
			}

			cc_read_token(ctx, &ctx->_currtk);
			ty = cc_type_tmp(Type_Function, ty);
			cc_symbol_enterscope();
			if (!cc_parser_parameters(ctx, ty, names))
			{
				logger_output_s("error: parsing parameter failed. %w\n", &ctx->_currtk._loc);
				return FALSE;
			}
			if (gCurrentLevel > SCOPE_PARAM)
			{
				cc_symbol_exitscope();
			}
		}
		else if (ctx->_currtk._type == TK_LBRACKET) /* '[' */
		{
			int cnt = 0;

			cc_read_token(ctx, &ctx->_currtk);
			if (cc_parser_is_constant(ctx->_currtk._type))
			{
				if (!cc_expr_parse_constant_int(ctx, &cnt))
				{
					logger_output_s("error: need integer constant. at %w.\n", &ctx->_currtk._loc);
					return FALSE;
				}
				if (cnt <= 0)
				{
					logger_output_s("error: illegal array size %d. at %w.\n", cnt, &ctx->_currtk._loc);
					return FALSE;
				}
			}

			if (!cc_parser_expect(ctx, TK_RBRACKET)) /* ']' */
			{
				return FALSE;
			}

			ty = cc_type_tmp(Type_Array, ty);
			ty->_size = cnt;
		}
	} /* end while */

	*outty = ty;
	return TRUE;
}

BOOL cc_parser_parameters(FCCContext* ctx, FCCType* fn, FArray* params)
{
	FArray protos;
	BOOL has_ellipsis = FALSE;
	
	array_init(&protos, 32, sizeof(FCCType*), CC_MM_PERMPOOL);
	if (cc_parser_is_typename(&ctx->_currtk))
	{
		BOOL hasvoid, hasunvoid;

		hasvoid = hasunvoid = FALSE;
		for (;;)
		{
			FCCType* ty, * basety;
			FCCSymbol* p;

			const char* paramname = NULL;
			int storage = 0;
			FLocation loc = { NULL, 0, 0 };
			

			if (ctx->_currtk._type == TK_ELLIPSIS) /* ... */
			{
				if (!hasunvoid)
				{
					logger_output_s("error: illegal formal parameter '...' at %w\n", &ctx->_currtk._loc);
					return FALSE;
				}

				has_ellipsis = TRUE;
				cc_read_token(ctx, &ctx->_currtk);
				break;
			}

			basety = cc_parser_declspecifier(ctx, &storage);
			if (!basety || !cc_parser_declarator(ctx, basety, &paramname, &loc, NULL, &ty)) {
				logger_output_s("error: illegal formal parameter, expect type at '%w'\n", &ctx->_currtk._loc);
				return FALSE;
			}

			if ((hasunvoid || hasvoid) && UnQual(ty)->_op == Type_Void)
			{
				logger_output_s("error: illegal formal parameter, 'void' is unexpected at '%w'\n", &ctx->_currtk._loc);
				return FALSE;
			}

			if (UnQual(ty)->_op != Type_Void) {
				hasunvoid = TRUE;
			}
			else {
				hasvoid = TRUE;
				break;
			}

			array_append(&protos, &ty);
			if (!(p = cc_parser_declparam(ctx, storage, paramname, &loc, ty)))
			{
				return FALSE;
			}

			/* save param */
			if (params) {
				array_append(params, &p);
			}

			if (ctx->_currtk._type != TK_COMMA) /* ',' */
			{
				break;
			}

			cc_read_token(ctx, &ctx->_currtk);
		} /* end for ;; */
	}

	/* save protos */
	{
		FCCType* ty = NULL;

		array_append(&protos, &ty); /* append end null */
		fn->_u._f._protos = (FCCType**)protos._data;
		fn->_u._f._has_ellipsis = has_ellipsis;
	}

	return cc_parser_expect(ctx, TK_RPAREN); /* ')' */
}

FCCSymbol* cc_parser_declglobal(FCCContext* ctx, int storage, const char* id, const FLocation* loc, FCCType* ty)
{
	FCCSymbol* p;

	if (storage == 0) {
		storage = SC_Auto;
	}
	else if (storage != SC_External && storage != SC_Static) {
		logger_output_s("error: invalid storage class '%s', at %w\n", cc_sclass_displayname(storage), loc);
		return NULL;
	}

	p = cc_symbol_lookup(id, gIdentifiers);
	if (p && p->_scope == SCOPE_GLOBAL)
	{ 
		if (p->_sclass != SC_Typedef && cc_type_isequal(ty, p->_type, TRUE)) {
			ty = cc_type_compose(ty, p->_type);
		}
		else {
			logger_output_s("error: redeclaration of '%s' at %w. previously declared at %w\n", p->_name, loc, &p->_loc);
			return FALSE;
		}

		if (!IsFunction(ty) && p->_defined && ctx->_currtk._type == TK_ASSIGN) { /* '=' */
			logger_output_s("error: redefinition of '%s' at %w. previously defined at %w\n", p->_name, loc, &p->_loc);
			return FALSE;
		}

		if (p->_sclass == SC_External && storage == SC_Static
			|| p->_sclass == SC_Static && storage == SC_Auto
			|| p->_sclass == SC_Auto && storage == SC_Static)
		{
			logger_output_s("warning: inconsistent linkage for '%s' previously declared at '%w'.\n", p->_name, &p->_loc);
		}
	}

	if (p == NULL || p->_scope != SCOPE_GLOBAL)
	{
		FCCSymbol* q = cc_symbol_lookup(id, gExternals);
		if (q) 
		{
			if (storage == SC_Static || !cc_type_isequal(ty, q->_type, TRUE)) {
				logger_output_s("warning: declaration of '%s' does not match previous declaration at %w.\n", id, &q->_loc);
			}

			p = cc_symbol_relocate(id, gExternals, gGlobals);
			p->_sclass = storage;
		}
		else 
		{
			p = cc_symbol_install(id, &gGlobals, SCOPE_GLOBAL, CC_MM_PERMPOOL);
			p->_sclass = storage;
		}
	}
	else if (p->_sclass == SC_External)
	{
		p->_sclass = storage;
	}

	p->_type = ty;
	p->_loc = *loc;

	cc_gen_internalname(p);
	if (ctx->_currtk._type == TK_ASSIGN && IsFunction(p->_type)) {
		logger_output_s("error: illegal initialization for '%s' at %w\n", p->_name, loc);
		return NULL;
	}
	else if (ctx->_currtk._type == TK_ASSIGN) /* '=' */
	{
		FVarInitializer* initializer;

		cc_read_token(ctx, &ctx->_currtk);
		if (!cc_parser_initializer(ctx, &initializer, TRUE, CC_MM_PERMPOOL)) {
			logger_output_s("error: illegal initialization for '%s' at %w\n", p->_name, loc);
			return FALSE;
		}

		if (!cc_varinit_check(ctx, p->_type, initializer, TRUE, CC_MM_PERMPOOL)) {
			return FALSE;
		}

		if (p->_sclass == SC_External) {
			p->_sclass = SC_Auto;
		}

		p->_defined = 1;
		p->_u._initializer = initializer;
	}
	else if (p->_sclass == SC_Static && !IsFunction(p->_type) && p->_type->_size == 0)
	{
		logger_output_s("error: undefined size of '%s' at %w\n", p->_name, &p->_loc);
		return NULL;
	}

	return p;
}

FCCSymbol* cc_parser_decllocal(FCCContext* ctx, int storage, const char* id, const FLocation* loc, FCCType* ty)
{
	FCCSymbol* p, * q;

	if (storage == SC_Unknown) 
	{
		storage = IsFunction(ty) ? SC_External : SC_Auto;
	}
	else if (IsFunction(ty) && storage != SC_External) 
	{
		logger_output_s("warning: invalid storage class for function '%s' at %w.\n", id, loc);
		storage = SC_External;
	}
	else if (storage == SC_Register && (IsVolatile(ty) || IsStruct(ty) || IsArray(ty))) {
		logger_output_s("warning: register declaration ignored for '%s' at %w.\n", id, loc);
		storage = SC_Auto;
	}

	q = cc_symbol_lookup(id, gIdentifiers);
	if (q && (q->_scope >= gCurrentLevel || (q->_scope == SCOPE_PARAM && gCurrentLevel == SCOPE_LOCAL)))
	{
		if (storage == SC_External && q->_sclass == SC_External && cc_type_isequal(q->_type, ty, TRUE))
		{
			ty = cc_type_compose(ty, q->_type);
		}
		else
		{
			logger_output_s("error: redeclaration of '%s' previously declared at '%w'.\n", q->_name, &q->_loc);
			return NULL;
		}
	}

	assert(gCurrentLevel >= SCOPE_LOCAL);

	p = cc_symbol_install(id, &gIdentifiers, gCurrentLevel, CC_MM_TEMPPOOL);
	p->_type = ty;
	p->_sclass = storage;
	p->_loc = *loc;

	switch (storage)
	{
	case SC_External:
		q = cc_symbol_lookup(id, gGlobals);
		if (q == NULL || q->_sclass == SC_Typedef || q->_sclass == SC_Enum)
		{
			q = cc_symbol_lookup(id, gExternals);
			if (q == NULL)
			{
				q = cc_symbol_install(id, &gExternals, SCOPE_GLOBAL, CC_MM_PERMPOOL);
				q->_type = p->_type;
				q->_sclass = SC_External;
				q->_loc = *loc;

				cc_gen_internalname(q);
			}
		}
		if (!cc_type_isequal(p->_type, q->_type, TRUE)) {
			logger_output_s("error: declaration of '%s' does not match previous declaration at %w.\n", q->_name, &q->_loc);
			return NULL;
		}

		p->_u._alias = q;
		break;
	case SC_Static:
		q = cc_symbol_install(hs_hashstr(util_itoa(cc_symbol_genlabel(1))), &gGlobals, SCOPE_GLOBAL, CC_MM_PERMPOOL);
		q->_type = p->_type;
		q->_sclass = SC_Static;
		q->_loc = *loc;
		q->_generated = 1;
		cc_gen_internalname(q);

		p->_u._alias = q;
		p->_defined = 1;
		break;
	case SC_Register:
		p->_defined = 1;
		break;
	case SC_Auto:
		p->_defined = 1;
		if (IsArray(ty)) {
			p->_addressed = 1;
		}
		break;
	default:
		assert(0); break;
	}

	if (ctx->_currtk._type == TK_ASSIGN) /* '=' */
	{
		FVarInitializer* initializer;
		BOOL bExpectConstant = FALSE;

		if (storage == SC_External) {
			logger_output_s("error: illegal initialization of 'extern %s' at %w\n", id, loc);
			return NULL;
		}
		
		bExpectConstant = (storage == SC_Static);
		cc_read_token(ctx, &ctx->_currtk);
		if (!cc_parser_initializer(ctx, &initializer, bExpectConstant, storage == SC_Static ? CC_MM_PERMPOOL : CC_MM_TEMPPOOL)) {
			logger_output_s("error: illegal initialization for '%s' at %w\n", p->_name, loc);
			return FALSE;
		}

		if (!cc_varinit_check(ctx, p->_type, initializer, bExpectConstant, storage == SC_Static ? CC_MM_PERMPOOL : CC_MM_TEMPPOOL)) {
			return FALSE;
		}

		if (storage == SC_Static) {
			p->_u._alias->_u._initializer = initializer;
			q->_u._alias->_defined = initializer ? 1 : 0;
		}
		else {
			p->_u._initializer = initializer;
		}
		
	}

	assert(ctx->_codes);
	if (storage != SC_Static && storage != SC_External) {
		cc_ir_codelist_append(ctx->_codes, cc_ir_newcode_var(p, CC_MM_TEMPPOOL));
		/* generate initialization codes */
		if (p->_u._initializer && !cc_gen_localvar_initcodes(ctx->_codes, p))
		{
			logger_output_s("error: generate local variable initialization codes failed for '%s' at %w\n", id, loc);
			return NULL;
		}
	}

	if (!IsFunction(p->_type) && p->_defined && p->_type->_size <= 0) {
		logger_output_s("error: undefined size for '%s' at %w\n", id, loc);
		return NULL;
	}

	return p;
}

FCCSymbol* cc_parser_declparam(FCCContext* ctx, int storage, const char* id, const FLocation* loc, FCCType* ty)
{
	FCCSymbol* p;

	if (IsFunction(ty)) {
		ty = cc_type_ptr(ty);
	}
	else if (IsArray(ty)) {
		ty = cc_type_arraytoptr(ty);
	}

	if (storage == SC_Unknown) {
		storage = SC_Auto;
	}
	else if (storage != SC_Register) {
		logger_output_s("warning: invalid storage class for '%s' at %w\n", id, loc);
		storage = SC_Auto;
	}
	else if (IsVolatile(ty) || IsStruct(ty)) {
		logger_output_s("warning: register storage class ignored for '%s' at %w\n", id, loc);
		storage = SC_Auto;
	}

	if (!id || *id == '\0') {
		id = hs_hashstr(util_itoa(cc_symbol_genlabel(1)));
	}

	p = cc_symbol_lookup(id, gIdentifiers);
	if (p && p->_scope == gCurrentLevel)
	{
		logger_output_s("error: duplicate declaration for '%s' previously declared at %w\n", id, &p->_loc);
		return NULL;
	}

	p = cc_symbol_install(id, &gIdentifiers, gCurrentLevel, CC_MM_TEMPPOOL);
	p->_sclass = storage;
	p->_loc = *loc;
	p->_type = ty;
	p->_defined = 1;
	if (ctx->_currtk._type == TK_ASSIGN) /* '=' */
	{
		logger_output_s("error: illegal initialization for parameter '%s' at %w\n", id, loc);
		return NULL;
	}

	return p;
}

BOOL cc_parser_funcdefinition(FCCContext* ctx, int storage, const char* name, FCCType* fty, const FLocation* loc, FArray* params)
{
	FCCSymbol* p;

	if ((p = cc_symbol_lookup(name, gGlobals)))
	{
		if (IsFunction(p->_type))
		{
			if (p->_defined) {
				logger_output_s("error: function has defined %w, previous declared at %w\n", loc, &p->_loc);
				return FALSE;
			}
			if (!cc_type_isequal(p->_type, fty, FALSE)) {
				logger_output_s("error: function protos is not equal at %w, previous declared at %w\n", loc, &p->_loc);
				return FALSE;
			}
		}
		else
		{
			logger_output_s("error: '%s' redeclared as different type at %w, previous declared at %w\n", name, loc, &p->_loc);
			return FALSE;
		}
	}
	else
	{
		p = cc_symbol_install(name, &gGlobals, SCOPE_GLOBAL, CC_MM_PERMPOOL);
	}
	p->_sclass = storage;
	p->_loc = *loc;
	p->_type = fty;
	p->_defined = 1;
	cc_gen_internalname(p);

	/* clear labels table */
	cc_symbol_reset(gLabels);
	/* body */
	{
		FArray caller, callee;
		FCCIRCodeList codelist = { NULL, NULL };
		FCCSymbol* exitlab;
		FCCIRBasicBlock* basicblocks;
		int parambytes;

		exitlab = cc_symbol_label(NULL, NULL, CC_MM_TEMPPOOL);
		ctx->_function = p;
		ctx->_funcexit = exitlab;
		ctx->_codes = &codelist;

		/* generate ir-codes */
		cc_ir_codelist_append(&codelist, cc_ir_newcode(IR_FENTER, CC_MM_TEMPPOOL));

		/* caller & callee parameters converting */
		{
			FCCSymbol* retb, *p0, *p1;
			int n;
			
			if (IsStruct(fty->_type)) 
			{ 
				if (!(retb = cc_symbol_install("#", &gIdentifiers, gCurrentLevel, CC_MM_TEMPPOOL))) {
					return FALSE;
				}
				retb->_type = cc_type_ptr(fty->_type);
				retb->_loc = ctx->_currtk._loc;
				retb->_sclass = SC_Auto;
				retb->_generated = 1;
				retb->_temporary = 1;
				retb->_defined = 1;
				cc_gen_internalname(retb);

				array_init(&caller, params->_elecount + 1, sizeof(FCCSymbol*), CC_MM_TEMPPOOL);
				array_init(&callee, params->_elecount + 1, sizeof(FCCSymbol*), CC_MM_TEMPPOOL);
			} else {
				retb = NULL;
				array_init(&caller, params->_elecount, sizeof(FCCSymbol*), CC_MM_TEMPPOOL);
				array_init(&callee, params->_elecount, sizeof(FCCSymbol*), CC_MM_TEMPPOOL);
			}

			ctx->_funcretb = retb;
			if (retb) { array_append(&callee, &retb); }
			for (n=0; n<params->_elecount; ++n)
			{
				array_append(&callee, array_element(params, n));
			}
			for (n=0; n<callee._elecount; ++n)
			{
				p0 = p1 = *(FCCSymbol**)array_element(&callee, n);
				if (IsInt(p0->_type) && p0->_type->_size != gbuiltintypes._sinttype->_size)
				{
					FCCIRTree* tree, * lhs, * rhs;

					p1 = cc_symbol_dup(p0, CC_MM_TEMPPOOL);
					p1->_type = cc_type_promote(p0->_type);

					lhs = cc_expr_id(p0, NULL, CC_MM_TEMPPOOL);
					rhs = cc_expr_cast(p0->_type, cc_expr_id(p1, NULL, CC_MM_TEMPPOOL), NULL, CC_MM_TEMPPOOL);
					tree = cc_expr_assign(p0->_type, lhs, rhs, NULL, CC_MM_TEMPPOOL);
					/* p0 = (typecast)p1 */
					cc_ir_codelist_append(&codelist, cc_ir_newcode_expr(tree, CC_MM_TEMPPOOL));
				}

				array_append(&caller, &p1);
			} /* end for n */

			/* get parameter's total size */
			for (parambytes = 0, n = 0; n < caller._elecount; ++n)
			{
				p0 = *(FCCSymbol**)array_element(&caller, n);
				parambytes += p0->_type->_size;
			} /* end for */
		}

		if (!cc_stmt_compound(ctx, &codelist, NULL, NULL)) 
		{
			return FALSE;
		}

		exitlab->_loc = ctx->_currtk._loc;
		cc_ir_codelist_append(&codelist, cc_ir_newcode_label(exitlab, CC_MM_TEMPPOOL));
		cc_ir_codelist_append(&codelist, cc_ir_newcode_fexit(parambytes, CC_MM_TEMPPOOL));
		if (cc_ir_check_undeflabels(&codelist) > 0) {
			return FALSE;
		}

		/* basic blocks & generate DAG */
		if (!(basicblocks = cc_canon_uber(&codelist, CC_MM_TEMPPOOL))) {
			logger_output_s("canon ir-codes failed.\n");
			return FALSE;
		}

		/* check return paths */
		if (!cc_canon_check_return(p, basicblocks)) {
			return FALSE;
		}

		if (!cc_dag_translate_basicblocks(basicblocks, CC_MM_TEMPPOOL)) {
			logger_output_s("translate DAG failed.\n");
			return FALSE;
		}

		/* output debug dag */
		if (gccconfig._output_dag)
		{
			logger_output_s("function '%s' after dag:\n", p->_name);
			cc_ir_basicblock_display(basicblocks, 5);
			logger_output_s("\n");
		}

		/* generate & dump back-end codes */
		ctx->_backend->_deffunction_begin(ctx, p);
		if (!cc_gen_dumpfunction(ctx, p, &caller, &callee, basicblocks)) {
			return FALSE;
		}
		ctx->_backend->_deffunction_end(ctx, p);
	}
	
	/* exit param scope */
	cc_symbol_exitscope();
	assert(gCurrentLevel == SCOPE_GLOBAL);
	
	/* free temp mm pool */
	mm_free_area(CC_MM_TEMPPOOL);
	return TRUE;
}

FCCType* cc_parser_declenum(FCCContext* ctx)
{
	const char* tag = NULL;
	FCCType* ty = NULL;
	FCCSymbol* p = NULL;
	FLocation loc = { NULL, 0, 0 };

	cc_read_token(ctx, &ctx->_currtk);
	if (ctx->_currtk._type == TK_ID)
	{
		tag = ctx->_currtk._val._astr._str;
		cc_read_token(ctx, &ctx->_currtk);
	}
	loc = ctx->_currtk._loc;

	if (ctx->_currtk._type == TK_LBRACE) /* '{' */
	{
		FArray enumerators;
		int ek = -1;

		ty = cc_type_newstruct(Type_Enum, tag, &loc, gCurrentLevel);
		if (!ty) {
			return NULL;
		}

		cc_read_token(ctx, &ctx->_currtk);
		if (ctx->_currtk._type != TK_ID)
		{
			logger_output_s("error: expecting an enumerator identifier. at %w\n", &ctx->_currtk._loc);
			return NULL;
		}

		array_init(&enumerators, 64, sizeof(FCCSymbol*), CC_MM_PERMPOOL);
		while (ctx->_currtk._type == TK_ID)
		{
			const char* id = ctx->_currtk._val._astr._str;
			
			p = cc_symbol_lookup(id, gIdentifiers);
			if (p && p->_scope == gCurrentLevel) {
				logger_output_s("error: redeclaration of '%s' at %w. previously declared at %w\n", id, &ctx->_currtk._loc, &p->_loc);
				return NULL;
			}
			loc = ctx->_currtk._loc;

			cc_read_token(ctx, &ctx->_currtk);
			if (ctx->_currtk._type == TK_ASSIGN) /* '=' */
			{
				cc_read_token(ctx, &ctx->_currtk);
				if (!cc_expr_parse_constant_int(ctx, &ek))
				{
					return FALSE;
				}
			}
			else
			{
				if (ek == gbuiltintypes._sinttype->_u._symbol->_u._limits._max._sint) {
					logger_output_s("error: overflow in value for enumeration constant '%s' at %w\n", id, &loc);
					return NULL;
				}

				ek++;
			}

			p = cc_symbol_install(id, &gIdentifiers, gCurrentLevel, gCurrentLevel <= SCOPE_GLOBAL ? CC_MM_PERMPOOL : CC_MM_TEMPPOOL);
			p->_loc = loc;
			p->_type = ty;
			p->_sclass = SC_Enum;
			p->_u._cnstval._sint = ek;
			array_append(&enumerators, &p);
			
			if (ctx->_currtk._type != TK_COMMA) /* ',' */
			{
				break;
			}

			cc_read_token(ctx, &ctx->_currtk);
		} /* end while */

		if (!cc_parser_expect(ctx, TK_RBRACE)) /* '}' */
		{
			return NULL;
		}

		p = NULL;
		array_append(&enumerators, &p); /* append end null */

		ty->_type = gbuiltintypes._sinttype;
		ty->_size = ty->_type->_size;
		ty->_align = ty->_type->_align;
		ty->_u._symbol->_u._enumids = (FCCSymbol**)enumerators._data;
		ty->_u._symbol->_defined = 1;
	}
	else if ((p = cc_symbol_lookup(tag, gTypes)) != NULL && p->_type->_op == Type_Enum)
	{
		ty = p->_type;
		if (ctx->_currtk._type == TK_SEMICOLON) /* ';' */
		{
			logger_output_s("warning: empty declaration '%s' at %w\n", tag, &loc);
		}
	}
	else 
	{
		logger_output_s("warning: unknown enumeration '%s' at %w\n", tag, &loc);
		ty = cc_type_newstruct(Type_Enum, tag, &loc, gCurrentLevel);
		if (!ty) {
			return NULL;
		}

		ty->_type = gbuiltintypes._sinttype;
		ty->_size = ty->_type->_size;
		ty->_align = ty->_type->_align;
	}

	/* logger_output_s("enum decl: %T\n", ty); */
	return ty;
}

FCCType* cc_parser_declstruct(FCCContext* ctx, int op)
{
	const char* tag = NULL;
	FCCType* ty = NULL;
	FCCSymbol* p = NULL;
	FLocation loc = { NULL, 0, 0 };

	cc_read_token(ctx, &ctx->_currtk);
	if (ctx->_currtk._type == TK_ID)
	{
		tag = ctx->_currtk._val._astr._str;
		cc_read_token(ctx, &ctx->_currtk);
	}
	loc = ctx->_currtk._loc;

	if (ctx->_currtk._type == TK_LBRACE) /* '{' */
	{
		ty = cc_type_newstruct(op, tag, &loc, gCurrentLevel);
		if (!ty) {
			return NULL;
		}
		ty->_u._symbol->_loc = loc;
		ty->_u._symbol->_defined = 1;

		cc_read_token(ctx, &ctx->_currtk);
		if (cc_parser_is_typename(&ctx->_currtk))
		{
			if (!cc_parser_structfields(ctx, ty))
			{
				return NULL;
			}
		}
		else if (ctx->_currtk._type == TK_RBRACE) /* '}' */
		{
			logger_output_s("error: C requires that a struct or union have at least one member. at %w\n", &ctx->_currtk._loc);
			return NULL;
		}
		else {
			logger_output_s("error: invalid field declarations. at %w\n", &ctx->_currtk._loc);
			return NULL;
		}

		if (!cc_parser_expect(ctx, TK_RBRACE)) /* '}' */
		{
			return NULL;
		}
	}
	else if ((p = cc_symbol_lookup(tag, gTypes)) != NULL && p->_type->_op == op)
	{
		ty = p->_type;
		if (ctx->_currtk._type == TK_SEMICOLON && p->_scope < gCurrentLevel) /* ';' previous declaration */
		{
			if (!(ty = cc_type_newstruct(op, tag, &loc, gCurrentLevel)))
			{
				return NULL;
			}
		}
	}
	else
	{
		if (!tag) {
			logger_output_s("error: missing struct or union tag. at %w\n", &ctx->_currtk._loc);
			return NULL;
		}

		ty = cc_type_newstruct(Type_Enum, tag, &loc, gCurrentLevel);
		if (!ty) {
			return NULL;
		}
	}

	/* logger_output_s("struct decl: %T\n", ty); */
	return ty;
}

BOOL cc_parser_structfields(FCCContext* ctx, FCCType* sty)
{
	FCCField *field, **where;
	int cnt = 0;

	where = &(sty->_u._symbol->_u._s._fields);
	while (cc_parser_is_typename(&ctx->_currtk))
	{
		int storage = SC_Unknown;
		FCCType *basety, *ty;
		const char* id;
		FLocation loc;

		basety = cc_parser_declspecifier(ctx, &storage);
		if (!basety) {
			return FALSE;
		}

		if (storage != SC_Unknown) {
			logger_output_s("error: '%s' is not allowed. at %w.\n", cc_sclass_displayname(storage), &ctx->_currtk._loc);
			return FALSE;
		}

		for (;;)
		{
			id = NULL;
			if (!cc_parser_declarator(ctx, basety, &id, &loc, NULL, &ty))
			{
				return FALSE;
			}

			field = cc_type_newfield(id, &loc, sty, ty);
			if (!field) {
				return FALSE;
			}

			if (ctx->_currtk._type == TK_COLON) /* ':' */
			{
				int bitsize;

				if (UnQual(ty) != gbuiltintypes._sinttype && UnQual(ty) != gbuiltintypes._uinttype)
				{
					logger_output_s("error: illegal bit-field type, expecting int or unsigned int, at %w.\n", &ctx->_currtk._loc);
					return FALSE;
				}

				cc_read_token(ctx, &ctx->_currtk);
				if (!cc_expr_parse_constant_int(ctx, &bitsize))
				{
					return FALSE;
				}
				field->_bitsize = (int16_t)bitsize;
				if (field->_bitsize > field->_type->_size * 8 || field->_bitsize <= 0)
				{
					logger_output_s("error: illegal bit-field size %d, at %w.\n", (int)field->_bitsize, &ctx->_currtk._loc);
					return FALSE;
				}
				field->_lsb = 1;
			}
			else
			{
				if (!id) {
					logger_output_s("warning: expecting a identifier at %w.\n", &ctx->_currtk._loc);
				}
				if (IsFunction(ty)) {
					logger_output_s("error: illegal field type %t at %w.\n", ty, &ctx->_currtk._loc);
					return FALSE;
				}
				else if (field->_type->_size == 0)
				{
					logger_output_s("error: undefined size for '%s' at %w.\n", field->_name, &field->_loc);
					return FALSE;
				}
			}

			if (IsConst(field->_type)) {
				sty->_u._symbol->_u._s._cfields = 1;
			}
			if (IsVolatile(field->_type)) {
				sty->_u._symbol->_u._s._vfields = 1;
			}

			if (IsStruct(field->_type)) {
				sty->_u._symbol->_u._s._cfields = cc_type_has_cfields(UnQual(field->_type));
			}

			cnt++;
			*where = field;
			where = &field->_next;
			if (ctx->_currtk._type != TK_COMMA) /* ',' */
			{
				break;
			}

			cc_read_token(ctx, &ctx->_currtk);
		} /* end for */

		if (!cc_parser_expect(ctx, TK_SEMICOLON)) /* ';' */
		{
			return FALSE;
		}
	} /* end while */

	/* calculate field offset & size */
	{
		int bitsmax, bitsused, offset;

		bitsmax = bitsused = offset = 0;
		field = sty->_u._symbol->_u._s._fields;
		for (; field; field = field->_next)
		{
			int align = field->_type->_align ? field->_type->_align : 1; /* align bytes */

			if (align > sty->_align) {
				sty->_align = align;
			}

			if (IsUnion(sty)) {
				bitsmax = bitsused = offset = 0;
			}

			if (field->_lsb == 0) /* it is not bitfield */
			{
				offset = util_roundup(offset + (bitsmax >> 3), align);
				field->_offset = offset;
				offset += field->_type->_size; /* update for next field offset */
				bitsmax = 0;
			}
			else
			{
				if (bitsmax != 0) /* check reuse previous bit-field's remained bits */
				{
					assert(util_roundup(offset, align) == offset);

					if ((bitsmax - bitsused) < field->_bitsize) /* can't reuse current bits */
					{
						offset = util_roundup(offset + (bitsmax >> 3), align);
						bitsmax = field->_type->_size * 8;
						bitsused = 0;
					}
				}
				else
				{
					offset = util_roundup(offset, align);
					bitsmax = field->_type->_size * 8;
					bitsused = 0;
				}

				field->_offset = offset;
				field->_lsb = bitsused + 1; /* lsb is one more than real index */
				bitsused += field->_bitsize;
			}

			/* expand structure */
			if (sty->_size < (offset + (bitsmax >> 3))) {
				sty->_size = offset + (bitsmax >> 3);
			}
		} /* end for */
	}

	sty->_size = util_roundup(sty->_size, sty->_align);
	return TRUE;
}

