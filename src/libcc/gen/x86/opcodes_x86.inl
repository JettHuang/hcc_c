/* \brief
 *		template for x86 assembly instructions.
 *  %0  dst
 *  %1  src
 *  %2  tagert or count
 */

#ifndef ASM_INSTRUCT
#error "You must define ASM_INSTRUCT before including this file."
#endif

ASM_INSTRUCT(X86_INS_INVALID, "--N/A--")
ASM_INSTRUCT(X86_LABEL, "%2: \n")

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
ASM_INSTRUCT(X86_JZ_U4M, "cmp dword ptr [%0], 0\n je %2\n")
ASM_INSTRUCT(X86_JZ_F4M, "fldz\n fcomp dword ptr[%0]\n fnstsw ax\n test ah, 44h\n jnp %2\n")
ASM_INSTRUCT(X86_JZ_F8M, "fldz\n fcomp qword ptr[%0]\n fnstsw ax\n test ah, 44h\n jnp %2\n")

ASM_INSTRUCT(X86_JNZ_I4R, "cmp %0, 0\n jne %2\n")
ASM_INSTRUCT(X86_JNZ_I4M, "cmp dword ptr [%0], 0\n jne %2\n")
ASM_INSTRUCT(X86_JNZ_U4R, "cmp %0, 0\n jne %2\n")
ASM_INSTRUCT(X86_JNZ_U4M, "cmp dword ptr [%0], 0\n jne %2\n")
ASM_INSTRUCT(X86_JNZ_F4M, "fldz\n fcomp dword ptr[%0]\n fnstsw ax\n test ah, 44h\n jp %2\n")
ASM_INSTRUCT(X86_JNZ_F8M, "fldz\n fcomp qword ptr[%0]\n fnstsw ax\n test ah, 44h\n jp %2\n")

ASM_INSTRUCT(X86_JE_I4RR, "cmp %0, %1\n je %2\n")
ASM_INSTRUCT(X86_JE_I4RM, "cmp %0, dword ptr[%1]\n je %2\n")
ASM_INSTRUCT(X86_JE_I4RI, "cmp %0, %1\n je %2\n")
ASM_INSTRUCT(X86_JE_U4RR, "cmp %0, %1\n je %2\n")
ASM_INSTRUCT(X86_JE_U4RM, "cmp %0, dword ptr[%1]\n je %2\n")
ASM_INSTRUCT(X86_JE_U4RI, "cmp %0, %1\n je %2\n")
ASM_INSTRUCT(X86_JE_F4MM,  "fld dword ptr [%0]\n fcomp dword ptr[%1]\n fnstsw ax\n test ah, 44h\n jnp %2\n")
ASM_INSTRUCT(X86_JE_F8MM,  "fld qword ptr [%0]\n fcomp qword ptr[%1]\n fnstsw ax\n test ah, 44h\n jnp %2\n")

ASM_INSTRUCT(X86_JNE_I4RR, "cmp %0, %1\n jne %2\n")
ASM_INSTRUCT(X86_JNE_I4RM, "cmp %0, dword ptr[%1]\n jne %2\n")
ASM_INSTRUCT(X86_JNE_I4RI, "cmp %0, %1\n jne %2\n")
ASM_INSTRUCT(X86_JNE_U4RR, "cmp %0, %1\n jne %2\n")
ASM_INSTRUCT(X86_JNE_U4RM, "cmp %0, dword ptr[%1]\n jne %2\n")
ASM_INSTRUCT(X86_JNE_U4RI, "cmp %0, %1\n jne %2\n")
ASM_INSTRUCT(X86_JNE_F4MM,  "fld dword ptr [%0]\n fcomp dword ptr[%1]\n fnstsw ax\n test ah, 44h\n jp %2\n")
ASM_INSTRUCT(X86_JNE_F8MM,  "fld qword ptr [%0]\n fcomp qword ptr[%1]\n fnstsw ax\n test ah, 44h\n jp %2\n")

ASM_INSTRUCT(X86_JG_I4RR, "cmp %0, %1\n jg %2\n")
ASM_INSTRUCT(X86_JG_I4RM, "cmp %0, dword ptr[%1]\n jg %2\n")
ASM_INSTRUCT(X86_JG_I4RI, "cmp %0, %1\n jg %2\n")
ASM_INSTRUCT(X86_JG_U4RR, "cmp %0, %1\n ja %2\n")
ASM_INSTRUCT(X86_JG_U4RM, "cmp %0, dword ptr[%1]\n ja %2\n")
ASM_INSTRUCT(X86_JG_U4RI, "cmp %0, %1\n ja %2\n")
ASM_INSTRUCT(X86_JG_F4MM,  "fld dword ptr [%0]\n fcomp dword ptr[%1]\n fnstsw ax\n test ah, 1h\n jne %2\n")
ASM_INSTRUCT(X86_JG_F8MM,  "fld qword ptr [%0]\n fcomp qword ptr[%1]\n fnstsw ax\n test ah, 1h\n jne %2\n")

ASM_INSTRUCT(X86_JL_I4RR, "cmp %0, %1\n jl %2\n")
ASM_INSTRUCT(X86_JL_I4RM, "cmp %0, dword ptr[%1]\n jl %2\n")
ASM_INSTRUCT(X86_JL_I4RI, "cmp %0, %1\n jl %2\n")
ASM_INSTRUCT(X86_JL_U4RR, "cmp %0, %1\n jb %2\n")
ASM_INSTRUCT(X86_JL_U4RM, "cmp %0, dword ptr[%1]\n jb %2\n")
ASM_INSTRUCT(X86_JL_U4RI, "cmp %0, %1\n jb %2\n")
ASM_INSTRUCT(X86_JL_F4MM,  "fld dword ptr [%0]\n fcomp dword ptr[%1]\n fnstsw ax\n test ah, 41h\n jp %2\n")
ASM_INSTRUCT(X86_JL_F8MM,  "fld qword ptr [%0]\n fcomp qword ptr[%1]\n fnstsw ax\n test ah, 41h\n jp %2\n")

ASM_INSTRUCT(X86_JGE_I4RR, "cmp %0, %1\n jge %2\n")
ASM_INSTRUCT(X86_JGE_I4RM, "cmp %0, dword ptr[%1]\n jge %2\n")
ASM_INSTRUCT(X86_JGE_I4RI, "cmp %0, %1\n jge %2\n")
ASM_INSTRUCT(X86_JGE_U4RR, "cmp %0, %1\n jae %2\n")
ASM_INSTRUCT(X86_JGE_U4RM, "cmp %0, dword ptr[%1]\n jae %2\n")
ASM_INSTRUCT(X86_JGE_U4RI, "cmp %0, %1\n jae %2\n")
ASM_INSTRUCT(X86_JGE_F4MM,  "fld dword ptr [%0]\n fcomp dword ptr[%1]\n fnstsw ax\n test ah, 41h\n jne %2\n")
ASM_INSTRUCT(X86_JGE_F8MM,  "fld qword ptr [%0]\n fcomp qword ptr[%1]\n fnstsw ax\n test ah, 41h\n jne %2\n")

ASM_INSTRUCT(X86_JLE_I4RR, "cmp %0, %1\n jle %2\n")
ASM_INSTRUCT(X86_JLE_I4RM, "cmp %0, dword ptr[%1]\n jle %2\n")
ASM_INSTRUCT(X86_JLE_I4RI, "cmp %0, %1\n jle %2\n")
ASM_INSTRUCT(X86_JLE_U4RR, "cmp %0, %1\n jbe %2\n")
ASM_INSTRUCT(X86_JLE_U4RM, "cmp %0, dword ptr[%1]\n jbe %2\n")
ASM_INSTRUCT(X86_JLE_U4RI, "cmp %0, %1\n jbe %2\n")
ASM_INSTRUCT(X86_JLE_F4MM,  "fld dword ptr [%0]\n fcomp dword ptr[%1]\n fnstsw ax\n test ah, 5h\n jp %2\n")
ASM_INSTRUCT(X86_JLE_F8MM,  "fld qword ptr [%0]\n fcomp qword ptr[%1]\n fnstsw ax\n test ah, 5h\n jp %2\n")

/* load & extension byte & word */
ASM_INSTRUCT(X86_EXT_RI4MI1,  "movsx %0, byte ptr [%1]")
ASM_INSTRUCT(X86_EXT_RI4MI2,  "movsx %0, word ptr [%1]")
ASM_INSTRUCT(X86_EXT_RU4MU1,  "movzx %0, byte ptr [%1]")
ASM_INSTRUCT(X86_EXT_RU4MU2,  "movzx %0, word ptr [%1]")

/* convert integer to float */
ASM_INSTRUCT(X86_CVT_MF4RI4,  "push %1\n fild dword ptr [esp]\n fstp dword ptr [%0]\n add esp, 4\n")
ASM_INSTRUCT(X86_CVT_MF8RI4,  "push %1\n fild dword ptr [esp]\n fstp qword ptr [%0]\n add esp, 4\n")
ASM_INSTRUCT(X86_CVT_MF4RU4,  "push 0\n push %1\n fild qword ptr [esp]\n fstp dword ptr [%0]\n add esp, 8\n")
ASM_INSTRUCT(X86_CVT_MF8RU4,  "push 0\n push %1\n fild qword ptr [esp]\n fstp qword ptr [%0]\n add esp, 8\n")

ASM_INSTRUCT(X86_CVT_MF8MF4,  "fld dword ptr [%1]\n fstp qword ptr [%1]\n")
ASM_INSTRUCT(X86_CVT_MF4MF8,  "fld qword ptr [%1]\n fstp dword ptr [%0]\n")

/* convert float to integer (in eax) */
ASM_INSTRUCT(X86_CVT_RI4MF4,   "fld dword ptr [%1]\n sub esp, 16\n fnstcw [esp]\n movzx eax, word ptr [esp]\n"
                               "or eax, 0c00H\n mov 4[esp], eax\n fldcw 4[esp]\n fistp dword ptr 8[esp]\n"
                               "fldcw [esp]\n mov eax, 8[esp]\n  add esp, 16\n")
ASM_INSTRUCT(X86_CVT_RU4MF4,   "fld dword ptr [%1]\n sub esp, 16\n fnstcw [esp]\n movzx eax, word ptr [esp]\n"
                               "or eax, 0c00H\n mov 4[esp], eax\n fldcw 4[esp]\n  fistp qword ptr 8[esp]\n"
                               "fldcw [esp]\n mov eax, 8[esp]\n  add esp, 16\n")

ASM_INSTRUCT(X86_CVT_RI4MF8,   "fld qword ptr [%1]\n sub esp, 16\n fnstcw [esp]\n movzx eax, word ptr [esp]\n"
                               "or eax, 0c00H\n mov 4[esp], eax\n fldcw 4[esp]\n fistp dword ptr 8[esp]\n"
                               "fldcw [esp]\n mov eax, 8[esp]\n  add esp, 16\n")
ASM_INSTRUCT(X86_CVT_RU4MF8,   "fld qword ptr [%1]\n sub esp, 16\n fnstcw [esp]\n movzx eax, word ptr [esp]\n"
                               "or eax, 0c00H\n mov 4[esp], eax\n fldcw 4[esp]\n  fistp qword ptr 8[esp]\n"
                               "fldcw [esp]\n mov eax, 8[esp]\n  add esp, 16\n")
						
ASM_INSTRUCT(X86_ADDR,       "lea %0, [%1]\n")
ASM_INSTRUCT(X86_MOV_I1MRI,  "mov byte ptr [%0], %1\n")
ASM_INSTRUCT(X86_MOV_I2MRI,  "mov word ptr [%0], %1\n")
ASM_INSTRUCT(X86_MOV_I4MRI,  "mov dword ptr [%0], %1\n")
ASM_INSTRUCT(X86_MOV_U1MRI,  "mov byte ptr [%0], %1\n")
ASM_INSTRUCT(X86_MOV_U2MRI,  "mov word ptr [%0], %1\n")
ASM_INSTRUCT(X86_MOV_U4MRI,  "mov dword ptr [%0], %1\n")
ASM_INSTRUCT(X86_MOV_P4MRI,  "mov dword ptr [%0], %1\n")
ASM_INSTRUCT(X86_MOV_I1RM,   "mov %0, byte ptr [%1]\n")
ASM_INSTRUCT(X86_MOV_I2RM,   "mov %0, word ptr [%1]\n")
ASM_INSTRUCT(X86_MOV_I4RM,   "mov %0, dword ptr [%1]\n")
ASM_INSTRUCT(X86_MOV_U1RM,   "mov %0, byte ptr [%1]\n")
ASM_INSTRUCT(X86_MOV_U2RM,   "mov %0, word ptr [%1]\n")
ASM_INSTRUCT(X86_MOV_U4RM,   "mov %0, dword ptr [%1]\n")
ASM_INSTRUCT(X86_MOV_P4RM,   "mov %0, dword ptr [%1]\n")
ASM_INSTRUCT(X86_MOV_RRI,    "mov %0, %1\n")
ASM_INSTRUCT(X86_MOV_I8RM,   "----")
ASM_INSTRUCT(X86_MOV_U8RM,   "----")
ASM_INSTRUCT(X86_MOV_I8MR,   "----")
ASM_INSTRUCT(X86_MOV_U8MR,   "----")
ASM_INSTRUCT(X86_MOV_I8MI,   "----")
ASM_INSTRUCT(X86_MOV_U8MI,   "----")

ASM_INSTRUCT(X86_MOV_BMM,   "lea edi, [%0]\n lea esi, [%1]\n mov ecx, %2\n rep movsb\n")

ASM_INSTRUCT(X86_JMP,       "jmp %2\n")
ASM_INSTRUCT(X86_IJMP,      "jmp dword ptr [%0]\n")

ASM_INSTRUCT(X86_PROLOGUE,  "push ebp\n mov ebp, esp\n")
ASM_INSTRUCT(X86_PROLOGUE_CALLEESAVE, "push ebx\n push esi\n push edi\n")
ASM_INSTRUCT(X86_PUSH_I4R,  "push %0\n")
ASM_INSTRUCT(X86_PUSH_I4M,  "push dword ptr[%0]\n")
ASM_INSTRUCT(X86_PUSH_I4I,  "push %0\n")
ASM_INSTRUCT(X86_PUSH_U4R,  "push %0\n")
ASM_INSTRUCT(X86_PUSH_U4M,  "push dword ptr[%0]\n")
ASM_INSTRUCT(X86_PUSH_U4I,  "push %0\n")
ASM_INSTRUCT(X86_PUSH_P4M,  "push dword ptr[%0]\n")
ASM_INSTRUCT(X86_PUSH_P4R,  "push %0\n")
ASM_INSTRUCT(X86_PUSH_P4I,  "push %0\n")

ASM_INSTRUCT(X86_PUSH_I8I,  "-----")
ASM_INSTRUCT(X86_PUSH_I8M,  "-----")
ASM_INSTRUCT(X86_PUSH_I8R,  "-----")
ASM_INSTRUCT(X86_PUSH_U8I,  "-----")
ASM_INSTRUCT(X86_PUSH_U8M,  "-----")
ASM_INSTRUCT(X86_PUSH_U8R,  "-----")

ASM_INSTRUCT(X86_PUSH_F4R,  "push ecx\n fstp dword ptr [esp]\n")
ASM_INSTRUCT(X86_PUSH_F4M,  "push ecx\n fld dword ptr [%0]\n fstp dword ptr [esp]\n")
ASM_INSTRUCT(X86_PUSH_F8R,  "sub esp, 8\n fstp qword ptr [esp]\n")
ASM_INSTRUCT(X86_PUSH_F8M,  "sub esp, 8\n fld qword ptr [%0]\n fstp qword ptr [esp]\n")
ASM_INSTRUCT(X86_PUSH_BM,   "lea esi, [%1]\n sub esp, %2\n mov edi, esp\n mov ecx, %2\n rep movsb\n")
ASM_INSTRUCT(X86_EXPANDF,   "sub esp, %0\n")
ASM_INSTRUCT(X86_CALL,      "call %0\n")
ASM_INSTRUCT(X86_ICALL,     "call dword ptr [%0]\n")
ASM_INSTRUCT(X86_REDUCEF,   "add esp, %0\n")
ASM_INSTRUCT(X86_EPILOGUE,  "mov esp, ebp\n pop ebp\n ret\n")
ASM_INSTRUCT(X86_EPILOGUE_CALLEERESTORE, "pop edi\n pop esi\n pop ebx\n ret\n")

/* set zero */
ASM_INSTRUCT(X86_ZERO_M,    "lea edi, [%0]\n  mov al, 0\n mov ecx, %2\n rep stosb\n")

ASM_INSTRUCT(X86_LD_F4M,    "fld dword ptr [%0]\n")
ASM_INSTRUCT(X86_LD_F8M,    "fld qword ptr [%0]\n")
ASM_INSTRUCT(X86_ST_F4M,    "fst dword ptr [%0]\n")
ASM_INSTRUCT(X86_ST_F8M,    "fst qword ptr [%0]\n")
ASM_INSTRUCT(X86_STP_F4M,   "fstp dword ptr [%0]\n")
ASM_INSTRUCT(X86_STP_F8M,   "fstp qword ptr [%0]\n")
ASM_INSTRUCT(X86_STP_NIL,   "fstp st(0)")

