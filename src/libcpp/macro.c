/* \brief
 *		macro add, remove, find and expanding
 *
 */

#include "cpp.h"
#include "logger.h"
#include "hstring.h"


void cpp_add_macro(FCppContext* ctx, const FMacro* m)
{
	FMacroListNode* mnode = mm_alloc_area(sizeof(FMacroListNode), CPP_MM_PERMPOOL);
	if (mnode)
	{
		mnode->_macro = *m;
		mnode->_next = ctx->_macrolist;
		ctx->_macrolist = mnode;
	}
}

void cpp_remove_macro(FCppContext* ctx, const char* name)
{
	FMacroListNode* current, **ptrprev; 
	
	ptrprev = &ctx->_macrolist;
	current = ctx->_macrolist;
	for (; current; current = current->_next)
	{
		if (current->_macro._name == name)
		{
			*ptrprev = current->_next;
			break;
		}

		ptrprev = &current->_next;
	} /* end for */
}

const FMacro* cpp_find_macro(FCppContext* ctx, const char* name)
{
	FMacroListNode* current = ctx->_macrolist;
	for (; current; current = current->_next)
	{
		if (current->_macro._name == name)
		{
			if (current->_macro._flags & MACRO_FLAG_HIDDEN)
			{
				return NULL;
			}

			return &current->_macro;
		}
	} /* end for */

	return NULL;
}

/* --------------------------------------------------------------------
 * expand macro
 *
 * -------------------------------------------------------------------*/

typedef struct tagExpandContext
{
	FArray _hidden_sets;
} FExpandContext;

static void macro_expand_init(FExpandContext* ctx)
{
	array_init(&ctx->_hidden_sets, 128, sizeof(FArray), CPP_MM_TEMPPOOL);
}

static int macro_expand_alloc_hiddenset(FExpandContext* ctx, int dupfrom)
{
	FArray* newset = NULL;
	int setid = ctx->_hidden_sets._elecount;
	
	array_enlarge(&ctx->_hidden_sets, setid+1);
	newset = (FArray*)(ctx->_hidden_sets._data) + setid;
	array_init(newset, 128, sizeof(const char*), CPP_MM_TEMPPOOL);
	if (dupfrom >= 0)
	{
		FArray* srcset = NULL;

		assert(dupfrom < setid);
		srcset = (FArray*)(ctx->_hidden_sets._data) + dupfrom;
		array_enlarge(newset, srcset->_elecount);
		array_copy(newset, srcset);
	}

	return setid;
}

static void macro_expand_add_hidden(FExpandContext* ctx, int setid, const char* name)
{
	FArray* dstset = NULL;
	
	assert(setid >= 0 && setid < ctx->_hidden_sets._elecount);
	dstset = (FArray*)(ctx->_hidden_sets._data) + setid;
	array_enlarge(dstset, dstset->_elecount + 1);
	*((const char**)(dstset->_data) + dstset->_elecount) = name;
}

static BOOL macro_epxand_is_hidden(FExpandContext* ctx, int setid, const char* name)
{
	FArray* dstset = NULL;
	int k;

	assert(setid >= 0 && setid < ctx->_hidden_sets._elecount);
	dstset = (FArray*)(ctx->_hidden_sets._data) + setid;
	for (k = 0; k < dstset->_elecount; k++)
	{
		if (*((const char**)(dstset->_data) + k) == name)
		{
			return TRUE;
		}
	} /* end for */

	return FALSE;
}

static BOOL macro_expand_builtin(FCppContext* ctx, FTKListNode* curtk, FTKListNode** replacement)
{
	char szbuf[512];

	FTKListNode* newtk = mm_alloc_area(sizeof(FTKListNode), CPP_MM_TEMPPOOL);
	if (!newtk)
	{
		logger_output_s("error: out of memory! %s:%d\n", __FILE__, __LINE__);
		return FALSE;
	}

	newtk->_next = NULL;
	newtk->_tk._type = TK_UNCLASS;
	newtk->_tk._loc._filename = hs_hashstr("cpp builtin");
	newtk->_tk._loc._line = 0;
	newtk->_tk._loc._col = 0;
	newtk->_tk._hidesetid = -1;
	newtk->_tk._wscnt = 1;
	newtk->_tk._str = NULL;

	if (curtk->_tk._str == ctx->_HS__DATE__)
	{
		newtk->_tk._str = ctx->_strdate;
		newtk->_tk._type = TK_CONSTANT_STR;
	}
	else if (curtk->_tk._str == ctx->_HS__TIME__)
	{
		newtk->_tk._str = ctx->_strtime;
		newtk->_tk._type = TK_CONSTANT_STR;
	}
	else if (curtk->_tk._str == ctx->_HS__FILE__)
	{
		sprintf(szbuf, "\"%s\"", curtk->_tk._loc._filename);
		newtk->_tk._str = hs_hashstr(szbuf);
		newtk->_tk._type = TK_CONSTANT_STR;
	}
	else if (curtk->_tk._str == ctx->_HS__LINE__)
	{
		sprintf(szbuf, "%d", curtk->_tk._loc._line);
		newtk->_tk._str = hs_hashstr(szbuf);
		newtk->_tk._type = TK_PPNUMBER;
	}
	else if (curtk->_tk._str == ctx->_HS__STDC__)
	{
		newtk->_tk._str = hs_hashstr("1");
		newtk->_tk._type = TK_PPNUMBER;
	}
	else if (curtk->_tk._str == ctx->_HS__STDC_HOSTED__)
	{
		newtk->_tk._str = hs_hashstr("0");
		newtk->_tk._type = TK_PPNUMBER;
	}
	else if (curtk->_tk._str == ctx->_HS__STDC_VERSION__)
	{
		newtk->_tk._str = hs_hashstr("199901L");
		newtk->_tk._type = TK_PPNUMBER;
	}
	else
	{
		return FALSE;
	}

	*replacement = newtk;
	return TRUE;
}

static BOOL macro_gather_args(FCppContext* ctx, FTKListNode ** tklist, int bscannextlines, FArray *args, int *argc)
{
	FSourceCodeContext* top = ctx->_sourcestack;
	*argc = -1;

	/* look for '(' */
	for (;;)
	{
		if (*tklist == NULL)
		{
			if (bscannextlines)
			{
				FPPToken aheadtk;

				if (!cpp_lookahead_token(ctx, top->_cs, &aheadtk, FALSE, FALSE))
				{
					return FALSE;
				}
				if (aheadtk._type == TK_POUND) /* the next line is # control line */
				{
					return TRUE;
				}

				if (!cpp_read_rowtokens(ctx, top->_cs, tklist, FALSE, FALSE))
				{
					return FALSE;
				}
			}
			else
			{
				return TRUE;
			}
		}

		if ((*tklist)->_tk._type == TK_LPAREN)
		{
			break;
		}
		else if ((*tklist)->_tk._type != TK_NEWLINE)
		{
			return TRUE;
		}

		(*tklist) = (*tklist)->_next;
	} /* end for ;; */

	// TODO:

	return FALSE;
}

static BOOL macro_expand_userdefined(FExpandContext* expctx, const FMacro* m, FArray* args, FTKListNode** replacement)
{
	return FALSE;
}

static BOOL macro_expand_inner(FExpandContext* expctx, FCppContext* ctx, FTKListNode** tklist, int bscannextlines)
{
	FTKListNode** where = tklist;
	FTKListNode* tkitr = *tklist;

	while (tkitr)
	{
		FTKListNode* replacement = NULL, *tkmacro = NULL;
		const FMacro* m = NULL;

		if (tkitr->_tk._type != TK_ID) {
			where = &tkitr->_next;
			tkitr = tkitr->_next;
			continue;
		}

		if (tkitr->_tk._str != ctx->_HS__DEFINED__)
		{
			m = cpp_find_macro(ctx, tkitr->_tk._str);
			if (!m || macro_epxand_is_hidden(expctx, tkitr->_tk._hidesetid, tkitr->_tk._str))
			{
				where = &tkitr->_next;
				tkitr = tkitr->_next;
				continue;
			}
		}

		tkmacro = tkitr;
		/* special for defined   defined(ID) */
		if (tkitr->_tk._str == ctx->_HS__DEFINED__)
		{
			FTKListNode* defined = tkitr;
			FTKListNode* defined_arg = NULL;

			tkitr = tkitr->_next;
			if (tkitr && tkitr->_tk._type == TK_ID)
			{
				defined_arg = tkitr;
				tkitr = tkitr->_next;
			}
			else if (tkitr && tkitr->_tk._type == TK_LPAREN)
			{
				tkitr = tkitr->_next;
				if (tkitr && tkitr->_tk._type == TK_ID)
				{
					defined_arg = tkitr;
					tkitr = tkitr->_next;
					if (tkitr && tkitr->_tk._type == TK_RPAREN)
					{
						tkitr = tkitr->_next;
					}
					else
					{
						logger_output_s("syntax error: defined(ID) require ')', at %s:%d\n", defined->_tk._loc._filename, defined->_tk._loc._line);
						return FALSE;
					}
				}
			}

			if (!defined_arg)
			{
				logger_output_s("syntax error: defined(ID) require 'ID', at %s:%d\n", defined->_tk._loc._filename, defined->_tk._loc._line);
				return FALSE;
			}

			replacement = mm_alloc_area(sizeof(FTKListNode), CPP_MM_TEMPPOOL);
			if (!replacement)
			{
				logger_output_s("error: out of memory! %s:%d\n", __FILE__, __LINE__);
				return FALSE;
			}

			replacement->_next = NULL;
			replacement->_tk._type = TK_PPNUMBER;
			replacement->_tk._loc._filename = hs_hashstr("cpp generated");
			replacement->_tk._loc._line = 0;
			replacement->_tk._loc._col = 0;
			replacement->_tk._hidesetid = -1;
			replacement->_tk._wscnt = 1;
			replacement->_tk._str = cpp_find_macro(ctx, defined_arg->_tk._str) ? hs_hashstr("1") : hs_hashstr("0");
		}
		else if (m->_flags & MACRO_FLAG_BUILTIN)
		{
			if (!macro_expand_builtin(ctx, tkitr, &replacement))
			{
				logger_output_s("error: expand builtin '%s' failed, at %s:%d\n", tkitr->_tk._str, tkitr->_tk._loc._filename, tkitr->_tk._loc._line);
				return FALSE;
			}

			tkitr = tkitr->_next;
		}
		else
		{
			/* expanding normal macro */
			FTKListNode *repitr;
			FArray args;

			array_init(&args, 32, sizeof(FTKListNode*), CPP_MM_TEMPPOOL);
			tkitr = tkitr->_next;
			if (m->_argc != -1) /* function like macro */
			{
				/* gather parameters */
				int argc = -1;
				if (!macro_gather_args(ctx, &tkitr, bscannextlines, &args, &argc))
				{
					return FALSE;
				}

				if (argc < 0) /* not actually a call (no '()'), so don't expanding */
				{
					where = &tkitr->_next;
					tkitr = tkitr->_next;
					continue;
				}
				else if (argc != m->_argc)
				{
					logger_output_s("error: expand macro '%s' failed, disagreement arguments count at %s:%d\n", tkmacro->_tk._str, tkmacro->_tk._loc._filename, tkmacro->_tk._loc._line);
					return FALSE;
				}
			}
			
			if (!macro_expand_userdefined(expctx, m, &args, &replacement))
			{
				return FALSE;
			}

			/* distribute hidden-set */
			if (tkmacro->_tk._hidesetid == -1)
			{
				tkmacro->_tk._hidesetid = macro_expand_alloc_hiddenset(expctx, -1);
			}
			macro_expand_add_hidden(expctx, tkmacro->_tk._hidesetid, tkmacro->_tk._str);
			
			for (repitr = replacement; repitr; repitr = repitr->_next)
			{
				if (repitr->_tk._hidesetid == -1)
				{
					repitr->_tk._hidesetid = macro_expand_alloc_hiddenset(expctx, tkmacro->_tk._hidesetid);
				}
			}
		} /* end normal macro */

		/* do replacement */
		if (replacement)
		{
			replacement->_tk._wscnt = tkmacro->_tk._wscnt;
			*where = replacement;

			for (; replacement->_next; replacement = replacement->_next)
			{
				/* do nothing */
			}
			where = &replacement->_next;
			replacement->_next = tkitr;
		}
		else
		{
			*where = tkitr;
		}
	} /* end while tkitr */

	return TRUE;
}

BOOL cpp_expand_rowtokens(FCppContext* ctx, FTKListNode** tklist, int bscannextlines)
{
	FExpandContext expctx;
	macro_expand_init(&expctx);

	return macro_expand_inner(&expctx, ctx, tklist, bscannextlines);
}
