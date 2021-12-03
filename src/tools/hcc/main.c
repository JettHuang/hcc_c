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

void printvalue(int kind, ...)
{
	char c;
	short s;
	int i;
	int64_t k;
	float f;
	double d;
	long double l;

	va_list vl;
	va_start(vl, kind);

	switch (kind)
	{
	case 'c':
		c = va_arg(vl, char);
		break;
	case 's':
		s = va_arg(vl, short);
		break;
	case 'i':
		i = va_arg(vl, int);
		break;
	case 'k':
		k = va_arg(vl, int64_t);
		break;
	case 'f':
		f = va_arg(vl, float);
		break;
	case 'd':
		d = va_arg(vl, double);
		break;
	case 'l':
		//l = va_arg(vl, long double);
		break;
	}

	va_end(vl);
}

int main(int argc, char* argv[])
{
	const char* arg, *localename;
	int option, bsuccess;
	struct optparse options;
	const char* srcfilename, *outfilename;
	FCCContext cc;

	char c = 0x8F;

	printvalue('c', c);
	
	localename = setlocale(LC_ALL, "");
	logger_output_s("LOCAL %s\n", localename);

	cc_init();
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
	cc_uninit();

	return bsuccess ? 0 : -1;
}
