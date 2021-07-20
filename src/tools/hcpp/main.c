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
#include <assert.h>
#include <string.h>

#define OPTPARSE_IMPLEMENTATION
#define OPTPARSE_API static
#include "optparse.h"


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
			logger_output_s("D %s\n", options.optarg);
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
	
	bsuccess = cpp_process(&cpp, srcfilename, outfilename);
	cpp_contex_release(&cpp);
	cpp_lexer_uninit();

	return bsuccess ? 0 : -1;
}
