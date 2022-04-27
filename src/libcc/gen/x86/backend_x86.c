/* \brief 
 *		C compiler x86 back-end
 */

#include "mm.h"
#include "cc.h"
#include "hstring.h"
#include "gen/backend.h"
#include "parser/symbols.h"
#include "parser/types.h"
#include "parser/expr.h"

#include <stdio.h>


static void program_begin(struct tagCCContext* ctx);
static void program_end(struct tagCCContext* ctx);

static void comment(struct tagCCContext* ctx, const char* szstr);
static void switchsegment(struct tagCCContext* ctx, enum tagCCSegment seg);
static void importsymbol(struct tagCCContext* ctx, struct tagCCSymbol* sym);
static void exportsymbol(struct tagCCContext* ctx, struct tagCCSymbol* sym);
static void additionalsymbols(struct tagCCContext* ctx);
static void defglobal_begin(struct tagCCContext* ctx, struct tagCCSymbol* sym, enum tagCCSegment seg);
static void defconst_space(struct tagCCContext* ctx, int count);
static void defconst_signed(struct tagCCContext* ctx, int size, int64_t val, int count);
static void defconst_unsigned(struct tagCCContext* ctx, int size, uint64_t val, int count);
static void defconst_real(struct tagCCContext* ctx, int size, double val, int count);
static void defconst_string(struct tagCCContext* ctx, const void* str, int count, int charsize);
static void defconst_address(struct tagCCContext* ctx, struct tagCCExprTree* expr);
static void defglobal_end(struct tagCCContext* ctx, struct tagCCSymbol* sym);

static void deffunction_begin(struct tagCCContext* ctx, struct tagCCSymbol* func);
static void deffunction_end(struct tagCCContext* ctx, struct tagCCSymbol* func);

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
	backend->_comment = &comment;
	backend->_importsymbol = &importsymbol;
	backend->_exportsymbol = &exportsymbol;
	backend->_additionalsymbols = &additionalsymbols;
	backend->_defglobal_begin = &defglobal_begin;
	backend->_defconst_space = &defconst_space;
	backend->_defconst_signed = &defconst_signed;
	backend->_defconst_unsigned = &defconst_unsigned;
	backend->_defconst_real = &defconst_real;
	backend->_defconst_string = &defconst_string;
	backend->_defconst_address = &defconst_address;
	backend->_defglobal_end = &defglobal_end;
	backend->_deffunction_begin = &deffunction_begin;
	backend->_deffunction_end = &deffunction_end;

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
	fprintf(ctx->_outfp, "\nend");
}

static void comment(struct tagCCContext* ctx, const char* szstr)
{
	fprintf(ctx->_outfp, "\n;%s\n", szstr);
}

static void importsymbol(struct tagCCContext* ctx, struct tagCCSymbol* sym)
{
	fprintf(ctx->_outfp, "extern %s:near\n", sym->_x._name);
}

static void exportsymbol(struct tagCCContext* ctx, struct tagCCSymbol* sym)
{
	fprintf(ctx->_outfp, "public %s\n", sym->_x._name);
}

static void additionalsymbols(struct tagCCContext* ctx)
{
	fprintf(ctx->_outfp, "extern __aullshr:near\n");
	fprintf(ctx->_outfp, "extern __aullrem:near\n");
	fprintf(ctx->_outfp, "extern __aulldiv:near\n");
	fprintf(ctx->_outfp, "extern __allshr:near\n");
	fprintf(ctx->_outfp, "extern __allshl:near\n");
	fprintf(ctx->_outfp, "extern __allrem:near\n");
	fprintf(ctx->_outfp, "extern __allmul:near\n");
	fprintf(ctx->_outfp, "extern __alldiv:near\n");
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

	/* fprintf(ctx->_outfp, "align %d\n", sym->_type->_align); */
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

void defconst_signed(struct tagCCContext* ctx, int size, int64_t val, int count)
{
	if (ctx->_backend->_curr_seg != SEG_BSS)
	{
		if (count > 1) {
			switch (size)
			{
			case 1:
				fprintf(ctx->_outfp, "    db  %d  dup (%d)\n", count, (int)val); break;
			case 2:
				fprintf(ctx->_outfp, "    dw  %d  dup (%d)\n", count, (int)val); break;
			case 4:
				fprintf(ctx->_outfp, "    dd  %d  dup (%d)\n", count, (int)val); break;
			case 8:
				fprintf(ctx->_outfp, "    dq  %d  dup (0%llxh)\n", count, val); break;
			default:
				assert(0); break;
			}
		}
		else {
			switch (size)
			{
			case 1:
				fprintf(ctx->_outfp, "    db  %d\n", (int)val); break;
			case 2:
				fprintf(ctx->_outfp, "    dw  %d\n", (int)val); break;
			case 4:
				fprintf(ctx->_outfp, "    dd  %d\n", (int)val); break;
			case 8:
				fprintf(ctx->_outfp, "    dq  0%llxh\n", val); break;
			default:
				assert(0); break;
			}
		}
	}
	else
	{
		if (count > 1) {
			switch (size)
			{
			case 1:
				fprintf(ctx->_outfp, "    db  %d  dup (?)\n", count); break;
			case 2:
				fprintf(ctx->_outfp, "    dw  %d  dup (?)\n", count); break;
			case 4:
				fprintf(ctx->_outfp, "    dd  %d  dup (?)\n", count); break;
			case 8:
				fprintf(ctx->_outfp, "    dq  %d  dup (?)\n", count); break;
			default:
				assert(0); break;
			}
		}
		else {
			switch (size)
			{
			case 1:
				fprintf(ctx->_outfp, "    db  ?\n"); break;
			case 2:
				fprintf(ctx->_outfp, "    dw  ?\n"); break;
			case 4:
				fprintf(ctx->_outfp, "    dd  ?\n"); break;
			case 8:
				fprintf(ctx->_outfp, "    dq  ?\n"); break;
			default:
				assert(0); break;
			}
		}
	}
}

void defconst_unsigned(struct tagCCContext* ctx, int size, uint64_t val, int count)
{
	if (ctx->_backend->_curr_seg != SEG_BSS)
	{
		if (count > 1) {
			switch (size)
			{
			case 1:
				fprintf(ctx->_outfp, "    db  %d  dup (%u)\n", count, (uint32_t)val); break;
			case 2:
				fprintf(ctx->_outfp, "    dw  %d  dup (%u)\n", count, (uint32_t)val); break;
			case 4:
				fprintf(ctx->_outfp, "    dd  %d  dup (%u)\n", count, (uint32_t)val); break;
			case 8:
				fprintf(ctx->_outfp, "    dq  %d  dup (0%llxh)\n", count, val); break;
			default:
				assert(0); break;
			}
		}
		else {
			switch (size)
			{
			case 1:
				fprintf(ctx->_outfp, "    db  %u\n", (uint32_t)val); break;
			case 2:
				fprintf(ctx->_outfp, "    dw  %u\n", (uint32_t)val); break;
			case 4:
				fprintf(ctx->_outfp, "    dd  %u\n", (uint32_t)val); break;
			case 8:
				fprintf(ctx->_outfp, "    dq  0%llxh\n", val); break;
			default:
				assert(0); break;
			}
		}
	}
	else
	{
		if (count > 1) {
			switch (size)
			{
			case 1:
				fprintf(ctx->_outfp, "    db  %d  dup (?)\n", count); break;
			case 2:
				fprintf(ctx->_outfp, "    dw  %d  dup (?)\n", count); break;
			case 4:
				fprintf(ctx->_outfp, "    dd  %d  dup (?)\n", count); break;
			case 8:
				fprintf(ctx->_outfp, "    dq  %d  dup (?)\n", count); break;
			default:
				assert(0); break;
			}
		}
		else {
			switch (size)
			{
			case 1:
				fprintf(ctx->_outfp, "    db  ?\n"); break;
			case 2:
				fprintf(ctx->_outfp, "    dw  ?\n"); break;
			case 4:
				fprintf(ctx->_outfp, "    dd  ?\n"); break;
			case 8:
				fprintf(ctx->_outfp, "    dq  ?\n"); break;
			default:
				assert(0); break;
			}
		}
	}
}

void defconst_real(struct tagCCContext* ctx, int size, double val, int count)
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

static const int kNumPerLine = 32;
static void defconst_string(struct tagCCContext* ctx, const void* str, int count, int charsize)
{
	if (charsize == 1) {
		int n, thisline;
		
		for (n=0; count > 0;)
		{
			thisline = count > kNumPerLine ? kNumPerLine : count;
			count -= thisline;

			fprintf(ctx->_outfp, "   db   ");
			for (; thisline > 1; --thisline)
			{
				fprintf(ctx->_outfp, "%u, ", ((const uint8_t*)str)[n++]);
			}
			fprintf(ctx->_outfp, "%u \n", ((const uint8_t*)str)[n++]);
		} /* end for count */
		
	}
	else {
		int n, thisline;

		assert(charsize == 2);
		for (n = 0; count > 0;)
		{
			thisline = count > kNumPerLine ? kNumPerLine : count;
			count -= thisline;

			fprintf(ctx->_outfp, "   dw   ");
			for (; thisline > 1; --thisline)
			{
				fprintf(ctx->_outfp, "%u, ", ((const uint16_t*)str)[n++]);
			}
			fprintf(ctx->_outfp, "%u \n", ((const uint16_t*)str)[n++]);
		} /* end for count */
	}
}


static void output_constant_expr(struct tagCCContext* ctx, struct tagCCExprTree* expr)
{
	int ircode, lhscode, rhscode;
	
	ircode = IR_OP(expr->_op);
	switch (ircode)
	{
	case IR_ADDRG:
		fprintf(ctx->_outfp, " %s ", expr->_symbol->_x._name); break;
	case IR_CONST:
		fprintf(ctx->_outfp, " %d ", (int)expr->_u._val._sint); break;
	case IR_ADD:
	case IR_SUB:
		lhscode = IR_OP(expr->_u._kids[0]->_op);
		rhscode = IR_OP(expr->_u._kids[1]->_op);
		if (lhscode == IR_ADD || lhscode == IR_SUB) {
			fprintf(ctx->_outfp, "(");
			output_constant_expr(ctx, expr->_u._kids[0]);
			fprintf(ctx->_outfp, ")");
		}
		else {
			output_constant_expr(ctx, expr->_u._kids[0]);
		}
		
		fprintf(ctx->_outfp, (ircode == IR_ADD) ? "+" : "-");

		if (rhscode == IR_ADD || rhscode == IR_SUB) {
			fprintf(ctx->_outfp, "(");
			output_constant_expr(ctx, expr->_u._kids[1]);
			fprintf(ctx->_outfp, ")");
		}
		else {
			output_constant_expr(ctx, expr->_u._kids[1]);
		}
		break;
	case IR_CVT:
		output_constant_expr(ctx, expr->_u._kids[0]); break;
	default:
		break;
	}
}

static void defconst_address(struct tagCCContext* ctx, struct tagCCExprTree* expr)
{
	fprintf(ctx->_outfp, "   dd   ");
	output_constant_expr(ctx, expr);
	fprintf(ctx->_outfp, "\n");
}

static void defglobal_end(struct tagCCContext* ctx, struct tagCCSymbol* sym)
{
	/* do nothing */
}

static void deffunction_begin(struct tagCCContext* ctx, struct tagCCSymbol* func)
{
	switchsegment(ctx, SEG_CODE);

	fprintf(ctx->_outfp, ";function %s begin\n", func->_name);
	fprintf(ctx->_outfp, "%s: \n", func->_x._name);
}

static void deffunction_end(struct tagCCContext* ctx, struct tagCCSymbol* func)
{
	fprintf(ctx->_outfp, ";function %s end\n", func->_name);
}
