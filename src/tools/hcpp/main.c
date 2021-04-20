/* \brief
 *		main entry of c preprocessor
 */

#include "mm.h"
#include "hstring.h"
#include "charstream.h"
#include "logger.h"
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

		do
		{
			char ch = cs_current(cs);
			if (ch == EOF) break;
			logger_output_s("%d:%d %c\n", cs->_line, cs->_col, ch);
			
			cs_forward(cs);
		} while (1);

		cs_release(cs);
	}

	return 0;
}


