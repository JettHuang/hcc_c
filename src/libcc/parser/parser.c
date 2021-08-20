/* \brief
 *		c parser
 */

#include "cc.h"
#include "lexer/token.h"
#include "parser.h"
#include "logger.h"
#include "generator/gen.h"


static const char* cc_sclass_displayname(int sclass);

BOOL cc_parser_program(FCCContext* ctx)
{
	gCurrentLevel = SCOPE_GLOBAL;

	if (!cc_read_token(ctx, &ctx->_currtk)) {
		return FALSE;
	}

	while (ctx->_currtk._type != TK_EOF)
	{
		if (ctx->_currtk._type == TK_ID || cc_parser_is_specifier(ctx->_currtk._type))
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
	if (tktype == TK_ID || tktype == TK_MUL || tktype == TK_LPAREN || tktype == TK_LBRACKET) /* id * ( [ */
	{
		const char* id = NULL;
		FCCType* ty = NULL;
		FLocation loc = { NULL, 0, 0 };

		if (gCurrentLevel == SCOPE_GLOBAL)
		{
			FArray params;
			array_init(&params, 32, sizeof(FCCSymbol*), CC_MM_TEMPPOOL);
			if (!cc_parser_declarator(ctx, basety, &id, &loc, &params, &ty)) {
				logger_output_s("error: parser declarator failed. at %s:%d:%d\n", ctx->_cs->_srcfilename, ctx->_cs->_line, ctx->_cs->_col);
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
				logger_output_s("error: parser declarator failed. at %s:%d:%d\n", ctx->_cs->_srcfilename, ctx->_cs->_line, ctx->_cs->_col);
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
			}

			if (ctx->_currtk._type == TK_COMMA) /* ',' */
			{
				break;
			}

			if (!cc_read_token(ctx, &ctx->_currtk)) {
				return FALSE;
			}

			id = NULL;
			if (!cc_parser_declarator(ctx, basety, &id, &loc, NULL, &ty))
			{
				logger_output_s("error: parser declarator failed. at %s:%d:%d\n", ctx->_cs->_srcfilename, ctx->_cs->_line, ctx->_cs->_col);
				return FALSE;
			}
		} /* end for ;; */
	}
	else
	{
		BOOL bStructDecl = IsStruct(basety) && !cc_symbol_isgenlabel(UnQual(basety)->_u._symbol->_name);
		if (!IsEnum(basety) && !bStructDecl)
		{
			logger_output_s("warning: empty declaration. at %s:%d:%d\n", ctx->_cs->_srcfilename, ctx->_cs->_line, ctx->_cs->_col);
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

BOOL cc_parser_is_typename(FCCToken* tk)
{
	FCCSymbol* p;

	return (gCCTokenMetas[tk->_type]._flags & TK_IS_DECL_SPECIFIER)
		|| (tk->_type == TK_ID && (p = cc_symbol_lookup(tk->_val._astr, gIdentifiers)) && p->_sclass == SC_Typedef);
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

	logger_output_s("error: expect %k, at %w\n", tk, &ctx->_currtk._loc);
	return FALSE;
}

FCCType* cc_parser_declspecifier(FCCContext* ctx, int* storage)
{
	FCCType* ty = NULL;
	int sclass = SC_Unknown;
	int typespec = Type_Unknown;
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
			ty = gBuiltinTypes._voidtype;
			cc_read_token(ctx, &ctx->_currtk);
			break;
		case TK_CHAR:
			if (typespec != Type_Unknown)
			{
				logger_output_s("error: invalid use 'char', at %w\n", &ctx->_currtk._loc);
				return NULL;
			}
			typespec = Type_Char;
			cc_read_token(ctx, &ctx->_currtk);
			break;
		case TK_INT:
			if (typespec != Type_Unknown)
			{
				logger_output_s("error: invalid use 'int', at %w\n", &ctx->_currtk._loc);
				return NULL;
			}
			typespec = Type_SInteger;
			cc_read_token(ctx, &ctx->_currtk);
			break;
		case TK_FLOAT:
			if (typespec != Type_Unknown)
			{
				logger_output_s("error: invalid use 'float', at %w\n", &ctx->_currtk._loc);
				return NULL;
			}
			typespec = Type_Float;
			ty = gBuiltinTypes._floattype;
			cc_read_token(ctx, &ctx->_currtk);
			break;
		case TK_DOUBLE:
			if (typespec != Type_Unknown)
			{
				logger_output_s("error: invalid use 'double', at %w\n", &ctx->_currtk._loc);
				return NULL;
			}
			typespec = Type_Double;
			ty = gBuiltinTypes._doubletype;
			cc_read_token(ctx, &ctx->_currtk);
			break;
		case TK_ENUM:
			if (typespec != Type_Unknown)
			{
				logger_output_s("error: invalid use 'enum', at %w\n", &ctx->_currtk._loc);
				return NULL;
			}
			typespec = Type_Enum;
			ty = cc_parser_declenum(ctx);
			break;
		case TK_STRUCT:
			if (typespec != Type_Unknown)
			{
				logger_output_s("error: invalid use 'struct', at %w\n", &ctx->_currtk._loc);
				return NULL;
			}
			typespec = Type_Struct;
			ty = cc_parser_declstruct(ctx, Type_Struct);
			break;
		case TK_UNION:
			if (typespec != Type_Unknown)
			{
				logger_output_s("error: invalid use 'union', at %w\n", &ctx->_currtk._loc);
				return NULL;
			}
			typespec = Type_Union;
			ty = cc_parser_declstruct(ctx, Type_Union);
			break;
		case TK_ID:
		{
			FCCSymbol* p = NULL;
			if (typespec == Type_Unknown && size == Size_Unknown && sign == Sign_Unknown
				&& (p = cc_symbol_lookup(ctx->_currtk._val._astr, gIdentifiers)) && p->_sclass == SC_Typedef)
			{
				typespec = Type_Defined;
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
	} // end while

	if (typespec == Type_Unknown)
	{
		if (size != Size_Unknown || sign != Sign_Unknown)
		{
			typespec = Type_SInteger;
		}
		else
		{
			logger_output_s("syntax error: no type specifier, at %w", &ctx->_currtk._loc);
			return NULL;
		}
	}

	if (typespec == Type_Char)
	{
		if (size != Size_Unknown)
		{
			logger_output_s("syntax error: invalid size specifier for char, at %w", &ctx->_currtk._loc);
			return NULL;
		}

		ty = gBuiltinTypes._chartype;
		if (sign == Sign_Signed)
		{
			ty = gBuiltinTypes._schartype;
		}
		else if (sign == Sign_Unsigned)
		{
			ty = gBuiltinTypes._uchartype;
		}
	}
	else if (typespec == Type_SInteger)
	{
		if (sign == Sign_Signed || sign == Sign_Unknown)
		{
			ty = gBuiltinTypes._sinttype;
			if (size == Size_Short)
			{
				ty = gBuiltinTypes._sshorttype;
			}
			else if (size == Size_Long)
			{
				ty = gBuiltinTypes._slongtype;
			}
			else if (size == Size_Longlong)
			{
				ty = gBuiltinTypes._sllongtype;
			}
		}
		else
		{
			ty = gBuiltinTypes._uinttype;
			if (size == Size_Short)
			{
				ty = gBuiltinTypes._ushorttype;
			}
			else if (size == Size_Long)
			{
				ty = gBuiltinTypes._ulongtype;
			}
			else if (size == Size_Longlong)
			{
				ty = gBuiltinTypes._ullongtype;
			}
		}
	}
	else
	{
		if (sign != Sign_Unknown || size != Size_Unknown)
		{
			logger_output_s("syntax error: invalid type specifier, at %w", &ctx->_currtk._loc);
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
		*storage = sclass;
	}

	return ty;
}

BOOL cc_parser_declarator(FCCContext* ctx, FCCType* basety, const char** id, FLocation* loc, FArray* params, FCCType** outty)
{
	BOOL bgetparams = TRUE;
	FCCType* ty = NULL;

	if (!cc_parser_declarator1(ctx, id, loc, params, &bgetparams, &ty))
	{
		return FALSE;
	}
	for (; ty; ty = ty->_type)
	{
		switch (ty->_op)
		{
		case Type_Array:
			basety = cc_type_newarray(basety, ty->_size, 0);
			break;
		case Type_Function:
			basety = cc_type_func(basety, ty->_u._f._protos);
			break;
		case Type_Pointer:
			basety = cc_type_ptr(basety);
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
	} /* end for */

	if (basety->_size > 32767) {
		logger_output_s("warning: more than 32767 bytes in `%t'\n", basety);
	}
	
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
			logger_output_s("error: extraneous identifier '%k' at %w.\n", &ctx->_currtk, &ctx->_currtk._loc);
			return FALSE;
		}

		*id = ctx->_currtk._val._astr;
		*loc = ctx->_currtk._loc;
		cc_read_token(ctx, &ctx->_currtk);
	}
	break;
	case TK_MUL: /* '*' */
	{
		int qual = 0;

		ty = cc_type_tmp(Type_Pointer, NULL);
		cc_read_token(ctx, &ctx->_currtk);
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

		if (qual) {
			ty->_type = cc_type_tmp(qual, NULL);
			ty = ty->_type;
		}
		
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

	while (ctx->_currtk._type == TK_LPAREN || ctx->_currtk._type == TK_LBRACKET) /* '(' ']' */
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
				logger_output_s("error: parsing parameter failed. %w\n", &ctx->_currtk);
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
				if (!cc_parser_intexpression(ctx, &cnt))
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

				array_append(&protos, &gBuiltinTypes._ellipsistype);
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
	if (p )
	{ }

	return NULL;
}

FCCSymbol* cc_parser_decllocal(FCCContext* ctx, int storage, const char* id, const FLocation* loc, FCCType* ty)
{
	return NULL;
}

FCCSymbol* cc_parser_declparam(FCCContext* ctx, int storage, const char* id, const FLocation* loc, FCCType* ty)
{
	return NULL;
}

BOOL cc_parser_funcdefinition(FCCContext* ctx, int storage, const char* name, FCCType* ty, const FLocation* loc, FArray* params)
{
	// TODO: 

	cc_parser_expect(ctx, TK_LBRACE);
	cc_parser_expect(ctx, TK_RBRACE);
	
	/* exit param scope */
	cc_symbol_exitscope();
	assert(gCurrentLevel == SCOPE_GLOBAL);
	return FALSE;
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
		tag = ctx->_currtk._val._astr;
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
			const char* id = ctx->_currtk._val._astr;
			
			p = cc_symbol_lookup(id, gIdentifiers);
			if (p && p->_scope == gCurrentLevel) {
				logger_output_s("error: redeclaration of '%s' previously declared at %w\n", id, &p->_loc);
				return NULL;
			}
			loc = ctx->_currtk._loc;

			cc_read_token(ctx, &ctx->_currtk);
			if (ctx->_currtk._type == TK_ASSIGN) /* '=' */
			{
				cc_read_token(ctx, &ctx->_currtk);
				if (!cc_parser_intexpression(ctx, &ek))
				{
					return FALSE;
				}
			}
			else
			{
				if (ek == gBuiltinTypes._sinttype->_u._symbol->_u._limits._max._int) {
					logger_output_s("error: overflow in value for enumeration constant '%s' at %w\n", id, &loc);
					return NULL;
				}

				ek++;
			}

			p = cc_symbol_install(id, &gIdentifiers, gCurrentLevel, gCurrentLevel <= SCOPE_GLOBAL ? CC_MM_PERMPOOL : CC_MM_TEMPPOOL);
			p->_loc = loc;
			p->_type = ty;
			p->_sclass = SC_Enum;
			p->_u._value._int = ek;
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

		ty->_type = gBuiltinTypes._sinttype;
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

		ty->_type = gBuiltinTypes._sinttype;
		ty->_size = ty->_type->_size;
		ty->_align = ty->_type->_align;
	}

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
		tag = ctx->_currtk._val._astr;
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

	return ty;
}

static const char* cc_sclass_displayname(int sclass)
{
	const char* name;
	
	switch (sclass)
	{
	case SC_Auto: name = "auto"; break;
	case SC_Register: name = "register"; break;
	case SC_Static: name = "static"; break;
	case SC_External: name = "external"; break;
	case SC_Typedef: name = "typedef"; break;
	case SC_Enum: name = "enum"; break;
	default: name = "unknown"; break;
	}

	return name;
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

				if (UnQual(ty)->_op != Type_SInteger && UnQual(ty)->_op != Type_UInteger && UnQual(ty)->_op != Type_Char)
				{
					logger_output_s("error: illegal bit-field type, expecting integer, at %w.\n", &ctx->_currtk._loc);
					return FALSE;
				}

				cc_read_token(ctx, &ctx->_currtk);
				if (!cc_parser_intexpression(ctx, &bitsize))
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
				int suboffset, subbitsused;

				suboffset = 0;
				if (bitsmax != 0) /* check reuse previous bit-field's remained bits */
				{
					if (util_roundup(offset, align) != offset)
					{
						offset = suboffset = util_roundup(offset + (bitsmax >> 3), align);
						bitsmax = field->_type->_size * 8;
						bitsused = 0;
					}
					else
					{
						int newbitsmax = bitsmax;
						if (bitsmax < field->_type->_size * 8)
						{
							newbitsmax = field->_type->_size * 8;
						}
						if ((newbitsmax - bitsused) >= field->_bitsize)
						{
							int crossbytes;

							suboffset = offset;
							/* check across bytes */
							crossbytes = ((bitsused + field->_bitsize - 1) >> 3) - (bitsused >> 3) + 1;
							if (crossbytes > field->_type->_size)
							{
								bitsused = util_roundup(bitsused, 8);
								suboffset = offset + (bitsused >> 3);
							}
							/* check align */
							if (((bitsused >> 3) % field->_type->_align) != 0)
							{
								bitsused = util_roundup(bitsused, field->_type->_align * 8);
								suboffset = offset + (bitsused >> 3);
							}

							/* check space again */
							if ((newbitsmax - bitsused) >= field->_bitsize)
							{
								bitsmax = newbitsmax;
							}
							else
							{
								offset = suboffset = util_roundup(offset + (bitsmax >> 3), align);
								bitsmax = field->_type->_size * 8;
								bitsused = 0;
							}
						}
						else
						{
							offset = suboffset = util_roundup(offset + (bitsmax >> 3), align);
							bitsmax = field->_type->_size * 8;
							bitsused = 0;
						}
					}
				}
				else
				{
					offset = suboffset = util_roundup(offset, align);
					bitsmax = field->_type->_size * 8;
					bitsused = 0;
				}

				subbitsused = (bitsused - (suboffset - offset) * 8);
				field->_offset = suboffset;
				if (ctx->_backend->_is_little_ending)
				{
					field->_lsb = subbitsused + 1; /* lsb is one more than real index */
				}
				else
				{
					field->_lsb = (field->_type->_size - subbitsused - field->_bitsize) + 1;
				}

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

BOOL cc_parser_intexpression(FCCContext* ctx, int* val)
{
	*val = 1;
	return FALSE;
}