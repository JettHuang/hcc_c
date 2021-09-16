/* \brief
 *	C compiler's backend
 *
 *
 */

#ifndef __CC_GEN_H__
#define __CC_GEN_H__


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

	void (*_defsymbol)(struct tagCCContext* ctx, struct tagCCSymbol* sym);
	void (*_importsymbol)(struct tagCCContext* ctx, struct tagCCSymbol* sym);
	void (*_exportsymbol)(struct tagCCContext* ctx, struct tagCCSymbol* sym);
	void (*_defconst)(struct tagCCContext* ctx, struct tagCCSymbol* sym);
	void (*_defglobal)(struct tagCCContext* ctx, struct tagCCSymbol* sym);
	void (*_deflocal)(struct tagCCContext* ctx, struct tagCCSymbol* sym);
	void (*_space)(struct tagCCContext* ctx, int nbytes, char ch);
} FCCBackend;


struct tagCCBackend* cc_new_backend();

#endif /* __CC_GEN_H__ */
