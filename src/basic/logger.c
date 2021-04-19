/* \brief 
 *		logger implementation
 */

#include "logger.h"
#include <stdio.h>


static void default_outc(int c)
{
	fputc(c, stdout);
}

static void default_outs(const char* str)
{
	fputs(str, stdout);
}

static void (*g_outc)(int) = default_outc;
static void (*g_outs)(const char*) = default_outs;

void logger_set(void (*outc)(int), void (*outs)(const char*))
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

void logger_output_s(const char* str)
{
	if (g_outs) {
		g_outs(str);
	}
}