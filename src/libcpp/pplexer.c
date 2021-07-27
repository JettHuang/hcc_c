/* \brief
 *		lexer for preprocessor
 */

#include "pplexer.h"
#include "logger.h"
#include "hstring.h"
#include "mm.h"

#include <string.h>
#include <assert.h>



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

	/* pp-number */
	S_PPNUMBER,
	S_PPNUMBER_EXP,

	/* header name */
	S_HCHAR_SEQ,
	S_HCHAR_SEQ_NEWLINE,
	S_HCHAR_SEQ_EOF,

	S_QCHAR_SEQ,
	S_QCHAR_SEQ_NEWLINE,
	S_QCHAR_SEQ_EOF,

	S_MAX,
	S_END,   /* end of state-machine */
	S_END_STAR /* end of state-machine without eat current char */
};

/* action indicators */
#define ACT_KEEPSTATE		0x00
#define ACT_NEWSTATE		0x01
#define ACT_OLDSTATE		0x02

/* character classes */
#define	C_ALPH	1
#define	C_NUM	2
#define	C_XX	3
#define C_EOF	((unsigned char)(EOF))

#define KCHAR_COUNT			256


#define ACTION_DISCHK_LINEFOLDING	0x01 // disable line-folding checking

typedef struct tagFSMAction
{
	enum EState		_newstate;
	enum EPPToken	_type;
	uint8_t			_flags;
} FAction;

#define MAKE_ACTION(S, T)			{ S, T, 0 }
#define MAKE_ACTION_FLAGS(S, T, F)	{ S, T, F }

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
	{ S_START, { C_EOF },  MAKE_ACTION(S_END_STAR, TK_EOF)},
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
	{ S_START, { C_NUM }, MAKE_ACTION(S_PPNUMBER, TK_NONE)},


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
	{ S_CC, { '\\'}, MAKE_ACTION_FLAGS(S_CC_ESCAPE, TK_NONE, ACTION_DISCHK_LINEFOLDING) },
	{ S_CC, { '\n'}, MAKE_ACTION(S_CC_NEWLINE, TK_NONE) },
	{ S_CC, { C_EOF }, MAKE_ACTION(S_CC_EOF, TK_NONE) },

	{ S_CC_ESCAPE, { C_XX }, MAKE_ACTION(S_CC, TK_NONE) },
	{ S_CC_ESCAPE, { '\n'}, MAKE_ACTION(S_CC_NEWLINE, TK_NONE) },
	{ S_CC_ESCAPE, { C_EOF }, MAKE_ACTION(S_CC_EOF, TK_NONE) },

	/* string-literal */
	{ S_STR, { C_XX }, MAKE_ACTION(S_SELF, TK_NONE) },
	{ S_STR, { '\"'}, MAKE_ACTION(S_END, TK_CONSTANT_STR) },
	{ S_STR, { '\\'}, MAKE_ACTION_FLAGS(S_STR_ESCAPE, TK_NONE, ACTION_DISCHK_LINEFOLDING) },
	{ S_STR, { '\n'}, MAKE_ACTION(S_STR_NEWLINE, TK_NONE) },
	{ S_STR, { C_EOF }, MAKE_ACTION(S_STR_EOF, TK_NONE) },

	{ S_STR_ESCAPE, { C_XX }, MAKE_ACTION(S_STR, TK_NONE) },
	{ S_STR_ESCAPE, { '\n'}, MAKE_ACTION(S_STR_NEWLINE, TK_NONE) },
	{ S_STR_ESCAPE, { C_EOF }, MAKE_ACTION(S_STR_EOF, TK_NONE) },

	/* punctuator */
	{ S_DOT1, { C_XX },  MAKE_ACTION(S_END_STAR, TK_DOT)  },
	{ S_DOT1, { '.' },   MAKE_ACTION(S_DOT2, TK_NONE) },
	{ S_DOT1, { C_NUM},  MAKE_ACTION(S_PPNUMBER, TK_NONE) },
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

	/* header-name */
	{ S_HCHAR_SEQ, { C_XX }, MAKE_ACTION(S_SELF, TK_NONE) },
	{ S_HCHAR_SEQ, { '>'}, MAKE_ACTION(S_END, TK_HEADER_NAME) },
	{ S_HCHAR_SEQ, { '\n'}, MAKE_ACTION(S_HCHAR_SEQ_NEWLINE, TK_NONE) },
	{ S_HCHAR_SEQ, { C_EOF }, MAKE_ACTION(S_HCHAR_SEQ_EOF, TK_NONE) },

	{ S_QCHAR_SEQ, { C_XX }, MAKE_ACTION(S_SELF, TK_NONE) },
	{ S_QCHAR_SEQ, { '\"'}, MAKE_ACTION(S_END, TK_HEADER_NAME) },
	{ S_QCHAR_SEQ, { '\n'}, MAKE_ACTION(S_QCHAR_SEQ_NEWLINE, TK_NONE) },
	{ S_QCHAR_SEQ, { C_EOF }, MAKE_ACTION(S_QCHAR_SEQ_EOF, TK_NONE) },

	/* pp-number */
	{ S_PPNUMBER, { C_XX }, MAKE_ACTION(S_END_STAR, TK_PPNUMBER) },
	{ S_PPNUMBER, { C_NUM, C_ALPH, '.'}, MAKE_ACTION(S_SELF, TK_NONE) },
	{ S_PPNUMBER, { 'e', 'E', 'p', 'P'}, MAKE_ACTION(S_PPNUMBER_EXP, TK_NONE) },
	{ S_PPNUMBER_EXP, { C_XX }, MAKE_ACTION(S_END_STAR, TK_PPNUMBER) },
	{ S_PPNUMBER_EXP, { '+', '-' }, MAKE_ACTION(S_PPNUMBER, TK_NONE) },

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

void cpp_lexer_init()
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

void cpp_lexer_uninit()
{
	charbuffer_release();
}

static int trigraph(FCharStream* cs)
{
	char ch1, ch2, newch;

	assert(cs_current(cs) == '?');
	ch1 = cs_lookahead(cs, 1);
	ch2 = cs_lookahead(cs, 2);
	//??= #  ??( [  ??< {
	//??/ \  ??) ]  ??> }
	//??' ^  ??! |  ??- ~
	if (ch1 != '?') { return 0; }

	newch = 0;
	switch (ch2)
	{
	case '=':
		newch = '#'; break;
	case '(':
		newch = '['; break;
	case '<':
		newch = '{'; break;
	case '/':
		newch = '\\'; break;
	case ')':
		newch = ']'; break;
	case '>':
		newch = '}'; break;
	case '\'':
		newch = '^'; break;
	case '!':
		newch = '|'; break;
	case '-':
		newch = '~'; break;
	default:
		break;
	}

	if (newch)
	{
		cs_forward(cs);
		cs_forward(cs);
		cs_replace_current(cs, newch);
		return 1;
	}

	return 0;
}

int cpp_lexer_read_token(FCharStream* cs, int bwantheader, FPPToken *tk)
{
	enum EState currstate = S_START;
	int bCheckLinefolding = 1;

	tk->_type = TK_UNCLASS;
	tk->_str = NULL;
	tk->_wscnt = 0;
	tk->_hidesetid = -1;

#if 0 /* #include <> "" parsing */
	if (bwantheader)
	{
		static FAction action0 = MAKE_ACTION(S_HCHAR_SEQ, TK_NONE);
		static FAction action1 = MAKE_ACTION(S_QCHAR_SEQ, TK_NONE);

		g_lexerfsm[currstate]['<'] = action0;
		g_lexerfsm[currstate]['\"'] = action1;
	}
	else
	{
		static FAction action0 = MAKE_ACTION(S_LESS, TK_NONE);
		static FAction action1 = MAKE_ACTION(S_STR, TK_NONE);

		g_lexerfsm[currstate]['<'] = action0;
		g_lexerfsm[currstate]['\"'] = action1;
	}
#endif

	charbuffer_empty();
	while (1)
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
		if (ch == '?' && trigraph(cs)) {
			ch = cs_current(cs); /* re-read */
		}
		else if (bCheckLinefolding && ch == '\\' && cs_lookahead(cs, 1) == '\n') {
			/* line-folding */
			cs_forward(cs); 
			cs_forward(cs);
			continue;
		}

		action = &g_lexerfsm[currstate][(unsigned char)ch];
		if (action->_newstate)
		{
			currstate = action->_newstate;
			bCheckLinefolding = !(action->_flags & ACTION_DISCHK_LINEFOLDING);
		}

		switch (currstate)
		{
		case S_WS:
			tk->_wscnt++;
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
			logger_output_s("error: comment occur eof at line %d:%d\n", cs->_line, cs->_col);
			return 0;

		case S_CC_NEWLINE:
		case S_STR_NEWLINE:
			logger_output_s("error: string occur newline at line %d:%d\n", cs->_line, cs->_col);
			return 0;

		case S_CC_EOF:
		case S_STR_EOF:
			logger_output_s("error: string occur eof at line %d:%d\n", cs->_line, cs->_col);
			return 0;

		case S_HCHAR_SEQ_NEWLINE:
		case S_QCHAR_SEQ_NEWLINE:
			logger_output_s("error: header name occur newline at line %d:%d\n", cs->_line, cs->_col);
			return 0;

		case S_HCHAR_SEQ_EOF:
		case S_QCHAR_SEQ_EOF:
			logger_output_s("error: header name occur eof at line %d:%d\n", cs->_line, cs->_col);
			return 0;

			/* terminate */
		case S_END:
			charbuffer_pushback(ch);
			cs_forward(cs);
			/* go through */
		case S_END_STAR:
			charbuffer_pushback('\0');
			tk->_type = action->_type;
			tk->_str = hs_hashnstr(g_cbuffer._ptrdata, g_cbuffer._itemcnt);
			return 1;
		default:
			charbuffer_pushback(ch);
			cs_forward(cs);
			break;
		}
	} /* end while */

	assert(0 && "can't execute to here");
	return 0;
}
