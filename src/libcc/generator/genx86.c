/* \brief 
 *		C compiler x86 back-end
 */

#include "gen.h"
#include "mm.h"
#include "cc.h"
#include "hstring.h"
#include "parser/symbols.h"
#include "parser/types.h"
#include "parser/expr.h"

#include <stdio.h>


static void program_begin(struct tagCCContext* ctx);
static void program_end(struct tagCCContext* ctx);

static void block_begin(struct tagCCContext* ctx);
static void block_end(struct tagCCContext* ctx);

static void switchsegment(struct tagCCContext* ctx, enum tagCCSegment seg);
static void importsymbol(struct tagCCContext* ctx, struct tagCCSymbol* sym);
static void exportsymbol(struct tagCCContext* ctx, struct tagCCSymbol* sym);
static void defglobal_begin(struct tagCCContext* ctx, struct tagCCSymbol* sym, enum tagCCSegment seg);
static void defconst_space(struct tagCCContext* ctx, int count);
static void defconst_ubyte(struct tagCCContext* ctx, uint8_t val, int count);
static void defconst_sbyte(struct tagCCContext* ctx, int8_t val, int count);
static void defconst_uword(struct tagCCContext* ctx, uint16_t val, int count);
static void defconst_sword(struct tagCCContext* ctx, int16_t val, int count);
static void defconst_udword(struct tagCCContext* ctx, uint32_t val, int count);
static void defconst_sdword(struct tagCCContext* ctx, int32_t val, int count);
static void defconst_uqword(struct tagCCContext* ctx, uint64_t val, int count);
static void defconst_sqword(struct tagCCContext* ctx, int64_t val, int count);
static void defconst_real4(struct tagCCContext* ctx, float val, int count);
static void defconst_real8(struct tagCCContext* ctx, double val, int count);
static void defconst_string(struct tagCCContext* ctx, const void* str, int count, int charsize);
static void defconst_address(struct tagCCContext* ctx, struct tagCCExprTree* expr);
static void defglobal_end(struct tagCCContext* ctx, struct tagCCSymbol* sym);


struct tagCCBackend* cc_new_backend()
{
	struct tagCCBackend* backend;

	if (!(backend = mm_alloc_area(sizeof(struct tagCCBackend), CC_MM_PERMPOOL)))
	{
		return NULL;
	}

	backend->_is_little_ending = 1;
	backend->_curr_seg = SEG_NONE;

	backend->_program_begin = &program_begin;
	backend->_program_end = &program_end;
	backend->_block_begin = &block_begin;
	backend->_block_end = &block_end;
	backend->_importsymbol = &importsymbol;
	backend->_exportsymbol = &exportsymbol;
	backend->_defglobal_begin = &defglobal_begin;
	backend->_defconst_space = &defconst_space;
	backend->_defconst_ubyte = &defconst_ubyte;
	backend->_defconst_sbyte = &defconst_sbyte;
	backend->_defconst_uword = &defconst_uword;
	backend->_defconst_sword = &defconst_sword;
	backend->_defconst_udword = &defconst_udword;
	backend->_defconst_sdword = &defconst_sdword;
	backend->_defconst_uqword = &defconst_uqword;
	backend->_defconst_sqword = &defconst_sqword;
	backend->_defconst_real4 = &defconst_real4;
	backend->_defconst_real8 = &defconst_real8;
	backend->_defconst_string = &defconst_string;
	backend->_defconst_address = &defconst_address;
	backend->_defglobal_end = &defglobal_end;

	return backend;
}

static void program_begin(struct tagCCContext* ctx)
{
	fprintf(ctx->_outfp, ";-----------------------------------------\n");
	fprintf(ctx->_outfp, "; %s\n", ctx->_srcfilename);
	fprintf(ctx->_outfp, ";-----------------------------------------\n");
	fprintf(ctx->_outfp, ".486\n");
	fprintf(ctx->_outfp, ".model flat\n");
	fprintf(ctx->_outfp, "option casemap:none\n");
}

static void program_end(struct tagCCContext* ctx)
{
	fprintf(ctx->_outfp, "end");
}

static void block_begin(struct tagCCContext* ctx)
{

}

static void block_end(struct tagCCContext* ctx)
{

}


static void importsymbol(struct tagCCContext* ctx, struct tagCCSymbol* sym)
{
	fprintf(ctx->_outfp, "extern %s:near\n", sym->_x._name);
}

static void exportsymbol(struct tagCCContext* ctx, struct tagCCSymbol* sym)
{
	fprintf(ctx->_outfp, "public %s\n", sym->_x._name);
}

static void switchsegment(struct tagCCContext* ctx, enum tagCCSegment seg)
{
	if (ctx->_backend->_curr_seg != seg)
	{
		ctx->_backend->_curr_seg = seg;
		switch (seg)
		{
		case SEG_NONE:
			break;
		case SEG_BSS:
			fprintf(ctx->_outfp, "\n.data?\n"); break;
		case SEG_DATA:
			fprintf(ctx->_outfp, "\n.data\n"); break;
		case SEG_CONST:
			fprintf(ctx->_outfp, "\n.const\n"); break;
		case SEG_CODE:
			fprintf(ctx->_outfp, "\n.code\n"); break;
		default:
			break;
		}
	}
}

static void defglobal_begin(struct tagCCContext* ctx, struct tagCCSymbol* sym, enum tagCCSegment seg)
{
	switchsegment(ctx, seg);

	fprintf(ctx->_outfp, "align %d\n", sym->_type->_align);
	fprintf(ctx->_outfp, "%s  ", sym->_x._name);
}

static void defconst_space(struct tagCCContext* ctx, int count)
{
	char c;

	c = (ctx->_backend->_curr_seg != SEG_BSS) ? '0' : '?';
	if (count > 1) {
		fprintf(ctx->_outfp, "    db  %d  dup (%c)\n", count, c);
	}
	else {
		fprintf(ctx->_outfp, "    db  %c\n", c);
	}
}

static void defconst_ubyte(struct tagCCContext* ctx, uint8_t val, int count)
{
	if (ctx->_backend->_curr_seg != SEG_BSS)
	{
		if (count > 1) {
			fprintf(ctx->_outfp, "    db  %d  dup (%d)\n", count, (uint32_t)val);
		}
		else {
			fprintf(ctx->_outfp, "    db  %d\n", (uint32_t)val);
		}
	}
	else 
	{
		if (count > 1) {
			fprintf(ctx->_outfp, "    db  %d  dup (?)\n", count);
		}
		else {
			fprintf(ctx->_outfp, "    db  ?\n");
		}
	}
}

static void defconst_sbyte(struct tagCCContext* ctx, int8_t val, int count)
{
	if (ctx->_backend->_curr_seg != SEG_BSS)
	{
		if (count > 1) {
			fprintf(ctx->_outfp, "    sbyte  %d  dup (%d)\n", count, (int32_t)val);
		}
		else {
			fprintf(ctx->_outfp, "    sbyte  %d\n", (int32_t)val);
		}
	}
	else
	{
		if (count > 1) {
			fprintf(ctx->_outfp, "    sbyte  %d  dup (?)\n", count);
		}
		else {
			fprintf(ctx->_outfp, "    sbyte  ?\n");
		}
	}
}

static void defconst_uword(struct tagCCContext* ctx, uint16_t val, int count)
{
	if (ctx->_backend->_curr_seg != SEG_BSS)
	{
		if (count > 1) {
			fprintf(ctx->_outfp, "    dw  %d  dup (%d)\n", count, (int32_t)val);
		}
		else {
			fprintf(ctx->_outfp, "    dw  %d\n", (int32_t)val);
		}
	}
	else
	{
		if (count > 1) {
			fprintf(ctx->_outfp, "    dw  %d  dup (?)\n", count);
		}
		else {
			fprintf(ctx->_outfp, "    dw  ?\n");
		}
	}
}

static void defconst_sword(struct tagCCContext* ctx, int16_t val, int count)
{
	if (ctx->_backend->_curr_seg != SEG_BSS)
	{
		if (count > 1) {
			fprintf(ctx->_outfp, "    sword  %d  dup (%d)\n", count, (int32_t)val);
		}
		else {
			fprintf(ctx->_outfp, "    sword  %d\n", (int32_t)val);
		}
	}
	else
	{
		if (count > 1) {
			fprintf(ctx->_outfp, "    sword  %d  dup (?)\n", count);
		}
		else {
			fprintf(ctx->_outfp, "    sword  ?\n");
		}
	}
}

static void defconst_udword(struct tagCCContext* ctx, uint32_t val, int count)
{
	if (ctx->_backend->_curr_seg != SEG_BSS)
	{
		if (count > 1) {
			fprintf(ctx->_outfp, "    dd  %d  dup (%u)\n", count, val);
		}
		else {
			fprintf(ctx->_outfp, "    dd  %u\n", val);
		}
	}
	else
	{
		if (count > 1) {
			fprintf(ctx->_outfp, "    dd  %d  dup (?)\n", count);
		}
		else {
			fprintf(ctx->_outfp, "    dd  ?\n");
		}
	}
}

static void defconst_sdword(struct tagCCContext* ctx, int32_t val, int count)
{
	if (ctx->_backend->_curr_seg != SEG_BSS)
	{
		if (count > 1) {
			fprintf(ctx->_outfp, "    sdword  %d  dup (%d)\n", count, (int32_t)val);
		}
		else {
			fprintf(ctx->_outfp, "    sdword  %d\n", (int32_t)val);
		}
	}
	else
	{
		if (count > 1) {
			fprintf(ctx->_outfp, "    sdword  %d  dup (?)\n", count);
		}
		else {
			fprintf(ctx->_outfp, "    sdword  ?\n");
		}
	}
}

static void defconst_uqword(struct tagCCContext* ctx, uint64_t val, int count)
{
	if (ctx->_backend->_curr_seg != SEG_BSS)
	{
		if (count > 1) {
			fprintf(ctx->_outfp, "    dq  %d  dup (0%llxh)\n", count, val);
		}
		else {
			fprintf(ctx->_outfp, "    dq  0%llxh\n", val);
		}
	}
	else
	{
		if (count > 1) {
			fprintf(ctx->_outfp, "    dq  %d  dup (?)\n", count);
		}
		else {
			fprintf(ctx->_outfp, "    dq  ?\n");
		}
	}
}

static void defconst_sqword(struct tagCCContext* ctx, int64_t val, int count)
{
	if (ctx->_backend->_curr_seg != SEG_BSS)
	{
		if (count > 1) {
			fprintf(ctx->_outfp, "    dq  %d  dup (0%llxh)\n", count, val);
		}
		else {
			fprintf(ctx->_outfp, "    dq  0%llxh\n", val);
		}
	}
	else
	{
		if (count > 1) {
			fprintf(ctx->_outfp, "    dq  %d  dup (?)\n", count);
		}
		else {
			fprintf(ctx->_outfp, "    dq  ?\n");
		}
	}
}

static void defconst_real4(struct tagCCContext* ctx, float val, int count)
{
	if (ctx->_backend->_curr_seg != SEG_BSS)
	{
		if (count > 1) {
			fprintf(ctx->_outfp, "    real4  %d  dup (%f)\n", count, val);
		}
		else {
			fprintf(ctx->_outfp, "    real4  %f\n", val);
		}
	}
	else
	{
		if (count > 1) {
			fprintf(ctx->_outfp, "    real4  %d  dup (?)\n", count);
		}
		else {
			fprintf(ctx->_outfp, "    real4  ?\n");
		}
	}
}

static void defconst_real8(struct tagCCContext* ctx, double val, int count)
{
	if (ctx->_backend->_curr_seg != SEG_BSS)
	{
		if (count > 1) {
			fprintf(ctx->_outfp, "    real4  %d  dup (%f)\n", count, val);
		}
		else {
			fprintf(ctx->_outfp, "    real4  %f\n", val);
		}
	}
	else
	{
		if (count > 1) {
			fprintf(ctx->_outfp, "    real4  %d  dup (?)\n", count);
		}
		else {
			fprintf(ctx->_outfp, "    real4  ?\n");
		}
	}
}

static void defconst_string(struct tagCCContext* ctx, const void* str, int count, int charsize)
{
	int n;

	if (charsize == 1) {
		fprintf(ctx->_outfp, "   db   ");
		for (n=0; n<count-1; n++)
		{
			fprintf(ctx->_outfp, "%u, ", ((const uint8_t*)str)[n]);
		}
		fprintf(ctx->_outfp, "%u \n", ((const uint8_t*)str)[n]);
	}
	else {
		assert(charsize == 2);

		fprintf(ctx->_outfp, "   dw   ");
		for (n = 0; n < count - 1; n++)
		{
			fprintf(ctx->_outfp, "%u, ", ((const uint16_t*)str)[n]);
		}
		fprintf(ctx->_outfp, "%u \n", ((const uint16_t*)str)[n]);
	}
}


static void defconst_address(struct tagCCContext* ctx, struct tagCCExprTree* expr)
{
	if (expr->_op == EXPR_CONSTANT_STR) { /* "string" */
		expr->_u._symbol->_x._refcnt++;
		fprintf(ctx->_outfp, "  dd   %s \n", expr->_u._symbol->_x._name);
	}
	else {

	}
}

static void defglobal_end(struct tagCCContext* ctx, struct tagCCSymbol* sym)
{
	/* do nothing */
}
