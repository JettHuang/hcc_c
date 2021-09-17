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


/* public APIS */
BOOL cc_parser_program(FCCContext* ctx);

/* internal APIs */

BOOL cc_parser_declaration(FCCContext* ctx, FDeclCallback callback);
FCCSymbol* cc_parser_declglobal(FCCContext* ctx, int storage, const char* id, const FLocation* loc, FCCType* ty);
FCCSymbol* cc_parser_decllocal(FCCContext* ctx, int storage, const char* id, const FLocation* loc, FCCType* ty);
FCCSymbol* cc_parser_declparam(FCCContext* ctx, int storage, const char* id, const FLocation* loc, FCCType* ty);

BOOL cc_parser_is_specifier(enum ECCToken tk);
BOOL cc_parser_is_constant(enum ECCToken tk);
BOOL cc_parser_is_assign(enum ECCToken tk);
BOOL cc_parser_is_stmtspecifier(enum ECCToken tk);
BOOL cc_parser_is_typename(FCCToken *tk);
FCCType* cc_parser_declspecifier(FCCContext* ctx, int *storage);

BOOL cc_parser_declarator(FCCContext* ctx, FCCType* basety, const char** id, FLocation* loc, FArray* params, FCCType** outty);
BOOL cc_parser_declarator1(FCCContext* ctx, const char** id, FLocation* loc, FArray* params, BOOL* bgetparams, FCCType** outty);
BOOL cc_parser_parameters(FCCContext* ctx, FCCType* fn, FArray* params);

BOOL cc_parser_funcdefinition(FCCContext* ctx, int storage, const char* name, FCCType* ty, const FLocation* loc, FArray* params);
FCCType* cc_parser_declenum(FCCContext* ctx);
FCCType* cc_parser_declstruct(FCCContext* ctx, int op);
BOOL cc_parser_structfields(FCCContext* ctx, FCCType* ty);

BOOL cc_parser_expect(FCCContext* ctx, enum ECCToken tk);

#endif /* __CC_PARSER_H__ */
