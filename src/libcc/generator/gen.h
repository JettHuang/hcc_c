/* \brief
 *	C compiler's backend
 *
 *
 */

#ifndef __CC_GEN_H__
#define __CC_GEN_H__

#include <stdint.h>


/* back-end */
enum tagCCSegment {
	SEG_NONE,
	SEG_BSS,
	SEG_DATA,
	SEG_CONST,
	SEG_CODE
};

typedef struct tagCCBackend {
	int _is_little_ending : 1;
	enum tagCCSegment _curr_seg;

	void (*_program_begin)(struct tagCCContext* ctx);
	void (*_program_end)(struct tagCCContext* ctx);

	void (*_block_begin)(struct tagCCContext* ctx);
	void (*_block_end)(struct tagCCContext* ctx);

	void (*_importsymbol)(struct tagCCContext* ctx, struct tagCCSymbol* sym);
	void (*_exportsymbol)(struct tagCCContext* ctx, struct tagCCSymbol* sym);

	void (*_defglobal_begin)(struct tagCCContext* ctx, struct tagCCSymbol* sym, enum tagCCSegment seg);
	void (*_defconst_space)(struct tagCCContext* ctx, int count);
	void (*_defconst_ubyte)(struct tagCCContext* ctx, uint8_t val, int count);
	void (*_defconst_sbyte)(struct tagCCContext* ctx, int8_t val, int count);
	void (*_defconst_uword)(struct tagCCContext* ctx, uint16_t val, int count);
	void (*_defconst_sword)(struct tagCCContext* ctx, int16_t val, int count);
	void (*_defconst_udword)(struct tagCCContext* ctx, uint32_t val, int count);
	void (*_defconst_sdword)(struct tagCCContext* ctx, int32_t val, int count);
	void (*_defconst_uqword)(struct tagCCContext* ctx, uint64_t val, int count);
	void (*_defconst_sqword)(struct tagCCContext* ctx, int64_t val, int count);
	void (*_defconst_real4)(struct tagCCContext* ctx, float val, int count);
	void (*_defconst_real8)(struct tagCCContext* ctx, double val, int count);
	void (*_defconst_string)(struct tagCCContext* ctx, const void* str, int count, int charsize);
	void (*_defconst_address)(struct tagCCContext* ctx, struct tagCCExprTree *expr);
	void (*_defglobal_end)(struct tagCCContext* ctx, struct tagCCSymbol* sym);

} FCCBackend;


struct tagCCBackend* cc_new_backend();

void cc_gen_internalname(struct tagCCSymbol* sym);
void cc_gen_dumpsymbols(struct tagCCContext* ctx);
void cc_gen_eval_constant_address(struct tagCCContext* ctx, struct tagCCExprTree* expr, struct tagCCSymbol** addrsym, int64_t*offset);

#endif /* __CC_GEN_H__ */
