/* \brief
 *		c compiler.
 */

#ifndef __CC_H__
#define __CC_H__

#include <stdio.h>
#include "charstream.h"
#include "utils.h"
#include "mm.h"
#include "lexer/token.h"


#define CC_MM_PERMPOOL		MMA_Area_1
#define CC_MM_TEMPPOOL		MMA_Area_2
#define CC_MM_TEMPPOOL_MAXCNT		4	/* free temporary memory when temp pools exceed this count */


/* token list node */
typedef struct tagTokenListNode {
	FCCToken	_tk;
	struct tagTokenListNode* _next;
} FTKListNode;

#define CHECK_IS_EOFLINE(ty)	((ty) == TK_EOF || (ty) == TK_NEWLINE)

/* runtime context of cc */
typedef struct tagCCContext {
	const char* _outfilename;
	FILE* _outfp;

	/* source code stream */
	FCharStream* _cs;

	struct {
		int16_t	_valid : 1;
		FCCToken _tk;
	} _lookaheadtk;
} FCCContext;


/************************************************************************/
/* public interface                                                     */
/************************************************************************/

void cc_init();
void cc_uninit();

void cc_contex_init(FCCContext* ctx);
void cc_contex_release(FCCContext* ctx);
BOOL cc_process(FCCContext* ctx, const char* srcfilename, const char* outfilename);

BOOL cc_read_token(FCCContext* ctx, FCharStream* cs, FCCToken* tk);
BOOL cc_lookahead_token(FCCContext* ctx, FCharStream* cs, FCCToken* tk);
BOOL cc_read_tokentolist(FCCContext* ctx, FCharStream* cs, FTKListNode** tail);
BOOL cc_read_rowtokens(FCCContext* ctx, FCharStream* cs, FTKListNode** tail);

void cc_print_token(FCCToken* tk);

#endif /* __CC_H__ */
