/* 
* tokens
* chapter 6.4 in n1124_c99_specs_final.pdf
*
* token:
*    keyword
*    identifier
*    constant
*    string-literal
*    punctuator
*
* preprocessing-token:
*    header-name
*    identifier
*    pp-number
*    character-constant
*    string-literal
*    punctuator
*    each non-white-space character that cannot be one of the above
*/

#define TK_IS_KEYWORD				0x0001
#define TK_IS_RESERVE				0x0002
#define TK_IS_PUNCTUATOR			0x0004
#define TK_IS_STORAGE_SPECIFIER		0x0008
#define TK_IS_TYPE_SPECIFIER		0x0010
#define TK_IS_TYPE_QUALIFIER		0x0020
#define TK_IS_FUNC_SPECIFIER		0x0040
#define TK_IS_CONSTEXPR				0x0080
#define TK_IS_DECL_SPECIFIER		(TK_IS_STORAGE_SPECIFIER | TK_IS_TYPE_SPECIFIER | TK_IS_TYPE_QUALIFIER | TK_IS_FUNC_SPECIFIER)
#define TK_IS_STATEMENT				0x0100
#define TK_IS_ASSIGN				0x0200


#ifndef TOKEN
#error "You must define TOKEN macro before include this file"
#endif

TOKEN(TK_NONE, "none", 0)
/* keywords */
TOKEN(TK_AUTO, "auto", TK_IS_KEYWORD | TK_IS_STORAGE_SPECIFIER)
TOKEN(TK_ENUM, "enum", TK_IS_KEYWORD | TK_IS_TYPE_SPECIFIER)
TOKEN(TK_RESTRICT, "restrict", TK_IS_KEYWORD | TK_IS_TYPE_QUALIFIER)
TOKEN(TK_UNSIGNED, "unsigned", TK_IS_KEYWORD | TK_IS_TYPE_SPECIFIER)
TOKEN(TK_BREAK, "break", TK_IS_KEYWORD | TK_IS_STATEMENT)
TOKEN(TK_EXTERN, "extern", TK_IS_KEYWORD | TK_IS_STORAGE_SPECIFIER)
TOKEN(TK_RETURN, "return", TK_IS_KEYWORD | TK_IS_STATEMENT)
TOKEN(TK_VOID, "void", TK_IS_KEYWORD | TK_IS_TYPE_SPECIFIER)
TOKEN(TK_CASE, "case", TK_IS_KEYWORD | TK_IS_STATEMENT)
TOKEN(TK_FLOAT, "float", TK_IS_KEYWORD | TK_IS_TYPE_SPECIFIER)
TOKEN(TK_SHORT, "short", TK_IS_KEYWORD | TK_IS_TYPE_SPECIFIER)
TOKEN(TK_VOLATILE, "volatile", TK_IS_KEYWORD | TK_IS_TYPE_QUALIFIER)
TOKEN(TK_CHAR, "char", TK_IS_KEYWORD | TK_IS_TYPE_SPECIFIER)
TOKEN(TK_FOR, "for", TK_IS_KEYWORD | TK_IS_STATEMENT)
TOKEN(TK_SIGNED, "signed", TK_IS_KEYWORD | TK_IS_TYPE_SPECIFIER)
TOKEN(TK_WHILE, "while", TK_IS_KEYWORD | TK_IS_STATEMENT)
TOKEN(TK_CONST, "const", TK_IS_KEYWORD | TK_IS_TYPE_QUALIFIER)
TOKEN(TK_GOTO, "goto", TK_IS_KEYWORD | TK_IS_STATEMENT)
TOKEN(TK_SIZEOF, "sizeof", TK_IS_KEYWORD | TK_IS_CONSTEXPR)
TOKEN(TK_CONTINUE, "continue", TK_IS_KEYWORD | TK_IS_STATEMENT)
TOKEN(TK_IF, "if", TK_IS_KEYWORD | TK_IS_STATEMENT)
TOKEN(TK_STATIC, "static", TK_IS_KEYWORD | TK_IS_STORAGE_SPECIFIER)
TOKEN(TK_DEFAULT, "default", TK_IS_KEYWORD | TK_IS_STATEMENT)
TOKEN(TK_INLINE, "inline", TK_IS_KEYWORD | TK_IS_FUNC_SPECIFIER)
TOKEN(TK_STRUCT, "struct", TK_IS_KEYWORD | TK_IS_TYPE_SPECIFIER)
TOKEN(TK_DO, "do", TK_IS_KEYWORD | TK_IS_STATEMENT)
TOKEN(TK_INT, "int", TK_IS_KEYWORD | TK_IS_TYPE_SPECIFIER)
TOKEN(TK_SWITCH, "switch", TK_IS_KEYWORD | TK_IS_STATEMENT)
TOKEN(TK_DOUBLE, "double", TK_IS_KEYWORD | TK_IS_TYPE_SPECIFIER)
TOKEN(TK_LONG, "long", TK_IS_KEYWORD | TK_IS_TYPE_SPECIFIER)
TOKEN(TK_TYPEDEF, "typedef", TK_IS_KEYWORD | TK_IS_STORAGE_SPECIFIER)
TOKEN(TK_ELSE, "else", TK_IS_KEYWORD | TK_IS_STATEMENT)
TOKEN(TK_REGISTER, "register", TK_IS_KEYWORD | TK_IS_STORAGE_SPECIFIER)
TOKEN(TK_UNION, "union", TK_IS_KEYWORD | TK_IS_TYPE_SPECIFIER)

/* identifier */
TOKEN(TK_ID, "identifier", TK_IS_CONSTEXPR)

/* constant */
TOKEN(TK_CONSTANT_INT, "constant-int", TK_IS_CONSTEXPR)
TOKEN(TK_CONSTANT_UINT, "constant-unsigned-int", TK_IS_CONSTEXPR)
TOKEN(TK_CONSTANT_LONG, "constant-long", TK_IS_CONSTEXPR)
TOKEN(TK_CONSTANT_ULONG, "constant-unsigned-long", TK_IS_CONSTEXPR)
TOKEN(TK_CONSTANT_LLONG, "constant-long-long", TK_IS_CONSTEXPR)
TOKEN(TK_CONSTANT_ULLONG, "constant-unsigned-long-long", TK_IS_CONSTEXPR)
TOKEN(TK_CONSTANT_FLOAT, "constant-float", TK_IS_CONSTEXPR)
TOKEN(TK_CONSTANT_DOUBLE, "constant-double", TK_IS_CONSTEXPR)
TOKEN(TK_CONSTANT_LDOUBLE, "constant-long-double", TK_IS_CONSTEXPR)
TOKEN(TK_CONSTANT_CHAR, "constant-character", TK_IS_CONSTEXPR)
TOKEN(TK_CONSTANT_WCHAR, "constant-wide-character", TK_IS_CONSTEXPR)

/* string literal */
TOKEN(TK_CONSTANT_STR, "string-literal", 0)
TOKEN(TK_CONSTANT_WSTR, "string-wide-literal", 0)

/* punctuator */
TOKEN(TK_LBRACKET,      "[", TK_IS_PUNCTUATOR)
TOKEN(TK_RBRACKET,      "]", TK_IS_PUNCTUATOR)
TOKEN(TK_LPAREN,        "(", TK_IS_PUNCTUATOR | TK_IS_CONSTEXPR)
TOKEN(TK_RPAREN,        ")", TK_IS_PUNCTUATOR)
TOKEN(TK_LBRACE,        "{", TK_IS_PUNCTUATOR)
TOKEN(TK_RBRACE,        "}", TK_IS_PUNCTUATOR)
TOKEN(TK_DOT,           ".", TK_IS_PUNCTUATOR)
TOKEN(TK_POINTER,       "->", TK_IS_PUNCTUATOR)
TOKEN(TK_INC,           "++", TK_IS_PUNCTUATOR | TK_IS_CONSTEXPR)
TOKEN(TK_DEC,           "--", TK_IS_PUNCTUATOR | TK_IS_CONSTEXPR)
TOKEN(TK_BITAND,        "&", TK_IS_PUNCTUATOR | TK_IS_CONSTEXPR)
TOKEN(TK_MUL,           "*", TK_IS_PUNCTUATOR | TK_IS_CONSTEXPR)
TOKEN(TK_ADD,           "+", TK_IS_PUNCTUATOR | TK_IS_CONSTEXPR)
TOKEN(TK_SUB,           "-", TK_IS_PUNCTUATOR | TK_IS_CONSTEXPR)
TOKEN(TK_COMP,          "~", TK_IS_PUNCTUATOR | TK_IS_CONSTEXPR)
TOKEN(TK_NOT,           "!", TK_IS_PUNCTUATOR | TK_IS_CONSTEXPR)
TOKEN(TK_DIV,           "/", TK_IS_PUNCTUATOR)
TOKEN(TK_MOD,           "%", TK_IS_PUNCTUATOR)
TOKEN(TK_LSHIFT,        "<<", TK_IS_PUNCTUATOR)
TOKEN(TK_RSHIFT,        ">>", TK_IS_PUNCTUATOR)
TOKEN(TK_LESS,          "<", TK_IS_PUNCTUATOR)
TOKEN(TK_GREAT,         ">", TK_IS_PUNCTUATOR)
TOKEN(TK_LESS_EQ,       "<=", TK_IS_PUNCTUATOR)
TOKEN(TK_GREAT_EQ,      ">=", TK_IS_PUNCTUATOR)
TOKEN(TK_EQUAL,         "==", TK_IS_PUNCTUATOR)
TOKEN(TK_UNEQUAL,       "!=", TK_IS_PUNCTUATOR)
TOKEN(TK_BITXOR,        "^", TK_IS_PUNCTUATOR)
TOKEN(TK_BITOR,         "|", TK_IS_PUNCTUATOR)
TOKEN(TK_AND,           "&&", TK_IS_PUNCTUATOR)
TOKEN(TK_OR,            "||", TK_IS_PUNCTUATOR)
TOKEN(TK_QUESTION,      "?", TK_IS_PUNCTUATOR)
TOKEN(TK_COLON,         ":", TK_IS_PUNCTUATOR)
TOKEN(TK_SEMICOLON,     ";", TK_IS_PUNCTUATOR)
TOKEN(TK_ELLIPSIS,      "...", TK_IS_PUNCTUATOR)
TOKEN(TK_ASSIGN,        "=", TK_IS_PUNCTUATOR | TK_IS_ASSIGN)
TOKEN(TK_MUL_ASSIGN,    "*=", TK_IS_PUNCTUATOR | TK_IS_ASSIGN)
TOKEN(TK_DIV_ASSIGN,    "/=", TK_IS_PUNCTUATOR | TK_IS_ASSIGN)
TOKEN(TK_MOD_ASSIGN,    "%=", TK_IS_PUNCTUATOR | TK_IS_ASSIGN)
TOKEN(TK_ADD_ASSIGN,    "+=", TK_IS_PUNCTUATOR | TK_IS_ASSIGN)
TOKEN(TK_SUB_ASSIGN,    "-=", TK_IS_PUNCTUATOR | TK_IS_ASSIGN)
TOKEN(TK_LSHIFT_ASSIGN, "<<=", TK_IS_PUNCTUATOR | TK_IS_ASSIGN)
TOKEN(TK_RSHIFT_ASSIGN, ">>=", TK_IS_PUNCTUATOR | TK_IS_ASSIGN)
TOKEN(TK_BITAND_ASSIGN, "&=", TK_IS_PUNCTUATOR | TK_IS_ASSIGN)
TOKEN(TK_BITXOR_ASSIGN, "^=", TK_IS_PUNCTUATOR | TK_IS_ASSIGN)
TOKEN(TK_BITOR_ASSIGN,  "|=", TK_IS_PUNCTUATOR | TK_IS_ASSIGN)
TOKEN(TK_COMMA,         ",", TK_IS_PUNCTUATOR)
TOKEN(TK_POUND,         "#", TK_IS_PUNCTUATOR)
TOKEN(TK_DOUBLE_POUND,  "##", TK_IS_PUNCTUATOR)
TOKEN(TK_UNCLASS,		"unclass", 0)

/* for preprocessing-token */
TOKEN(TK_HEADER_NAME, "header-name", 0)
TOKEN(TK_PPNUMBER, "pp-number", 0)
TOKEN(TK_NEWLINE,  "new line", 0)
TOKEN(TK_EOF,      "end of file", 0)
