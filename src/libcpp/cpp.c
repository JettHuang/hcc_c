/* \brief
 *		c preprocessor
 */

#include "cpp.h"
#include "utils.h"
#include "logger.h"
#include "hstring.h"
#include "pplexer.h"

#include <stdio.h>
#include <string.h>
#include <time.h>


static FLocation empty_loc = { NULL, 0, 0 };

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

	sprintf(result, "\"%.3s %2d %d\"", mon_name[timeinfo->tm_mon], timeinfo->tm_mday, 1900 + timeinfo->tm_year);
	ctx->_strdate = hs_hashstr(result);

	sprintf(result, "\"%.2d:%.2d:%.2d\"", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
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
	
	ctx->_HS__DATE__ = hs_hashstr("__DATE__");
	ctx->_HS__FILE__ = hs_hashstr("__FILE__");
	ctx->_HS__LINE__ = hs_hashstr("__LINE__");
	ctx->_HS__STDC__ = hs_hashstr("__STDC__");
	ctx->_HS__STDC_HOSTED__ = hs_hashstr("__STDC_HOSTED__");
	ctx->_HS__STDC_VERSION__ = hs_hashstr("__STDC_VERSION__");
	ctx->_HS__TIME__ = hs_hashstr("__TIME__");
	ctx->_HS__VA_ARGS__ = hs_hashstr("__VA_ARGS__");
	ctx->_HS__DEFINED__ = hs_hashstr("defined");

	cpp_init_date(ctx);
	// built-in macros
	{
		FMacro m = { ctx->_HS__DATE__, empty_loc, MACRO_FLAG_BUILTIN | MACRO_FLAG_UNCHANGE, 0, NULL, NULL };
		cpp_add_macro(ctx, &m);
	}
	{
		FMacro m = { ctx->_HS__FILE__, empty_loc, MACRO_FLAG_BUILTIN | MACRO_FLAG_UNCHANGE, 0, NULL, NULL };
		cpp_add_macro(ctx, &m);
	}
	{
		FMacro m = { ctx->_HS__LINE__, empty_loc, MACRO_FLAG_BUILTIN | MACRO_FLAG_UNCHANGE, 0, NULL, NULL };
		cpp_add_macro(ctx, &m);
	}
	{
		FMacro m = { ctx->_HS__STDC__, empty_loc, MACRO_FLAG_BUILTIN | MACRO_FLAG_UNCHANGE, 0, NULL, NULL };
		cpp_add_macro(ctx, &m);
	}
	{
		FMacro m = { ctx->_HS__STDC_HOSTED__, empty_loc, MACRO_FLAG_BUILTIN | MACRO_FLAG_UNCHANGE, 0, NULL, NULL };
		cpp_add_macro(ctx, &m);
	}
	{
		FMacro m = { ctx->_HS__STDC_VERSION__, empty_loc, MACRO_FLAG_BUILTIN | MACRO_FLAG_UNCHANGE, 0, NULL, NULL };
		cpp_add_macro(ctx, &m);
	}
	{
		FMacro m = { ctx->_HS__TIME__, empty_loc, MACRO_FLAG_BUILTIN | MACRO_FLAG_UNCHANGE, 0, NULL, NULL };
		cpp_add_macro(ctx, &m);
	}
	{
		FMacro m = { ctx->_HS__VA_ARGS__, empty_loc, MACRO_FLAG_BUILTIN | MACRO_FLAG_UNCHANGE, 0, NULL, NULL };
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

BOOL cpp_read_token(FCppContext* ctx, FCharStream* cs, FPPToken* tk, BOOL bwantheader, BOOL ballowerr)
{
	if (ctx->_lookaheadtk._valid) {
		ctx->_lookaheadtk._valid = 0;
		*tk = ctx->_lookaheadtk._tk;
		return TRUE;
	}

	return cpp_lexer_read_token(cs, bwantheader, tk) || ballowerr;
}

BOOL cpp_lookahead_token(FCppContext* ctx, FCharStream* cs, FPPToken* tk, BOOL bwantheader, BOOL ballowerr)
{
	if (ctx->_lookaheadtk._valid) {
		*tk = ctx->_lookaheadtk._tk;
		return TRUE;
	}

	int result = cpp_lexer_read_token(cs, bwantheader, &(ctx->_lookaheadtk._tk));
	ctx->_lookaheadtk._valid = 1;
	*tk = ctx->_lookaheadtk._tk;
	return result || ballowerr;
}

BOOL cpp_read_tokentolist(FCppContext* ctx, FCharStream* cs, FTKListNode** tail, BOOL bwantheader, BOOL ballowerr)
{
	FTKListNode* tknode = NULL;

	tknode = mm_alloc_area(sizeof(FTKListNode), CPP_MM_TEMPPOOL);
	if (!tknode) {
		logger_output_s("error: out of memory, at %s:%d\n", __FILE__, __LINE__);
		return FALSE;
	}

	tknode->_next = NULL;

	if (!cpp_read_token(ctx, cs, &tknode->_tk, bwantheader, ballowerr))
	{
		return FALSE;
	}

	*tail = tknode;
	return TRUE;
}

BOOL cpp_read_rowtokens(FCppContext* ctx, FCharStream* cs, FTKListNode** tail, BOOL bwantheader, BOOL ballowerr)
{
	enum EPPToken tktype = TK_UNCLASS;
	FTKListNode* tknode = NULL;
	do 
	{
		tknode = mm_alloc_area(sizeof(FTKListNode), CPP_MM_TEMPPOOL);
		if (!tknode) {
			logger_output_s("error: out of memory, at %s:%d\n", __FILE__, __LINE__);
			return FALSE;
		}
		
		tknode->_next = NULL;

		if (!cpp_read_token(ctx, cs, &tknode->_tk, bwantheader, ballowerr))
		{
			return FALSE;
		}

		/* append to list */
		*tail = tknode;
		tail = &tknode->_next;
		tktype = tknode->_tk._type;
	} while (tktype != TK_EOF && tktype != TK_NEWLINE);
	return TRUE;
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

BOOL cpp_process(FCppContext* ctx, const char* srcfilename, const char* outfilename)
{
	FCharStream* maincs = NULL;
	FSourceCodeContext* srcctx = NULL;
	const char* absfilename = util_convert_abs_pathname(srcfilename);

	maincs = cs_create_fromfile(absfilename);
	if (!maincs) { return FALSE; }

	srcctx = mm_alloc_area(sizeof(FSourceCodeContext), CPP_MM_PERMPOOL);
	if (!srcctx) { return FALSE; }

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
		FTKListNode* tklist = NULL;
		int start_linenum = top->_cs->_line, ctrloutputlines = 0;

		if (!cpp_read_rowtokens(ctx, top->_cs, &tklist, 0, !bcondblockpass))
		{
			return FALSE;
		}
		if (tklist->_tk._type == TK_POUND) /* '#' */
		{
			if (!cpp_do_control(ctx, tklist, &ctrloutputlines)) {
				return FALSE;
			}
			if (ctrloutputlines >= 0) {
				cpp_output_blankline(ctx, top->_cs->_line - start_linenum - ctrloutputlines);
			}
		}
		else
		{
			if (bcondblockpass)
			{
				if (!cpp_expand_rowtokens(ctx, &tklist, 1)) {
					return FALSE;
				}

				cpp_output_tokens(ctx, tklist);
				cpp_output_blankline(ctx, top->_cs->_line - start_linenum - 1);
			}
			else
			{
				cpp_output_blankline(ctx, top->_cs->_line - start_linenum);
			}

			/* check end of source file */
			if (tklist->_tk._type == TK_EOF)
			{
				/* release char stream */
				cs_release(top->_cs);

				/* check #if else stack empty ? */
				if (top->_condstack != NULL)
				{
					logger_output_s("error: #if #endif is not matching. in source %s\n", top->_srcfilename);
					return FALSE;
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

	return TRUE;
}

FCharStream* cpp_open_includefile(FCppContext* ctx, const char* filename, const char* dir, int bsearchsys)
{
	char strbuf[1024];
	FCharStream* cs = NULL;
	const char* headername = util_normalize_pathname(filename);

	if (util_is_relative_pathname(headername))
	{
		if (!bsearchsys)
		{
			strcpy(strbuf, dir);
			strcat(strbuf, headername);
			cs = cs_create_fromfile(strbuf);
			if (!cs) {
				bsearchsys = 1;
			}
		}
		if (bsearchsys)
		{
			FStrListNode* sysdir = ctx->_includedirs;
			for (; sysdir; sysdir = sysdir->_next)
			{
				strcpy(strbuf, sysdir->_str);
				strcat(strbuf, headername);

				cs = cs_create_fromfile(strbuf);
				if (cs) { break; }
			} /* end for */
		}
	}
	else
	{
		cs = cs_create_fromfile(headername);
	}

	return cs;
}
