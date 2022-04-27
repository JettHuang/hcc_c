/* \brief
 *		C lexer
 *
 * https://en.cppreference.com/w/cpp/header/cstdlib
 * Numeric string conversion
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "lexer.h"
#include "logger.h"
#include "hstring.h"
#include "mm.h"
#include "cc.h"


FCCTokenDesc gCCTokenMetas[TK_MAX_COUNT] =
{
#define TOKEN(k, s, f)		{ k, f, s},
#include "../basic/tokens.inl"
#undef TOKEN
};


enum EState
{
	S_SELF = 0,
	S_START = 0,
	S_WS, /* white space */
	S_DIV,
	S_DIV_ASSIGN,

	/* c comment */
	S_CCOMENT,
	S_CCOMENT_WAITSLASH,
	S_CCOMENT_END,
	S_CCOMENT_EOF,

	/* identifier */
	S_ID,

	/* saw L (start of wide string?) */
	S_SAW_L,

	/* character-constant */
	S_CC,
	/* escape-sequence in '' */
	S_CC_ESCAPE,
	S_CC_NEWLINE,
	S_CC_EOF,

	/* string-constant */
	S_STR,
	/* escape-sequence in "" */
	S_STR_ESCAPE,
	S_STR_NEWLINE,
	S_STR_EOF,

	/* dots . ... */
	S_DOT1,
	S_DOT2,

	S_SUB,
	S_ADD,
	S_MUL,
	S_BITAND,
	S_NOT,
	S_MOD,
	S_BITXOR,

	/* < << <= <<= */
	S_LESS,
	S_LESS2,

	/* > >> >= >>= */
	S_GREAT,
	S_GREAT2,

	/* = == */
	S_ASSIGN,

	S_BITOR,
	S_BITOR2,

	/* # ## */
	S_POUND,

	/* integer constant */
	S_DECIMAL_NUM,
	S_DECIMAL_NUM_U,
	S_DECIMAL_NUM_UL,
	S_DECIMAL_NUM_ULL,
	S_DECIMAL_NUM_L,
	S_DECIMAL_NUM_LU,
	S_DECIMAL_NUM_LL,
	S_DECIMAL_NUM_LLU,
	S_DECIMAL_ERROR,

	S_OCTAL_NUM_PREFIX,
	S_OCTAL_NUM,
	S_OCTAL_NUM_INVALID,
	S_OCTAL_NUM_U,
	S_OCTAL_NUM_UL,
	S_OCTAL_NUM_ULL,
	S_OCTAL_NUM_L,
	S_OCTAL_NUM_LU,
	S_OCTAL_NUM_LL,
	S_OCTAL_NUM_LLU,
	S_OCTAL_ERROR,

	S_HEX_NUM_PREFIX,
	S_HEX_NUM,
	S_HEX_NUM_U,
	S_HEX_NUM_UL,
	S_HEX_NUM_ULL,
	S_HEX_NUM_L,
	S_HEX_NUM_LU,
	S_HEX_NUM_LL,
	S_HEX_NUM_LLU,
	S_HEX_ERROR,

	/* floating constant */
	/* decimal-floating-constant */
	S_DECIMAL_FLOAT_FRACT0_DOT,
	S_DECIMAL_FLOAT_FRACT0_DOT_FRACT1,
	S_DECIMAL_FLOAT_EXP,
	S_DECIMAL_FLOAT_EXP_SIGN,
	S_DECIMAL_FLOAT_EXP_DIGIT,
	S_DECIMAL_FLOAT_SUFFIX_F,
	S_DECIMAL_FLOAT_SUFFIX_L,
	S_DECIMAL_FLOAT_ERROR,

	/* hexadecimal floating constant */
	S_HEX_FLOAT_FRACT0_DOT,
	S_HEX_FLOAT_FRACT0_DOT_FRACT1,
	S_HEX_FLOAT_EXP,
	S_HEX_FLOAT_EXP_SIGN,
	S_HEX_FLOAT_EXP_DIGIT,
	S_HEX_FLOAT_SUFFIX_F,
	S_HEX_FLOAT_SUFFIX_L,
	S_HEX_FLOAT_ERROR,

	S_MAX,
	S_END,   // end of state-machine
	S_END_STAR // end of state-machine without eat current char
};

// action indicators
#define ACT_KEEPSTATE		0x00
#define ACT_NEWSTATE		0x01
#define ACT_OLDSTATE		0x02

// character classes
#define	C_ALPH	1
#define	C_NUM	2
#define	C_XX	3
#define C_OCTAL_DIGIT	4
#define C_HEX_DIGIT		5
#define C_EOF	((unsigned char)(EOF))

#define KCHAR_COUNT			256


typedef struct tagFSMAction
{
	enum EState		_newstate;
	enum ECCToken	_type;
} FAction;

#define MAKE_ACTION(S, T)			{ S, T }

typedef struct tagLexerRule
{
	uint32_t		_state;
	unsigned char	_inputs[4];
	FAction			_action;
} FLexerRule;

static const FLexerRule kRules[] =
{
	{ S_START, { C_XX },  MAKE_ACTION(S_END, TK_UNCLASS) },

	{ S_START, { ' ', '\t', '\v' }, MAKE_ACTION(S_WS, TK_NONE) },
	{ S_START, { '/'},    MAKE_ACTION(S_DIV, TK_NONE)          },
	{ S_START, { C_EOF},  MAKE_ACTION(S_END_STAR, TK_EOF)},
	{ S_START, { '\n' },  MAKE_ACTION(S_END, TK_NEWLINE) },
	{ S_START, { C_ALPH}, MAKE_ACTION(S_ID, TK_NONE) },
	{ S_START, { 'L' },   MAKE_ACTION(S_SAW_L, TK_NONE) },
	{ S_START, { '\'' },  MAKE_ACTION(S_CC, TK_NONE) },
	{ S_START, { '\"' },  MAKE_ACTION(S_STR, TK_NONE) },
	{ S_START, { '[' },   MAKE_ACTION(S_END, TK_LBRACKET) },
	{ S_START, { ']' },   MAKE_ACTION(S_END, TK_RBRACKET) },
	{ S_START, { '(' },   MAKE_ACTION(S_END, TK_LPAREN) },
	{ S_START, { ')' },   MAKE_ACTION(S_END, TK_RPAREN) },
	{ S_START, { '{' },   MAKE_ACTION(S_END, TK_LBRACE) },
	{ S_START, { '}' },   MAKE_ACTION(S_END, TK_RBRACE) },
	{ S_START, { '.' },   MAKE_ACTION(S_DOT1, TK_NONE)  },
	{ S_START, { '-' },   MAKE_ACTION(S_SUB, TK_NONE)   },
	{ S_START, { '+' },   MAKE_ACTION(S_ADD, TK_NONE)   },
	{ S_START, { '*' },   MAKE_ACTION(S_MUL, TK_NONE)   },
	{ S_START, { '&' },   MAKE_ACTION(S_BITAND, TK_NONE)   },
	{ S_START, { '~' },   MAKE_ACTION(S_END, TK_COMP)   },
	{ S_START, { '!' },   MAKE_ACTION(S_NOT, TK_NONE)   },
	{ S_START, { '%' },   MAKE_ACTION(S_MOD, TK_NONE)   },
	{ S_START, { '^' },   MAKE_ACTION(S_BITXOR, TK_NONE) },
	{ S_START, { '<' },   MAKE_ACTION(S_LESS, TK_NONE) },
	{ S_START, { '>' },   MAKE_ACTION(S_GREAT, TK_NONE)  },
	{ S_START, { '=' },   MAKE_ACTION(S_ASSIGN, TK_NONE) },
	{ S_START, { '|' },   MAKE_ACTION(S_BITOR, TK_NONE) },
	{ S_START, { '?' },   MAKE_ACTION(S_END, TK_QUESTION) },
	{ S_START, { ':' },   MAKE_ACTION(S_END, TK_COLON) },
	{ S_START, { ';' },   MAKE_ACTION(S_END, TK_SEMICOLON)},
	{ S_START, { ',' },   MAKE_ACTION(S_END, TK_COMMA)},
	{ S_START, { '#' },   MAKE_ACTION(S_POUND, TK_NONE)},
	{ S_START, { C_NUM }, MAKE_ACTION(S_DECIMAL_NUM, TK_NONE)},
	{ S_START, { '0' },   MAKE_ACTION(S_OCTAL_NUM_PREFIX, TK_NONE)},

	{ S_DIV,   { C_XX },  MAKE_ACTION(S_END_STAR, TK_DIV) },
	{ S_DIV,   { '*'},    MAKE_ACTION(S_CCOMENT, TK_NONE)      },
	{ S_DIV,   { '='},    MAKE_ACTION(S_END, TK_DIV_ASSIGN) },

	/* C Comment */
	{ S_CCOMENT, {C_XX},  MAKE_ACTION(S_SELF, TK_NONE) },
	{ S_CCOMENT, {'*' },  MAKE_ACTION(S_CCOMENT_WAITSLASH, TK_NONE) },
	{ S_CCOMENT, { C_EOF }, MAKE_ACTION(S_CCOMENT_EOF, TK_NONE) },
	{ S_CCOMENT_WAITSLASH, { C_XX },  MAKE_ACTION(S_CCOMENT, TK_NONE) },
	{ S_CCOMENT_WAITSLASH, { '*' },   MAKE_ACTION(S_SELF, TK_NONE) },
	{ S_CCOMENT_WAITSLASH, { '/' },   MAKE_ACTION(S_CCOMENT_END, TK_NONE) },
	{ S_CCOMENT_WAITSLASH, { C_EOF }, MAKE_ACTION(S_CCOMENT_EOF, TK_NONE) },

	/* identifier */
	{ S_ID, { C_XX }, MAKE_ACTION(S_END_STAR, TK_ID) },
	{ S_ID, { C_ALPH, C_NUM }, MAKE_ACTION(S_SELF, TK_NONE) },

	/* saw L */
	{ S_SAW_L, { C_XX }, MAKE_ACTION(S_END_STAR, TK_ID) },
	{ S_SAW_L, { C_ALPH, C_NUM }, MAKE_ACTION(S_ID, TK_NONE) },
	{ S_SAW_L, { '\'' }, MAKE_ACTION(S_CC, TK_NONE) },
	{ S_SAW_L, { '\"' }, MAKE_ACTION(S_STR, TK_NONE) },

	/* character-constant */
	{ S_CC, { C_XX }, MAKE_ACTION(S_SELF, TK_NONE) },
	{ S_CC, { '\''}, MAKE_ACTION(S_END, TK_CONSTANT_CHAR) },
	{ S_CC, { '\\'}, MAKE_ACTION(S_CC_ESCAPE, TK_NONE) },
	{ S_CC, { '\n'}, MAKE_ACTION(S_CC_NEWLINE, TK_NONE) },
	{ S_CC, { C_EOF }, MAKE_ACTION(S_CC_EOF, TK_NONE) },

	{ S_CC_ESCAPE, { C_XX }, MAKE_ACTION(S_CC, TK_NONE) },
	{ S_CC_ESCAPE, { '\n'}, MAKE_ACTION(S_CC_NEWLINE, TK_NONE) },
	{ S_CC_ESCAPE, { C_EOF }, MAKE_ACTION(S_CC_EOF, TK_NONE) },

	/* string-literal */
	{ S_STR, { C_XX }, MAKE_ACTION(S_SELF, TK_NONE) },
	{ S_STR, { '\"'}, MAKE_ACTION(S_END, TK_CONSTANT_STR) },
	{ S_STR, { '\\'}, MAKE_ACTION(S_STR_ESCAPE, TK_NONE) },
	{ S_STR, { '\n'}, MAKE_ACTION(S_STR_NEWLINE, TK_NONE) },
	{ S_STR, { C_EOF }, MAKE_ACTION(S_STR_EOF, TK_NONE) },

	{ S_STR_ESCAPE, { C_XX }, MAKE_ACTION(S_STR, TK_NONE) },
	{ S_STR_ESCAPE, { '\n'}, MAKE_ACTION(S_STR_NEWLINE, TK_NONE) },
	{ S_STR_ESCAPE, { C_EOF }, MAKE_ACTION(S_STR_EOF, TK_NONE) },

	/* punctuator */
	{ S_DOT1, { C_XX },  MAKE_ACTION(S_END_STAR, TK_DOT)  },
	{ S_DOT1, { '.' },   MAKE_ACTION(S_DOT2, TK_NONE) },
	{ S_DOT1, { C_NUM},  MAKE_ACTION(S_DECIMAL_FLOAT_FRACT0_DOT_FRACT1, TK_NONE) },
	{ S_DOT2, { C_XX },  MAKE_ACTION(S_END_STAR, TK_UNCLASS)},
	{ S_DOT2, { '.' },   MAKE_ACTION(S_END, TK_ELLIPSIS) },

	{ S_SUB,  { C_XX },  MAKE_ACTION(S_END_STAR, TK_SUB)  },
	{ S_SUB,  { '>'  },  MAKE_ACTION(S_END, TK_POINTER)   },
	{ S_SUB,  { '-'  },  MAKE_ACTION(S_END, TK_DEC)   },
	{ S_SUB,  { '='  },  MAKE_ACTION(S_END, TK_SUB_ASSIGN)   },

	{ S_ADD,  { C_XX },  MAKE_ACTION(S_END_STAR, TK_ADD)  },
	{ S_ADD,  { '+' },  MAKE_ACTION(S_END, TK_INC)  },
	{ S_ADD,  { '=' },  MAKE_ACTION(S_END, TK_ADD_ASSIGN)  },

	{ S_MUL,  { C_XX },  MAKE_ACTION(S_END_STAR, TK_MUL)  },
	{ S_MUL,  { '=' },  MAKE_ACTION(S_END, TK_MUL_ASSIGN)  },

	{ S_BITAND,  { C_XX },  MAKE_ACTION(S_END_STAR, TK_BITAND)  },
	{ S_BITAND,  { '&' },  MAKE_ACTION(S_END, TK_AND)  },
	{ S_BITAND,  { '=' },  MAKE_ACTION(S_END, TK_BITAND_ASSIGN)  },

	{ S_NOT, { C_XX }, MAKE_ACTION(S_END_STAR, TK_NOT) },
	{ S_NOT,  { '=' },  MAKE_ACTION(S_END, TK_UNEQUAL)  },

	{ S_MOD, { C_XX }, MAKE_ACTION(S_END_STAR, TK_MOD) },
	{ S_MOD, { '=' },  MAKE_ACTION(S_END, TK_MOD_ASSIGN) },

	{ S_BITXOR, { C_XX }, MAKE_ACTION(S_END_STAR, TK_BITXOR) },
	{ S_BITXOR, { '=' }, MAKE_ACTION(S_END, TK_BITXOR_ASSIGN) },

	/* less */
	{ S_LESS, { C_XX }, MAKE_ACTION(S_END_STAR, TK_LESS) },
	{ S_LESS, { '<' },  MAKE_ACTION(S_LESS2, TK_NONE) },
	{ S_LESS, { '=' },  MAKE_ACTION(S_END, TK_LESS_EQ) },
	{ S_LESS2, { C_XX }, MAKE_ACTION(S_END_STAR, TK_LSHIFT) },
	{ S_LESS2, { '=' },  MAKE_ACTION(S_END, TK_LSHIFT_ASSIGN) },

	/* great */
	{ S_GREAT, { C_XX }, MAKE_ACTION(S_END_STAR, TK_GREAT)},
	{ S_GREAT, { '>' }, MAKE_ACTION(S_GREAT2, TK_NONE) },
	{ S_GREAT, { '=' }, MAKE_ACTION(S_END, TK_GREAT_EQ) },
	{ S_GREAT2, { C_XX }, MAKE_ACTION(S_END_STAR, TK_RSHIFT) },
	{ S_GREAT2, { '=' }, MAKE_ACTION(S_END, TK_RSHIFT_ASSIGN) },

	{ S_ASSIGN, { C_XX }, MAKE_ACTION(S_END_STAR, TK_ASSIGN) },
	{ S_ASSIGN, { '=' }, MAKE_ACTION(S_END, TK_EQUAL) },

	{ S_BITOR, { C_XX },  MAKE_ACTION(S_END_STAR, TK_BITOR) },
	{ S_BITOR, { '|' }, MAKE_ACTION(S_END, TK_OR) },
	{ S_BITOR, { '=' }, MAKE_ACTION(S_END, TK_BITOR_ASSIGN) },

	{ S_POUND, { C_XX }, MAKE_ACTION(S_END_STAR, TK_POUND) },
	{ S_POUND, { '#' }, MAKE_ACTION(S_END, TK_DOUBLE_POUND) },

	/* integer constant */
	/* decimal number */
	{ S_DECIMAL_NUM, { C_XX }, MAKE_ACTION(S_END_STAR, TK_CONSTANT_INT) },
	{ S_DECIMAL_NUM, { C_NUM }, MAKE_ACTION(S_SELF, TK_NONE) },
	{ S_DECIMAL_NUM, { C_ALPH}, MAKE_ACTION(S_DECIMAL_ERROR, TK_NONE) },
	{ S_DECIMAL_NUM, { 'u', 'U' }, MAKE_ACTION(S_DECIMAL_NUM_U, TK_NONE) },
	{ S_DECIMAL_NUM, { 'l', 'L' }, MAKE_ACTION(S_DECIMAL_NUM_L, TK_NONE) },
	{ S_DECIMAL_NUM, { 'e', 'E' }, MAKE_ACTION(S_DECIMAL_FLOAT_EXP, TK_NONE) },
	{ S_DECIMAL_NUM, { '.' }, MAKE_ACTION(S_DECIMAL_FLOAT_FRACT0_DOT, TK_NONE) },

	{ S_DECIMAL_NUM_U, { C_XX }, MAKE_ACTION(S_END_STAR, TK_CONSTANT_UINT) },
	{ S_DECIMAL_NUM_U, { C_ALPH, C_NUM }, MAKE_ACTION(S_DECIMAL_ERROR, TK_NONE) },
	{ S_DECIMAL_NUM_U, { 'l', 'L' }, MAKE_ACTION(S_DECIMAL_NUM_UL, TK_NONE) },
	{ S_DECIMAL_NUM_UL, { C_XX }, MAKE_ACTION(S_END_STAR, TK_CONSTANT_ULONG) },
	{ S_DECIMAL_NUM_UL, { C_ALPH, C_NUM }, MAKE_ACTION(S_DECIMAL_ERROR, TK_NONE) },
	{ S_DECIMAL_NUM_UL, { 'l', 'L' }, MAKE_ACTION(S_DECIMAL_NUM_ULL, TK_NONE) },
	{ S_DECIMAL_NUM_ULL, { C_XX }, MAKE_ACTION(S_END_STAR, TK_CONSTANT_ULLONG) },
	{ S_DECIMAL_NUM_ULL, { C_ALPH, C_NUM }, MAKE_ACTION(S_DECIMAL_ERROR, TK_NONE) },
	{ S_DECIMAL_NUM_L, { C_XX  }, MAKE_ACTION(S_END_STAR, TK_CONSTANT_LONG) },
	{ S_DECIMAL_NUM_L, { C_ALPH, C_NUM }, MAKE_ACTION(S_DECIMAL_ERROR, TK_NONE) },
	{ S_DECIMAL_NUM_L, { 'l', 'L' }, MAKE_ACTION(S_DECIMAL_NUM_LL, TK_NONE) },
	{ S_DECIMAL_NUM_L, { 'u', 'U' }, MAKE_ACTION(S_DECIMAL_NUM_LU, TK_NONE) },

	{ S_DECIMAL_NUM_LL, { C_XX }, MAKE_ACTION(S_END_STAR, TK_CONSTANT_LLONG) },
	{ S_DECIMAL_NUM_LL, { C_ALPH, C_NUM }, MAKE_ACTION(S_DECIMAL_ERROR, TK_NONE) },
	{ S_DECIMAL_NUM_LL, { 'u', 'U' }, MAKE_ACTION(S_DECIMAL_NUM_LLU, TK_NONE) },
	{ S_DECIMAL_NUM_LLU, { C_XX }, MAKE_ACTION(S_END_STAR, TK_CONSTANT_ULLONG) },
	{ S_DECIMAL_NUM_LLU, { C_ALPH, C_NUM }, MAKE_ACTION(S_DECIMAL_ERROR, TK_NONE) },

	{ S_DECIMAL_NUM_LU, { C_XX }, MAKE_ACTION(S_END_STAR, TK_CONSTANT_ULONG) },
	{ S_DECIMAL_NUM_LU, { C_ALPH, C_NUM }, MAKE_ACTION(S_DECIMAL_ERROR, TK_NONE) },
	
	/* octal number */
	{ S_OCTAL_NUM_PREFIX, { C_XX }, MAKE_ACTION(S_END_STAR, TK_CONSTANT_INT) },
	{ S_OCTAL_NUM_PREFIX, { C_NUM }, MAKE_ACTION(S_OCTAL_NUM_INVALID, TK_NONE) },
	{ S_OCTAL_NUM_PREFIX, { C_OCTAL_DIGIT }, MAKE_ACTION(S_OCTAL_NUM, TK_NONE) },
	{ S_OCTAL_NUM_PREFIX, { C_ALPH}, MAKE_ACTION(S_OCTAL_ERROR, TK_NONE) },
	{ S_OCTAL_NUM_PREFIX, { 'u', 'U' }, MAKE_ACTION(S_OCTAL_NUM_U, TK_NONE) },
	{ S_OCTAL_NUM_PREFIX, { 'l', 'L' }, MAKE_ACTION(S_OCTAL_NUM_L, TK_NONE) },
	{ S_OCTAL_NUM_PREFIX, { 'x', 'X' }, MAKE_ACTION(S_HEX_NUM_PREFIX, TK_NONE) },
	{ S_OCTAL_NUM_PREFIX, { 'e', 'E' }, MAKE_ACTION(S_DECIMAL_FLOAT_EXP, TK_NONE) },
	{ S_OCTAL_NUM_PREFIX, { '.' }, MAKE_ACTION(S_DECIMAL_FLOAT_FRACT0_DOT, TK_NONE) },

	{ S_OCTAL_NUM, { C_XX }, MAKE_ACTION(S_END_STAR, TK_CONSTANT_INT) },
	{ S_OCTAL_NUM, { C_NUM }, MAKE_ACTION(S_OCTAL_NUM_INVALID, TK_NONE) },
	{ S_OCTAL_NUM, { C_OCTAL_DIGIT }, MAKE_ACTION(S_SELF, TK_NONE) },
	{ S_OCTAL_NUM, { C_ALPH}, MAKE_ACTION(S_OCTAL_ERROR, TK_NONE) },
	{ S_OCTAL_NUM, { 'u', 'U' }, MAKE_ACTION(S_OCTAL_NUM_U, TK_NONE) },
	{ S_OCTAL_NUM, { 'l', 'L' }, MAKE_ACTION(S_OCTAL_NUM_L, TK_NONE) },
	{ S_OCTAL_NUM, { 'e', 'E' }, MAKE_ACTION(S_DECIMAL_FLOAT_EXP, TK_NONE) },
	{ S_OCTAL_NUM, { '.' },		 MAKE_ACTION(S_DECIMAL_FLOAT_FRACT0_DOT, TK_NONE) },

	{ S_OCTAL_NUM_INVALID, { C_XX }, MAKE_ACTION(S_OCTAL_ERROR, TK_NONE) },
	{ S_OCTAL_NUM_INVALID, { C_NUM }, MAKE_ACTION(S_SELF, TK_NONE) },
	{ S_OCTAL_NUM_INVALID, { C_ALPH}, MAKE_ACTION(S_OCTAL_ERROR, TK_NONE) },
	{ S_OCTAL_NUM_INVALID, { 'e', 'E' }, MAKE_ACTION(S_DECIMAL_FLOAT_EXP, TK_NONE) },
	{ S_OCTAL_NUM_INVALID, { '.' },		 MAKE_ACTION(S_DECIMAL_FLOAT_FRACT0_DOT, TK_NONE) },

	{ S_OCTAL_NUM_U, { C_XX }, MAKE_ACTION(S_END_STAR, TK_CONSTANT_UINT) },
	{ S_OCTAL_NUM_U, { C_ALPH, C_NUM }, MAKE_ACTION(S_OCTAL_ERROR, TK_NONE) },
	{ S_OCTAL_NUM_U, { 'l', 'L' }, MAKE_ACTION(S_OCTAL_NUM_UL, TK_NONE) },
	{ S_OCTAL_NUM_UL, { C_XX }, MAKE_ACTION(S_END_STAR, TK_CONSTANT_ULONG) },
	{ S_OCTAL_NUM_UL, { C_ALPH, C_NUM }, MAKE_ACTION(S_OCTAL_ERROR, TK_NONE) },
	{ S_OCTAL_NUM_UL, { 'l', 'L' }, MAKE_ACTION(S_OCTAL_NUM_ULL, TK_NONE) },
	{ S_OCTAL_NUM_ULL, { C_XX }, MAKE_ACTION(S_END_STAR, TK_CONSTANT_ULLONG) },
	{ S_OCTAL_NUM_ULL, { C_ALPH, C_NUM }, MAKE_ACTION(S_OCTAL_ERROR, TK_NONE) },
	{ S_OCTAL_NUM_L, { C_XX  }, MAKE_ACTION(S_END_STAR, TK_CONSTANT_LONG) },
	{ S_OCTAL_NUM_L, { C_ALPH, C_NUM }, MAKE_ACTION(S_OCTAL_ERROR, TK_NONE) },
	{ S_OCTAL_NUM_L, { 'l', 'L' }, MAKE_ACTION(S_OCTAL_NUM_LL, TK_NONE) },
	{ S_OCTAL_NUM_L, { 'u', 'U' }, MAKE_ACTION(S_OCTAL_NUM_LU, TK_NONE) },

	{ S_OCTAL_NUM_LL, { C_XX }, MAKE_ACTION(S_END_STAR, TK_CONSTANT_LLONG) },
	{ S_OCTAL_NUM_LL, { C_ALPH, C_NUM }, MAKE_ACTION(S_OCTAL_ERROR, TK_NONE) },
	{ S_OCTAL_NUM_LL, { 'u', 'U' }, MAKE_ACTION(S_OCTAL_NUM_LLU, TK_NONE) },
	{ S_OCTAL_NUM_LLU, { C_XX }, MAKE_ACTION(S_END_STAR, TK_CONSTANT_ULLONG) },
	{ S_OCTAL_NUM_LLU, { C_ALPH, C_NUM }, MAKE_ACTION(S_OCTAL_ERROR, TK_NONE) },

	{ S_OCTAL_NUM_LU, { C_XX }, MAKE_ACTION(S_END_STAR, TK_CONSTANT_ULONG) },
	{ S_OCTAL_NUM_LU, { C_ALPH, C_NUM }, MAKE_ACTION(S_OCTAL_ERROR, TK_NONE) },

	/* hex number */
	{ S_HEX_NUM_PREFIX, { C_XX }, MAKE_ACTION(S_HEX_ERROR, TK_NONE) },
	{ S_HEX_NUM_PREFIX, { C_NUM }, MAKE_ACTION(S_HEX_ERROR, TK_NONE) },
	{ S_HEX_NUM_PREFIX, { C_ALPH}, MAKE_ACTION(S_HEX_ERROR, TK_NONE) },
	{ S_HEX_NUM_PREFIX, { C_HEX_DIGIT }, MAKE_ACTION(S_HEX_NUM, TK_NONE) },
	{ S_HEX_NUM_PREFIX, { '.' }, MAKE_ACTION(S_HEX_FLOAT_FRACT0_DOT, TK_NONE) },

	{ S_HEX_NUM, { C_XX }, MAKE_ACTION(S_END_STAR, TK_CONSTANT_UINT) },
	{ S_HEX_NUM, { C_NUM }, MAKE_ACTION(S_HEX_ERROR, TK_NONE) },
	{ S_HEX_NUM, { C_ALPH}, MAKE_ACTION(S_HEX_ERROR, TK_NONE) },
	{ S_HEX_NUM, { C_HEX_DIGIT }, MAKE_ACTION(S_SELF, TK_NONE) },
	{ S_HEX_NUM, { 'u', 'U' }, MAKE_ACTION(S_HEX_NUM_U, TK_NONE) },
	{ S_HEX_NUM, { 'l', 'L' }, MAKE_ACTION(S_HEX_NUM_L, TK_NONE) },
	{ S_HEX_NUM, { '.' }, MAKE_ACTION(S_HEX_FLOAT_FRACT0_DOT, TK_NONE) },
	{ S_HEX_NUM, { 'p', 'P' }, MAKE_ACTION(S_HEX_FLOAT_EXP, TK_NONE) },

	{ S_HEX_NUM_U, { C_XX }, MAKE_ACTION(S_END_STAR, TK_CONSTANT_UINT) },
	{ S_HEX_NUM_U, { C_ALPH, C_NUM }, MAKE_ACTION(S_HEX_ERROR, TK_NONE) },
	{ S_HEX_NUM_U, { 'l', 'L' }, MAKE_ACTION(S_HEX_NUM_UL, TK_NONE) },
	{ S_HEX_NUM_UL, { C_XX }, MAKE_ACTION(S_END_STAR, TK_CONSTANT_ULONG) },
	{ S_HEX_NUM_UL, { C_ALPH, C_NUM }, MAKE_ACTION(S_HEX_ERROR, TK_NONE) },
	{ S_HEX_NUM_UL, { 'l', 'L' }, MAKE_ACTION(S_HEX_NUM_ULL, TK_NONE) },
	{ S_HEX_NUM_ULL, { C_XX }, MAKE_ACTION(S_END_STAR, TK_CONSTANT_ULLONG) },
	{ S_HEX_NUM_ULL, { C_ALPH, C_NUM }, MAKE_ACTION(S_HEX_ERROR, TK_NONE) },
	{ S_HEX_NUM_L, { C_XX  }, MAKE_ACTION(S_END_STAR, TK_CONSTANT_LONG) },
	{ S_HEX_NUM_L, { C_ALPH, C_NUM }, MAKE_ACTION(S_HEX_ERROR, TK_NONE) },
	{ S_HEX_NUM_L, { 'l', 'L' }, MAKE_ACTION(S_HEX_NUM_LL, TK_NONE) },
	{ S_HEX_NUM_L, { 'u', 'U' }, MAKE_ACTION(S_HEX_NUM_LU, TK_NONE) },

	{ S_HEX_NUM_LL, { C_XX }, MAKE_ACTION(S_END_STAR, TK_CONSTANT_LLONG) },
	{ S_HEX_NUM_LL, { C_ALPH, C_NUM }, MAKE_ACTION(S_HEX_ERROR, TK_NONE) },
	{ S_HEX_NUM_LL, { 'u', 'U' }, MAKE_ACTION(S_HEX_NUM_LLU, TK_NONE) },
	{ S_HEX_NUM_LLU, { C_XX }, MAKE_ACTION(S_END_STAR, TK_CONSTANT_ULLONG) },
	{ S_HEX_NUM_LLU, { C_ALPH, C_NUM }, MAKE_ACTION(S_HEX_ERROR, TK_NONE) },

	{ S_HEX_NUM_LU, { C_XX }, MAKE_ACTION(S_END_STAR, TK_CONSTANT_ULONG) },
	{ S_HEX_NUM_LU, { C_ALPH, C_NUM }, MAKE_ACTION(S_HEX_ERROR, TK_NONE) },

	/* decimal floating constant */
	{ S_DECIMAL_FLOAT_FRACT0_DOT, { C_XX }, MAKE_ACTION(S_END_STAR, TK_CONSTANT_DOUBLE) },
	{ S_DECIMAL_FLOAT_FRACT0_DOT, { C_NUM }, MAKE_ACTION(S_DECIMAL_FLOAT_FRACT0_DOT_FRACT1, TK_NONE) },
	{ S_DECIMAL_FLOAT_FRACT0_DOT, { C_ALPH, '.' }, MAKE_ACTION(S_DECIMAL_FLOAT_ERROR, TK_NONE) },
	{ S_DECIMAL_FLOAT_FRACT0_DOT, { 'e', 'E' }, MAKE_ACTION(S_DECIMAL_FLOAT_EXP, TK_NONE) },
	{ S_DECIMAL_FLOAT_FRACT0_DOT, { 'f', 'F' }, MAKE_ACTION(S_DECIMAL_FLOAT_SUFFIX_F, TK_NONE) },
	{ S_DECIMAL_FLOAT_FRACT0_DOT, { 'l', 'L' }, MAKE_ACTION(S_DECIMAL_FLOAT_SUFFIX_L, TK_NONE) },

	{ S_DECIMAL_FLOAT_FRACT0_DOT_FRACT1, { C_XX }, MAKE_ACTION(S_END_STAR, TK_CONSTANT_DOUBLE) },
	{ S_DECIMAL_FLOAT_FRACT0_DOT_FRACT1, { C_NUM }, MAKE_ACTION(S_SELF, TK_NONE) },
	{ S_DECIMAL_FLOAT_FRACT0_DOT_FRACT1, { C_ALPH, '.' }, MAKE_ACTION(S_DECIMAL_FLOAT_ERROR, TK_NONE) },
	{ S_DECIMAL_FLOAT_FRACT0_DOT_FRACT1, { 'e', 'E' }, MAKE_ACTION(S_DECIMAL_FLOAT_EXP, TK_NONE) },
	{ S_DECIMAL_FLOAT_FRACT0_DOT_FRACT1, { 'f', 'F' }, MAKE_ACTION(S_DECIMAL_FLOAT_SUFFIX_F, TK_NONE) },
	{ S_DECIMAL_FLOAT_FRACT0_DOT_FRACT1, { 'l', 'L' }, MAKE_ACTION(S_DECIMAL_FLOAT_SUFFIX_L, TK_NONE) },

	{ S_DECIMAL_FLOAT_EXP, { C_XX }, MAKE_ACTION(S_DECIMAL_FLOAT_ERROR, TK_NONE) },
	{ S_DECIMAL_FLOAT_EXP, { C_NUM }, MAKE_ACTION(S_DECIMAL_FLOAT_EXP_DIGIT, TK_NONE) },
	{ S_DECIMAL_FLOAT_EXP, { '-', '+' }, MAKE_ACTION(S_DECIMAL_FLOAT_EXP_SIGN, TK_NONE) },

	{ S_DECIMAL_FLOAT_EXP_SIGN, { C_XX }, MAKE_ACTION(S_DECIMAL_FLOAT_ERROR, TK_NONE) },
	{ S_DECIMAL_FLOAT_EXP_SIGN, { C_NUM }, MAKE_ACTION(S_DECIMAL_FLOAT_EXP_DIGIT, TK_NONE) },
	
	{ S_DECIMAL_FLOAT_EXP_DIGIT, { C_XX }, MAKE_ACTION(S_END_STAR, TK_CONSTANT_DOUBLE) },
	{ S_DECIMAL_FLOAT_EXP_DIGIT, { C_NUM }, MAKE_ACTION(S_DECIMAL_FLOAT_EXP_DIGIT, TK_NONE) },
	{ S_DECIMAL_FLOAT_EXP_DIGIT, { C_ALPH, '.' }, MAKE_ACTION(S_DECIMAL_FLOAT_ERROR, TK_NONE) },
	{ S_DECIMAL_FLOAT_EXP_DIGIT, { 'f', 'F' }, MAKE_ACTION(S_DECIMAL_FLOAT_SUFFIX_F, TK_NONE) },
	{ S_DECIMAL_FLOAT_EXP_DIGIT, { 'l', 'L' }, MAKE_ACTION(S_DECIMAL_FLOAT_SUFFIX_L, TK_NONE) },

	{ S_DECIMAL_FLOAT_SUFFIX_F, { C_XX }, MAKE_ACTION(S_END_STAR, TK_CONSTANT_FLOAT) },
	{ S_DECIMAL_FLOAT_SUFFIX_F, { C_NUM }, MAKE_ACTION(S_DECIMAL_FLOAT_ERROR, TK_NONE) },
	{ S_DECIMAL_FLOAT_SUFFIX_F, { C_ALPH, '.' }, MAKE_ACTION(S_DECIMAL_FLOAT_ERROR, TK_NONE) },

	{ S_DECIMAL_FLOAT_SUFFIX_L, { C_XX }, MAKE_ACTION(S_END_STAR, TK_CONSTANT_LDOUBLE) },
	{ S_DECIMAL_FLOAT_SUFFIX_L, { C_NUM }, MAKE_ACTION(S_DECIMAL_FLOAT_ERROR, TK_NONE) },
	{ S_DECIMAL_FLOAT_SUFFIX_L, { C_ALPH, '.' }, MAKE_ACTION(S_DECIMAL_FLOAT_ERROR, TK_NONE) },

	/* hexadecimal floating constant */
	{ S_HEX_FLOAT_FRACT0_DOT, { C_XX }, MAKE_ACTION(S_HEX_FLOAT_ERROR, TK_NONE) },
	{ S_HEX_FLOAT_FRACT0_DOT, { C_HEX_DIGIT }, MAKE_ACTION(S_HEX_FLOAT_FRACT0_DOT_FRACT1, TK_NONE) },
	{ S_HEX_FLOAT_FRACT0_DOT, { 'p', 'P' }, MAKE_ACTION(S_HEX_FLOAT_EXP, TK_NONE) },

	{ S_HEX_FLOAT_FRACT0_DOT_FRACT1, { C_XX }, MAKE_ACTION(S_HEX_FLOAT_ERROR, TK_NONE) },
	{ S_HEX_FLOAT_FRACT0_DOT_FRACT1, { C_HEX_DIGIT }, MAKE_ACTION(S_SELF, TK_NONE) },
	{ S_HEX_FLOAT_FRACT0_DOT_FRACT1, { 'p', 'P' }, MAKE_ACTION(S_HEX_FLOAT_EXP, TK_NONE) },

	{ S_HEX_FLOAT_EXP, { C_XX }, MAKE_ACTION(S_HEX_FLOAT_ERROR, TK_NONE) },
	{ S_HEX_FLOAT_EXP, { C_NUM }, MAKE_ACTION(S_HEX_FLOAT_EXP_DIGIT, TK_NONE) },
	{ S_HEX_FLOAT_EXP, { '-', '+' }, MAKE_ACTION(S_HEX_FLOAT_EXP_SIGN, TK_NONE) },

	{ S_HEX_FLOAT_EXP_SIGN, { C_XX }, MAKE_ACTION(S_HEX_FLOAT_ERROR, TK_NONE) },
	{ S_HEX_FLOAT_EXP_SIGN, { C_NUM }, MAKE_ACTION(S_HEX_FLOAT_EXP_DIGIT, TK_NONE) },

	{ S_HEX_FLOAT_EXP_DIGIT, { C_XX }, MAKE_ACTION(S_END_STAR, TK_CONSTANT_DOUBLE) },
	{ S_HEX_FLOAT_EXP_DIGIT, { C_NUM }, MAKE_ACTION(S_HEX_FLOAT_EXP_DIGIT, TK_NONE) },
	{ S_HEX_FLOAT_EXP_DIGIT, { C_ALPH, '.' }, MAKE_ACTION(S_HEX_FLOAT_ERROR, TK_NONE) },
	{ S_HEX_FLOAT_EXP_DIGIT, { 'f', 'F' }, MAKE_ACTION(S_HEX_FLOAT_SUFFIX_F, TK_NONE) },
	{ S_HEX_FLOAT_EXP_DIGIT, { 'l', 'L' }, MAKE_ACTION(S_HEX_FLOAT_SUFFIX_L, TK_NONE) },

	{ S_HEX_FLOAT_SUFFIX_F, { C_XX }, MAKE_ACTION(S_END_STAR, TK_CONSTANT_FLOAT) },
	{ S_HEX_FLOAT_SUFFIX_F, { C_NUM }, MAKE_ACTION(S_HEX_FLOAT_ERROR, TK_NONE) },
	{ S_HEX_FLOAT_SUFFIX_F, { C_ALPH, '.' }, MAKE_ACTION(S_HEX_FLOAT_ERROR, TK_NONE) },

	{ S_HEX_FLOAT_SUFFIX_L, { C_XX }, MAKE_ACTION(S_END_STAR, TK_CONSTANT_LDOUBLE) },
	{ S_HEX_FLOAT_SUFFIX_L, { C_NUM }, MAKE_ACTION(S_HEX_FLOAT_ERROR, TK_NONE) },
	{ S_HEX_FLOAT_SUFFIX_L, { C_ALPH, '.' }, MAKE_ACTION(S_HEX_FLOAT_ERROR, TK_NONE) },

	/* unclass */
};


static FAction g_lexerfsm[S_MAX][KCHAR_COUNT] = { MAKE_ACTION(S_END, TK_NONE) };
struct tagCharBuffer {
	char* _ptrdata;
	uint32_t _capacity;
	uint32_t _itemcnt;
} g_cbuffer = { NULL, 0, 0 };

static void charbuffer_init()
{
	g_cbuffer._itemcnt = 0;
	g_cbuffer._capacity = 2048;
	g_cbuffer._ptrdata = mm_alloc(g_cbuffer._capacity);
	assert(g_cbuffer._ptrdata);
}

static void charbuffer_release()
{
	if (g_cbuffer._ptrdata)
	{
		mm_free(g_cbuffer._ptrdata);
	}
	g_cbuffer._ptrdata = NULL;
	g_cbuffer._itemcnt = 0;
	g_cbuffer._capacity = 0;
}

static void charbuffer_pushback(char c)
{
	if (g_cbuffer._itemcnt >= g_cbuffer._capacity)
	{
		uint32_t newcap = g_cbuffer._capacity * 2;
		char* newptr = mm_alloc(newcap);
		if (!newptr)
		{
			logger_output_s("out of memory at %s:%d\n", __FILE__, __LINE__);
			return;
		}

		memcpy(newptr, g_cbuffer._ptrdata, g_cbuffer._itemcnt);
		mm_free(g_cbuffer._ptrdata);
		g_cbuffer._capacity = newcap;
		g_cbuffer._ptrdata = newptr;
	}

	g_cbuffer._ptrdata[g_cbuffer._itemcnt++] = c;
}

static char charbuffer_popback()
{
	char c = 0;

	if (g_cbuffer._itemcnt > 0)
	{
		c = g_cbuffer._ptrdata[--g_cbuffer._itemcnt];
	}

	return c;
}

static void charbuffer_empty()
{
	g_cbuffer._itemcnt = 0;
}

void cc_lexer_init()
{
	int32_t i, j, c;


	/* fill finite-state-machine */
	for (i = 0; i < sizeof(kRules) / sizeof(kRules[0]); ++i)
	{
		const FLexerRule* rule = &kRules[i];

		for (c = 0; rule->_inputs[c]; ++c)
		{
			const int32_t ch = rule->_inputs[c];
			switch (ch)
			{
			case C_XX:
			{
				for (j = 0; j < KCHAR_COUNT; ++j)
				{
					g_lexerfsm[rule->_state][j] = rule->_action;
				} /* end for j */
			}
			break;
			case C_ALPH:
			{
				for (j = 0; j < KCHAR_COUNT; ++j)
				{
					if ((j >= 'a' && j <= 'z')
						|| (j >= 'A' && j <= 'Z')
						|| j == '_')
					{
						g_lexerfsm[rule->_state][j] = rule->_action;
					}
				} /* end for j */
			}
			break;
			case C_NUM:
			{
				for (j = '0'; j <= '9'; ++j)
				{
					g_lexerfsm[rule->_state][j] = rule->_action;
				} /* end for j */
			}
			break;
			case C_OCTAL_DIGIT:
			{
				for (j = '0'; j <= '7'; ++j)
				{
					g_lexerfsm[rule->_state][j] = rule->_action;
				} /* end for j */
			}
			break;
			case C_HEX_DIGIT:
			{
				for (j = '0'; j <= '9'; ++j)
				{
					g_lexerfsm[rule->_state][j] = rule->_action;
				} /* end for j */
				for (j = 'a'; j <= 'f'; ++j)
				{
					g_lexerfsm[rule->_state][j] = rule->_action;
				} /* end for j */
				for (j = 'A'; j <= 'F'; ++j)
				{
					g_lexerfsm[rule->_state][j] = rule->_action;
				} /* end for j */
			}
			break;
			default:
			{
				g_lexerfsm[rule->_state][ch] = rule->_action;
			}
			break;
			}
		} /* end for c */
	} /* end for i */

	/* init char buffer */
	charbuffer_init();
}

void cc_lexer_uninit()
{
	charbuffer_release();
}

static BOOL cc_lexer_read_token_inner(FCharStream* cs, FCCToken* tk);
BOOL cc_lexer_read_token(struct tagLexerContext* ctx, FCCToken* tk, BOOL ismergestr)
{
	FCCToken strs[256], newlinetk;
	int strcnt, totallen;

	if (ctx->_cached[0]._isvalid) 
	{
		*tk = ctx->_cached[0]._tk;
		ctx->_cached[0]._isvalid = FALSE; /* take it away.. */
		return TRUE;
	}
	else if (ctx->_cached[1]._isvalid)
	{
		*tk = ctx->_cached[1]._tk;
		ctx->_cached[1]._isvalid = FALSE; /* take it away.. */
		return TRUE;
	}
	
	strcnt = totallen = 0;
	newlinetk._type = TK_NONE;
	while (1)
	{
		if (!cc_lexer_read_token_inner(ctx->_cs, tk)) { return FALSE; }
		if (!ismergestr) { return TRUE; }

		if (tk->_type == TK_CONSTANT_STR || tk->_type == TK_CONSTANT_WSTR)
		{
			newlinetk._type = TK_NONE; /* clear the previous newline tag */
			if (strcnt <= 0 || strs[strcnt - 1]._type == tk->_type)
			{
				if (strcnt >= sizeof(strs) / sizeof(strs[0]))
				{
					logger_output_s("error: too much string token at %w\n", &tk->_loc);
					return FALSE;
				}

				strs[strcnt++] = *tk;
				totallen += tk->_val._astr._chcnt;
			}
			else
			{
				logger_output_s("error: inconsistent string token at %w and %w\n", &strs[strcnt - 1]._loc, &tk->_loc);
				return FALSE;
			}
		}
		else if (tk->_type == TK_NEWLINE)
		{
			if (strcnt <= 0) 
			{ 
				break;
			}
			newlinetk = *tk;
		}
		else 
		{
			break;
		}
	} /* end while */

	if (strcnt <= 0) { return TRUE; }
	if (newlinetk._type == TK_NEWLINE) 
	{
		ctx->_cached[0]._tk = newlinetk;
		ctx->_cached[0]._isvalid = TRUE;
	}
	ctx->_cached[1]._tk = *tk;
	ctx->_cached[1]._isvalid = TRUE;

	/* merge all the string literal */
	if (strcnt == 1)
	{
		*tk = strs[0];
	}
	else 
	{
		int i, tailbytes, copybytes;
		char* szbuf, *ptr;

		tailbytes = (strs[0]._type == TK_CONSTANT_WSTR) ? 2 : 1;
		totallen -= tailbytes * (strcnt - 1);
		if (!(szbuf = mm_alloc_area(totallen, CC_MM_TEMPPOOL)))
		{
			logger_output_s("error: out of memory when merging string literal at %w\n", &strs[0]._loc);
			return FALSE;
		}

		ptr = szbuf;
		for (i = 0; i < strcnt-1; ++i, ptr += copybytes)
		{
			copybytes = strs[i]._val._astr._chcnt - tailbytes;
			memcpy(ptr, strs[i]._val._astr._str, copybytes);
		}
		copybytes = strs[i]._val._astr._chcnt;
		memcpy(ptr, strs[i]._val._astr._str, copybytes);

		*tk = strs[0];
		tk->_val._astr._chcnt = totallen;
		tk->_val._astr._str = hs_hashnstr(szbuf, totallen);
	}
	
	return TRUE;
}

static BOOL cc_lexer_postprocess_token(FCCToken* tk, struct tagCharBuffer* charbuf);
static BOOL cc_lexer_read_token_inner(FCharStream* cs, FCCToken* tk)
{
	enum EState currstate = S_START;
	int bContinue = TRUE;

	tk->_type = TK_UNCLASS;

	charbuffer_empty();
	while (bContinue)
	{
		char ch;
		const FAction* action;

		if (currstate == S_START)
		{
			tk->_loc._filename = cs->_srcfilename;
			tk->_loc._line = cs->_line;
			tk->_loc._col = cs->_col;
		}

		ch = cs_current(cs);
		action = &g_lexerfsm[currstate][(unsigned char)ch];
		if (action->_newstate)
		{
			currstate = action->_newstate;
		}

		switch (currstate)
		{
		case S_WS:
			cs_forward(cs);
			currstate = S_START;
			break;
		case S_CCOMENT:
		case S_CCOMENT_WAITSLASH:
			cs_forward(cs);
			break;
		case S_CCOMENT_END:
			ch = charbuffer_popback(); /* pop the '/' */
			assert(ch == '/');
			cs_replace_current(cs, ' ');
			currstate = S_START;
			break;
		case S_CCOMENT_EOF:
			logger_output_s("error: comment occur eof at %s:%d:%d\n", cs->_srcfilename, cs->_line, cs->_col);
			return FALSE;

		case S_CC_NEWLINE:
		case S_STR_NEWLINE:
			logger_output_s("error: string occur newline at %s:%d:%d\n", cs->_srcfilename, cs->_line, cs->_col);
			return FALSE;

		case S_CC_EOF:
		case S_STR_EOF:
			logger_output_s("error: string occur eof at %s:%d:%d\n", cs->_srcfilename, cs->_line, cs->_col);
			return FALSE;

		case S_DECIMAL_ERROR:
		case S_OCTAL_ERROR:
		case S_HEX_ERROR:
			charbuffer_pushback('\0');
			logger_output_s("error: invalid number syntax at %s:%d:%d, %s\n", cs->_srcfilename, cs->_line, cs->_col, g_cbuffer._ptrdata);
			return FALSE;

		case S_DECIMAL_FLOAT_ERROR:
		case S_HEX_FLOAT_ERROR:
			charbuffer_pushback('\0');
			logger_output_s("error: invalid float syntax at %s:%d:%d, %s\n", cs->_srcfilename, cs->_line, cs->_col, g_cbuffer._ptrdata);
			return FALSE;

			/* terminate */
		case S_END:
			charbuffer_pushback(ch);
			cs_forward(cs);
			/* go through */
		case S_END_STAR:
			charbuffer_pushback('\0');
			tk->_type = action->_type;
			bContinue = FALSE;
			break;
		default:
			charbuffer_pushback(ch);
			cs_forward(cs);
			break;
		}
	} /* end while */

	return cc_lexer_postprocess_token(tk, &g_cbuffer);
}

static BOOL convert_rawstring_to_string(const char* str, int len, FArray* strarray);
static BOOL convert_rawstring_to_widestring(const char* str, int len, FArray* strarray);
static BOOL cc_lexer_postprocess_token(FCCToken* tk, struct tagCharBuffer* charbuf)
{
	int k;
	char* p_end;


	switch (tk->_type)
	{
	case TK_ID:
	{
		for (k = 0; k < sizeof(gCCTokenMetas) / sizeof(gCCTokenMetas[0]); ++k)
		{
			if ((gCCTokenMetas[k]._flags & TK_IS_KEYWORD) && strcmp(gCCTokenMetas[k]._text, charbuf->_ptrdata) == 0)
			{
				tk->_type = gCCTokenMetas[k]._type;
				break;
			}
		} // end for k

		tk->_val._astr._str = hs_hashnstr(charbuf->_ptrdata, charbuf->_itemcnt);
		tk->_val._astr._chcnt = charbuf->_itemcnt;
	}
	break;
	case TK_CONSTANT_INT:
	{
		errno = 0;
		tk->_val._int = strtol(charbuf->_ptrdata, &p_end, 0);
		if (errno == ERANGE)
		{
			logger_output_s("error: int number out of range at %s:%d:%d, %s\n", tk->_loc._filename, tk->_loc._line, tk->_loc._col, charbuf->_ptrdata);
			return FALSE;
		}
	}
	break;
	case TK_CONSTANT_UINT:
	{
		errno = 0;
		tk->_val._uint = strtoul(charbuf->_ptrdata, &p_end, 0);
		if (errno == ERANGE)
		{
			logger_output_s("error: uint number out of range at %s:%d:%d, %s\n", tk->_loc._filename, tk->_loc._line, tk->_loc._col, charbuf->_ptrdata);
			return FALSE;
		}
	}
	break;
	case TK_CONSTANT_LONG:
	{
		errno = 0;
		tk->_val._long = strtol(charbuf->_ptrdata, &p_end, 0);
		if (errno == ERANGE)
		{
			logger_output_s("error: long number out of range at %s:%d:%d, %s\n", tk->_loc._filename, tk->_loc._line, tk->_loc._col, charbuf->_ptrdata);
			return FALSE;
		}
	}
	break;
	case TK_CONSTANT_ULONG:
	{
		errno = 0;
		tk->_val._ulong = strtoul(charbuf->_ptrdata, &p_end, 0);
		if (errno == ERANGE)
		{
			logger_output_s("error: ulong number out of range at %s:%d:%d, %s\n", tk->_loc._filename, tk->_loc._line, tk->_loc._col, charbuf->_ptrdata);
			return FALSE;
		}
	}
	break;
	case TK_CONSTANT_LLONG:
	{
		errno = 0;
		tk->_val._llong = strtoll(charbuf->_ptrdata, &p_end, 0);
		if (errno == ERANGE)
		{
			logger_output_s("error: llong number out of range at %s:%d:%d, %s\n", tk->_loc._filename, tk->_loc._line, tk->_loc._col, charbuf->_ptrdata);
			return FALSE;
		}
	}
	break;
	case TK_CONSTANT_ULLONG:
	{
		errno = 0;
		tk->_val._ullong = strtoull(charbuf->_ptrdata, &p_end, 0);
		if (errno == ERANGE)
		{
			logger_output_s("error: ullong number out of range at %s:%d:%d, %s\n", tk->_loc._filename, tk->_loc._line, tk->_loc._col, charbuf->_ptrdata);
			return FALSE;
		}
	}
	break;
	case TK_CONSTANT_FLOAT:
	{
		errno = 0;
		tk->_val._float = strtof(charbuf->_ptrdata, &p_end);
		if (errno == ERANGE)
		{
			logger_output_s("error: float number out of range at %s:%d:%d, %s\n", tk->_loc._filename, tk->_loc._line, tk->_loc._col, charbuf->_ptrdata);
			return FALSE;
		}
	}
	break;
	case TK_CONSTANT_DOUBLE:
	{
		errno = 0;
		tk->_val._double = strtod(charbuf->_ptrdata, &p_end);
		if (errno == ERANGE)
		{
			logger_output_s("error: double number out of range at %s:%d:%d, %s\n", tk->_loc._filename, tk->_loc._line, tk->_loc._col, charbuf->_ptrdata);
			return FALSE;
		}
	}
	break;
	case TK_CONSTANT_LDOUBLE:
	{
		errno = 0;
		tk->_val._ldouble = strtold(charbuf->_ptrdata, &p_end);
		if (errno == ERANGE)
		{
			logger_output_s("error: long double number out of range at %s:%d:%d, %s\n", tk->_loc._filename, tk->_loc._line, tk->_loc._col, charbuf->_ptrdata);
			return FALSE;
		}
	}
	break;
	case TK_CONSTANT_CHAR:
	{
		BOOL bSuccess = FALSE;
		FArray strarray;

		array_init(&strarray, 0, sizeof(char), CC_MM_TEMPPOOL);
		if (charbuf->_ptrdata[0] == 'L')
		{
			tk->_type = TK_CONSTANT_WCHAR;

			bSuccess = convert_rawstring_to_widestring(charbuf->_ptrdata + 2, charbuf->_itemcnt - 4, &strarray);
			if (!bSuccess)
			{
				logger_output_s("error: convert to wide char failed, at line %s:%d:%d\n", tk->_loc._filename, tk->_loc._line, tk->_loc._col);
				return FALSE;
			}
			if (strarray._elecount == 2) /* '\0''\0' */
			{
				logger_output_s("error: empty char constant, at line %s:%d:%d\n", tk->_loc._filename, tk->_loc._line, tk->_loc._col);
				return FALSE;
			}

			if (strarray._elecount >= 4)
			{
				tk->_val._wch = (((char*)strarray._data)[0] << 8) | ((char*)strarray._data)[1];
			}
			if (strarray._elecount > 4)
			{
				logger_output_s("warning: characters beyond first in wide-character constant ignored, at line %s:%d:%d\n", tk->_loc._filename, tk->_loc._line, tk->_loc._col);
			}
		}
		else
		{
			bSuccess = convert_rawstring_to_string(charbuf->_ptrdata + 1, charbuf->_itemcnt - 3, &strarray);
			if (!bSuccess)
			{
				logger_output_s("error: convert char failed, at line %s:%d:%d\n", tk->_loc._filename, tk->_loc._line, tk->_loc._col);
				return FALSE;
			}
			if (strarray._elecount == 1) /* '\0' */
			{
				logger_output_s("error: empty char constant, at line %s:%d:%d\n", tk->_loc._filename, tk->_loc._line, tk->_loc._col);
				return FALSE;
			}

			if (strarray._elecount == 2) /* 'x''\0' */
			{
				tk->_val._ch = ((char*)strarray._data)[0];
			}
			else if (strarray._elecount <= 5) /* 'xyzw''\0' */
			{
				tk->_type = TK_CONSTANT_INT;
				tk->_val._int = 0;
				for (k = 0; k < strarray._elecount - 1; ++k)
				{
					tk->_val._int <<= 8;
					tk->_val._int += ((char*)strarray._data)[k];
				}
			}
			else
			{
				logger_output_s("error: too many characters in constant, at line %s:%d:%d\n", tk->_loc._filename, tk->_loc._line, tk->_loc._col);
				return FALSE;
			}
		}
	}
	break;
	case TK_CONSTANT_STR:
	{
		BOOL bSuccess = FALSE;
		FArray strarray;

		array_init(&strarray, 0, sizeof(char), CC_MM_TEMPPOOL);
		if (charbuf->_ptrdata[0] == 'L')
		{
			tk->_type = TK_CONSTANT_WSTR;

			bSuccess = convert_rawstring_to_widestring(charbuf->_ptrdata + 2, charbuf->_itemcnt - 4, &strarray);
			if (!bSuccess)
			{
				logger_output_s("error: convert to wide string failed, at line %s:%d:%d\n", tk->_loc._filename, tk->_loc._line, tk->_loc._col);
				return FALSE;
			}
			tk->_val._wstr._str = hs_hashnstr(strarray._data, strarray._elecount);
			tk->_val._wstr._chcnt = strarray._elecount;
		}
		else
		{
			bSuccess = convert_rawstring_to_string(charbuf->_ptrdata + 1, charbuf->_itemcnt - 3, &strarray);
			if (!bSuccess)
			{
				logger_output_s("error: convert to string failed, at line %s:%d:%d\n", tk->_loc._filename, tk->_loc._line, tk->_loc._col);
				return FALSE;
			}
			tk->_val._astr._str = hs_hashnstr(strarray._data, strarray._elecount);
			tk->_val._astr._chcnt = strarray._elecount;
		}
	}
	break;
	default:
		break;
	}

	return TRUE;
}

//////////////////////////////////////////////////////////////////////////
// convert source string
static int32_t asciidigit(int i)
{
	if ('0' <= i && i <= '9')
		i -= '0';
	else if ('a' <= i && i <= 'f')
		i -= 'a' - 10;
	else if ('A' <= i && i <= 'F')
		i -= 'A' - 10;
	else
		i = -1;
	return i;
}

static BOOL convert_rawstring_to_string(const char* str, int InLength, FArray* strarray)
{
	int32_t Value;
	char ch, ch2;

	array_make_cap_enough(strarray, InLength + 1);
	while (InLength > 0)
	{
		const char* SavedStr = str;

		ch = *str++;
		InLength--;

		/* check escape-sequence: */
		if (ch == '\\')
		{
			int32_t i, k;

			if (InLength == 0)
			{
				logger_output_s("error: invalid escape-sequence syntax in string\n");
				return FALSE;
			}

			ch = *str++;
			InLength--;

			if ((i = asciidigit(ch)) >= 0 && i <= 7) /* octal-escape-sequence /000 */
			{
				Value = i;

				for (k = 0; k < 2 && InLength > 0; ++k)
				{
					ch = *str;

					if ((i = asciidigit(ch)) >= 0 && i <= 7)
					{
						Value <<= 3;
						Value += i;

						str++;
						InLength--;
					}
					else
					{
						break; /* not octal number */
					}
				} /* end for k */

				if (Value < 0 || Value > 0xFF)
				{
					logger_output_s("error: octal-escape-sequence is out of range: %d in '%s'\n", Value, SavedStr);
					return FALSE;
				}

				ch2 = Value;
				array_append(strarray, &ch2);
			}
			else if (ch == 'x') /* hexadecimal-escape-sequence */
			{
				if (InLength == 0)
				{
					logger_output_s("error: invalid hexadecimal-escape-sequence syntax in string\n");
					return FALSE;
				}

				ch = *str++;
				InLength--;

				if ((i = asciidigit(ch)) >= 0 && i <= 15)
				{
					Value = i;

					while (InLength > 0)
					{
						ch = *str;

						if ((i = asciidigit(ch)) >= 0 && i <= 15)
						{
							Value <<= 4;
							Value += i;

							str++;
							InLength--;
						}
						else
						{
							break; /* not hexadecimal number */
						}
					} /* end while */

					if (Value < 0 || Value > 0xFF)
					{
						logger_output_s("error: hexadecimal-escape-sequence is out of range: %d in '%s'\n", Value, SavedStr);
						return FALSE;
					}

					ch2 = Value;
					array_append(strarray, &ch2);
				}
				else
				{
					logger_output_s("error: invalid hexadecimal-escape-sequence syntax in string\n");
					return FALSE;
				}
			}
			else /* simple-escape-sequence: one of  \'  \"  \?  \\  \a  \b  \f  \n  \r  \t  \v */
			{
				static char kescape_table[] = "a\ab\bf\fn\nr\rt\tv\v'\'\"\"?\?\\\\";
				char newch = ch;
				for (i = 0; i < sizeof(kescape_table); i += 2) {
					if (ch == kescape_table[i]) {
						newch = kescape_table[i + 1];
						break;
					}
				}
				
				if (i >= sizeof(kescape_table))
				{
					logger_output_s("warning: undefined simple-escape-sequence: \\%c\n", ch);
				}
				array_append(strarray, &newch);
			}
		}
		else
		{
			array_append(strarray, &ch);
		}
	} /* end while */

	ch2 = '\0';
	array_append(strarray, &ch2);
	return TRUE;
}

/* unicode little endian */
static BOOL convert_rawstring_to_widestring(const char* str, int InLength, FArray* strarray)
{
	int32_t Value;
	char ch, ch2;

	array_make_cap_enough(strarray, InLength + 1);
	while (InLength > 0)
	{
		const char* SavedStr = str;

		ch = *str++;
		InLength--;

		/* check escape-sequence: */
		if (ch == '\\')
		{
			int32_t i, k;

			if (InLength == 0)
			{
				logger_output_s("error: invalid escape-sequence syntax in string\n");
				return FALSE;
			}

			ch = *str++;
			InLength--;

			if ((i = asciidigit(ch)) >= 0 && i <= 7) /* octal-escape-sequence /000 */
			{
				Value = i;

				for (k = 0; k < 2 && InLength > 0; ++k)
				{
					ch = *str;

					if ((i = asciidigit(ch)) >= 0 && i <= 7)
					{
						Value <<= 3;
						Value += i;

						str++;
						InLength--;
					}
					else
					{
						break; /* not octal number */
					}
				} // end for k

				if (Value < 0 || Value > 0xFFFF)
				{
					logger_output_s("error: octal-escape-sequence is out of range: %d in '%s'\n", Value, SavedStr);
					return FALSE;
				}

				ch2 = (char)(Value & 0xFF); array_append(strarray, &ch2);
				ch2 = (char)((Value >> 8) & 0xFF); array_append(strarray, &ch2);
			}
			else if (ch == 'x') /* hexadecimal-escape-sequence */
			{
				if (InLength == 0)
				{
					logger_output_s("error: invalid hexadecimal-escape-sequence syntax in string\n");
					return FALSE;
				}

				ch = *str++;
				InLength--;

				if ((i = asciidigit(ch)) >= 0 && i <= 15)
				{
					Value = i;

					while (InLength > 0)
					{
						ch = *str;

						if ((i = asciidigit(ch)) >= 0 && i <= 15)
						{
							Value <<= 4;
							Value += i;

							str++;
							InLength--;
						}
						else
						{
							break; // not hexadecimal number
						}
					} // end while

					if (Value < 0 || Value > 0xFFFF)
					{
						logger_output_s("error: hexadecimal-escape-sequence is out of range: %d in '%s'\n", Value, SavedStr);
						return FALSE;
					}

					ch2 = (char)(Value & 0xFF); array_append(strarray, &ch2);
					ch2 = (char)((Value >> 8) & 0xFF); array_append(strarray, &ch2);
				}
				else
				{
					logger_output_s("error: invalid hexadecimal-escape-sequence syntax in string\n");
					return FALSE;
				}
			}
			else /* simple-escape-sequence: one of  \'  \"  \?  \\  \a  \b  \f  \n  \r  \t  \v */
			{
				static char kescape_table[] = "a\ab\bf\fn\nr\rt\tv\v'\'\"\"?\?\\\\";
				int16_t newch = ch;
				for (i = 0; i < sizeof(kescape_table); i += 2) {
					if (ch == kescape_table[i]) {
						newch = kescape_table[i + 1];
						break;
					}
				}

				if (i >= sizeof(kescape_table))
				{
					logger_output_s("warning: undefined simple-escape-sequence: \\%c\n", ch);
				}

				ch2 = (char)(newch & 0xFF); array_append(strarray, &ch2);
				ch2 = (char)((newch >> 8) & 0xFF); array_append(strarray, &ch2);
			}
		}
		else
		{
			/* check character whether can convert to Unicode. */
			wchar_t wch;
			int32_t nBytes;

			str--; InLength++;
			mbtowc(NULL, NULL, 0); /* reset the conversion state */
			nBytes = mbtowc(&wch, str, InLength);
			if (nBytes > 0)
			{
				ch2 = (char)(wch & 0xFF); array_append(strarray, &ch2);
				ch2 = (char)((wch >> 8) & 0xFF); array_append(strarray, &ch2);

				assert(InLength >= nBytes);
				str += nBytes;
				InLength -= nBytes;
			}
			else
			{
				ch2 = ch; array_append(strarray, &ch2);
				ch2 = '\0'; array_append(strarray, &ch2);
				str++; InLength--;
			}
			
		}
	} /* end while */

	ch2 = '\0';
	array_append(strarray, &ch2);
	array_append(strarray, &ch2);
	return TRUE;
}
