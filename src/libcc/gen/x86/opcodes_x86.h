/* \brief
 *  x86 assembly code set 
 */

#ifndef __CC_OPCODES_X86_H__
#define __CC_OPCODES_X86_H__


/* assembly instructions */
enum X86ASMCode
{
    X86_INVALID,

    /* virtual instructions */
    X86_BLKENTER,
    X86_BLKLEAVE,
    X86_LOCALVAR,
    X86_MOVERET,
    X86_LOAD, /* load memory to register */

    X86_LABEL,

    X86_OR,
    X86_XOR,
    X86_AND,
    X86_LSH,
    X86_RSH,
    X86_ADD,
    X86_SUB,
    X86_MUL,
    X86_DIV,
    X86_MOD,
    X86_NEG,
    X86_NOT,

    X86_JZ,
    X86_JNZ,
    X86_JE,
    X86_JNE,
    X86_JG,
    X86_JL,
    X86_JGE,
    X86_JLE,
    X86_JMP,

    /* convert */
    X86_CVT,
    X86_ADDR,
    X86_MOV,

    X86_PUSH,
	X86_CALL,

    X86_EXPAND_STACK,
    X86_REDUCE_STACK,

	X86_PROLOGUE,
    X86_EPILOGUE,

    /* set memory to zero */
    X86_ZERO_M,

    /* x86 float */
    X87_LD,
    X87_ST,
    X87_STP,
    X87_POP
};

#endif /* __CC_OPCODES_X86_H__ */
