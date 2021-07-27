/* \brief
 *		c-compiler lexer
 */

#ifndef __CC_LEXER_H__
#define __CC_LEXER_H__

#include "token.h"
#include "charstream.h"


void cc_lexer_init();
void cc_lexer_uninit();
BOOL cc_lexer_read_token(FCharStream* cs, FCCToken* tk);

#endif /* __CC_LEXER_H__ */
