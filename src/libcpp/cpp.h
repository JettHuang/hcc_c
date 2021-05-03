/* \brief
 *		c preprocessor.
 */

#ifndef __CPP_H__
#define __CPP_H__

#include "pptoken.h"
#include "charstream.h"
#include "utils.h"
#include "mm.h"
#include <stdio.h>


#define CPP_MM_PERMPOOL		MMA_Area_1
#define CPP_MM_TEMPPOOL		MMA_Area_2

/* macro structure */
#define MACRO_FLAG_BUILTIN		0x0001
#define MACRO_FLAG_UNCHANGE		0X0002
#define MACRO_FLAG_HIDDEN		0x0004

/* token list node */
typedef struct tagTokenListNode {
	FPPToken	_tk;
	struct tagTokenListNode* _next;
} FTKListNode;

typedef struct tagMacro {
	const char*  _name;
	FLocation	 _loc;
	int16_t		 _flags;
	int16_t      _argc; /* -1: object macro,  0:no args(#define X() xxx) */
	FTKListNode* _args;
	FTKListNode* _body;
} FMacro;

/* macro list node */
typedef struct tagMacroListNode {
	FMacro	_macro;
	struct tagMacroListNode* _next;
} FMacroListNode;


/* control keywords */
enum ECtrlType
{
	Ctrl_none,
	Ctrl_if,
	Ctrl_ifdef,
	Ctrl_ifndef,
	Ctrl_elif,
	Ctrl_else,
	Ctrl_endif,
	Ctrl_include,
	Ctrl_define,
	Ctrl_undef,
	Ctrl_line,
	Ctrl_error,
	Ctrl_pragma
};

/* #if #else condition flags */
#define COND_FAILED_PARENT		0x01
#define COND_FAILED_SELF		0x02
#define CHECK_COND_PASS(flags)	(!((flags) & (COND_FAILED_PARENT | COND_FAILED_SELF)))
#define CHECK_OUTER_COND_PASS(flags)	(!((flags) & (COND_FAILED_PARENT)))

#define CHECK_IS_EOFLINE(ty)	((ty) == TK_EOF || (ty) == TK_NEWLINE)

/* condition section */
typedef struct tagConditionBlock {
	enum ECtrlType	_ctrltype;
	int16_t			_flags;

	struct tagConditionBlock* _up;
} FConditionBlock;

/* runtime context per source code */
typedef struct tagSourceCodeContext {
	FCharStream* _cs;
	const char* _srcfilename;
	const char* _srcdir;
	FConditionBlock* _condstack;

	struct tagSourceCodeContext* _up;
} FSourceCodeContext;

/* string list */
typedef struct tagStringListNode {
	const char* _str;
	struct tagStringListNode* _next;
} FStrListNode;

/* runtime context of cpp */
typedef struct tagCppContext {
	FStrListNode* _includedirs;
	const char* _outfilename;
	FILE* _outfp;

	FMacroListNode* _macrolist;
	FSourceCodeContext* _sourcestack;

	const char* _strdate;
	const char* _strtime;

	struct {
		int16_t	_valid : 1;
		FPPToken _tk;
	} _lookaheadtk;

	const char* _HS__DATE__;
	const char* _HS__FILE__;
	const char* _HS__LINE__;
	const char* _HS__STDC__;
	const char* _HS__STDC_HOSTED__;
	const char* _HS__STDC_VERSION__;
	const char* _HS__TIME__;
	const char* _HS__VA_ARGS__;
	const char* _HS__DEFINED__;
} FCppContext;


/************************************************************************/
/* public interface                                                     */
/************************************************************************/

void cpp_contex_init(FCppContext* ctx);
void cpp_contex_release(FCppContext* ctx);

void cpp_add_includedir(FCppContext* ctx, const char* dir);
void cpp_add_definition(FCppContext* ctx, const char* str);
BOOL cpp_process(FCppContext* ctx, const char* srcfilename, const char* outfilename);

/************************************************************************/
/* internal interface                                                     */
/************************************************************************/
void cpp_add_macro(FCppContext* ctx, const FMacro* m);
void cpp_remove_macro(FCppContext* ctx, const char* name);
const FMacro* cpp_find_macro(FCppContext* ctx, const char* name);

BOOL cpp_read_token(FCppContext* ctx, FCharStream* cs, FPPToken *tk, int bwantheader, int ballowerr);
BOOL cpp_lookahead_token(FCppContext* ctx, FCharStream* cs, FPPToken *tk, int bwantheader, int ballowerr);
BOOL cpp_read_tokentolist(FCppContext* ctx, FCharStream* cs, FTKListNode** tail, int bwantheader, int ballowerr);
BOOL cpp_read_rowtokens(FCppContext* ctx, FCharStream* cs, FTKListNode** tail, int bwantheader, int ballowerr);

void cpp_output_s(FCppContext* ctx, const char* format, ...);
void cpp_output_blankline(FCppContext* ctx, int lines);
void cpp_output_linectrl(FCppContext* ctx, const char* filename, int line);
void cpp_output_tokens(FCppContext* ctx, FTKListNode* tklist);

BOOL cpp_do_control(FCppContext* ctx, FTKListNode* tklist, int *outputlines);
BOOL cpp_expand_rowtokens(FCppContext* ctx, FTKListNode** tklist, int bscannextlines);
FTKListNode* cpp_duplicate_tklist(FTKListNode* orglist, enum EMMArea where);

BOOL cpp_eval_constexpr(FCppContext* ctx, FTKListNode* tklist, int* result);
FCharStream* cpp_open_includefile(FCppContext* ctx, const char* filename, const char*dir, int bsearchsys);

#endif /* __CPP_H__ */
