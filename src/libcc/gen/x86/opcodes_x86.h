/* \brief
 *  x86 assembly code set 
 */

#ifndef __CC_OPCODES_X86_H__
#define __CC_OPCODES_X86_H__


/* assembly instructions */
#define ASM_INSTRUCT(op, str)	op,
enum X86ASMCode
{
#include "opcodes_x86.inl"
};
#undef ASM_INSTRUCT

#endif /* __CC_OPCODES_X86_H__ */
