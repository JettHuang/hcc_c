/* \brief
 *		C compiler
 */

#include "cc.h"
#include "mm.h"
#include "hstring.h"
#include "charstream.h"
#include "logger.h"
#include "lexer/lexer.h"
#include "parser/types.h"
#include "parser/symbols.h"
#include "parser/parser.h"

#include <string.h>


extern void cc_logger_outc(int c);
extern void cc_logger_outs(const char* format, va_list arg);

/* token list node */
typedef struct tagTokenListNode {
	FCCToken	_tk;
	struct tagTokenListNode* _next;
} FTKListNode;

static BOOL cc_read_token_with_handlectrl(FCCContext* ctx, FCCToken* tk);
static BOOL cc_read_rowtokens(FCharStream* cs, FTKListNode** tail);

static FCCTypeMetrics defaultmetrics = 
{
	{ 1, 1 }, /* _charmetric */
	{ 2, 2 }, /* _wcharmetric */
	{ 2, 2 }, /* _shortmetric */
	{ 4, 4 }, /* _intmetric */
	{ 4, 4 }, /* _longmetric */
	{ 8, 4 }, /* _longlongmetric */
	{ 4, 4 }, /* _floatmetric */
	{ 8, 4 }, /* _doublemetric */
	{ 8, 4 }, /* _longdoublemetric */
	{ 4, 4 }, /* _ptrmetric */
	{ 0, 4 }, /* _structmetric */
};

void cc_init()
{
	logger_set(cc_logger_outc, cc_logger_outs);

	cc_lexer_init();
	cc_symbol_init();
	cc_type_init(&defaultmetrics);
}

void cc_uninit()
{
	cc_lexer_uninit();
}

void cc_contex_init(FCCContext* ctx)
{
	ctx->_outfilename = NULL;
	ctx->_outfp = NULL;
	ctx->_cs = NULL;
	ctx->_lookaheadtk._valid = 0;
	ctx->_bnewline = 1;
}

void cc_contex_release(FCCContext* ctx)
{
	if (ctx->_outfp) {
		fclose(ctx->_outfp);
	}
	if (ctx->_cs) {
		cs_release(ctx->_cs);
	}

	ctx->_outfilename = NULL;
	ctx->_outfp = NULL;
	ctx->_cs = NULL;
	ctx->_lookaheadtk._valid = 0;
	ctx->_bnewline = 0;
}

BOOL cc_process(FCCContext* ctx, const char* srcfilename, const char* outfilename)
{
	FCharStream* cs = NULL;
	const char* absfilename = util_convert_abs_pathname(srcfilename);

	cs = cs_create_fromfile(absfilename);
	if (!cs) { return FALSE; }
	ctx->_cs = cs;

	if (outfilename) {
		ctx->_outfp = fopen(outfilename, "wt");
	}
	if (!ctx->_outfp) {
		logger_output_s("failed to open file %s\n", outfilename);
		return FALSE;
	}
	
	return cc_parser_program(ctx);
}

BOOL cc_read_token(FCCContext* ctx, FCCToken* tk)
{
	if (ctx->_lookaheadtk._valid) {
		ctx->_lookaheadtk._valid = 0;
		*tk = ctx->_lookaheadtk._tk;
		return TRUE;
	}

	return cc_read_token_with_handlectrl(ctx, tk);
}

BOOL cc_lookahead_token(FCCContext* ctx, FCCToken* tk)
{
	int result;

	if (ctx->_lookaheadtk._valid) {
		*tk = ctx->_lookaheadtk._tk;
		return TRUE;
	}

	result = cc_read_token_with_handlectrl(ctx, &(ctx->_lookaheadtk._tk));
	ctx->_lookaheadtk._valid = 1;
	*tk = ctx->_lookaheadtk._tk;
	return result;
}

static BOOL cc_read_token_with_handlectrl1(FCCContext* ctx, FCCToken* tk, BOOL *bfoundctrl)
{
	*bfoundctrl = FALSE;

	if (!cc_lexer_read_token(ctx->_cs, tk))
	{
		return FALSE;
	}

	if (ctx->_bnewline && tk->_type == TK_POUND) /* '#' */
	{
		FTKListNode* tklist = NULL;
		
		*bfoundctrl = TRUE;
		if (!cc_read_rowtokens(ctx->_cs, &tklist))
		{
			return FALSE;
		}

		if (tklist->_tk._type == TK_ID && !strcmp(tklist->_tk._val._astr, "line"))
		{
			const char* filename = NULL;
			int linenum = 0;
			FTKListNode* expr = tklist->_next;

			/* #line number filename */
			if (expr->_tk._type == TK_CONSTANT_INT && CHECK_IS_EOFLINE(expr->_next->_tk._type))
			{
				linenum = expr->_tk._val._int;
			}
			else if (expr->_tk._type == TK_CONSTANT_INT && expr->_next->_tk._type == TK_CONSTANT_STR && CHECK_IS_EOFLINE(expr->_next->_next->_tk._type))
			{
				const char* cnststr = expr->_next->_tk._val._astr;
				linenum = expr->_tk._val._int;
				filename = hs_hashnstr2(cnststr + 1, strlen(cnststr) - 2); /* omit 2 " */
				filename = util_normalize_pathname(filename);
			}
			else
			{
				logger_output_s("error syntax: #line number filename(opt), at %s:%d\n", tklist->_tk._loc._filename, tklist->_tk._loc._line);
				return TRUE;
			}

			ctx->_cs->_line = linenum;
			ctx->_cs->_srcfilename = filename;
		}
	}

	return TRUE;
}

static BOOL cc_read_token_with_handlectrl(FCCContext* ctx, FCCToken* tk)
{
	BOOL bfoundctrl;

	do
	{
		if (!cc_read_token_with_handlectrl1(ctx, tk, &bfoundctrl))
		{
			logger_output_s("read token failed.\n");
			return FALSE;
		}

		ctx->_bnewline = bfoundctrl || (tk->_type == TK_NEWLINE);
	} while (ctx->_bnewline);

	return TRUE;
}

static BOOL cc_read_rowtokens(FCharStream* cs, FTKListNode** tail)
{
	enum EPPToken tktype = TK_UNCLASS;
	FTKListNode* tknode = NULL;
	
	do
	{
		tknode = mm_alloc_area(sizeof(FTKListNode), CC_MM_TEMPPOOL);
		if (!tknode) {
			logger_output_s("error: out of memory, at %s:%d\n", __FILE__, __LINE__);
			return FALSE;
		}

		tknode->_next = NULL;

		if (!cc_lexer_read_token(cs, &tknode->_tk))
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

void cc_print_token(FCCToken* tk)
{
	switch (tk->_type)
	{
	case TK_ID:
		logger_output_s("Identifier: %s\n", tk->_val._astr);
		break;
	case TK_CONSTANT_STR:
		logger_output_s("constant string: %s\n", tk->_val._astr);
		break;
	case TK_CONSTANT_WSTR:
		logger_output_s("constant wide string: %ls\n", tk->_val._wstr._str);
		break;
	case TK_CONSTANT_CHAR:
		logger_output_s("constant char: %c\n", tk->_val._ch);
		break;
	case TK_CONSTANT_WCHAR:
		logger_output_s("constant wide char:%d\n", tk->_val._wch);
		break;
	case TK_CONSTANT_INT:
		logger_output_s("constant int:%d\n", tk->_val._int);
		break;
	case TK_CONSTANT_UINT:
		logger_output_s("constant uint:%u\n", tk->_val._uint);
		break;
	case TK_CONSTANT_LONG:
		logger_output_s("constant long:%ld\n", tk->_val._long);
		break;
	case TK_CONSTANT_ULONG:
		logger_output_s("constant ulong:%lu\n", tk->_val._ulong);
		break;
	case TK_CONSTANT_LLONG:
		logger_output_s("constant llong:%lld\n", tk->_val._llong);
		break;
	case TK_CONSTANT_ULLONG:
		logger_output_s("constant ullong:%llu\n", tk->_val._ullong);
		break;
	case TK_CONSTANT_FLOAT:
		logger_output_s("constant float:%f\n", tk->_val._float);
		break;
	case TK_CONSTANT_DOUBLE:
		logger_output_s("constant double:%lf\n", tk->_val._double);
		break;
	case TK_CONSTANT_LDOUBLE:
		logger_output_s("constant ldouble:%Lf\n", tk->_val._ldouble);
		break;
	default:
		logger_output_s("%s\n", gCCTokenMetas[tk->_type]._text);
		break;
	}
}
