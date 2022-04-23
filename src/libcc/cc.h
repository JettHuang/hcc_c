/* \brief
 *		c compiler.
 */

#ifndef __CC_H__
#define __CC_H__

#include <stdio.h>
#include "charstream.h"
#include "utils.h"
#include "mm.h"
#include "lexer/lexer.h"


#define CC_MM_PERMPOOL		MMA_Area_1
#define CC_MM_TEMPPOOL		MMA_Area_2
#define CC_MM_TEMPPOOL_MAXCNT		4	/* free temporary memory when temp pools exceed this count */


#define CHECK_IS_EOFLINE(ty)	((ty) == TK_EOF || (ty) == TK_NEWLINE)

/* runtime context of cc */
typedef struct tagCCContext {
	const char* _srcfilename;
	const char* _outfilename;
	FILE* _outfp;

	/* source code stream */
	FCCLexerContext _lexer;

	FCCToken _currtk;
	struct {
		int16_t	_valid : 1;
		FCCToken _tk;
	} _lookaheadtk;
	int16_t _bnewline : 1; /* next token is in the newline */
	int16_t _errors;

	struct tagCCSymbol* _function;
	struct tagCCSymbol* _funcexit;
	struct tagCCSymbol* _funcretb;
	struct tagCCIRCodeList* _codes;
	struct tagCCBackend* _backend;
} FCCContext;


/************************************************************************/
/* public interface                                                     */
/************************************************************************/

void cc_init();
void cc_uninit();

void cc_contex_init(FCCContext* ctx);
void cc_contex_release(FCCContext* ctx);
BOOL cc_process(FCCContext* ctx, const char* srcfilename, const char* outfilename);

BOOL cc_read_token(FCCContext* ctx, FCCToken* tk);
BOOL cc_lookahead_token(FCCContext* ctx, FCCToken* tk);

void cc_print_token(FCCToken* tk);

void cc_error_occurred(FCCContext* ctx);
BOOL cc_has_errors(FCCContext* ctx);

#endif /* __CC_H__ */
