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
#include <string.h>


int main(int argc, char* argv[])
{
	const char* h0, *h1;
	const char* str = "huangheshui...";

	h0 = hs_hashstr("huangheshui...");
	h1 = hs_hashstr(str);
	assert(h0 == h1);
	
	h0 = hs_hashnstr("xyz\0uvw...", sizeof("xyz\0uvw..."));
	h1 = hs_hashnstr2("xyz\0uvw...", sizeof("xyz\0uvw...") - 1);
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

	{
		const char* bindir = util_process_bin_dir();
		const char* workdir = util_process_working_dir();
		const char* filename = util_normalize_pathname("huang\\cpp.h");
		const char* path = util_getpath_from_pathname(filename);

		logger_output_s("bindir: %s\n", bindir);
		logger_output_s("workdir: %s\n", workdir);
		logger_output_s("filename: %s\n", filename);
		logger_output_s("path: %s\n", path);
	}
	{
		const char* bindir = util_process_bin_dir();
		const char* workdir = util_process_working_dir();
		const char* filename = util_normalize_pathname("working\\cpp.h");
		const char* path = util_getpath_from_pathname(filename);

		logger_output_s("bindir: %s\n", bindir);
		logger_output_s("workdir: %s\n", workdir);
		logger_output_s("filename: %s\n", filename);
		logger_output_s("path: %s\n", path);
	}
	return 0;
}


