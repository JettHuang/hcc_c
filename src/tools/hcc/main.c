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

int B;
/* int A = { {10} }; */ /* only one level of braces is allowed on an initializer for an object of type "int" */
int grids[10] = { { 10 }, { 1000} };

char str[] = { "jett" };

struct FRectangle {
	int x, y;
	struct FSize {
		int sx, sy;
	} size;

	int a[10];
};

union U {
	struct A { int a, b; } _s;
	struct B { int a, b; } _d;

} u = { 1000, 200 };

struct FRectangle rt = { .y = 20, .x = 10,  .size.sx = 100, { 20 }, { 0,[1] = 100 } };

void sayHello()
{
	char str[] = "huangehsui";
	char* s = str;

	int a = 100;
	int b = sizeof (--a);
	int i;

	switch (a)
	case 100:
		a = 200;

	switch (a)
	{
		printf("hello, world!");
	case 100:
		if (a > 100)
		{
	case 10:
		break;
		}
		break;
	default:
		break;
		break;
	case 101:
		a = 10;
		break;
	}

	*s = *s++ = *s++;
	printf("a=%d, b=%d\n", a, b);

	for (i = 0; i < 10; i++)
	{
		printf("girds[%d]=%d\n", i, grids[i]);
	}
}

int main(int argc, char* argv[])
{
	sayHello();

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
