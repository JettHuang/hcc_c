/* \brief
 *		template for x86 assembly instructions.
 *  %0  dst
 *  %1  src
 *  %2  
 */

#define ASM_INSTRUCT

#ifndef ASM_INSTRUCT
#error "You must define ASM_INSTRUCT before including this file."
#endif

ASM_INSTRUCT(X86_OR_I4RR, "or %0, %1 \n")
ASM_INSTRUCT(X86_OR_I4RI, "or %0, %1 \n")
ASM_INSTRUCT(X86_OR_I4RM, "or %0, dword ptr [%1] \n")
ASM_INSTRUCT(X86_OR_U4RI, "or %0, %1 \n")
ASM_INSTRUCT(X86_OR_U4RR, "or %0, %1 \n")
ASM_INSTRUCT(X86_OR_U4RM, "or %0, dword ptr [%1] \n")

ASM_INSTRUCT(X86_XOR_I4RR, "xor %0, %1 \n")
ASM_INSTRUCT(X86_XOR_I4RI, "xor %0, %1 \n")
ASM_INSTRUCT(X86_XOR_I4RM, "xor %0, dword ptr [%1] \n")
ASM_INSTRUCT(X86_XOR_U4RR, "xor %0, %1 \n")
ASM_INSTRUCT(X86_XOR_U4RI, "xor %0, %1 \n")
ASM_INSTRUCT(X86_XOR_U4RM, "xor %0, dword ptr [%1] \n")

ASM_INSTRUCT(X86_AND_I4RR, "and %0, %1 \n")
ASM_INSTRUCT(X86_AND_I4RI, "and %0, %1 \n")
ASM_INSTRUCT(X86_AND_I4RM, "and %0, dword ptr [%1] \n")
ASM_INSTRUCT(X86_AND_U4RR, "and %0, %1 \n")
ASM_INSTRUCT(X86_AND_U4RI, "and %0, %1 \n")
ASM_INSTRUCT(X86_AND_U4RM, "and %0, dword ptr [%1] \n")

ASM_INSTRUCT(X86_LSH_I4RR, "shl %0, cl \n")
ASM_INSTRUCT(X86_LSH_I4RI, "shl %0, %1 \n")
ASM_INSTRUCT(X86_LSH_U4RR, "shl %0, cl \n")
ASM_INSTRUCT(X86_LSH_U4RI, "shl %0, %1 \n")

ASM_INSTRUCT(X86_RSH_I4RR, "sar %0, cl \n")
ASM_INSTRUCT(X86_RSH_I4RI, "sar %0, %1 \n")
ASM_INSTRUCT(X86_RSH_U4RR, "shr %0, cl \n")
ASM_INSTRUCT(X86_RSH_U4RI, "shr %0, %1 \n")

ASM_INSTRUCT(X86_ADD_I4RR, "add %0, %1 \n")
ASM_INSTRUCT(X86_ADD_I4RI, "add %0, %1 \n")
ASM_INSTRUCT(X86_ADD_I4RM, "add %0, dword ptr [%1] \n")
ASM_INSTRUCT(X86_ADD_U4RR, "add %0, %1 \n")
ASM_INSTRUCT(X86_ADD_U4RI, "add %0, %1 \n")
ASM_INSTRUCT(X86_ADD_U4RM, "add %0, dword ptr [%1] \n")
ASM_INSTRUCT(X86_ADD_F4RM, "fadd dword ptr [%1] \n")
ASM_INSTRUCT(X86_ADD_F8RM, "fadd qword ptr [%1] \n")

ASM_INSTRUCT(X86_SUB_I4RR, "sub %0, %1 \n")
ASM_INSTRUCT(X86_SUB_I4RI, "sub %0, %1 \n")
ASM_INSTRUCT(X86_SUB_I4RM, "sub %0, dword ptr [%1] \n")
ASM_INSTRUCT(X86_SUB_U4RR, "sub %0, %1 \n")
ASM_INSTRUCT(X86_SUB_U4RI, "sub %0, %1 \n")
ASM_INSTRUCT(X86_SUB_U4RM, "sub %0, dword ptr [%1] \n")
ASM_INSTRUCT(X86_SUB_F4RM, "fsub dword ptr [%1] \n")
ASM_INSTRUCT(X86_SUB_F8RM, "fsub qword ptr [%1] \n")

ASM_INSTRUCT(X86_MUL_I4RR, "imul %1 \n") /* eax * src =>  edx:eax */
ASM_INSTRUCT(X86_MUL_I4RM, "imul dword ptr [%1] \n")
ASM_INSTRUCT(X86_MUL_I4RR2, "imul %0, %1 \n") /* %0 * %1 =>  %0 */
ASM_INSTRUCT(X86_MUL_I4RM2, "imul %0, dword ptr [%1] \n")
ASM_INSTRUCT(X86_MUL_I4RI2, "imul %0, %1 \n") /* %0 * %1 =>  %0 */
ASM_INSTRUCT(X86_MUL_U4RR, "mul %1 \n") /* eax * src =>  edx:eax */
ASM_INSTRUCT(X86_MUL_U4RM, "mul dword ptr [%1] \n")
ASM_INSTRUCT(X86_MUL_F4RM, "fmul dword ptr [%1] \n")
ASM_INSTRUCT(X86_MUL_F8RM, "fmul qword ptr [%1] \n")

ASM_INSTRUCT(X86_DIV_I4RR, "cdq\n idiv %1 \n")  /* edx:eax / %1 => eax:quotient edx:remainder */
ASM_INSTRUCT(X86_DIV_I4RM, "cdq\n idiv dword ptr [%1] \n")
ASM_INSTRUCT(X86_DIV_U4RR, "mov edx, 0\n div %1 \n")
ASM_INSTRUCT(X86_DIV_U4RM, "mov edx, 0\n div dword ptr [%1] \n")
ASM_INSTRUCT(X86_DIV_F4RM, "fdiv dword ptr [%1] \n") /* Divide ST(0) by m32fp and store result in ST(0) */
ASM_INSTRUCT(X86_DIV_F8RM, "fdiv qword ptr [%1] \n")

ASM_INSTRUCT(X86_MOD_I4RR, "cdq\n idiv %1 \n")  /* edx:eax / %1 => eax:quotient edx:remainder */
ASM_INSTRUCT(X86_MOD_I4RM, "cdq\n idiv dword ptr [%1] \n")
ASM_INSTRUCT(X86_MOD_U4RR, "mov edx, 0\n div %1 \n")
ASM_INSTRUCT(X86_MOD_U4RM, "mov edx, 0\n div dword ptr [%1] \n")

ASM_INSTRUCT(X86_NEG_I4R, "neg %0 \n")
ASM_INSTRUCT(X86_NEG_I4M, "neg dword ptr [%0] \n")
ASM_INSTRUCT(X86_NEG_U4R, "neg %0 \n")
ASM_INSTRUCT(X86_NEG_U4M, "neg dword ptr [%0] \n")
ASM_INSTRUCT(X86_NEG_F4R, "fchs \n")  /* Complements the sign bit of ST(0) */
ASM_INSTRUCT(X86_NEG_F8R, "fchs \n")

ASM_INSTRUCT(X86_NOT_I4R, "not %0 \n")
ASM_INSTRUCT(X86_NOT_I4M, "not dword ptr [%0] \n")
ASM_INSTRUCT(X86_NOT_U4R, "not %0 \n")
ASM_INSTRUCT(X86_NOT_U4M, "not dword ptr [%0] \n")

ASM_INSTRUCT(X86_JZ_I4R, "cmp %0, 0\n je %2\n")
ASM_INSTRUCT(X86_JZ_I4M, "cmp dword ptr [%0], 0\n je %2\n")
ASM_INSTRUCT(X86_JZ_U4R, "cmp %0, 0\n je %2\n")
ASM_INSTRUCT(X86_JZ_F4M, "fldz\n fcomp dword[%0]\n fnstsw ax\n test ah, 44h\n jnp %2\n")
ASM_INSTRUCT(X86_JZ_F8M, "fldz\n fcomp qword[%0]\n fnstsw ax\n test ah, 44h\n jnp %2\n")

ASM_INSTRUCT(X86_JNZI4,     "cmp %d1, 0\njne %0\n")
ASM_INSTRUCT(X86_JNZU4,     "cmp %d1, 0\njne %0\n")
ASM_INSTRUCT(X86_JNZF4,     "fldz\nfucompp\nfnstsw ax\n test ah, 44h\njp %0\n")
ASM_INSTRUCT(X86_JNZF8,     "fldz\nfucompp\nfnstsw ax\n test ah, 44h\njp %0\n")

ASM_INSTRUCT(X86_JEI4,      "cmp %d1, %d2\nje %0\n")
ASM_INSTRUCT(X86_JEU4,      "cmp %d1, %d2\nje %0\n")
ASM_INSTRUCT(X86_JEF4,      "fld dword ptr %2\nfucompp\nfnstsw ax\ntest ah, 44h\njnp %0\n")
ASM_INSTRUCT(X86_JEF8,      "fld qword ptr %2\nfucompp\nfnstsw ax\ntest ah, 44h\njnp %0\n")

ASM_INSTRUCT(X86_JNEI4,     "cmp %d1, %d2\njne %0\n")
ASM_INSTRUCT(X86_JNEU4,     "cmp %d1, %d2\njne %0\n")
ASM_INSTRUCT(X86_JNEF4,     "fld dword ptr %2\nfucompp\nfnstsw ax\ntest ah, 44h\njp %0\n")
ASM_INSTRUCT(X86_JNEF8,     "fld qword ptr %2\nfucompp\nfnstsw ax\ntest ah, 44h\njp %0\n")

ASM_INSTRUCT(X86_JGI4,      "cmp %d1, %d2\njg %0\n")
ASM_INSTRUCT(X86_JGU4,      "cmp %d1, %d2\nja %0\n")
ASM_INSTRUCT(X86_JGF4,      "fld dword ptr %2\nfucompp\nfnstsw ax\ntest ah, 1\njne %0\n")
ASM_INSTRUCT(X86_JGF8,      "fld qword ptr %2\nfucompp\nfnstsw ax\ntest ah, 1\njne %0\n")

ASM_INSTRUCT(X86_JLI4,      "cmp %d1, %d2\njl %0\n")
ASM_INSTRUCT(X86_JLU4,      "cmp %d1, %d2\njb %0\n")
ASM_INSTRUCT(X86_JLF4,      "fld dword ptr %2\nfucompp\nfnstsw ax\ntest ah, 41h\njp %0\n")
ASM_INSTRUCT(X86_JLF8,      "fld qword ptr %2\nfucompp\nfnstsw ax\ntest ah, 41h\njp %0\n")

ASM_INSTRUCT(X86_JGEI4,     "cmp %d1, %d2\njge %0\n")
ASM_INSTRUCT(X86_JGEU4,     "cmp %d1, %d2\njae %0\n")
ASM_INSTRUCT(X86_JGEF4,     "fld dword ptr %2\nfucompp\nfnstsw ax\ntest ah, 41h\njne %0\n")
ASM_INSTRUCT(X86_JGEF8,     "fld qword ptr %2\nfucompp\nfnstsw ax\ntest ah, 41h\njne %0\n")

ASM_INSTRUCT(X86_JLEI4,     "cmp %d1, %d2\njle %0\n")
ASM_INSTRUCT(X86_JLEU4,     "cmp %d1, %d2\njbe %0\n")
ASM_INSTRUCT(X86_JLEF4,     "fld dword ptr %2\nfucompp\nfnstsw ax\ntest ah, 5\njp %0\n")
ASM_INSTRUCT(X86_JLEF8,     "fld qword ptr %2\nfucompp\nfnstsw ax\ntest ah, 5\njp %0\n")

ASM_INSTRUCT(X86_EXT_I1RM,  "movsx %0, byte ptr [%1]")
ASM_INSTRUCT(X86_EXT_I2RM,  "movsx %0, word ptr [%1]")
ASM_INSTRUCT(X86_EXT_U1RM,  "movzx %0, byte ptr [%1]")
ASM_INSTRUCT(X86_EXT_U2RM,  "movzx %0, word ptr [%1]")

ASM_INSTRUCT(X86_CVTI4F4,   "push %d1\nfild DWORD PTR [esp];fstp DWORD PTR %0\nadd esp,4\n")
ASM_INSTRUCT(X86_CVTI4F8,   "push %d1\nfild DWORD PTR [esp];fstp QWORD PTR %0\nadd esp,4\n")
ASM_INSTRUCT(X86_CVTU4F4,   "push 0\npush %d1\nfild QWORD PTR [esp];fstp DWORD PTR %0\nadd esp,8\n")
ASM_INSTRUCT(X86_CVTU4F8,   "push 0\npush %d1\nfild QWORD PTR [esp];fstp QWORD PTR %0\nadd esp,8\n")
ASM_INSTRUCT(X86_CVTF4,     "fld DWORD PTR %1\nfstp QWORD PTR %0\n")


ASM_INSTRUCT(X86_CVTF4I4,   "fld DWORD PTR %1\nsub esp, 16\nfnstcw [esp];movzx eax, WORD PTR [esp];"
                            "or eax, 0c00H\nmov 4[esp], eax\nfldcw 4[esp];fistp DWORD PTR 8[esp];"
                            "fldcw [esp];mov eax, 8[esp];add esp, 16\n")
ASM_INSTRUCT(X86_CVTF4U4,   "fld DWORD PTR %1\nsub esp, 16\nfnstcw [esp];movzx eax, WORD PTR [esp];"
                            "or eax, 0c00H\nmov 4[esp], eax\nfldcw 4[esp];fistp QWORD PTR 8[esp];"
                            "fldcw [esp];mov eax, 8[esp];add esp, 16\n")
ASM_INSTRUCT(X86_CVTF8,     "fld QWORD PTR %1\nfstp DWORD PTR %0\n")
ASM_INSTRUCT(X86_CVTF8I4,   "fld QWORD PTR %1\nsub esp, 16\nfnstcw [esp];movzx eax, WORD PTR [esp];"
                            "or eax, 0c00H\nmov 4[esp], eax\nfldcw 4[esp];fistp DWORD PTR 8[esp];"
                            "fldcw [esp];mov eax, 8[esp];add esp, 16\n")
ASM_INSTRUCT(X86_CVTF8U4,   "fld QWORD PTR %1\nsub esp, 16\nfnstcw [esp];movzx eax, WORD PTR [esp];"
                            "or eax, 0c00H\nmov 4[esp], eax\nfldcw 4[esp];fistp QWORD PTR 8[esp];"
                            "fldcw [esp];mov eax, 8[esp];add esp, 16\n")
						

ASM_INSTRUCT(X86_ADDR,      "lea %0, [%1]")
ASM_INSTRUCT(X86_MOVI1,     "mov %b0, %b1\n")
ASM_INSTRUCT(X86_MOVI2,     "mov %w0, %w1\n")
ASM_INSTRUCT(X86_MOVI4,     "mov %d0, %d1\n")
ASM_INSTRUCT(X86_MOVBLK,    "lea edi, %0\nlea esi, %1\nmov ecx, %2\nrep movsb\n")

ASM_INSTRUCT(X86_JMP,       "jmp %0\n")
ASM_INSTRUCT(X86_IJMP,      "jmp DWORD PTR %0[%1*4]")

ASM_INSTRUCT(X86_PROLOGUE,  "push ebp\npush ebx\npush esi\npush edi\nmov ebp, esp\n")
ASM_INSTRUCT(X86_PUSH,      "push %d0\n")
ASM_INSTRUCT(X86_PUSHF4,    "push ecx\nfld DWORD PTR %0\nfstp DWORD PTR [esp]")
ASM_INSTRUCT(X86_PUSHF8,    "sub esp, 8\nfld QWORD PTR %0\nfstp QWORD PTR [esp]")
ASM_INSTRUCT(X86_PUSHB,     "lea esi, %0\nsub esp, %2\nmov edi, esp\nmov ecx, %1\nrep movsb\n")
ASM_INSTRUCT(X86_EXPANDF,   "sub esp, %0\n")
ASM_INSTRUCT(X86_CALL,      "call %1\n")
ASM_INSTRUCT(X86_ICALL,     "call DWORD PTR %1\n")
ASM_INSTRUCT(X86_REDUCEF,   "add esp, %0\n")
ASM_INSTRUCT(X86_EPILOGUE,  "mov esp, ebp\npop edi\npop esi\npop ebx\npop ebp\nret\n")


ASM_INSTRUCT(X86_CLEAR,     "push %1\npush 0\nlea eax, %0\npush eax\ncall _memset\nadd esp, 12\n")

ASM_INSTRUCT(X86_LDF4,      "fld DWORD PTR %0\n")
ASM_INSTRUCT(X86_LDF8,      "fld QWORD PTR %0\n")
ASM_INSTRUCT(X86_STF4,      "fstp DWORD PTR %0\n")
ASM_INSTRUCT(X86_STF8,      "fstp QWORD PTR %0\n")


ASM_INSTRUCT(X86_STF4_NO_POP,     "fst DWORD PTR %0\n")
ASM_INSTRUCT(X86_STF8_NO_POP,     "fst QWORD PTR %0\n")

ASM_INSTRUCT(X86_X87_POP,     "fstp st(0)")


