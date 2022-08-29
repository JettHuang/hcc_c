# hcc_c
A C Compiler implemented in C language.

# articles:
https://zhuanlan.zhihu.com/p/506669546

# references
 1. C99 standard final draft http://www.open-std.org/jtc1/sc22/wg14/www/docs/n1124.pdf
 2. Dave Prosser's C Preprocessing Algorithm http://www.spinellis.gr/blog/20060626/
 3. 8cc Preprocessing Algorithm https://github.com/rui314/8cc/wiki/cpp.algo.pdf
 4. ANSI C Grammar, Lex specification https://www.lysator.liu.se/c/ANSI-C-grammar-l.html
 5. ANSI C Grammar production http://www.lysator.liu.se/c/ANSI-C-grammar-y.html#translation-unit
 6. C89 spec http://www.lysator.liu.se/c/rat/title.html
 7. https://blog.csdn.net/sheisc/article/list/1?t=1
 
# source tree 
     hcc
	+--dist (final distribute compiler tools)
           +--bin
	   +--examples
	+--include
        +--doc (document of development)
        +--src
           +--basic (commone used codes)
           +--libcpp (lib of c preprocessor)
           +--libcc (lib of c compiler)
           +--tools 
              +-- common (common codes for tools)
              +-- hcpp (c preprocessor app)
              +-- hcc (c compiler app)
        +--build (solution)

# pipeline
 1. lexer
 2. cpp
 3. parser
 4. intermediate code
 5. backend
 
# features
  1. using c99 as reference
  2. not support universal char name
  
# example code
```C
 /* the following definitons are used to compile the visual-stdio includes file successfully 
  *
  * BEGIN
  */
 #define __declspec(...)	/* __declspec is un-support */
 #define __inline	inline	/* __inline is un-support */
 #define __forceinline inline
 #define __pragma(...)		 /* __pragma is un-support */
 
 #define __int64 	long long 
 #define _NO_CRT_STDIO_INLINE
 #define _CRT_FUNCTIONS_REQUIRED	1  /* errno.h */
 #define __STDC_WANT_SECURE_LIB__	0
 /* END */
 

#include <stdio.h>
#include <stdlib.h>


int fib(int n)
{
	if (n <= 2)
		return 1;
	else
		return fib(n-1) + fib(n-2);
}

int main(int argc, char **argv) 
{
	int n;
	if (argc < 2) {
		printf("usage: fib n\n"
			   "Compute nth Fibonacci number\n");
		return 1;
	}
		
	n = atoi(argv[1]);
	printf("fib(%d) = %d\n", n, fib(n));
	return 0;
}

```
==>The Assamble code
```asm
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

;--------additional----------
extern __aullshr:near
extern __aullrem:near
extern __aulldiv:near
extern __allshr:near
extern __allshl:near
extern __allrem:near
extern __allmul:near
extern __alldiv:near

end
```
# snapshots of console & gui examples
![console example](https://pic2.zhimg.com/80/v2-d8da4dfc2242d64ca6608c7440d06195_720w.jpg)
![gui example](https://pic2.zhimg.com/80/v2-bd8f2f88b5ff601a9e7dbaf0d4d5d2b1_720w.jpg)


