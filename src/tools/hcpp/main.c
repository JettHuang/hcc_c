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


/* options */
#define OPTION_HELP				0x01
#define OPTION_DEFINE			0x02
#define OPTION_INCDIR			0x02
#define OPTION_OUTFILE			0x03

static struct optparse_long g_options[] =
{
	{ "help",    OPTION_HELP, OPTPARSE_NONE },
	{ NULL, 0, OPTPARSE_NONE }
};

const char* szhelp = "help: hcpp [options] sourcefile\n"
					"options:\n"
					"\t -I  include directory\n"
					"\t -D  macro define\n"
					"\t -o  output filename\n"
					"\t --help print help\n";


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

	while ((option = optparse_long(&options, g_options, NULL)) != -1)
	{
		switch (option)
		{
		case OPTION_HELP:
			printf(szhelp);
			return 0;
		default:
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
