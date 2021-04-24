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

static int ctrl_if_handler(FCppContext* ctx, FTKListNode* tklist, int* outputlines);
static int ctrl_ifdef_handler(FCppContext* ctx, FTKListNode* tklist, int* outputlines);
static int ctrl_ifndef_handler(FCppContext* ctx, FTKListNode* tklist, int* outputlines);
static int ctrl_elif_handler(FCppContext* ctx, FTKListNode* tklist, int* outputlines);
static int ctrl_else_handler(FCppContext* ctx, FTKListNode* tklist, int* outputlines);
static int ctrl_endif_handler(FCppContext* ctx, FTKListNode* tklist, int* outputlines);
static int ctrl_include_handler(FCppContext* ctx, FTKListNode* tklist, int* outputlines);
static int ctrl_define_handler(FCppContext* ctx, FTKListNode* tklist, int* outputlines);
static int ctrl_undef_handler(FCppContext* ctx, FTKListNode* tklist, int* outputlines);
static int ctrl_line_handler(FCppContext* ctx, FTKListNode* tklist, int* outputlines);
static int ctrl_error_handler(FCppContext* ctx, FTKListNode* tklist, int* outputlines);
static int ctrl_pragma_handler(FCppContext* ctx, FTKListNode* tklist, int* outputlines);
static int ctrl_unknown_handler(FCppContext* ctx, FTKListNode* tklist, int* outputlines);

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

static int ctrl_if_handler(FCppContext* ctx, FTKListNode* tklist, int* outputlines)
{
	FSourceCodeContext* top = ctx->_sourcestack;
	int bcondblockpass = top->_condstack == NULL || CHECK_COND_PASS(top->_condstack->_flags);
	int16_t flags = 0;

	if (bcondblockpass)
	{
		int eval = 0;
		FTKListNode* expr = tklist->_next->_next; /* # if expr */
		
		assert(!expr);
		if (CHECK_IS_EOFLINE(expr->_tk._type))
		{
			logger_output_s("syntax error: #if expression, at %s:%d\n", expr->_tk._loc._filename, expr->_tk._loc._line);
			return 0;
		}

		if (!cpp_eval_constexpr(ctx, expr, &eval));
		{
			return 0;
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
		return 0;
	}

	condblk->_ctrltype = Ctrl_if;
	condblk->_flags = flags;
	condblk->_up = top->_condstack;
	top->_condstack = condblk;
	return 1;
}

static int ctrl_ifdef_handler(FCppContext* ctx, FTKListNode* tklist, int* outputlines)
{
	FSourceCodeContext* top = ctx->_sourcestack;
	int bcondblockpass = top->_condstack == NULL || CHECK_COND_PASS(top->_condstack->_flags);
	int16_t flags = 0;

	if (bcondblockpass)
	{
		FTKListNode* id = tklist->_next->_next; /* # ifdef ID */

		assert(id);
		if (id->_tk._type != TK_ID || CHECK_IS_EOFLINE(id->_next->_tk._type))
		{
			logger_output_s("syntax error: #ifdef expression, at %s:%d\n", id->_tk._loc._filename, id->_tk._loc._line);
			return 0;
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
		return 0;
	}

	condblk->_ctrltype = Ctrl_ifdef;
	condblk->_flags = flags;
	condblk->_up = top->_condstack;
	top->_condstack = condblk;
	return 1;
}

static int ctrl_ifndef_handler(FCppContext* ctx, FTKListNode* tklist, int* outputlines)
{
	FSourceCodeContext* top = ctx->_sourcestack;
	int bcondblockpass = top->_condstack == NULL || CHECK_COND_PASS(top->_condstack->_flags);
	int16_t flags = 0;

	if (bcondblockpass)
	{
		FTKListNode* id = tklist->_next->_next; /* # ifdef ID */

		assert(id);
		if (id->_tk._type != TK_ID || CHECK_IS_EOFLINE(id->_next->_tk._type))
		{
			logger_output_s("syntax error: #ifndef expression, at %s:%d\n", id->_tk._loc._filename, id->_tk._loc._line);
			return 0;
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
		return 0;
	}

	condblk->_ctrltype = Ctrl_ifndef;
	condblk->_flags = flags;
	condblk->_up = top->_condstack;
	top->_condstack = condblk;

	return 1;
}

static int ctrl_elif_handler(FCppContext* ctx, FTKListNode* tklist, int* outputlines)
{
	FSourceCodeContext* top = ctx->_sourcestack;
	int boutercondblockpass = top->_condstack == NULL || CHECK_OUTER_COND_PASS(top->_condstack->_flags);
	int16_t flags = 0;

	/* check if else endif matching */
	if (top->_condstack == NULL || top->_condstack->_ctrltype != Ctrl_if ||
		top->_condstack->_ctrltype != Ctrl_ifdef || top->_condstack->_ctrltype != Ctrl_ifndef ||
		top->_condstack->_ctrltype != Ctrl_elif)
	{
		logger_output_s("error: #elif is not matching. at %s:%d\n", top->_cs->_srcfilename, top->_cs->_line - 1);
		return 0;
	}

	if (boutercondblockpass)
	{
		FConditionBlock* condblk = NULL;
		int eval = 0, bprevsiblingpass = 0;
		FTKListNode* expr = tklist->_next->_next; /* # if expr */

		assert(!expr);
		if (CHECK_IS_EOFLINE(expr->_tk._type))
		{
			logger_output_s("syntax error: #elif expression, at %s:%d\n", expr->_tk._loc._filename, expr->_tk._loc._line);
			return 0;
		}

		if (!cpp_eval_constexpr(ctx, expr, &eval));
		{
			return 0;
		}

		/* check prev siblings condition result */
		for (condblk = top->_condstack; condblk; condblk=condblk->_up)
		{
			bprevsiblingpass = !(condblk->_flags & COND_FAILED_SELF);
			if (bprevsiblingpass) {
				break;
			}
		} /* end for */

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
		return 0;
	}

	condblk->_ctrltype = Ctrl_elif;
	condblk->_flags = flags;
	condblk->_up = top->_condstack;
	top->_condstack = condblk;

	return 1;
}

static int ctrl_else_handler(FCppContext* ctx, FTKListNode* tklist, int* outputlines)
{
	FSourceCodeContext* top = ctx->_sourcestack;
	int boutercondblockpass = top->_condstack == NULL || CHECK_OUTER_COND_PASS(top->_condstack->_flags);
	int16_t flags = 0;

	/* check if else endif matching */
	if (top->_condstack == NULL || top->_condstack->_ctrltype != Ctrl_if ||
		top->_condstack->_ctrltype != Ctrl_ifdef || top->_condstack->_ctrltype != Ctrl_ifndef ||
		top->_condstack->_ctrltype != Ctrl_elif)
	{
		logger_output_s("error: #else is not matching. at %s:%d\n", top->_cs->_srcfilename, top->_cs->_line - 1);
		return 0;
	}

	if (boutercondblockpass)
	{
		FConditionBlock* condblk = NULL;
		int bprevsiblingpass = 0;
		FTKListNode* expr = tklist->_next->_next; /* # else \n */

		assert(!expr);
		if (!CHECK_IS_EOFLINE(expr->_tk._type))
		{
			logger_output_s("syntax error: #else expression, at %s:%d\n", expr->_tk._loc._filename, expr->_tk._loc._line);
			return 0;
		}

		/* check prev siblings condition result */
		for (condblk = top->_condstack; condblk; condblk = condblk->_up)
		{
			bprevsiblingpass = !(condblk->_flags & COND_FAILED_SELF);
			if (bprevsiblingpass) {
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
		return 0;
	}

	condblk->_ctrltype = Ctrl_else;
	condblk->_flags = flags;
	condblk->_up = top->_condstack;
	top->_condstack = condblk;

	return 1;
}

static int ctrl_endif_handler(FCppContext* ctx, FTKListNode* tklist, int* outputlines)
{
	FSourceCodeContext* top = ctx->_sourcestack;
	FTKListNode* expr = tklist->_next->_next; /* # endif \n */

	assert(!expr);
	if (!CHECK_IS_EOFLINE(expr->_tk._type))
	{
		logger_output_s("syntax error: #endif expression, at %s:%d\n", expr->_tk._loc._filename, expr->_tk._loc._line);
		return 0;
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
		return 0;
	}

	top->_condstack = top->_condstack->_up;
	return 1;
}

static int ctrl_include_handler(FCppContext* ctx, FTKListNode* tklist, int* outputlines)
{
	FSourceCodeContext* top = ctx->_sourcestack;
	int bcondblockpass = top->_condstack == NULL || CHECK_COND_PASS(top->_condstack->_flags);
	int bsearchsysdir = 0;
	const char* headerfile = NULL;
	FCharStream* newcs = NULL;

	if (!bcondblockpass) {
		return 1;
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
			return 0;
		}
	}

	newcs = cpp_open_includefile(ctx, headerfile, top->_srcdir, bsearchsysdir);
	if (!newcs) {
		logger_output_s("error: open header file failed, %s at %s:%d\n", headerfile, tklist->_tk._loc._filename, tklist->_tk._loc._line);
		return 0;
	}

	/* check recursive including */
	for (top = ctx->_sourcestack; top; top = top->_up)
	{
		if (top->_cs->_srcfilename == newcs->_srcfilename)
		{
			logger_output_s("error: open header file recursive, %s at %s:%d\n", headerfile, tklist->_tk._loc._filename, tklist->_tk._loc._line);
			return 0;
		}
	} /* end for */

	/* push new source code */
	top = mm_alloc_area(sizeof(FSourceCodeContext), CPP_MM_PERMPOOL);
	if (!top) { return 0; }

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
	return 1;
}

static int ctrl_undef_handler(FCppContext* ctx, FTKListNode* tklist, int* outputlines)
{
	FSourceCodeContext* top = ctx->_sourcestack;
	int bcondblockpass = top->_condstack == NULL || CHECK_COND_PASS(top->_condstack->_flags);
	FTKListNode* id = NULL;
	const FMacro* macro = NULL;

	if (!bcondblockpass) {
		return 1;
	}

	/* #undef ID */
	id = tklist->_next->_next;
	if (id->_tk._type != TK_ID || !CHECK_IS_EOFLINE(id->_next->_tk._type))
	{
		logger_output_s("syntax error: #undef marco, at %s:%d\n", id->_tk._loc._filename, id->_tk._loc._line);
		return 0;
	}

	if (id->_tk._str == ctx->_HS__DEFINED__)
	{
		logger_output_s("error: #undef defined is not allowed, at %s:%d\n", id->_tk._loc._filename, id->_tk._loc._line);
		return 0;
	}

	macro = cpp_find_macro(ctx, id->_tk._str);
	if (macro && (macro->_flags & MACRO_FLAG_UNCHANGE))
	{
		logger_output_s("error: #undef %s is not allowed, at %s:%d\n", id->_tk._str, id->_tk._loc._filename, id->_tk._loc._line);
		return 0;
	}

	if (macro)
	{
		cpp_remove_macro(ctx, id->_tk._str);
	}
	return 1;
}

static int ctrl_line_handler(FCppContext* ctx, FTKListNode* tklist, int* outputlines)
{
	FSourceCodeContext* top = ctx->_sourcestack;
	int bcondblockpass = top->_condstack == NULL || CHECK_COND_PASS(top->_condstack->_flags);
	FTKListNode* expr = tklist->_next->_next;

	const char* filename = NULL;
	int linenum = 0;

	if (!bcondblockpass) {
		return 1;
	}

	if (!cpp_expand_rowtokens(ctx, &expr, 0))
	{
		return 0;
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
		return 0;
	}

	top->_cs->_line = linenum;
	if (filename) {
		top->_cs->_srcfilename = filename;
	}

	cpp_output_linectrl(ctx, filename, linenum);
	*outputlines = 1;

	return 1;
}

static int ctrl_error_handler(FCppContext* ctx, FTKListNode* tklist, int* outputlines)
{
	FSourceCodeContext* top = ctx->_sourcestack;
	int bcondblockpass = top->_condstack == NULL || CHECK_COND_PASS(top->_condstack->_flags);

	if (!bcondblockpass) {
		return 1;
	}

	cpp_output_tokens(ctx, tklist);
	*outputlines = 1;
	return 0;
}

static int ctrl_pragma_handler(FCppContext* ctx, FTKListNode* tklist, int* outputlines)
{
	FSourceCodeContext* top = ctx->_sourcestack;
	int bcondblockpass = top->_condstack == NULL || CHECK_COND_PASS(top->_condstack->_flags);

	if (!bcondblockpass) {
		return 1;
	}

	cpp_output_tokens(ctx, tklist);
	*outputlines = 1;
	return 1;
}

static int ctrl_unknown_handler(FCppContext* ctx, FTKListNode* tklist, int* outputlines)
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
			return 0;
		}
	}
	return 1;
}

static int ctrl_define_handler(FCppContext* ctx, FTKListNode* tklist, int* outputlines)
{
	// TODO:
	return 1;
}
