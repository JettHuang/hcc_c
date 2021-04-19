/* \brief
 *		main entry of c preprocessor
 */

#include "mm.h"
#include "hstring.h"
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
	
	return 0;
}


