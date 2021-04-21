/* \brief
 *		lexer for preprocessor
 */

#ifndef __PP_LEXER_H__
#define __PP_LEXER_H__

#include "pptoken.h"
#include "charstream.h"


void cpp_lexer_init();
void cpp_lexer_uninit();
int cpp_lexer_read_token(FCharStream* cs, int bwantheader, FPPToken *tk);

#endif // __PP_LEXER_H__
