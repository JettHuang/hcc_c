/* \brief
 *		c preprocessor
 */

#include "cpp.h"
#include "utils.h"
#include "logger.h"
#include "hstring.h"
#include "pplexer.h"

#include <stdio.h>
#include <time.h>


static void cpp_init_date(FCppContext* ctx)
{
	static const char wday_name[][4] = {
	   "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
	};
	static const char mon_name[][4] = {
	  "Jan", "Feb", "Mar", "Apr", "May", "Jun",
	  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
	};
	static char result[26];

	time_t rawtime;
	struct tm* timeinfo;

	time(&rawtime);
	timeinfo = localtime(&rawtime);

	sprintf(result, "%.3s %2d %d", mon_name[timeinfo->tm_mon], timeinfo->tm_mday, 1900 + timeinfo->tm_year);
	ctx->_strdate = hs_hashstr(result);

	sprintf(result, "%.2d:%.2d:%.2d", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
	ctx->_strtime = hs_hashstr(result);
}

void cpp_contex_init(FCppContext* ctx)
{
	ctx->_includedirs = NULL;
	ctx->_outfilename = NULL;
	ctx->_outfp = NULL;
	ctx->_macrolist = NULL;
	ctx->_sourcestack = NULL;
	ctx->_strdate = NULL;
	ctx->_strtime = NULL;
	ctx->_lookaheadtk._valid = 0;

	cpp_init_date(ctx);
	// built-in macros
	{
		FMacro m = { hs_hashstr("__DATE__"), MACRO_FLAG_BUILTIN | MACRO_FLAG_UNCHANGE, 0, NULL, NULL };
		cpp_add_macro(ctx, &m);
	}
	{
		FMacro m = { hs_hashstr("__FILE__"), MACRO_FLAG_BUILTIN | MACRO_FLAG_UNCHANGE, 0, NULL, NULL };
		cpp_add_macro(ctx, &m);
	}
	{
		FMacro m = { hs_hashstr("__LINE__"), MACRO_FLAG_BUILTIN | MACRO_FLAG_UNCHANGE, 0, NULL, NULL };
		cpp_add_macro(ctx, &m);
	}
	{
		FMacro m = { hs_hashstr("__STDC__"), MACRO_FLAG_BUILTIN | MACRO_FLAG_UNCHANGE, 0, NULL, NULL };
		cpp_add_macro(ctx, &m);
	}
	{
		FMacro m = { hs_hashstr("__STDC_HOSTED__"), MACRO_FLAG_BUILTIN | MACRO_FLAG_UNCHANGE, 0, NULL, NULL };
		cpp_add_macro(ctx, &m);
	}
	{
		FMacro m = { hs_hashstr("__STDC_VERSION__"), MACRO_FLAG_BUILTIN | MACRO_FLAG_UNCHANGE, 0, NULL, NULL };
		cpp_add_macro(ctx, &m);
	}
	{
		FMacro m = { hs_hashstr("__TIME__"), MACRO_FLAG_BUILTIN | MACRO_FLAG_UNCHANGE, 0, NULL, NULL };
		cpp_add_macro(ctx, &m);
	}
	{
		FMacro m = { hs_hashstr("__VA_ARGS__"), MACRO_FLAG_BUILTIN | MACRO_FLAG_UNCHANGE, 0, NULL, NULL };
		cpp_add_macro(ctx, &m);
	}
	{
		FMacro m = { hs_hashstr("defined"), MACRO_FLAG_BUILTIN | MACRO_FLAG_UNCHANGE | MACRO_FLAG_HIDDEN, 0, NULL, NULL };
		cpp_add_macro(ctx, &m);
	}
}

void cpp_contex_release(FCppContext* ctx)
{
	if (ctx->_outfp) {
		fclose(ctx->_outfp);
	}
	ctx->_includedirs = NULL;
	ctx->_outfilename = NULL;
	ctx->_outfp = NULL;
	ctx->_macrolist = NULL;
	ctx->_sourcestack = NULL;
	ctx->_strdate = NULL;
	ctx->_strtime = NULL;
	ctx->_lookaheadtk._valid = 0;
}

void cpp_add_includedir(FCppContext* ctx, const char* dir)
{
	FStrListNode* ptrhead = ctx->_includedirs;
	const char* ndir = util_normalize_pathname(dir);
	
	for (; ptrhead; ptrhead = ptrhead->_next)
	{
		if (ptrhead->_str == ndir)
		{
			return;
		}
	} /* end for */

	FStrListNode* ptrnode = mm_alloc_area(sizeof(FStrListNode), CPP_MM_PERMPOOL);
	if (!ptrnode) { 
		logger_output_s("error: out of memory, at %s:%d\n", __FILE__, __LINE__);
		return; 
	}

	ptrnode->_str = ndir;
	ptrnode->_next = ctx->_includedirs;
	ctx->_includedirs = ptrnode;
}

void cpp_add_definition(FCppContext* ctx, const char* str)
{
	/* parsing from command line -DXXX=YYY */
}

int cpp_read_token(FCppContext* ctx, FCharStream* cs, FPPToken* tk, int bwantheader, int ballowerr)
{
	if (ctx->_lookaheadtk._valid) {
		ctx->_lookaheadtk._valid = 0;
		*tk = ctx->_lookaheadtk._tk;
		return 1;
	}

	return cpp_lexer_read_token(cs, bwantheader, tk) || ballowerr;
}

int cpp_lookahead_token(FCppContext* ctx, FCharStream* cs, FPPToken* tk, int bwantheader, int ballowerr)
{
	if (ctx->_lookaheadtk._valid) {
		*tk = ctx->_lookaheadtk._tk;
		return 1;
	}

	int result = cpp_lexer_read_token(cs, bwantheader, &(ctx->_lookaheadtk._tk));
	ctx->_lookaheadtk._valid = 1;
	*tk = ctx->_lookaheadtk._tk;
	return result || ballowerr;
}

int cpp_read_rowtokens(FCppContext* ctx, FCharStream* cs, FTKListNode** tail, int bwantheader, int ballowerr)
{
	enum EPPToken tktype = TK_UNCLASS;
	FTKListNode* tknode = NULL;
	do 
	{
		tknode = mm_alloc_area(sizeof(FTKListNode), CPP_MM_TEMPPOOL);
		assert(tknode);
		tknode->_next = NULL;

		if (!cpp_read_token(ctx, cs, &tknode->_tk, bwantheader, ballowerr))
		{
			return 0;
		}

		/* append to list */
		*tail = tknode;
		tail = &tknode->_next;
		tktype = tknode->_tk._type;
	} while (tktype != TK_EOF && tktype != TK_NEWLINE);
	return 1;
}

void cpp_output_s(FCppContext* ctx, const char* format, ...)
{
	if (ctx->_outfp)
	{
		va_list args;
		va_start(args, format);
		vfprintf(ctx->_outfp, format, args);
		va_end(args);
	}
}

void cpp_output_blankline(FCppContext* ctx, int lines)
{
	while (lines-- > 0)
	{
		cpp_output_s(ctx, "\n");
	}
}

void cpp_output_linectrl(FCppContext* ctx, const char* filename, int line)
{
	cpp_output_s(ctx, "#line  %d  \"%s\"\n", line, filename);
}

int cpp_process(FCppContext* ctx, const char* srcfilename, const char* outfilename)
{
	FCharStream* maincs = NULL;
	FSourceCodeContext* srcctx = NULL;
	const char* absfilename = util_convert_abs_pathname(srcfilename);

	maincs = cs_create_fromfile(absfilename);
	if (!maincs) { return 0; }

	srcctx = mm_alloc_area(sizeof(FSourceCodeContext), CPP_MM_PERMPOOL);
	if (!srcctx) { return 0; }

	if (outfilename) {
		ctx->_outfp = fopen(outfilename, "wt");
	}

	srcctx->_cs = maincs;
	srcctx->_srcfilename = absfilename;
	srcctx->_srcdir = util_getpath_from_pathname(absfilename);
	srcctx->_condstack = NULL;
	srcctx->_up = NULL;

	/* push to stack */
	ctx->_sourcestack = srcctx;
	/* process source codes. */
	cpp_output_linectrl(ctx, maincs->_srcfilename, maincs->_line);

	while (ctx->_sourcestack)
	{
		FSourceCodeContext* top = ctx->_sourcestack;
		int bcondblockpass = top->_condstack == NULL || CHECK_COND_PASS(top->_condstack->_flags);
		int savedlinenum = top->_cs->_line;
		FTKListNode* tklist = NULL;

		if (!cpp_read_rowtokens(ctx, top->_cs, &tklist, 0, !bcondblockpass))
		{
			return 0;
		}

		if (tklist->_tk._type == TK_POUND) /* '#' */
		{
			cpp_output_blankline(ctx, 1);
			if (!cpp_do_control(ctx, tklist)) {
				return 0;
			}
		}
		else
		{
			if (bcondblockpass)
			{
				if (!cpp_expand_rowtokens(ctx, tklist, 1)) {
					return 0;
				}

				cpp_output_tokens(ctx, tklist);
			}
			else
			{
				cpp_output_blankline(ctx, 1);
			}

			cpp_output_blankline(ctx, top->_cs->_line - savedlinenum - 1);

			/* check end of source file */
			if (tklist->_tk._type == TK_EOF)
			{
				/* check #if else stack empty ? */
				if (top->_condstack != NULL)
				{
					logger_output_s("error: #if #endif is not matching. in source %s\n", top->_srcfilename);
					return 0;
				}

				top = top->_up;
				if (top)
				{
					cpp_output_linectrl(ctx, top->_cs->_srcfilename, top->_cs->_line);
				}
				ctx->_sourcestack = top;
			}
		}
	} /* end while */

	return 1;
}
