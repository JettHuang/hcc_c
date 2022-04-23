/* \brief
 *		c-compiler lexer
 */

#ifndef __CC_LEXER_H__
#define __CC_LEXER_H__

#include "token.h"
#include "charstream.h"


typedef struct tagLexerContext {
	FCharStream* _cs;
	struct {
		FCCToken	 _tk;
		BOOL		 _isvalid;
	} _cached[2];
} FCCLexerContext;

void cc_lexer_init();
void cc_lexer_uninit();
BOOL cc_lexer_read_token(struct tagLexerContext *ctx, FCCToken* tk);

#endif /* __CC_LEXER_H__ */
