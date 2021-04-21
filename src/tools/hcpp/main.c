/* \brief
 *		main entry of c preprocessor
 */

#include "mm.h"
#include "hstring.h"
#include "charstream.h"
#include "logger.h"
#include "pplexer.h"

#include <stdio.h>
#include <assert.h>


int main(int argc, char* argv[])
{
	const char* h0, *h1;

	h0 = hs_hashstr("huangheshui...");
	h1 = hs_hashstr("huangheshui...");
	assert(h0 == h1);
	
	h0 = hs_hashnstr("xyz\0uvw...", sizeof("xyz\0uvw..."));
	h1 = hs_hashnstr("xyz\0uvw...", sizeof("xyz\0uvw..."));
	assert(h0 == h1);
	
	{
		FCharStream* cs = cs_create_fromfile("dist\\samples\\types.c");
		assert(cs);

		cpp_lexer_init();
		do
		{
			FPPToken tk;

			if (cpp_lexer_read_token(cs, 0, &tk))
			{
				if (tk._type == TK_EOF) {
					logger_output_s("EOF");
					break;
				}

				logger_output_s("token:%s, ws:%d, type: %d, at %d:%d\n", tk._str, tk._wscnt, tk._type, tk._loc._line, tk._loc._col);
			}
			else
			{
				logger_output_s("error occurred.\n");
				break;
			}
		} while (1);

		cs_release(cs);
		cpp_lexer_uninit();
	}

	return 0;
}


