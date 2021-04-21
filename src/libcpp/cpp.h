/* \brief
 *		c preprocessor.
 */

#ifndef __CPP_H__
#define __CPP_H__

#include "pptoken.h"
#include "charstream.h"
#include <stdio.h>


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

	int32_t		_prev_linenum;
	struct {
		int16_t	_valid : 1;
		FPPToken _tk;
	} _lookaheadtk;
} FCppContext;


/************************************************************************/
/* public interface                                                     */
/************************************************************************/

void cpp_contex_init(FCppContext* ctx);
void cpp_contex_release(FCppContext* ctx);

void cpp_add_includedir(FCppContext* ctx, const char* dir);
void cpp_add_definition(FCppContext* ctx, const char* str);
int cpp_process(FCppContext* ctx, const char* srcfilename, const char* outfilename);

#endif /* __CPP_H__ */
