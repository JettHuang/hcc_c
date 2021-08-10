/* \brief
 *	 C parser
 */

#ifndef __CC_PARSER_H__
#define __CC_PARSER_H__

#include "utils.h"
#include "types.h"
#include "symbols.h"
#include "lexer/token.h"


typedef struct tagCCContext FCCContext;
typedef FCCSymbol* (*FDeclCallback)(FCCContext* ctx, int storage, const char* id, const FLocation* loc, FCCType* ty);


BOOL cc_parser_program(FCCContext* ctx);

/* internal apis */

BOOL cc_parser_declaration(FCCContext* ctx, FDeclCallback callback);
FCCSymbol* cc_parser_declglobal(FCCContext* ctx, int storage, const char* id, const FLocation* loc, FCCType* ty);
FCCSymbol* cc_parser_decllocal(FCCContext* ctx, int storage, const char* id, const FLocation* loc, FCCType* ty);

BOOL cc_parser_is_specifier(enum ECCToken tk);
FCCType* cc_parser_declspecifier(FCCContext* ctx, int *storage);

FCCType* cc_parser_declarator(FCCContext* ctx, FCCType* basety, const char** id, FLocation* loc);
FCCType* cc_parser_declarator1(FCCContext* ctx, const char** id, FLocation* loc);

#endif /* __CC_PARSER_H__ */
