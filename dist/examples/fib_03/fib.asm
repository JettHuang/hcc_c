;-----------------------------------------
; F:/GitHub/hcc_c/dist/examples/fib_03/fib.i
;-----------------------------------------
.486
.model flat
option casemap:none

.code
;function __local_stdio_printf_options begin
___local_stdio_printf_options: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	lea eax, dword ptr [$2_]
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function __local_stdio_printf_options end
;function __local_stdio_scanf_options begin
___local_stdio_scanf_options: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	lea eax, dword ptr [$6_]
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function __local_stdio_scanf_options end
;function fib begin
_fib: 
	push ebp
	mov ebp, esp
	sub esp, 4
	push ebx
	push esi
	push edi
	mov eax, dword ptr [ebp+8]
	cmp eax, 2
	jg $28_
	mov eax, 1
	jmp $26_
$28_:
	mov eax, dword ptr [ebp+8]
	sub eax, 1
	push eax
	call _fib
	add esp, 4
	mov ebx, dword ptr [ebp+8]
	sub ebx, 2
	push ebx
	mov dword ptr [ebp-4], eax
	call _fib
	add esp, 4
	mov ebx, dword ptr [ebp-4]
	add ebx, eax
	mov eax, ebx
$26_:
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function fib end
;function main begin
_main: 
	push ebp
	mov ebp, esp
	sub esp, 4
	push ebx
	push esi
	push edi
	mov eax, dword ptr [ebp+8]
	cmp eax, 2
	jge $40_
	lea eax, dword ptr [$41_]
	push eax
	call _printf
	add esp, 4
	mov eax, 1
	jmp $37_
$40_:
	mov eax, dword ptr [ebp+12]
	add eax, 4
	push dword ptr [eax]
	call _atoi
	add esp, 4
	mov dword ptr [ebp-4], eax
	push dword ptr [ebp-4]
	call _fib
	add esp, 4
	push eax
	push dword ptr [ebp-4]
	lea eax, dword ptr [$42_]
	push eax
	call _printf
	add esp, 12
	mov eax, 0
$37_:
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function main end

.data?
$6_      db  8  dup (?)
$2_      db  8  dup (?)

.const
$42_     db   102, 105, 98, 40, 37, 100, 41, 32, 61, 32, 37, 100, 10, 0 
$41_     db   117, 115, 97, 103, 101, 58, 32, 102, 105, 98, 32, 110, 10, 67, 111, 109, 112, 117, 116, 101, 32, 110, 116, 104, 32, 70, 105, 98, 111, 110, 97, 99 
   db   99, 105, 32, 110, 117, 109, 98, 101, 114, 10, 0 

;--------export & import-----------
public _main
public _fib
extern _atoi:near
extern _printf:near

end