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

#include <stdio.h>
#include <stdarg.h>


#define OPTION_HELP				0x01
#define OPTION_DEFINE			0x02
#define OPTION_INCDIR			0x02
#define OPTION_OUTFILE			0x03
#define OPTION_PRINTDAG			0x04
#define OPTION_PRINTTRIPPLE		0x05
#define OPTION_CHECKRET			0x06
#define OPTION_COMPILEINLINE	0x07

/* options */
static struct optparse_long g_options[] =
{
	{ "help",    OPTION_HELP, OPTPARSE_NONE },
	{ "dag",     OPTION_PRINTDAG, OPTPARSE_NONE },
	{ "tripple", OPTION_PRINTTRIPPLE, OPTPARSE_NONE },
	{ "checkret", OPTION_CHECKRET,    OPTPARSE_REQUIRED },
	{ "inline",  OPTION_COMPILEINLINE, OPTPARSE_REQUIRED },

	{ NULL, 0, OPTPARSE_NONE }
};

const char* szhelp = "help: hcc [options] sourcefile\n"
					"options:\n"
					"\t -I  include directory\n"
					"\t -D  macro define\n"
					"\t -o  output filename\n"
					"\t --help print help\n"
					"\t --dag  print dag\n"
					"\t --tripple print tripple code\n"
					"\t --checkret=0/1 check function return\n"
					"\t --inline=0/1 enable compile inline function\n";

int main(int argc, char* argv[])
{
	const char* arg, *localename;
	int option, bsuccess;
	struct optparse options;
	const char* srcfilename, *outfilename;
	FCCContext cc;
	
	localename = setlocale(LC_ALL, "");
	logger_output_s("LOCAL %s\n", localename);

	cc_init();
	cc_contex_init(&cc);

	optparse_init(&options, argv);
	srcfilename = NULL;
	outfilename = "out.i.asm";
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

	while ((option = optparse_long(&options, g_options, NULL)) != -1)
	{
		switch (option)
		{
		case OPTION_HELP:
			printf(szhelp); 
			return 0;
		case OPTION_PRINTDAG:
			gccconfig._output_dag = 1; break;
		case OPTION_PRINTTRIPPLE:
			gccconfig._output_tripple = 1; break;
		case OPTION_CHECKRET:
			gccconfig._must_retvalue = atoi(options.optarg) ? 1 : 0;
			break;
		case OPTION_COMPILEINLINE:
			gccconfig._omit_inlinefunc = atoi(options.optarg) ? 0 : 1;
			break;
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
	
	bsuccess = cc_process(&cc, srcfilename, outfilename);
	cc_contex_release(&cc);
	cc_uninit();

	return bsuccess ? 0 : 1;
}
