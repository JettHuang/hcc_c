/* \brief
 *		main entry of c preprocessor
 */

#include "mm.h"
#include "hstring.h"
#include "charstream.h"
#include "logger.h"
#include "pplexer.h"
#include "libcpp.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#define OPTPARSE_IMPLEMENTATION
#define OPTPARSE_API static
#include "optparse.h"

static void add_sysinclues(FCppContext* ctx)
{
	const char* paths = getenv("include");
	const char sep = ';';

	if (!paths) { return; }
	while (*paths)
	{
		char* p, buf[1024];

		if (p = strchr(paths, sep)) {
			assert(p - paths < sizeof(buf));
			strncpy(buf, paths, p - paths);
			buf[p - paths] = '\0';
		}
		else {
			assert(strlen(paths) < sizeof(buf));
			strcpy(buf, paths);
		}
		/* add include dir*/
		cpp_add_includedir(ctx, buf);

		if (p == NULL)
			break;
		paths = p + 1;
	}
}

int main(int argc, char* argv[])
{
	const char* arg;
	int option, bsuccess;
	struct optparse options;
	const char* srcfilename, *outfilename;
	FCppContext cpp;

	cpp_lexer_init();
	cpp_contex_init(&cpp);

	optparse_init(&options, argv);
	srcfilename = NULL;
	outfilename = "out.i";
	while ((option = optparse(&options, "I:D:o:")) != -1) {
		switch (option) {
		case 'I':
			cpp_add_includedir(&cpp, options.optarg);
			break;
		case 'D':
			cpp_add_definition(&cpp, options.optarg);
			break;
		case 'o':
			outfilename = options.optarg;
			break;
		case '?':
			logger_output_s("%s: %s\n", argv[0], options.errmsg);
			break;
		}
	} /* end while */

	/* Print remaining arguments. */
	while ((arg = optparse_arg(&options)))
	{
		logger_output_s("source %s\n", arg);
		srcfilename = arg;
		break;
	}

	/* add environment include */
	add_sysinclues(&cpp);
	/* add definition*/
	cpp_add_definition(&cpp, "win32");
	cpp_add_definition(&cpp, "_WIN32");
	cpp_add_definition(&cpp, "_M_IX86");

	bsuccess = cpp_process(&cpp, srcfilename, outfilename);
	cpp_contex_release(&cpp);
	cpp_lexer_uninit();

	return bsuccess ? 0 : 1;
}
