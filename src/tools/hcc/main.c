/* \brief
 *		main entry of c preprocessor
 */

#include "logger.h"
#include "libcc.h"

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <locale.h>

#define OPTPARSE_IMPLEMENTATION
#define OPTPARSE_API static
#include "optparse.h"


int main(int argc, char* argv[])
{
	const char* arg, *localename;
	int option, bsuccess;
	struct optparse options;
	const char* srcfilename, *outfilename;
	FCCContext cc;

	localename = setlocale(LC_ALL, "");
	logger_output_s("LOCAL %s\n", localename);

	cc_lexer_init();
	cc_contex_init(&cc);

	optparse_init(&options, argv);
	srcfilename = NULL;
	outfilename = "out.i";
	while ((option = optparse(&options, "I:D:o:")) != -1) {
		switch (option) {
		case 'I':
			break;
		case 'D':
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
	
	bsuccess = cc_process(&cc, srcfilename, outfilename);
	cc_contex_release(&cc);
	cc_lexer_uninit();

	return bsuccess ? 0 : -1;
}
