/* \brief
 *		C compiler
 */

#include "cc.h"
#include "mm.h"
#include "hstring.h"
#include "charstream.h"
#include "logger.h"
#include "lexer/lexer.h"


void cc_contex_init(FCCContext* ctx)
{
	ctx->_outfilename = NULL;
	ctx->_outfp = NULL;
	ctx->_cs = NULL;
	ctx->_lookaheadtk._valid = 0;
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

	{
		FCCToken tk;
		do
		{
			if (!cc_read_token(ctx, ctx->_cs, &tk))
			{
				logger_output_s("cc_process failed when read token...\n");
				return FALSE;
			}

			cc_print_token(&tk);

		} while (tk._type != TK_EOF);
	}

	return TRUE;
}

BOOL cc_read_token(FCCContext* ctx, FCharStream* cs, FCCToken* tk)
{
	if (ctx->_lookaheadtk._valid) {
		ctx->_lookaheadtk._valid = 0;
		*tk = ctx->_lookaheadtk._tk;
		return TRUE;
	}

	return cc_lexer_read_token(cs, tk);
}

BOOL cc_lookahead_token(FCCContext* ctx, FCharStream* cs, FCCToken* tk)
{
	if (ctx->_lookaheadtk._valid) {
		*tk = ctx->_lookaheadtk._tk;
		return TRUE;
	}

	int result = cc_lexer_read_token(cs, &(ctx->_lookaheadtk._tk));
	ctx->_lookaheadtk._valid = 1;
	*tk = ctx->_lookaheadtk._tk;
	return result;
}

BOOL cc_read_tokentolist(FCCContext* ctx, FCharStream* cs, FTKListNode** tail)
{
	FTKListNode* tknode = NULL;

	tknode = mm_alloc_area(sizeof(FTKListNode), CC_MM_TEMPPOOL);
	if (!tknode) {
		logger_output_s("error: out of memory, at %s:%d\n", __FILE__, __LINE__);
		return FALSE;
	}

	tknode->_next = NULL;

	if (!cc_read_token(ctx, cs, &tknode->_tk))
	{
		return FALSE;
	}

	*tail = tknode;
	return TRUE;
}

BOOL cc_read_rowtokens(FCCContext* ctx, FCharStream* cs, FTKListNode** tail)
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

		if (!cc_read_token(ctx, cs, &tknode->_tk))
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
