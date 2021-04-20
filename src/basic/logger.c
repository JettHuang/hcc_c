/* \brief 
 *		logger implementation
 */

#include "logger.h"
#include <stdio.h>


static void default_outc(int c)
{
	fputc(c, stdout);
}

static void default_outs(const char* format, va_list arg)
{
	vfprintf(stdout, format, arg);
}

static void (*g_outc)(int) = default_outc;
static void (*g_outs)(const char*, va_list arg) = default_outs;

void logger_set(void (*outc)(int), void (*outs)(const char*, va_list))
{
	g_outc = outc;
	g_outs = outs;
}

void logger_output_c(int c)
{
	if (g_outc) {
		g_outc(c);
	}
}

void logger_output_s(const char* format, ...)
{
	if (g_outs) {
		va_list args;
		va_start(args, format);
		g_outs(format, args);
		va_end(args);
	}
}
