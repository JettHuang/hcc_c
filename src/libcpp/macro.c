/* \brief
 *		macro add, remove, find and expanding
 *
 */

#include "cpp.h"
#include "logger.h"
#include "hstring.h"
#include "pplexer.h"
#include <string.h>


#define MACRO_HASHKEY(x)	(((unsigned int)(x)) >> 3)

void cpp_add_macro(FCppContext* ctx, const FMacro* m)
{
	FMacroListNode* mnode = mm_alloc_area(sizeof(FMacroListNode), CPP_MM_PERMPOOL);
	if (mnode)
	{
		unsigned int hs = MACRO_HASHKEY(m->_name) & (MACRO_TABLE_SIZE - 1);

		mnode->_macro = *m;
		mnode->_next = ctx->_macros_hash._hash[hs];
		ctx->_macros_hash._hash[hs] = mnode;
	}
}

void cpp_remove_macro(FCppContext* ctx, const char* name)
{
	FMacroListNode* current, **ptrprev; 
	unsigned int hs = MACRO_HASHKEY(name) & (MACRO_TABLE_SIZE - 1);

	ptrprev = &(ctx->_macros_hash._hash[hs]);
	current = ctx->_macros_hash._hash[hs];
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
	unsigned int hs = MACRO_HASHKEY(name) & (MACRO_TABLE_SIZE - 1);

	FMacroListNode* current = ctx->_macros_hash._hash[hs];
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
	array_init(&ctx->_hidden_sets, 64, sizeof(FArray), CPP_MM_TEMPPOOL);
}

static int macro_expand_alloc_hiddenset(FExpandContext* ctx, int dupfrom)
{
	FArray* newset = NULL;
	int setid = ctx->_hidden_sets._elecount;
	
	array_append_zeroed(&ctx->_hidden_sets);
	newset = (FArray*)array_element(&ctx->_hidden_sets, setid);
	array_init(newset, 128, sizeof(const char*), CPP_MM_TEMPPOOL);
	if (dupfrom >= 0)
	{
		FArray* srcset = NULL;

		assert(dupfrom < setid);
		srcset = (FArray*)(ctx->_hidden_sets._data) + dupfrom;
		array_make_cap_enough(newset, srcset->_elecount);
		array_copy(newset, srcset);
	}

	return setid;
}

static void macro_expand_add_hidden(FExpandContext* ctx, int setid, const char* name)
{
	FArray* dstset = NULL;
	
	assert(setid >= 0 && setid < ctx->_hidden_sets._elecount);
	dstset = (FArray*)array_element(&ctx->_hidden_sets, setid);
	array_append(dstset, &name);
}

static BOOL macro_epxand_is_hidden(FExpandContext* ctx, int setid, const char* name)
{
	FArray* dstset = NULL;
	int k;

	if (setid < 0)
	{
		return FALSE;
	}

	assert(setid < ctx->_hidden_sets._elecount);
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
	newtk->_tk._loc._filename = hs_hashstr("cpp built-in");
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

static BOOL macro_gather_args(FCppContext* ctx, const FMacro *m, FTKListNode ** tklist, int bscannextlines, FArray *args, int *argc)
{
	FTKListNode *argsstart, *argsitr, *argument, *argumenttail;
	int lparen_cnt;
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

	/* search for terminating ), possibility extending the row */
	assert((*tklist)->_next != NULL);
	argsstart = (*tklist)->_next;
	*tklist = argsstart;
	lparen_cnt = 1;
	while (lparen_cnt > 0)
	{
		if ((*tklist)->_tk._type == TK_EOF)
		{
			logger_output_s("error: gather macro arguments failed, at %s:%d\n", (*tklist)->_tk._loc._filename, (*tklist)->_tk._loc._line);
			return FALSE;
		}
		else if ((*tklist)->_tk._type == TK_LPAREN)
		{
			lparen_cnt++;
		}
		else if ((*tklist)->_tk._type == TK_RPAREN)
		{
			lparen_cnt--;
		}

		if (lparen_cnt > 0 && (*tklist)->_next == NULL)
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
					return FALSE;
				}

				if (!cpp_read_rowtokens(ctx, top->_cs, &((*tklist)->_next), FALSE, FALSE))
				{
					return FALSE;
				}
			}
			else
			{
				return FALSE;
			}
		}

		*tklist = (*tklist)->_next;
	} /* end while */

	/* now go back and gather arguments */
	argument = argumenttail = NULL;
	for (argsitr = argsstart; lparen_cnt >= 0; argsitr = argsitr->_next)
	{
		FTKListNode *duptk = NULL;

		if (argsitr->_tk._type == TK_LPAREN)
		{
			lparen_cnt++;
		}
		else if (argsitr->_tk._type == TK_RPAREN)
		{
			lparen_cnt--;
		}
		else if (argsitr->_tk._type == TK_NEWLINE)
		{
			continue;
		}

		
		if (((argsitr->_tk._type == TK_COMMA && lparen_cnt == 0) && (!m->_isvarg || (args->_elecount < (m->_argc-1))))
			|| (lparen_cnt == -1 && (args->_elecount != 0 || argument != NULL)))
		{
			array_append(args, &argument);
			argument = argumenttail = NULL;
			continue;
		}

		duptk = cpp_duplicate_token(argsitr, CPP_MM_TEMPPOOL);
		if (argument == NULL)
		{
			duptk->_tk._wscnt = 0;
			argument = duptk;
			argumenttail = argument;
		}
		else
		{
			argumenttail->_next = duptk;
			argumenttail = duptk;
		}

	} /* end for */
	
	*argc = args->_elecount;
	return TRUE;
}

/* find argument index if exist*/
static int find_argument_index(FCppContext*ctx, const FMacro* m, const FPPToken *tk)
{
	int index;
	FTKListNode* itr = m->_args;
	for (index = 0;itr;itr = itr->_next, index++)
	{
		if (tk->_str == ctx->_HS__VA_ARGS__ && itr->_tk._type == TK_ELLIPSIS)
		{
			return index;
		}

		if (itr->_tk._str == tk->_str) {
			return index;
		}
	}

	return -1;
}

static FTKListNode* stringfy_tokenlist(FTKListNode* tklist, enum EMMArea where)
{
	const char* hstr;
	char* szbuf;
	int k, bufsize;
	FTKListNode* tkitr;

	bufsize = 3; /* " " '\0' */
	for (k = 0, tkitr = tklist; tkitr; tkitr = tkitr->_next, k++)
	{
		const char* s = NULL;
		int isstr = (tkitr->_tk._type == TK_CONSTANT_STR) || (tkitr->_tk._type == TK_CONSTANT_CHAR);
		
		if (tkitr->_tk._type == TK_NONE)
		{
			continue;
		}

		if (k != 0 && tkitr->_tk._wscnt > 0)
		{
			bufsize++;
		}

		s = tkitr->_tk._str;
		for (; *s; s++)
		{
			if (isstr && (*s == '"' || *s == '\\'))
			{
				bufsize++; /* add a '\\' */
			}

			bufsize++;
		}
	} /* end for */

	szbuf = mm_alloc(bufsize);
	if (!szbuf)
	{
		logger_output_s("error: out of memory! %s:%d\n", __FILE__, __LINE__);
		return NULL;
	}

	szbuf[0] = '\"';
	for (k = 1, tkitr = tklist; tkitr; tkitr = tkitr->_next)
	{
		const char* s = NULL;
		int isstr = (tkitr->_tk._type == TK_CONSTANT_STR) || (tkitr->_tk._type == TK_CONSTANT_CHAR);

		if (tkitr->_tk._type == TK_NONE)
		{
			continue;
		}

		if (k != 1 && tkitr->_tk._wscnt > 0)
		{
			szbuf[k++] = ' ';
		}

		s = tkitr->_tk._str;
		for (; *s; s++)
		{
			if (isstr && (*s == '"' || *s == '\\'))
			{
				szbuf[k++] = '\\';
			}

			szbuf[k++] = *s;
		}
	} /* end for */

	szbuf[k++] = '\"';
	szbuf[k] = '\0';

	hstr = hs_hashstr(szbuf);
	mm_free(szbuf);

	/* alloc new token */
	FTKListNode* newtk = mm_alloc_area(sizeof(FTKListNode), where);
	if (!newtk) {
		logger_output_s("error: out of memory! %s:%d\n", __FILE__, __LINE__);
		return NULL;
	}

	newtk->_next = NULL;
	newtk->_tk._type = TK_CONSTANT_STR;
	newtk->_tk._loc._filename = hs_hashstr("cpp generated");
	newtk->_tk._loc._line = 0;
	newtk->_tk._loc._col = 0;
	newtk->_tk._hidesetid = -1;
	newtk->_tk._wscnt = 1;
	newtk->_tk._str = hstr;

	return newtk;
}

/* concat tokens */
static FTKListNode* concat_tokenlist(FTKListNode* llist, FTKListNode* rlist, enum EMMArea where)
{
	int bufsize;
	char* szbuf, *dst;
	const char* src;
	FTKListNode *result, *lhs, *rhs, **tail;

	if (llist == NULL || llist->_tk._type == TK_NONE) {
		return rlist;
	}
	if (rlist == NULL) {
		return llist;
	}

	result = llist;
	tail = NULL;
	for (lhs = llist; lhs; lhs = lhs->_next)
	{
		if (lhs->_next == NULL) {
			break;
		}
		tail = &lhs->_next;
	}
	if (tail == NULL) {
		result = NULL;
	}

	rhs = rlist;
	rlist = rlist->_next;

	bufsize = 1;
	bufsize += strlen(lhs->_tk._str);
	bufsize += strlen(rhs->_tk._str);

	szbuf = mm_alloc(bufsize);
	if (!szbuf)
	{
		logger_output_s("error: out of memory! %s:%d\n", __FILE__, __LINE__);
		return NULL;
	}
	
	dst = szbuf;
	for (src=lhs->_tk._str; *src; )
	{
		*dst++ = *src++;
	}
	for (src=rhs->_tk._str; *src; )
	{
		*dst++ = *src++;
	}
	*dst = '\0';

	/* re-parsing */
	{
		FCharStream* cs = cs_create_fromstring(szbuf);
		FTKListNode* tknode = NULL;

		while (1)
		{
			tknode = mm_alloc_area(sizeof(FTKListNode), where);
			if (!tknode) {
				logger_output_s("error: out of memory, at %s:%d\n", __FILE__, __LINE__);
				mm_free(szbuf);
				return NULL;
			}

			tknode->_next = NULL;
			cpp_lexer_read_token(cs, 0, &tknode->_tk);

			if (tknode->_tk._type == TK_EOF || tknode->_tk._type == TK_NEWLINE)
			{
				break;
			}

			if (result == NULL)
			{
				result = tknode;
				tail = &(tknode->_next);
			}
			else
			{
				*tail = tknode;
				tail = &(tknode->_next);
			}
		} /* end while (1) */

		cs_release(cs);
	}
	
	assert(tail);
	*tail = rlist;
	
	mm_free(szbuf);
	return result;
}

static BOOL macro_expand_inner(FExpandContext* expctx, FCppContext* ctx, FTKListNode** tklisthd, BOOL bscannextlines);

/* \brief
   this function handle the following:
   1. substitute actual arguments for #X  X##Y
   2. expand actual arguments, and substitute it
*/
static BOOL macro_expand_userdefined(FExpandContext* expctx, FCppContext* ctx, const FMacro* m, FArray* args, FTKListNode** replacement)
{
	FTKListNode* dupbody, *prevprev, *prev, *curr, *next;
	int argidx0, argidx1;

	dupbody = cpp_duplicate_tklist(m->_body, CPP_MM_TEMPPOOL);
	for (prevprev = prev = NULL, curr = dupbody; curr; prevprev = prev, prev = curr, curr = curr->_next)
	{
		if (curr->_tk._type == TK_POUND) /* # string operator */
		{
			next = curr->_next;
			if (next && (argidx0 = find_argument_index(ctx, m, &next->_tk)) >= 0)
			{
				FTKListNode* newtk = stringfy_tokenlist(*(FTKListNode**)array_element(args, argidx0), CPP_MM_TEMPPOOL);
				curr = newtk;
				curr->_next = next->_next;
				if (prev == NULL) {
					dupbody = curr;
				}
				else {
					prev->_next = curr;
				}
			}
		}
		else if (curr->_tk._type == TK_DOUBLE_POUND) /* ## concat operator */
		{
			FTKListNode* llist, * rlist, *newtklist, *savednext;

			next = curr->_next;
			if (prev == NULL || next == NULL)
			{
				logger_output_s("macro syntax error: ## at %s:%d\n", curr->_tk._str, curr->_tk._loc._filename, curr->_tk._loc._line);
				return FALSE;
			}

			argidx0 = find_argument_index(ctx, m, &prev->_tk);
			argidx1 = find_argument_index(ctx, m, &next->_tk);
			if (argidx0 < 0) {
				llist = cpp_duplicate_token(prev, CPP_MM_TEMPPOOL);
			}
			else {
				llist = cpp_duplicate_tklist(*(FTKListNode**)array_element(args, argidx0), CPP_MM_TEMPPOOL);
			}

			if (argidx1 < 0) {
				rlist = cpp_duplicate_token(next, CPP_MM_TEMPPOOL);
			}
			else {
				rlist = cpp_duplicate_tklist(*(FTKListNode**)array_element(args, argidx1), CPP_MM_TEMPPOOL);
			}

			newtklist = concat_tokenlist(llist, rlist, CPP_MM_TEMPPOOL);
			if (newtklist == NULL)
			{
				newtklist = mm_alloc_area(sizeof(FTKListNode), CPP_MM_TEMPPOOL);
				if (!newtklist) {
					logger_output_s("error: out of memory, at %s:%d\n", __FILE__, __LINE__);
					return FALSE;
				}

				newtklist->_next = NULL;
				newtklist->_tk._type = TK_NONE;
				newtklist->_tk._loc._filename = hs_hashstr("cpp generated");
				newtklist->_tk._loc._line = 0;
				newtklist->_tk._loc._col = 0;
				newtklist->_tk._hidesetid = -1;
				newtklist->_tk._wscnt = 1;
				newtklist->_tk._str = NULL;
			}
			newtklist->_tk._wscnt = prev->_tk._wscnt;

			curr = newtklist;
			savednext = next->_next;
			if (prevprev == NULL) {
				dupbody = curr;
			}
			else {
				prevprev->_next = curr;
			}
			prev = prevprev;
			prevprev = NULL;

			for (; curr->_next; prevprev = prev, prev = curr, curr = curr->_next)
			{
				/* do nothing */
			}
			curr->_next = savednext;
		}
		else if (curr->_tk._type == TK_ID)
		{
			next = curr->_next;
			if (!next || next->_tk._type != TK_DOUBLE_POUND)
			{
				FTKListNode* expandarg;

				expandarg = NULL;
				argidx0 = find_argument_index(ctx, m, &curr->_tk);
				if (argidx0 >= 0)
				{
					expandarg = cpp_duplicate_tklist(*(FTKListNode**)array_element(args, argidx0), CPP_MM_TEMPPOOL);
					if (!macro_expand_inner(expctx, ctx, &expandarg, FALSE))
					{
						logger_output_s("error: expand macro argument failed, at %s:%d\n", curr->_tk._str, curr->_tk._loc._filename, curr->_tk._loc._line);
						return FALSE;
					}

					if (expandarg)
					{
						expandarg->_tk._wscnt += curr->_tk._wscnt;
					}
					else
					{
						expandarg = mm_alloc_area(sizeof(FTKListNode), CPP_MM_TEMPPOOL);
						if (!expandarg) {
							logger_output_s("error: out of memory, at %s:%d\n", __FILE__, __LINE__);
							return FALSE;
						}

						expandarg->_next = NULL;
						expandarg->_tk._type = TK_NONE;
						expandarg->_tk._loc._filename = hs_hashstr("cpp generated");
						expandarg->_tk._loc._line = 0;
						expandarg->_tk._loc._col = 0;
						expandarg->_tk._hidesetid = -1;
						expandarg->_tk._wscnt = 1;
						expandarg->_tk._str = NULL;
					}

					curr = expandarg;
					if (prev == NULL) {
						dupbody = curr;
					}
					else {
						prev->_next = curr;
					}

					for (; curr->_next; prevprev = prev, prev = curr, curr = curr->_next)
					{
						/* do nothing */
					}
					curr->_next = next;
				} /* end if */
			} /* end if */
		}

	} /* end for */

	/* remove none-head TK_NONE */
	for (prev = NULL, curr = dupbody; curr;)
	{
		if (curr->_tk._type == TK_NONE && curr != dupbody)
		{
			prev->_next = curr->_next;
			curr = curr->_next;
		}
		else
		{
			prev = curr; curr = curr->_next;
		}
	}

	*replacement = dupbody;
	return TRUE;
}

static BOOL macro_expand_inner(FExpandContext* expctx, FCppContext* ctx, FTKListNode** tklisthd, BOOL bscannextlines)
{
	FTKListNode** where = tklisthd;
	FTKListNode* tkitr = *tklisthd;

	while (tkitr)
	{
		FTKListNode* replacement = NULL, *replacementtail = NULL, *tkmacro = NULL;
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
				logger_output_s("error: expand built-in '%s' failed, at %s:%d\n", tkitr->_tk._str, tkitr->_tk._loc._filename, tkitr->_tk._loc._line);
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
				if (!macro_gather_args(ctx, m, &tkitr, bscannextlines, &args, &argc))
				{
					return FALSE;
				}

				if (argc < 0) /* not actually a call (no '()'), so don't expanding */
				{
					if (tkitr)
					{
						where = &tkitr->_next;
						tkitr = tkitr->_next;
					}
					continue;
				}
				else if (argc != m->_argc && (!m->_isvarg || argc < (m->_argc-1)))
				{
					logger_output_s("error: expand macro '%s' failed, disagreement arguments count at %s:%d\n", tkmacro->_tk._str, tkmacro->_tk._loc._filename, tkmacro->_tk._loc._line);
					return FALSE;
				}
			}
			
			if (!macro_expand_userdefined(expctx, ctx, m, &args, &replacement))
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
				/* only check macro id to alloc hidden-set */
				if (repitr->_tk._type == TK_ID && repitr->_tk._hidesetid == -1 && cpp_find_macro(ctx, repitr->_tk._str))
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
			replacementtail = replacement;

			for (; replacementtail->_next; replacementtail = replacementtail->_next)
			{
				/* do nothing */
			}
			replacementtail->_next = tkitr;
			tkitr = replacement; /* re-scan */
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
