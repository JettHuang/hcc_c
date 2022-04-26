/* \brief
		preprocessor control.
 */

#include "cpp.h"
#include "logger.h"
#include "hstring.h"
#include <string.h>


typedef int (*fnCtrlHandlerPtr)(FCppContext* ctx, FTKListNode* tklist, int* outputlines);

typedef struct tagCtrlMeta {
	const char* _name;
	enum ECtrlType	_type;
	fnCtrlHandlerPtr _handler;
} FCtrlMeta;

static BOOL ctrl_if_handler(FCppContext* ctx, FTKListNode* tklist, int* outputlines);
static BOOL ctrl_ifdef_handler(FCppContext* ctx, FTKListNode* tklist, int* outputlines);
static BOOL ctrl_ifndef_handler(FCppContext* ctx, FTKListNode* tklist, int* outputlines);
static BOOL ctrl_elif_handler(FCppContext* ctx, FTKListNode* tklist, int* outputlines);
static BOOL ctrl_else_handler(FCppContext* ctx, FTKListNode* tklist, int* outputlines);
static BOOL ctrl_endif_handler(FCppContext* ctx, FTKListNode* tklist, int* outputlines);
static BOOL ctrl_include_handler(FCppContext* ctx, FTKListNode* tklist, int* outputlines);
static BOOL ctrl_define_handler(FCppContext* ctx, FTKListNode* tklist, int* outputlines);
static BOOL ctrl_undef_handler(FCppContext* ctx, FTKListNode* tklist, int* outputlines);
static BOOL ctrl_line_handler(FCppContext* ctx, FTKListNode* tklist, int* outputlines);
static BOOL ctrl_error_handler(FCppContext* ctx, FTKListNode* tklist, int* outputlines);
static BOOL ctrl_pragma_handler(FCppContext* ctx, FTKListNode* tklist, int* outputlines);
static BOOL ctrl_unknown_handler(FCppContext* ctx, FTKListNode* tklist, int* outputlines);

static const FCtrlMeta g_ctrls[] =
{
	{ "if", Ctrl_if, &ctrl_if_handler},
	{ "ifdef", Ctrl_ifdef, &ctrl_ifdef_handler},
	{ "ifndef", Ctrl_ifndef, &ctrl_ifndef_handler },
	{ "elif", Ctrl_elif, &ctrl_elif_handler },
	{ "else", Ctrl_else, &ctrl_else_handler },
	{ "endif", Ctrl_endif, &ctrl_endif_handler },
	{ "include", Ctrl_include, &ctrl_include_handler },
	{ "define", Ctrl_define, &ctrl_define_handler },
	{ "undef", Ctrl_undef, &ctrl_undef_handler },
	{ "line", Ctrl_line, &ctrl_line_handler },
	{ "error", Ctrl_error, &ctrl_error_handler },
	{ "pragma", Ctrl_pragma, &ctrl_pragma_handler }
};


int cpp_do_control(FCppContext* ctx, FTKListNode* tklist, int* outputlines)
{
	fnCtrlHandlerPtr handler = &ctrl_unknown_handler;
	FTKListNode* tk2nd = tklist->_next;
	int i;

	assert(tk2nd);
	for (i=0; i<sizeof(g_ctrls)/sizeof(g_ctrls[0]); ++i)
	{
		if (!strcmp(g_ctrls[i]._name, tk2nd->_tk._str))
		{
			handler = g_ctrls[i]._handler;
			break;
		}
	} /* end for i*/

	*outputlines = 0;
	return (*handler)(ctx, tklist, outputlines);
}

static FConditionBlock* alloc_condtionblock()
{
	return mm_alloc_area(sizeof(FConditionBlock), CPP_MM_PERMPOOL);
}

static BOOL ctrl_if_handler(FCppContext* ctx, FTKListNode* tklist, int* outputlines)
{
	FSourceCodeContext* top = ctx->_sourcestack;
	int bcondblockpass = top->_condstack == NULL || CHECK_COND_PASS(top->_condstack->_flags);
	int16_t flags = 0;

	if (bcondblockpass)
	{
		int eval = 0;
		FTKListNode* expr = tklist->_next->_next; /* # if expr */
		
		assert(expr);
		if (CHECK_IS_EOFLINE(expr->_tk._type))
		{
			logger_output_s("syntax error: #if expression, at %s:%d\n", expr->_tk._loc._filename, expr->_tk._loc._line);
			return FALSE;
		}

		if (!cpp_expand_rowtokens(ctx, &expr, FALSE)) {
			return FALSE;
		}

		if (!cpp_eval_constexpr(ctx, expr, &eval))
		{
			return FALSE;
		}

		if (!eval) {
			flags |= COND_FAILED_SELF;
		}
	}
	else
	{
		flags |= COND_FAILED_PARENT;
	}


	FConditionBlock* condblk = alloc_condtionblock();
	if (!condblk) {
		logger_output_s("error: out of memory. at %s:%d\n", __FILE__, __LINE__);
		return FALSE;
	}

	condblk->_ctrltype = Ctrl_if;
	condblk->_flags = flags;
	condblk->_up = top->_condstack;
	top->_condstack = condblk;
	return TRUE;
}

static BOOL ctrl_ifdef_handler(FCppContext* ctx, FTKListNode* tklist, int* outputlines)
{
	FSourceCodeContext* top = ctx->_sourcestack;
	int bcondblockpass = top->_condstack == NULL || CHECK_COND_PASS(top->_condstack->_flags);
	int16_t flags = 0;

	if (bcondblockpass)
	{
		FTKListNode* id = tklist->_next->_next; /* # ifdef ID */

		assert(id);
		if (id->_tk._type != TK_ID || !CHECK_IS_EOFLINE(id->_next->_tk._type))
		{
			logger_output_s("syntax error: #ifdef expression, at %s:%d\n", id->_tk._loc._filename, id->_tk._loc._line);
			return FALSE;
		}

		if (!cpp_find_macro(ctx, id->_tk._str)) {
			flags |= COND_FAILED_SELF;
		}
	}
	else
	{
		flags |= COND_FAILED_PARENT;
	}


	FConditionBlock* condblk = alloc_condtionblock();
	if (!condblk) {
		logger_output_s("error: out of memory. at %s:%d\n", __FILE__, __LINE__);
		return FALSE;
	}

	condblk->_ctrltype = Ctrl_ifdef;
	condblk->_flags = flags;
	condblk->_up = top->_condstack;
	top->_condstack = condblk;
	return TRUE;
}

static BOOL ctrl_ifndef_handler(FCppContext* ctx, FTKListNode* tklist, int* outputlines)
{
	FSourceCodeContext* top = ctx->_sourcestack;
	int bcondblockpass = top->_condstack == NULL || CHECK_COND_PASS(top->_condstack->_flags);
	int16_t flags = 0;

	if (bcondblockpass)
	{
		FTKListNode* id = tklist->_next->_next; /* # ifdef ID */

		assert(id);
		if (id->_tk._type != TK_ID || !CHECK_IS_EOFLINE(id->_next->_tk._type))
		{
			logger_output_s("syntax error: #ifndef expression, at %s:%d\n", id->_tk._loc._filename, id->_tk._loc._line);
			return FALSE;
		}

		if (cpp_find_macro(ctx, id->_tk._str)) {
			flags |= COND_FAILED_SELF;
		}
	}
	else
	{
		flags |= COND_FAILED_PARENT;
	}


	FConditionBlock* condblk = alloc_condtionblock();
	if (!condblk) {
		logger_output_s("error: out of memory. at %s:%d\n", __FILE__, __LINE__);
		return FALSE;
	}

	condblk->_ctrltype = Ctrl_ifndef;
	condblk->_flags = flags;
	condblk->_up = top->_condstack;
	top->_condstack = condblk;

	return TRUE;
}

static BOOL ctrl_elif_handler(FCppContext* ctx, FTKListNode* tklist, int* outputlines)
{
	FSourceCodeContext* top = ctx->_sourcestack;
	int boutercondblockpass = top->_condstack == NULL || CHECK_OUTER_COND_PASS(top->_condstack->_flags);
	int16_t flags = 0;

	/* check if else endif matching */
	if (top->_condstack == NULL || (top->_condstack->_ctrltype != Ctrl_if &&
		top->_condstack->_ctrltype != Ctrl_ifdef && top->_condstack->_ctrltype != Ctrl_ifndef &&
		top->_condstack->_ctrltype != Ctrl_elif))
	{
		logger_output_s("error: #elif is not matching. at %s:%d\n", top->_cs->_srcfilename, top->_cs->_line - 1);
		return FALSE;
	}

	if (boutercondblockpass)
	{
		FConditionBlock* condblk = NULL;
		int eval = 0, bprevsiblingpass = 0;
		FTKListNode* expr = tklist->_next->_next; /* # if expr */

		assert(expr);
		if (CHECK_IS_EOFLINE(expr->_tk._type))
		{
			logger_output_s("syntax error: #elif expression, at %s:%d\n", expr->_tk._loc._filename, expr->_tk._loc._line);
			return FALSE;
		}

		/* check prev siblings condition result */
		for (condblk = top->_condstack; condblk; condblk = condblk->_up)
		{
			bprevsiblingpass = !(condblk->_flags & COND_FAILED_SELF);
			if (bprevsiblingpass) {
				break;
			}

			if (condblk->_ctrltype == Ctrl_if || condblk->_ctrltype == Ctrl_ifdef
				|| condblk->_ctrltype == Ctrl_ifndef)
			{
				break;
			}
		} /* end for */

		if (!bprevsiblingpass)
		{
			if (!cpp_expand_rowtokens(ctx, &expr, FALSE)) {
				return FALSE;
			}

			if (!cpp_eval_constexpr(ctx, expr, &eval))
			{
				return FALSE;
			}
		}

		if (bprevsiblingpass || !eval) {
			flags |= COND_FAILED_SELF;
		}
	}
	else
	{
		flags |= COND_FAILED_PARENT;
	}

	FConditionBlock* condblk = alloc_condtionblock();
	if (!condblk) {
		logger_output_s("error: out of memory. at %s:%d\n", __FILE__, __LINE__);
		return FALSE;
	}

	condblk->_ctrltype = Ctrl_elif;
	condblk->_flags = flags;
	condblk->_up = top->_condstack;
	top->_condstack = condblk;

	return TRUE;
}

static BOOL ctrl_else_handler(FCppContext* ctx, FTKListNode* tklist, int* outputlines)
{
	FSourceCodeContext* top = ctx->_sourcestack;
	int boutercondblockpass = top->_condstack == NULL || CHECK_OUTER_COND_PASS(top->_condstack->_flags);
	int16_t flags = 0;

	/* check if else endif matching */
	if (top->_condstack == NULL || (top->_condstack->_ctrltype != Ctrl_if &&
		top->_condstack->_ctrltype != Ctrl_ifdef && top->_condstack->_ctrltype != Ctrl_ifndef &&
		top->_condstack->_ctrltype != Ctrl_elif))
	{
		logger_output_s("error: #else is not matching. at %s:%d\n", top->_cs->_srcfilename, top->_cs->_line - 1);
		return FALSE;
	}

	if (boutercondblockpass)
	{
		FConditionBlock* condblk = NULL;
		int bprevsiblingpass = 0;
		FTKListNode* expr = tklist->_next->_next; /* # else \n */

		assert(expr);
		if (!CHECK_IS_EOFLINE(expr->_tk._type))
		{
			logger_output_s("syntax error: #else expression, at %s:%d\n", expr->_tk._loc._filename, expr->_tk._loc._line);
			return FALSE;
		}

		/* check prev siblings condition result */
		for (condblk = top->_condstack; condblk; condblk = condblk->_up)
		{
			bprevsiblingpass = !(condblk->_flags & COND_FAILED_SELF);
			if (bprevsiblingpass) {
				break;
			}

			if (condblk->_ctrltype == Ctrl_if || condblk->_ctrltype == Ctrl_ifdef
				|| condblk->_ctrltype == Ctrl_ifndef)
			{
				break;
			}
		} /* end for */

		if (bprevsiblingpass) {
			flags |= COND_FAILED_SELF;
		}
	}
	else
	{
		flags |= COND_FAILED_PARENT;
	}

	FConditionBlock* condblk = alloc_condtionblock();
	if (!condblk) {
		logger_output_s("error: out of memory. at %s:%d\n", __FILE__, __LINE__);
		return FALSE;
	}

	condblk->_ctrltype = Ctrl_else;
	condblk->_flags = flags;
	condblk->_up = top->_condstack;
	top->_condstack = condblk;

	return TRUE;
}

static BOOL ctrl_endif_handler(FCppContext* ctx, FTKListNode* tklist, int* outputlines)
{
	FSourceCodeContext* top = ctx->_sourcestack;
	FTKListNode* expr = tklist->_next->_next; /* # endif \n */

	assert(expr);
	if (!CHECK_IS_EOFLINE(expr->_tk._type))
	{
		logger_output_s("syntax error: #endif expression, at %s:%d\n", expr->_tk._loc._filename, expr->_tk._loc._line);
		return FALSE;
	}

	while (top->_condstack)
	{
		FConditionBlock* condblk = top->_condstack;
		if (condblk->_ctrltype == Ctrl_if || condblk->_ctrltype == Ctrl_ifdef
			|| condblk->_ctrltype == Ctrl_ifndef)
		{
			break;
		}
		top->_condstack = condblk->_up;
	} /* end while */

	if (!top->_condstack)
	{
		logger_output_s("syntax error: #endif is not matching, at %s:%d\n", expr->_tk._loc._filename, expr->_tk._loc._line);
		return FALSE;
	}

	top->_condstack = top->_condstack->_up;
	return TRUE;
}

static BOOL ctrl_include_handler(FCppContext* ctx, FTKListNode* tklist, int* outputlines)
{
	FSourceCodeContext* top = ctx->_sourcestack;
	int bcondblockpass = top->_condstack == NULL || CHECK_COND_PASS(top->_condstack->_flags);
	int bsearchsysdir = 0, repeatcnt;
	const char* headerfile = NULL;
	FCharStream* newcs = NULL;

	if (!bcondblockpass) {
		return TRUE;
	}

	/* syntax: #include <xxx>  #include "..." */
	{
		char strbuf[512];
		int bsyntaxerr = 1;
		FTKListNode* start = tklist->_next->_next;
		if (start->_tk._type == TK_CONSTANT_STR)
		{
			if (CHECK_IS_EOFLINE(start->_next->_tk._type))
			{
				size_t n = strlen(start->_tk._str);
				strncpy(strbuf, start->_tk._str + 1, n - 2); /* remove "" */
				strbuf[n - 2] = '\0';
				headerfile = hs_hashstr(strbuf);
				bsyntaxerr = 0;
			}
		}
		else if (start->_tk._type == TK_LESS) /* < */
		{
			FTKListNode* next = start->_next;
			strbuf[0] = '\0';
			while (next->_tk._type != TK_LESS && next->_tk._type != TK_GREAT && !CHECK_IS_EOFLINE(next->_tk._type))
			{
				strcat(strbuf, next->_tk._str);
				next = next->_next;
			}
			if (next->_tk._type == TK_GREAT && CHECK_IS_EOFLINE(next->_next->_tk._type))
			{
				headerfile = hs_hashstr(strbuf);
				bsyntaxerr = 0;
			}
			bsearchsysdir = 1;
		}

		if (bsyntaxerr)
		{
			logger_output_s("syntax error: #include is invalid at %s:%d\n", start->_tk._loc._filename, start->_tk._loc._line);
			return FALSE;
		}
	}

	newcs = cpp_open_includefile(ctx, headerfile, top->_srcdir, bsearchsysdir);
	if (!newcs) {
		logger_output_s("error: open header file '%s' failed at %s:%d\n", headerfile, tklist->_tk._loc._filename, tklist->_tk._loc._line);
		return FALSE;
	}

	/* check recursive including */
	repeatcnt = 0;
	for (top = ctx->_sourcestack; top; top = top->_up)
	{
		if (top->_cs->_srcfilename == newcs->_srcfilename)
		{
			repeatcnt++;
		}
	} /* end for */

	if (repeatcnt >= 2)
	{
		logger_output_s("error: open header file '%' recursively at %s:%d\n", headerfile, tklist->_tk._loc._filename, tklist->_tk._loc._line);
		return FALSE;
	}

	/* push new source code */
	top = mm_alloc_area(sizeof(FSourceCodeContext), CPP_MM_PERMPOOL);
	if (!top) { return FALSE; }

	top->_cs = newcs;
	top->_srcfilename = newcs->_srcfilename;
	top->_srcdir = util_getpath_from_pathname(newcs->_srcfilename);
	top->_condstack = NULL;
	top->_up = NULL;

	/* push to stack */
	top->_up = ctx->_sourcestack;
	ctx->_sourcestack = top;
	/* process source codes. */
	cpp_output_linectrl(ctx, newcs->_srcfilename, newcs->_line);

	*outputlines = 1;
	return TRUE;
}

static BOOL ctrl_undef_handler(FCppContext* ctx, FTKListNode* tklist, int* outputlines)
{
	FSourceCodeContext* top = ctx->_sourcestack;
	int bcondblockpass = top->_condstack == NULL || CHECK_COND_PASS(top->_condstack->_flags);
	FTKListNode* id = NULL;
	const FMacro* macro = NULL;

	if (!bcondblockpass) {
		return TRUE;
	}

	/* #undef ID */
	id = tklist->_next->_next;
	if (id->_tk._type != TK_ID || !CHECK_IS_EOFLINE(id->_next->_tk._type))
	{
		logger_output_s("syntax error: #undef marco, at %s:%d\n", id->_tk._loc._filename, id->_tk._loc._line);
		return FALSE;
	}

	if (id->_tk._str == ctx->_HS__DEFINED__)
	{
		logger_output_s("error: #undef defined is not allowed, at %s:%d\n", id->_tk._loc._filename, id->_tk._loc._line);
		return FALSE;
	}

	macro = cpp_find_macro(ctx, id->_tk._str);
	if (macro && (macro->_flags & MACRO_FLAG_UNCHANGE))
	{
		logger_output_s("error: #undef %s is not allowed, at %s:%d\n", id->_tk._str, id->_tk._loc._filename, id->_tk._loc._line);
		return FALSE;
	}

	if (macro)
	{
		cpp_remove_macro(ctx, id->_tk._str);
	}
	return TRUE;
}

static BOOL ctrl_line_handler(FCppContext* ctx, FTKListNode* tklist, int* outputlines)
{
	FSourceCodeContext* top = ctx->_sourcestack;
	int bcondblockpass = top->_condstack == NULL || CHECK_COND_PASS(top->_condstack->_flags);
	FTKListNode* expr = tklist->_next->_next;

	const char* filename = NULL;
	int linenum = 0;

	if (!bcondblockpass) {
		return TRUE;
	}

	if (!cpp_expand_rowtokens(ctx, &expr, 0))
	{
		return FALSE;
	}

	/* #line number filename */
	if (expr->_tk._type == TK_PPNUMBER && CHECK_IS_EOFLINE(expr->_next->_tk._type))
	{
		linenum = atol(expr->_tk._str);
	}
	else if (expr->_tk._type == TK_PPNUMBER && expr->_next->_tk._type == TK_CONSTANT_STR && CHECK_IS_EOFLINE(expr->_next->_next->_tk._type))
	{
		const char* cnststr = expr->_next->_tk._str;
		linenum = atol(expr->_tk._str);
		filename = hs_hashnstr2(cnststr + 1, strlen(cnststr) - 2); /* omit 2 " */
		filename = util_normalize_pathname(filename);
	}
	else
	{
		logger_output_s("syntax error: #line number filename(opt), at %s:%d\n", tklist->_tk._loc._filename, tklist->_tk._loc._line);
		return FALSE;
	}

	top->_cs->_line = linenum;
	if (filename) {
		top->_cs->_srcfilename = filename;
	}

	cpp_output_linectrl(ctx, filename, linenum);
	*outputlines = 1;

	return TRUE;
}

static BOOL ctrl_error_handler(FCppContext* ctx, FTKListNode* tklist, int* outputlines)
{
	FSourceCodeContext* top = ctx->_sourcestack;
	int bcondblockpass = top->_condstack == NULL || CHECK_COND_PASS(top->_condstack->_flags);

	if (!bcondblockpass) {
		return TRUE;
	}

	cpp_output_tokens(ctx, tklist);
	*outputlines = 1;
	return FALSE;
}

static BOOL ctrl_pragma_handler(FCppContext* ctx, FTKListNode* tklist, int* outputlines)
{
	FSourceCodeContext* top = ctx->_sourcestack;
	int bcondblockpass = top->_condstack == NULL || CHECK_COND_PASS(top->_condstack->_flags);
	FTKListNode* tkparam;

	if (!bcondblockpass) {
		return TRUE;
	}

	tkparam = tklist->_next->_next;
	if (tkparam && tkparam->_tk._str == ctx->_HS_ONCE__)
	{
		const char* currfile;
		int n;

		currfile = top->_srcfilename;
		for (n = 0; n < ctx->_pragma_onces._elecount; ++n)
		{
			const char* filename = *(const char**)array_element(&ctx->_pragma_onces, n);
			if (filename == currfile)
			{
				ctx->_stop_flag = TRUE;
				return TRUE;
			}
		} /* end for n */

		array_append(&ctx->_pragma_onces, &currfile);
		return TRUE;
	}

	cpp_output_tokens(ctx, tklist);
	*outputlines = 1;
	return TRUE;
}

static BOOL ctrl_unknown_handler(FCppContext* ctx, FTKListNode* tklist, int* outputlines)
{
	FSourceCodeContext* top = ctx->_sourcestack;
	int bcondblockpass = top->_condstack == NULL || CHECK_COND_PASS(top->_condstack->_flags);

	/* # newline is allowed */
	if (!CHECK_IS_EOFLINE(tklist->_next->_tk._type))
	{
		if (bcondblockpass)
		{
			FTKListNode* tknode = tklist->_next;
			logger_output_s("error: unknown preprocessor directive %s at %s:%d\n", tknode->_tk._str, tknode->_tk._loc._filename, tknode->_tk._loc._line);
			return FALSE;
		}
	}
	return TRUE;
}

/* compare tokens */
static BOOL compare_tokens(FTKListNode* lhs, FTKListNode* rhs)
{
	while (lhs != rhs)
	{
		if (!lhs || !rhs) { return FALSE; }
		if (lhs->_tk._type != rhs->_tk._type) { return FALSE; }
		if (lhs->_tk._str != rhs->_tk._str) { return FALSE; }
		if ((lhs->_tk._wscnt == 0) != (rhs->_tk._wscnt == 0)) { return FALSE; }

		lhs = lhs->_next;
		rhs = rhs->_next;
	} /* end while */

	return TRUE;
}

static BOOL ctrl_define_handler(FCppContext* ctx, FTKListNode* tklist, int* outputlines)
{
	FSourceCodeContext* top = ctx->_sourcestack;
	int bcondblockpass = top->_condstack == NULL || CHECK_COND_PASS(top->_condstack->_flags);
	FTKListNode* id = tklist->_next->_next;
	const FMacro* ptrexist;
	FTKListNode* args_head, *args_tail, *body_head, *body_tail, *tkitr;
	int argc, isvarg;


	if (!bcondblockpass) {
		return TRUE;
	}

	/* #define ID (xxx yyy zzz) NL */
	if (id->_tk._type != TK_ID)
	{
		logger_output_s("syntax error: #define ID xxx at %s:%d\n", tklist->_tk._loc._filename, tklist->_tk._loc._line);
		return FALSE;
	}

	if ((id->_tk._str == ctx->_HS__DEFINED__) || 
		((ptrexist = cpp_find_macro(ctx, id->_tk._str)) && (ptrexist->_flags & MACRO_FLAG_UNCHANGE)))
	{
		logger_output_s("error: #define %s is not allowed at %s:%d\n", id->_tk._str, tklist->_tk._loc._filename, tklist->_tk._loc._line);
		return FALSE;
	}

	argc = -1; isvarg = 0;
	args_head = args_tail = body_head = body_tail = NULL;
	tkitr = id->_next;
	if (tkitr->_tk._type == TK_LPAREN && tkitr->_tk._wscnt == 0)
	{
		/* macro with args. for example:
		   #define X()		YYY
		   #define X(a, b)	a b
		*/
		tkitr = tkitr->_next;
		argc = 0;
		if (tkitr->_tk._type != TK_RPAREN)
		{
			FTKListNode* tmp;

			/* gather arguments */
			for (;;)
			{
				argc++;

				if (tkitr->_tk._type != TK_ID && tkitr->_tk._type != TK_ELLIPSIS)
				{
					logger_output_s("syntax error: #define require ID, at %s:%d:%d\n", tkitr->_tk._loc._filename, tkitr->_tk._loc._line, tkitr->_tk._loc._col);
					return FALSE;
				}

				/* check if exist an argument has the same name */
				for (tmp = args_head; tmp; tmp = tmp->_next)
				{
					if (tmp->_tk._str == tkitr->_tk._str)
					{
						logger_output_s("syntax error: #define duplicate macro argument at %s:%d:%d\n", tkitr->_tk._loc._filename, tkitr->_tk._loc._line, tkitr->_tk._loc._col);
						return FALSE;
					}
				}

				if (!args_head) { 
					args_head = args_tail = tkitr; 
				}
				else {
					args_tail->_next = tkitr;
					args_tail = tkitr;
				}

				tkitr = tkitr->_next;
				args_tail->_next = NULL;
				if (args_tail->_tk._type == TK_ELLIPSIS) 
				{
					isvarg = 1;
					break;
				}
				if (tkitr->_tk._type == TK_RPAREN)
				{
					break;
				}
				if (tkitr->_tk._type != TK_COMMA)
				{
					logger_output_s("syntax error: #define require ',' at %s:%d:%d\n", tkitr->_tk._loc._filename, tkitr->_tk._loc._line, tkitr->_tk._loc._col);
					return FALSE;
				}

				tkitr = tkitr->_next;
			} /* end for ;; */
		}
	
		if (tkitr->_tk._type != TK_RPAREN)
		{
			logger_output_s("syntax error: ')' is required at %s:%d:%d\n", tkitr->_tk._loc._filename, tkitr->_tk._loc._line, tkitr->_tk._loc._col);
			return FALSE;
		}

		/* omit ')' */
		tkitr = tkitr->_next;
	}

	while (tkitr && !CHECK_IS_EOFLINE(tkitr->_tk._type))
	{
		if (!body_head) {
			body_head = body_tail = tkitr;
		}
		else {
			body_tail->_next = tkitr;
			body_tail = tkitr;
		}

		tkitr = tkitr->_next;
		body_tail->_next = NULL;
	}

	if (body_head && body_head->_tk._type == TK_DOUBLE_POUND)
	{
		logger_output_s("syntax error: ## should not be at here, at %s:%d:%d\n", body_head->_tk._loc._filename, body_head->_tk._loc._line, body_head->_tk._loc._col);
		return FALSE;
	}
	if (body_tail && body_tail->_tk._type == TK_DOUBLE_POUND)
	{
		logger_output_s("syntax error: ## should not be at here, at %s:%d:%d\n", body_tail->_tk._loc._filename, body_tail->_tk._loc._line, body_tail->_tk._loc._col);
		return FALSE;
	}

	/* compare with exist macro */
	if (ptrexist)
	{
		if (!compare_tokens(args_head, ptrexist->_args) || !compare_tokens(body_head, ptrexist->_body))
		{
			logger_output_s("syntax error: macro '%s're-defined at %s:%d, previous define is at %s:%d\n", id->_tk._str, id->_tk._loc._filename, id->_tk._loc._line,
				ptrexist->_loc._filename, ptrexist->_loc._line);
			return FALSE;
		}
	}
	else
	{
		FMacro m;

		m._name = id->_tk._str;
		m._loc = id->_tk._loc;
		m._flags = 0;
		m._argc = argc;
		m._isvarg = isvarg;
		m._args = cpp_duplicate_tklist(args_head, CPP_MM_PERMPOOL);
		m._body = cpp_duplicate_tklist(body_head, CPP_MM_PERMPOOL);

		cpp_add_macro(ctx, &m);
	}

	return TRUE;
}


FTKListNode* cpp_duplicate_tklist(const FTKListNode* orglist, enum EMMArea where)
{
	const FTKListNode *itr;
	FTKListNode *head, * tail;

	head = tail = NULL;
	for (itr = orglist; itr; itr = itr->_next)
	{
		FTKListNode* node = mm_alloc_area(sizeof(FTKListNode), where);
		if (!node) {
			logger_output_s("error: out of memory! %s:%d\n", __FILE__, __LINE__);
			return NULL;
		}

		*node = *itr;
		node->_next = NULL;
		if (!head) {
			head = tail = node;
		}
		else {
			tail->_next = node;
			tail = node;
		}
	} /* end for */

	return head;
}

FTKListNode* cpp_duplicate_token(const FTKListNode* orgtk, enum EMMArea where)
{
	FTKListNode* node = mm_alloc_area(sizeof(FTKListNode), where);
	if (!node) {
		logger_output_s("error: out of memory! %s:%d\n", __FILE__, __LINE__);
		return NULL;
	}

	*node = *orgtk;
	node->_next = NULL;

	return node;
}

void cpp_add_definition(FCppContext* ctx, const char* str)
{
	FSourceCodeContext srcctx;
	FCharStream* cs;
	FTKListNode* tklist;
	char* szbuf, * s;
	int szlen, lines;


	/* parsing from command line -DXXX -DXXX=YYY */
	if (!str)
	{
		return;
	}

	szlen = strlen(str) + sizeof("#define ");
	szbuf = mm_alloc_area(szlen, CPP_MM_TEMPPOOL);
	if (!szbuf) {
		return;
	}

	sprintf(szbuf, "#define %s", str);
	for (s = szbuf; *s; s++)
	{
		if (*s == '=')
		{
			*s = ' ';
			break;
		}
	} /* end while */

	cs = cs_create_fromstring(szbuf);
	if (!cs)
	{
		logger_output_s("error: out of memory, at %s:%d\n", __FILE__, __LINE__);
		return;
	}

	cs->_srcfilename = hs_hashstr("cmd-args");
	srcctx._cs = cs;
	srcctx._srcfilename = cs->_srcfilename;
	srcctx._srcdir = NULL;
	srcctx._condstack = NULL;
	srcctx._up = NULL;
	/* push to stack */
	ctx->_sourcestack = &srcctx;

	if (!cpp_read_rowtokens(ctx, cs, &tklist, 0, 0) ||
		!ctrl_define_handler(ctx, tklist, &lines))
	{
		logger_output_s("warning: command -D parsing failed,  %s\n", str);
	}

	ctx->_sourcestack = NULL;
	cs_release(cs);
}
