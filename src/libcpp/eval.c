/* \brief
 *		evaluate constant expression
 *
 * a?b:c?d:e  ==>  a?b:(c?d:e)
 */

#include "cpp.h"
#include "logger.h"


enum EAssociative
{
	LTOR, /* left to right */
	RTOL  /* right to left */
};

typedef struct 
{
	int		_priority;
	enum EAssociative	_associative;
} FOpPriorityDesc;

enum EOperator
{
	OP_LPAREN, /* ( */
	OP_RPAREN, /* ) */

	OP_NEG, /* - */
	OP_NOT, /* ! */
	OP_COMPLEMENT, /* ~ */

	OP_DIV, /* / */
	OP_MUL, /* * */
	OP_MOD, /* % */

	OP_ADD, /* + */
	OP_SUB, /* - */

	OP_LSHIFT, /* << */
	OP_RSHIFT, /* >> */

	OP_GREAT, /* > */
	OP_GREAT_EQ, /* >= */
	OP_LESS, /* < */
	OP_LESS_EQ, /* <= */

	OP_EQUAL, /* == */
	OP_UNEQUAL, /* != */

	OP_BITAND, /* & */

	OP_BITXOR, /* ^ */
	OP_BITOR,  /* | */

	OP_AND, /* && */
	OP_OR,  /* || */

	OP_CON_QUESTION, /* ? */
	OP_CON_COLON,	 /* : */

	OP_COMMA, /* , */

	OP_MAX
};

static const FOpPriorityDesc kOpPriorities[] = {
	{ 15, LTOR }, /* ( */
	{ 15, LTOR }, /* ) */

	{ 14, RTOL }, /* - */
	{ 14, RTOL }, /* ! */
	{ 14, RTOL }, /* ~ */

	{ 13, LTOR }, /* / */
	{ 13, LTOR }, /* * */
	{ 13, LTOR }, /* % */

	{ 12, LTOR }, /* + */
	{ 12, LTOR }, /* - */

	{ 11, LTOR }, /* << */
	{ 11, LTOR }, /* >> */

	{ 10, LTOR }, /* > */
	{ 10, LTOR }, /* >= */
	{ 10, LTOR }, /* < */
	{ 10, LTOR }, /* <= */

	{ 9, LTOR }, /* == */
	{ 9, LTOR }, /* != */

	{ 8, LTOR }, /* & */

	{ 7, LTOR }, /* ^ */
	{ 6, LTOR }, /* | */

	{ 5, LTOR }, /* && */
	{ 4, LTOR }, /* || */

	{ 3, RTOL }, /* ? */
	{ 2, LTOR }, /* : */

	{ 1, LTOR }  /* , */
};


static int asciidigit(int i)
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

static BOOL convert_to_integer(const FPPToken* InToken, int64_t* OutResult)
{

	switch (InToken->_type)
	{
	case TK_ID:
		*OutResult = 0; break;
	case TK_CONSTANT_CHAR:
	{
		const char* str = InToken->_str;
		int32_t i;

		if (*str == 'L') {
			str++;
			logger_output_s("error: Wide char constant value undefined, at %s:%d:%d\n", InToken->_loc._filename, InToken->_loc._line, InToken->_loc._col);
			return FALSE;
		}

		*OutResult = 0;

		str++;
		if (*str == '\\')
		{
			str++;
			if ((i = asciidigit(*str)) >= 0 && i <= 7) // octal
			{
				*OutResult = i;
				str++;
				if ((i = asciidigit(*str)) >= 0 && i <= 7)
				{
					*OutResult <<= 3;
					*OutResult += i;
					str++;

					if ((i = asciidigit(*str)) >= 0 && i <= 7)
					{
						*OutResult <<= 3;
						*OutResult += i;
						str++;
					}
				}
			}
			else if (*str == 'x') // hex
			{
				str++;
				while ((i = asciidigit(*str)) >= 0 && i <= 15)
				{
					*OutResult <<= 4;
					*OutResult += i;
					str++;
				}
			}
			else // escaped
			{
				static char cvcon[] = "b\bf\fn\nr\rt\tv\v''\"\"??\\\\";
				int32_t i;

				for (i = 0; i < sizeof(cvcon); i += 2) {
					if (*str == cvcon[i]) {
						*OutResult = cvcon[i + 1];
						break;
					}
				}
				str++;
				if (i >= sizeof(cvcon))
				{
					logger_output_s("error: Undefined escape in character constant, at %s:%d:%d\n", InToken->_loc._filename, InToken->_loc._line, InToken->_loc._col);
					return FALSE;
				}
			}
		}
		else if (*str == '\'')
		{
			logger_output_s("error: Empty character constant, at %s:%d:%d\n", InToken->_loc._filename, InToken->_loc._line, InToken->_loc._col);
			return FALSE;
		}
		else
		{
			*OutResult = *str++;
		}

		if (*str != '\'') {
			logger_output_s("warning: character constant is truncated, at %s:%d:%d\n", InToken->_loc._filename, InToken->_loc._line, InToken->_loc._col);
		}
		else if (*OutResult > 127) {
			logger_output_s("warning: Character constant taken as not signed, at %s:%d:%d\n", InToken->_loc._filename, InToken->_loc._line, InToken->_loc._col);
		}
	}
	break;
	case TK_PPNUMBER:
	{
		const char* str = InToken->_str;
		int32_t base = 10;
		int32_t i;

		*OutResult = 0;
		if (*str == '0') {
			base = 8;
			str++;
			if (*str == 'x' || *str == 'X') {
				base = 16;
				str++;
			}
		}

		for (;; str++) {
			if ((i = asciidigit(*str)) < 0)
				break;
			if (i >= base) {
				logger_output_s("warning: Bad digit in number, at %s:%d:%d\n", InToken->_loc._filename, InToken->_loc._line, InToken->_loc._col);
			}

			*OutResult *= base;
			*OutResult += i;
		} // end for

		for (; *str; str++) {
			if (*str == 'u' || *str == 'U')
				; // unsigned 
			else if (*str == 'l' || *str == 'L')
				; // long
			else {
				logger_output_s("error: Bad number in integer constant expression, at %s:%d:%d\n", InToken->_loc._filename, InToken->_loc._line, InToken->_loc._col);
				return FALSE;
			}
		}
	}
	break;
	default:
		return FALSE;
	}

	return TRUE;
}

/* stack */
typedef struct tagIntegerStack
{
	int64_t* _data;
	int	_max_top;
	int _cur_top;
} FValStack;

typedef struct tagOperatorStack
{
	enum EOperator* _data;
	int	_max_top;
	int _cur_top;
} FOpStack;

#define STACK_TOP(s)		((s)->_data[(s)->_cur_top])
#define STACK_PUSH(s, v)	((s)->_data[++((s)->_cur_top)] = v)
#define STACK_POP(s)		((s)->_data[(s)->_cur_top--])
#define STACK_CHECK(s)		(assert((s)->_cur_top < (s)->_max_top))
#define STACK_ISEMPTY(s)	((s)->_cur_top == -1)


// evaluate operator on the stack
// return TRUE if no error
static BOOL eval_op_stack(FValStack *InOperandStack, FOpStack *InOperatorStack, BOOL* bDoCalculation)
{
	const enum EOperator op = STACK_POP(InOperandStack);
	int64_t result = 0, rhs, lhs, cond;

	*bDoCalculation = TRUE;
	switch (op)
	{
	case OP_LPAREN:
		STACK_PUSH(InOperatorStack, op);
		*bDoCalculation = FALSE;
		break;
	case OP_RPAREN:
	{
		assert(!STACK_ISEMPTY(InOperatorStack));
		while (!STACK_ISEMPTY(InOperatorStack))
		{
			enum EOperator prevOp = STACK_TOP(InOperatorStack);
			BOOL bDoneCalculation = FALSE;
			if (prevOp == OP_LPAREN)
			{
				STACK_POP(InOperatorStack);
				break;
			}

			if (!eval_op_stack(InOperandStack, InOperatorStack, &bDoneCalculation))
			{
				return FALSE;
			}
			if (!bDoneCalculation) { break; }
		} // end while
	}
	break;
	case OP_NEG:
	{
		assert(!STACK_ISEMPTY(InOperandStack));
		rhs = STACK_POP(InOperandStack);
		result = -rhs;
		STACK_PUSH(InOperandStack, result);
	}
	break;
	case OP_NOT:
	{
		assert(!STACK_ISEMPTY(InOperandStack));
		rhs = STACK_POP(InOperandStack);
		result = (rhs == 0) ? 1 : 0;
		STACK_PUSH(InOperandStack, result);
	}
	break;
	case OP_COMPLEMENT:
	{
		assert(!STACK_ISEMPTY(InOperandStack));
		rhs = STACK_POP(InOperandStack);
		result = ~rhs;
		STACK_PUSH(InOperandStack, result);
	}
	break;
	case OP_DIV:
	{
		assert(InOperandStack->_cur_top >= 1);
		rhs = STACK_POP(InOperandStack);
		lhs = STACK_POP(InOperandStack);
		if (rhs == 0) {
			logger_output_s("error: divisor is zero in constant integer expression.\n");
			return FALSE;
		}
		result = lhs / rhs;
		STACK_PUSH(InOperandStack, result);
	}
	break;
	case OP_MUL:
	{
		assert(InOperandStack->_cur_top >= 1);
		rhs = STACK_POP(InOperandStack);
		lhs = STACK_POP(InOperandStack);
		result = lhs * rhs;
		STACK_PUSH(InOperandStack, result);
	}
	break;
	case OP_MOD:
	{
		assert(InOperandStack->_cur_top >= 1);
		rhs = STACK_POP(InOperandStack);
		lhs = STACK_POP(InOperandStack);
		if (rhs == 0) {
			logger_output_s("error: mod is zero in constant integer expression.\n");
			return FALSE;
		}
		result = lhs % rhs;
		STACK_PUSH(InOperandStack, result);
	}
	break;
	case OP_ADD:
	{
		assert(InOperandStack->_cur_top >= 1);
		rhs = STACK_POP(InOperandStack);
		lhs = STACK_POP(InOperandStack);
		result = lhs + rhs;
		STACK_PUSH(InOperandStack, result);
	}
	break;
	case OP_SUB:
	{
		assert(InOperandStack->_cur_top >= 1);
		rhs = STACK_POP(InOperandStack);
		lhs = STACK_POP(InOperandStack);
		result = lhs - rhs;
		STACK_PUSH(InOperandStack, result);
	}
	break;
	case OP_LSHIFT:
	{
		assert(InOperandStack->_cur_top >= 1);
		rhs = STACK_POP(InOperandStack);
		lhs = STACK_POP(InOperandStack);
		result = lhs << rhs;
		STACK_PUSH(InOperandStack, result);
	}
	break;
	case OP_RSHIFT:
	{
		assert(InOperandStack->_cur_top >= 1);
		rhs = STACK_POP(InOperandStack);
		lhs = STACK_POP(InOperandStack);
		result = lhs >> rhs;
		STACK_PUSH(InOperandStack, result);
	}
	break;
	case OP_GREAT:
	{
		assert(InOperandStack->_cur_top >= 1);
		rhs = STACK_POP(InOperandStack);
		lhs = STACK_POP(InOperandStack);
		result = lhs > rhs ? 1 : 0;
		STACK_PUSH(InOperandStack, result);
	}
	break;
	case OP_GREAT_EQ:
	{
		assert(InOperandStack->_cur_top >= 1);
		rhs = STACK_POP(InOperandStack);
		lhs = STACK_POP(InOperandStack);
		result = lhs >= rhs ? 1 : 0;
		STACK_PUSH(InOperandStack, result);
	}
	break;
	case OP_LESS:
	{
		assert(InOperandStack->_cur_top >= 1);
		rhs = STACK_POP(InOperandStack);
		lhs = STACK_POP(InOperandStack);
		result = lhs < rhs ? 1 : 0;
		STACK_PUSH(InOperandStack, result);
	}
	break;
	case OP_LESS_EQ:
	{
		assert(InOperandStack->_cur_top >= 1);
		rhs = STACK_POP(InOperandStack);
		lhs = STACK_POP(InOperandStack);
		result = lhs <= rhs ? 1 : 0;
		STACK_PUSH(InOperandStack, result);
	}
	break;
	case OP_EQUAL:
	{
		assert(InOperandStack->_cur_top >= 1);
		rhs = STACK_POP(InOperandStack);
		lhs = STACK_POP(InOperandStack);
		result = lhs == rhs ? 1 : 0;
		STACK_PUSH(InOperandStack, result);
	}
	break;
	case OP_UNEQUAL:
	{
		assert(InOperandStack->_cur_top >= 1);
		rhs = STACK_POP(InOperandStack);
		lhs = STACK_POP(InOperandStack);
		result = lhs != rhs ? 1 : 0;
		STACK_PUSH(InOperandStack, result);
	}
	break;
	case OP_BITAND:
	{
		assert(InOperandStack->_cur_top >= 1);
		rhs = STACK_POP(InOperandStack);
		lhs = STACK_POP(InOperandStack);
		result = lhs & rhs ? 1 : 0;
		STACK_PUSH(InOperandStack, result);
	}
	break;
	case OP_BITXOR:
	{
		assert(InOperandStack->_cur_top >= 1);
		rhs = STACK_POP(InOperandStack);
		lhs = STACK_POP(InOperandStack);
		result = lhs ^ rhs ? 1 : 0;
		STACK_PUSH(InOperandStack, result);
	}
	break;
	case OP_BITOR:
	{
		assert(InOperandStack->_cur_top >= 1);
		rhs = STACK_POP(InOperandStack);
		lhs = STACK_POP(InOperandStack);
		result = lhs | rhs ? 1 : 0;
		STACK_PUSH(InOperandStack, result);
	}
	break;
	case OP_AND:
	{
		assert(InOperandStack->_cur_top >= 1);
		rhs = STACK_POP(InOperandStack);
		lhs = STACK_POP(InOperandStack);
		result = lhs && rhs ? 1 : 0;
		STACK_PUSH(InOperandStack, result);
	}
	break;
	case OP_OR:
	{
		assert(InOperandStack->_cur_top >= 1);
		rhs = STACK_POP(InOperandStack);
		lhs = STACK_POP(InOperandStack);
		result = (lhs || rhs) ? 1 : 0;
		STACK_PUSH(InOperandStack, result);
	}
	break;
	case OP_CON_QUESTION:
		STACK_PUSH(InOperatorStack, op);
		*bDoCalculation = FALSE;
		break;
	case OP_CON_COLON:
		if (STACK_ISEMPTY(InOperatorStack) || STACK_TOP(InOperatorStack) != OP_CON_QUESTION || InOperandStack->_cur_top < 2)
		{
			logger_output_s("syntax error: unmatched ? : operator...\n");
			return FALSE;
		}
		STACK_POP(InOperatorStack); /* pop operator OP_CON_QUESTION */
		rhs = STACK_POP(InOperandStack);
		lhs = STACK_POP(InOperandStack);
		cond = STACK_POP(InOperandStack);
		result = cond ? lhs : rhs;
		STACK_PUSH(InOperandStack, result);
		break;
	case OP_COMMA:
	{
		assert(InOperandStack->_cur_top >= 1);
		rhs = STACK_POP(InOperandStack);
		lhs = STACK_POP(InOperandStack);
		result = rhs;
		STACK_PUSH(InOperandStack, result);
	}
	break;
	default:
		assert(0);
		return FALSE;
	}

	return TRUE;
}

static int calc_tokens_count(FTKListNode* tklist)
{
	int n = 0;

	while (tklist)
	{
		n++;
		tklist = tklist->_next;
	}

	return n;
}

/* 计算中缀表达式算法 */
BOOL cpp_eval_constexpr(FCppContext* ctx, FTKListNode* tklist, int* result)
{
	FSourceCodeContext* top = ctx->_sourcestack;
	BOOL bSyntaxError = FALSE;
	BOOL bPrevHasOperand = FALSE;

	FValStack OperandStack;
	FOpStack OperatorStack;
	FLocation loc = tklist->_tk._loc;

	int maxcount = calc_tokens_count(tklist);
	if (maxcount <= 0) { return FALSE; }

	OperandStack._data = mm_alloc_area(sizeof(int64_t) * maxcount, CPP_MM_TEMPPOOL);
	OperandStack._max_top = maxcount;
	OperandStack._cur_top = -1;

	OperatorStack._data = mm_alloc_area(sizeof(enum EOperator)* maxcount, CPP_MM_TEMPPOOL);
	OperatorStack._max_top = maxcount;
	OperatorStack._cur_top = -1;

	for (; tklist; tklist = tklist->_next)
	{
		enum EOperator op = OP_MAX;
		int64_t operand = 0;

		/* convert token */
		switch (tklist->_tk._type)
		{
		case TK_NEWLINE:
		case TK_EOF:
			continue;

		case TK_ID:
		case TK_CONSTANT_CHAR:
		case TK_PPNUMBER:
			bSyntaxError = !convert_to_integer(&(tklist->_tk), &operand) || bPrevHasOperand;
			STACK_PUSH(&OperandStack, operand);
			bPrevHasOperand = TRUE;
			break;
		case TK_LPAREN:
			op = OP_LPAREN;
			if (bPrevHasOperand) { bSyntaxError = TRUE; }
			break;
		case TK_RPAREN:
			op = OP_RPAREN;
			if (!bPrevHasOperand) { bSyntaxError = TRUE; }
			break;
		case TK_SUB:
			op = bPrevHasOperand ? OP_SUB : OP_NEG;
			if (op == OP_SUB) {
				bPrevHasOperand = FALSE;
			}
			break;
		case TK_ADD:
			op = OP_ADD;
			if (!bPrevHasOperand) { /* + sign */
				continue;
			}
			break;
		case TK_NOT:
			op = OP_NOT;
			if (bPrevHasOperand) { bSyntaxError = TRUE; }
			break;
		case TK_COMP:
			op = OP_COMPLEMENT;
			if (bPrevHasOperand) { bSyntaxError = TRUE; }
			break;

		case TK_DIV:
			op = OP_DIV;
			if (!bPrevHasOperand) { bSyntaxError = TRUE; }
			bPrevHasOperand = FALSE;
			break;
		case TK_MUL:
			op = OP_MUL;
			if (!bPrevHasOperand) { bSyntaxError = TRUE; }
			bPrevHasOperand = FALSE;
			break;
		case TK_MOD:
			op = OP_MOD;
			if (!bPrevHasOperand) { bSyntaxError = TRUE; }
			bPrevHasOperand = FALSE;
			break;
		case TK_LSHIFT:
			op = OP_LSHIFT;
			if (!bPrevHasOperand) { bSyntaxError = TRUE; }
			bPrevHasOperand = FALSE;
			break;
		case TK_RSHIFT:
			op = OP_RSHIFT;
			if (!bPrevHasOperand) { bSyntaxError = TRUE; }
			bPrevHasOperand = FALSE;
			break;
		case TK_GREAT:
			op = OP_GREAT;
			if (!bPrevHasOperand) { bSyntaxError = TRUE; }
			bPrevHasOperand = FALSE;
			break;
		case TK_GREAT_EQ:
			op = OP_GREAT_EQ;
			if (!bPrevHasOperand) { bSyntaxError = TRUE; }
			bPrevHasOperand = FALSE;
			break;
		case TK_LESS:
			op = OP_LESS;
			if (!bPrevHasOperand) { bSyntaxError = TRUE; }
			bPrevHasOperand = FALSE;
			break;
		case TK_LESS_EQ:
			op = OP_LESS_EQ;
			if (!bPrevHasOperand) { bSyntaxError = TRUE; }
			bPrevHasOperand = FALSE;
			break;
		case TK_EQUAL:
			op = OP_EQUAL;
			if (!bPrevHasOperand) { bSyntaxError = TRUE; }
			bPrevHasOperand = FALSE;
			break;
		case TK_UNEQUAL:
			op = OP_UNEQUAL;
			if (!bPrevHasOperand) { bSyntaxError = TRUE; }
			bPrevHasOperand = FALSE;
			break;
		case TK_BITAND:
			op = OP_BITAND;
			if (!bPrevHasOperand) { bSyntaxError = TRUE; }
			bPrevHasOperand = FALSE;
			break;
		case TK_BITXOR:
			op = OP_BITOR;
			if (!bPrevHasOperand) { bSyntaxError = TRUE; }
			bPrevHasOperand = FALSE;
			break;
		case TK_BITOR:
			op = OP_BITOR;
			if (!bPrevHasOperand) { bSyntaxError = TRUE; }
			bPrevHasOperand = FALSE;
			break;
		case TK_AND:
			op = OP_AND;
			if (!bPrevHasOperand) { bSyntaxError = TRUE; }
			bPrevHasOperand = FALSE;
			break;
		case TK_OR:
			op = OP_OR;
			if (!bPrevHasOperand) { bSyntaxError = TRUE; }
			bPrevHasOperand = FALSE;
			break;
		case TK_QUESTION:
			op = OP_CON_QUESTION;
			if (!bPrevHasOperand) { bSyntaxError = TRUE; }
			bPrevHasOperand = FALSE;
			break;
		case TK_COLON:
			op = OP_CON_COLON;
			if (!bPrevHasOperand) { bSyntaxError = TRUE; }
			bPrevHasOperand = FALSE;
			break;
		case TK_COMMA:
			op = OP_COMMA;
			if (!bPrevHasOperand) { bSyntaxError = TRUE; }
			bPrevHasOperand = FALSE;
			break;
		default:
			bSyntaxError = TRUE;
			break;
		}
		if (bSyntaxError) {
			break;
		}

		// current is operator
		if (op != OP_MAX)
		{
			while (!STACK_ISEMPTY(&OperatorStack) &&
				   ((kOpPriorities[op]._priority < kOpPriorities[STACK_TOP(&OperatorStack)]._priority) ||
					(kOpPriorities[op]._priority == kOpPriorities[STACK_TOP(&OperatorStack)]._priority && kOpPriorities[op]._associative == LTOR))
				)
			{
				BOOL bDoneCalculation = FALSE;
				if (!eval_op_stack(&OperandStack, &OperatorStack, &bDoneCalculation))
				{
					bSyntaxError = TRUE;
					break;
				}
				if (!bDoneCalculation)
				{
					break;
				}
			} /* end while */

			STACK_PUSH(&OperatorStack, op);
		}

		if (bSyntaxError) {
			break;
		}
	} //end for tkIndex

	if (!bPrevHasOperand) {
		bSyntaxError = TRUE;
	}

	while (!bSyntaxError && !STACK_ISEMPTY(&OperatorStack))
	{
		BOOL bDoneCalculation = FALSE;
		if (!eval_op_stack(&OperandStack, &OperatorStack, &bDoneCalculation))
		{
			bSyntaxError = TRUE;
			break;
		}
		if (!bDoneCalculation)
		{
			break;
		}
	} // end while

	if (bSyntaxError || OperandStack._cur_top != 0 || !STACK_ISEMPTY(&OperatorStack))
	{
		logger_output_s("error: integer constant expression syntax invalid, in file: %s:%d\n", loc._filename, loc._line);
		return FALSE;
	}

	*result = (int32_t)STACK_TOP(&OperandStack);
	return TRUE;
}
