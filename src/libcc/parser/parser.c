/* \brief
 *		c parser
 */

#include "parser.h"


BOOL cc_parser_program(FCCContext* ctx)
{
	return FALSE;
}

BOOL cc_parser_declaration(FCCContext* ctx, FDeclCallback callback)
{
	return FALSE;
}

FCCSymbol* cc_parser_declglobal(FCCContext* ctx, int storage, const char* id, const FLocation* loc, FCCType* ty)
{
	return NULL;
}

FCCSymbol* cc_parser_decllocal(FCCContext* ctx, int storage, const char* id, const FLocation* loc, FCCType* ty)
{
	return NULL;
}

BOOL cc_parser_is_specifier(enum ECCToken tk)
{
	return FALSE;
}

FCCType* cc_parser_declspecifier(FCCContext* ctx, int* storage)
{
	return NULL;
}

FCCType* cc_parser_declarator(FCCContext* ctx, FCCType* basety, const char** id, FLocation* loc)
{
	return NULL;
}

FCCType* cc_parser_declarator1(FCCContext* ctx, const char** id, FLocation* loc)
{
	return NULL;
}
