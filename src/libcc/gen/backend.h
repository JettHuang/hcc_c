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

	void (*_comment)(struct tagCCContext* ctx, const char* szstr);
	void (*_importsymbol)(struct tagCCContext* ctx, struct tagCCSymbol* sym);
	void (*_exportsymbol)(struct tagCCContext* ctx, struct tagCCSymbol* sym);

	void (*_defglobal_begin)(struct tagCCContext* ctx, struct tagCCSymbol* sym, enum tagCCSegment seg);
	void (*_defconst_space)(struct tagCCContext* ctx, int count);

	void (*_defconst_signed)(struct tagCCContext* ctx, int size, int64_t val, int count);
	void (*_defconst_unsigned)(struct tagCCContext* ctx, int size, uint64_t val, int count);
	void (*_defconst_real)(struct tagCCContext* ctx, int size, double val, int count);
	void (*_defconst_string)(struct tagCCContext* ctx, const void* str, int count, int charsize);
	void (*_defconst_address)(struct tagCCContext* ctx, struct tagCCExprTree *expr);

	void (*_defglobal_end)(struct tagCCContext* ctx, struct tagCCSymbol* sym);

	void (*_deffunction_begin)(struct tagCCContext* ctx, struct tagCCSymbol* func);
	void (*_deffunction_end)(struct tagCCContext* ctx, struct tagCCSymbol* func);
} FCCBackend;


struct tagCCBackend* cc_new_backend();

void cc_gen_internalname(struct tagCCSymbol* sym);
void cc_gen_dumpsymbols(struct tagCCContext* ctx);
BOOL cc_gen_dumpfunction(struct tagCCContext* ctx, struct tagCCSymbol* func, FArray* caller, FArray* callee, struct tagCCIRBasicBlock* body);

#endif /* __CC_GEN_H__ */
