/* \brief 
 *		C compiler back-end
 */

#include "gen.h"
#include "mm.h"
#include "cc.h"
#include "hstring.h"
#include "parser/symbols.h"
#include "parser/types.h"

#include <stdio.h>


static void program_begin(struct tagCCContext* ctx);
static void program_end(struct tagCCContext* ctx);

static void block_begin(struct tagCCContext* ctx);
static void block_end(struct tagCCContext* ctx);

static void defsymbol(struct tagCCContext* ctx, struct tagCCSymbol* sym);
static void importsymbol(struct tagCCContext* ctx, struct tagCCSymbol* sym);
static void exportsymbol(struct tagCCContext* ctx, struct tagCCSymbol* sym);
static void defconst(struct tagCCContext* ctx, struct tagCCSymbol* sym);
static void defglobal(struct tagCCContext* ctx, struct tagCCSymbol* sym);
static void deflocal(struct tagCCContext* ctx, struct tagCCSymbol* sym);
static void space(struct tagCCContext* ctx, int nbytes, char ch);


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
	backend->_defsymbol = &defsymbol;
	backend->_importsymbol = &importsymbol;
	backend->_exportsymbol = &exportsymbol;
	backend->_defconst = &defconst;
	backend->_defglobal = &defglobal;
	backend->_deflocal = &deflocal;
	backend->_space = &space;

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

static void defsymbol(struct tagCCContext* ctx, struct tagCCSymbol* sym)
{
	if (sym->_generated) {
		sym->_x._name = hs_hashstr(util_stringf("L%s", sym->_name));
	}
	else if (sym->_scope == SCOPE_GLOBAL || sym->_sclass == SC_External){
		sym->_x._name = hs_hashstr(util_stringf("_%s", sym->_name));
	}
	else {
		sym->_x._name = sym->_name;
	}
}

static void importsymbol(struct tagCCContext* ctx, struct tagCCSymbol* sym)
{
	fprintf(ctx->_outfp, "extern %s:near\n", sym->_x._name);
}

static void exportsymbol(struct tagCCContext* ctx, struct tagCCSymbol* sym)
{
	fprintf(ctx->_outfp, "public %s\n", sym->_x._name);
}

static void defconst(struct tagCCContext* ctx, struct tagCCSymbol* sym)
{

}

static void defglobal(struct tagCCContext* ctx, struct tagCCSymbol* sym)
{

}

static void deflocal(struct tagCCContext* ctx, struct tagCCSymbol* sym)
{

}

static void space(struct tagCCContext* ctx, int nbytes, char ch)
{
	fprintf(ctx->_outfp, "db %d dup (%c)\n", nbytes, ch);
}
