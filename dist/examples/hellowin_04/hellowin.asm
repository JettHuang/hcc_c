;-----------------------------------------
; F:/GitHub/hcc_c/dist/examples/hellowin_04/hellowin.i
;-----------------------------------------
.486
.model flat
option casemap:none

.code
;function __acrt_locale_get_ctype_array_value begin
___acrt_locale_get_ctype_array_value: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	mov eax, dword ptr [ebp+12]
	cmp eax, -1
	jl $4_
	cmp eax, 255
	jg $4_
	imul eax, 2
	mov ebx, dword ptr [ebp+8]
	add ebx, eax
	movzx eax, word ptr [ebx]
	and eax, dword ptr [ebp+16]
	jmp $1_
$4_:
	mov eax, 0
$1_:
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function __acrt_locale_get_ctype_array_value end
;function __ascii_tolower begin
___ascii_tolower: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	mov eax, dword ptr [ebp+8]
	cmp eax, 65
	jl $21_
	cmp eax, 90
	jg $21_
	sub eax, -32
	jmp $18_
$21_:
	mov eax, dword ptr [ebp+8]
$18_:
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function __ascii_tolower end
;function __ascii_toupper begin
___ascii_toupper: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	mov eax, dword ptr [ebp+8]
	cmp eax, 97
	jl $37_
	cmp eax, 122
	jg $37_
	sub eax, 32
	jmp $34_
$37_:
	mov eax, dword ptr [ebp+8]
$34_:
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function __ascii_toupper end
;function __ascii_iswalpha begin
___ascii_iswalpha: 
	push ebp
	mov ebp, esp
	sub esp, 4
	push ebx
	push esi
	push edi
	mov eax, dword ptr [ebp+8]
	cmp eax, 65
	jl $54_
	cmp eax, 90
	jg $54_
	jmp $51_
$54_:
	mov eax, dword ptr [ebp+8]
	cmp eax, 97
	jl $52_
	cmp eax, 122
	jg $52_
$51_:
	mov dword ptr [ebp-4], 1
	jmp $53_
$52_:
	mov dword ptr [ebp-4], 0
$53_:
	mov eax, dword ptr [ebp-4]
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function __ascii_iswalpha end
;function __ascii_iswdigit begin
___ascii_iswdigit: 
	push ebp
	mov ebp, esp
	sub esp, 4
	push ebx
	push esi
	push edi
	mov eax, dword ptr [ebp+8]
	cmp eax, 48
	jl $66_
	cmp eax, 57
	jg $66_
	mov dword ptr [ebp-4], 1
	jmp $67_
$66_:
	mov dword ptr [ebp-4], 0
$67_:
	mov eax, dword ptr [ebp-4]
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function __ascii_iswdigit end
;function __ascii_towlower begin
___ascii_towlower: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	push dword ptr [ebp+8]
	call ___ascii_tolower
	add esp, 4
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function __ascii_towlower end
;function __ascii_towupper begin
___ascii_towupper: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	push dword ptr [ebp+8]
	call ___ascii_toupper
	add esp, 4
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function __ascii_towupper end
;function __acrt_get_locale_data_prefix begin
___acrt_get_locale_data_prefix: 
	push ebp
	mov ebp, esp
	sub esp, 4
	push ebx
	push esi
	push edi
	mov eax, dword ptr [ebp+8]
	mov dword ptr [ebp-4], eax
	mov eax, dword ptr [ebp-4]
	add eax, 0
	mov eax, dword ptr [eax]
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function __acrt_get_locale_data_prefix end
;function _chvalidchk_l begin
__chvalidchk_l: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	mov eax, dword ptr [ebp+16]
	cmp eax, 0
	je $89_
	push dword ptr [ebp+12]
	push dword ptr [ebp+8]
	push eax
	call ___acrt_get_locale_data_prefix
	add esp, 4
	add eax, 0
	push dword ptr [eax]
	call ___acrt_locale_get_ctype_array_value
	add esp, 12
	jmp $86_
$89_:
	push dword ptr [ebp+12]
	push dword ptr [ebp+8]
	call ___pctype_func
	push eax
	call ___acrt_locale_get_ctype_array_value
	add esp, 12
$86_:
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function _chvalidchk_l end
;function _ischartype_l begin
__ischartype_l: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	mov eax, dword ptr [ebp+16]
	cmp eax, 0
	je $100_
	mov ebx, dword ptr [ebp+8]
	cmp ebx, -1
	jl $103_
	cmp ebx, 255
	jg $103_
	push eax
	call ___acrt_get_locale_data_prefix
	add esp, 4
	add eax, 0
	mov ebx, dword ptr [ebp+8]
	imul ebx, 2
	mov ecx, dword ptr [eax]
	add ecx, ebx
	movzx eax, word ptr [ecx]
	and eax, dword ptr [ebp+12]
	jmp $97_
$103_:
	push dword ptr [ebp+16]
	call ___acrt_get_locale_data_prefix
	add esp, 4
	add eax, 4
	mov ebx, dword ptr [eax]
	cmp ebx, 1
	jle $107_
	push dword ptr [ebp+16]
	push dword ptr [ebp+12]
	push dword ptr [ebp+8]
	call __isctype_l
	add esp, 12
	jmp $97_
$107_:
	mov eax, 0
	jmp $97_
$100_:
	push 0
	push dword ptr [ebp+12]
	push dword ptr [ebp+8]
	call __chvalidchk_l
	add esp, 12
$97_:
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function _ischartype_l end
;function PtrToPtr64 begin
_PtrToPtr64: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	mov eax, dword ptr [ebp+8]
	mov eax, eax
	xor edx, edx
	mov ebx, eax
	mov eax, ebx
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function PtrToPtr64 end
;function Ptr64ToPtr begin
_Ptr64ToPtr: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	mov ebx, dword ptr [ebp+8]
	mov eax, ebx
	xor edx, edx
	mov ebx, eax
	mov eax, ebx
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function Ptr64ToPtr end
;function HandleToHandle64 begin
_HandleToHandle64: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	mov eax, dword ptr [ebp+8]
	mov eax, eax
	cdq
	mov ebx, eax
	mov eax, ebx
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function HandleToHandle64 end
;function Handle64ToHandle begin
_Handle64ToHandle: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	mov ebx, dword ptr [ebp+8]
	mov eax, ebx
	xor edx, edx
	mov ebx, eax
	mov eax, ebx
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function Handle64ToHandle end
;function Int64ShllMod32 begin
_Int64ShllMod32: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	mov eax, dword ptr [ebp+8]
	mov ebx, dword ptr [ebp+12]
	mov dword ptr [ebp+8], eax
	mov dword ptr [ebp+12], ebx
$143_:
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function Int64ShllMod32 end
;function Int64ShraMod32 begin
_Int64ShraMod32: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	mov eax, dword ptr [ebp+8]
	mov ebx, dword ptr [ebp+12]
	mov dword ptr [ebp+8], eax
	mov dword ptr [ebp+12], ebx
$146_:
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function Int64ShraMod32 end
;function Int64ShrlMod32 begin
_Int64ShrlMod32: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	mov eax, dword ptr [ebp+8]
	mov ebx, dword ptr [ebp+12]
	mov dword ptr [ebp+8], eax
	mov dword ptr [ebp+12], ebx
$149_:
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function Int64ShrlMod32 end
;function _wcstok begin
__wcstok: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	push 0
	push dword ptr [ebp+12]
	push dword ptr [ebp+8]
	call _wcstok
	add esp, 12
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function _wcstok end
;function DbgRaiseAssertionFailure begin
_DbgRaiseAssertionFailure: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
$159_:
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function DbgRaiseAssertionFailure end
;function _InlineBitScanForward64 begin
__InlineBitScanForward64: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	mov eax, dword ptr [ebp+12]
	mov ebx, dword ptr [ebp+16]
	mov dword ptr [ebp+12], eax
	mov dword ptr [ebp+16], ebx
	mov eax, dword ptr[ebp+12]
	push eax
	push dword ptr [ebp+8]
	call __BitScanForward
	add esp, 8
	movzx ebx, al
	cmp ebx, 0
	je $165_
	mov al, 1
	jmp $162_
$165_:
	mov eax, dword ptr [ebp+12]
	mov edx, dword ptr [ebp+16]
	mov ecx, 32
	call __aullshr
	mov ebx, eax
	push ebx
	push dword ptr [ebp+8]
	call __BitScanForward
	add esp, 8
	movzx ebx, al
	cmp ebx, 0
	je $168_
	mov eax, dword ptr [ebp+8]
	mov ebx, dword ptr [eax]
	add ebx, 32
	mov dword ptr [eax], ebx
	mov al, 1
	jmp $162_
$168_:
	mov al, 0
$162_:
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function _InlineBitScanForward64 end
;function _InlineBitScanReverse64 begin
__InlineBitScanReverse64: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	mov eax, dword ptr [ebp+12]
	mov ebx, dword ptr [ebp+16]
	mov dword ptr [ebp+12], eax
	mov dword ptr [ebp+16], ebx
	mov eax, dword ptr [ebp+12]
	mov edx, dword ptr [ebp+16]
	mov ecx, 32
	call __aullshr
	mov ebx, eax
	push ebx
	push dword ptr [ebp+8]
	call __BitScanReverse
	add esp, 8
	movzx ebx, al
	cmp ebx, 0
	je $185_
	mov eax, dword ptr [ebp+8]
	mov ebx, dword ptr [eax]
	add ebx, 32
	mov dword ptr [eax], ebx
	mov al, 1
	jmp $182_
$185_:
	mov eax, dword ptr[ebp+12]
	push eax
	push dword ptr [ebp+8]
	call __BitScanReverse
	add esp, 8
	movzx ebx, al
	cmp ebx, 0
	je $188_
	mov al, 1
	jmp $182_
$188_:
	mov al, 0
$182_:
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function _InlineBitScanReverse64 end
;function _InlineInterlockedAdd begin
__InlineInterlockedAdd: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	push dword ptr [ebp+12]
	push dword ptr [ebp+8]
	call __InterlockedExchangeAdd
	add esp, 8
	add eax, dword ptr [ebp+12] 
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function _InlineInterlockedAdd end
;function _InlineInterlockedExchangePointer begin
__InlineInterlockedExchangePointer: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	mov eax, dword ptr [ebp+12]
	push eax
	push dword ptr [ebp+8]
	call __InterlockedExchange
	add esp, 8
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function _InlineInterlockedExchangePointer end
;function _InlineInterlockedCompareExchangePointer begin
__InlineInterlockedCompareExchangePointer: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	mov eax, dword ptr [ebp+16]
	push eax
	mov eax, dword ptr [ebp+12]
	push eax
	push dword ptr [ebp+8]
	call __InterlockedCompareExchange
	add esp, 12
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function _InlineInterlockedCompareExchangePointer end
;function _InlineInterlockedAnd64 begin
__InlineInterlockedAnd64: 
	push ebp
	mov ebp, esp
	sub esp, 8
	push ebx
	push esi
	push edi
	mov eax, dword ptr [ebp+12]
	mov ebx, dword ptr [ebp+16]
	mov dword ptr [ebp+12], eax
	mov dword ptr [ebp+16], ebx
$210_:
	mov eax, dword ptr [ebp+8]
	mov ebx, dword ptr [eax]
	mov ecx, dword ptr [eax+4]
	mov dword ptr [ebp-8], ebx
	mov dword ptr [ebp-4], ecx
	push dword ptr [ebp-4]
	push dword ptr [ebp-8]
	mov ebx, dword ptr [ebp-8]
	mov ebx, dword ptr [ebp-4]
	and ebx, dword ptr [ebp+12]
	and ebx, dword ptr [ebp+16]
	push ebx
	push ebx
	push eax
	call __InterlockedCompareExchange64
	add esp, 20
	cmp eax, dword ptr[ebp-8]
	jne @F
	cmp edx, dword ptr[ebp-4]
	je $211_
@@:
	jmp $210_
$211_:
	mov eax, dword ptr [ebp-8]
	mov edx, dword ptr [ebp-4]
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function _InlineInterlockedAnd64 end
;function _InlineInterlockedAdd64 begin
__InlineInterlockedAdd64: 
	push ebp
	mov ebp, esp
	sub esp, 8
	push ebx
	push esi
	push edi
	mov eax, dword ptr [ebp+12]
	mov ebx, dword ptr [ebp+16]
	mov dword ptr [ebp+12], eax
	mov dword ptr [ebp+16], ebx
$216_:
	mov eax, dword ptr [ebp+8]
	mov ebx, dword ptr [eax]
	mov ecx, dword ptr [eax+4]
	mov dword ptr [ebp-8], ebx
	mov dword ptr [ebp-4], ecx
	push dword ptr [ebp-4]
	push dword ptr [ebp-8]
	mov ebx, dword ptr [ebp-8]
	mov ebx, dword ptr [ebp-4]
	add ebx, dword ptr [ebp+12] 
	adc ebx, dword ptr [ebp+16] 
	push ebx
	push ebx
	push eax
	call __InterlockedCompareExchange64
	add esp, 20
	cmp eax, dword ptr[ebp-8]
	jne @F
	cmp edx, dword ptr[ebp-4]
	je $217_
@@:
	jmp $216_
$217_:
	mov eax, dword ptr [ebp-8]
	mov eax, dword ptr [ebp-4]
	add eax, dword ptr [ebp+12] 
	adc eax, dword ptr [ebp+16] 
	mov eax, eax
	mov edx, eax
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function _InlineInterlockedAdd64 end
;function _InlineInterlockedOr64 begin
__InlineInterlockedOr64: 
	push ebp
	mov ebp, esp
	sub esp, 8
	push ebx
	push esi
	push edi
	mov eax, dword ptr [ebp+12]
	mov ebx, dword ptr [ebp+16]
	mov dword ptr [ebp+12], eax
	mov dword ptr [ebp+16], ebx
$222_:
	mov eax, dword ptr [ebp+8]
	mov ebx, dword ptr [eax]
	mov ecx, dword ptr [eax+4]
	mov dword ptr [ebp-8], ebx
	mov dword ptr [ebp-4], ecx
	push dword ptr [ebp-4]
	push dword ptr [ebp-8]
	mov ebx, dword ptr [ebp-8]
	mov ebx, dword ptr [ebp-4]
	or ebx, dword ptr [ebp+12]
	or ebx, dword ptr [ebp+16]
	push ebx
	push ebx
	push eax
	call __InterlockedCompareExchange64
	add esp, 20
	cmp eax, dword ptr[ebp-8]
	jne @F
	cmp edx, dword ptr[ebp-4]
	je $223_
@@:
	jmp $222_
$223_:
	mov eax, dword ptr [ebp-8]
	mov edx, dword ptr [ebp-4]
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function _InlineInterlockedOr64 end
;function _InlineInterlockedXor64 begin
__InlineInterlockedXor64: 
	push ebp
	mov ebp, esp
	sub esp, 8
	push ebx
	push esi
	push edi
	mov eax, dword ptr [ebp+12]
	mov ebx, dword ptr [ebp+16]
	mov dword ptr [ebp+12], eax
	mov dword ptr [ebp+16], ebx
$228_:
	mov eax, dword ptr [ebp+8]
	mov ebx, dword ptr [eax]
	mov ecx, dword ptr [eax+4]
	mov dword ptr [ebp-8], ebx
	mov dword ptr [ebp-4], ecx
	push dword ptr [ebp-4]
	push dword ptr [ebp-8]
	mov ebx, dword ptr [ebp-8]
	mov ebx, dword ptr [ebp-4]
	xor ebx, dword ptr [ebp+12]
	xor ebx, dword ptr [ebp+16]
	push ebx
	push ebx
	push eax
	call __InterlockedCompareExchange64
	add esp, 20
	cmp eax, dword ptr[ebp-8]
	jne @F
	cmp edx, dword ptr[ebp-4]
	je $229_
@@:
	jmp $228_
$229_:
	mov eax, dword ptr [ebp-8]
	mov edx, dword ptr [ebp-4]
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function _InlineInterlockedXor64 end
;function _InlineInterlockedIncrement64 begin
__InlineInterlockedIncrement64: 
	push ebp
	mov ebp, esp
	sub esp, 8
	push ebx
	push esi
	push edi
$234_:
	mov eax, dword ptr [ebp+8]
	mov ebx, dword ptr [eax]
	mov ecx, dword ptr [eax+4]
	mov dword ptr [ebp-8], ebx
	mov dword ptr [ebp-4], ecx
	push dword ptr [ebp-4]
	push dword ptr [ebp-8]
	mov ebx, dword ptr [ebp-8]
	mov ebx, dword ptr [ebp-4]
	add ebx, 1
	adc ebx, 0
	push ebx
	push ebx
	push eax
	call __InterlockedCompareExchange64
	add esp, 20
	cmp eax, dword ptr[ebp-8]
	jne @F
	cmp edx, dword ptr[ebp-4]
	je $235_
@@:
	jmp $234_
$235_:
	mov eax, dword ptr [ebp-8]
	mov eax, dword ptr [ebp-4]
	add eax, 1
	adc eax, 0
	mov eax, eax
	mov edx, eax
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function _InlineInterlockedIncrement64 end
;function _InlineInterlockedDecrement64 begin
__InlineInterlockedDecrement64: 
	push ebp
	mov ebp, esp
	sub esp, 8
	push ebx
	push esi
	push edi
$241_:
	mov eax, dword ptr [ebp+8]
	mov ebx, dword ptr [eax]
	mov ecx, dword ptr [eax+4]
	mov dword ptr [ebp-8], ebx
	mov dword ptr [ebp-4], ecx
	push dword ptr [ebp-4]
	push dword ptr [ebp-8]
	mov ebx, dword ptr [ebp-8]
	mov ebx, dword ptr [ebp-4]
	add ebx, 1
	sbb ebx, 0
	push ebx
	push ebx
	push eax
	call __InterlockedCompareExchange64
	add esp, 20
	cmp eax, dword ptr[ebp-8]
	jne @F
	cmp edx, dword ptr[ebp-4]
	je $242_
@@:
	jmp $241_
$242_:
	mov eax, dword ptr [ebp-8]
	mov eax, dword ptr [ebp-4]
	add eax, 1
	sbb eax, 0
	mov eax, eax
	mov edx, eax
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function _InlineInterlockedDecrement64 end
;function _InlineInterlockedExchange64 begin
__InlineInterlockedExchange64: 
	push ebp
	mov ebp, esp
	sub esp, 8
	push ebx
	push esi
	push edi
	mov eax, dword ptr [ebp+12]
	mov ebx, dword ptr [ebp+16]
	mov dword ptr [ebp+12], eax
	mov dword ptr [ebp+16], ebx
$247_:
	mov eax, dword ptr [ebp+8]
	mov ebx, dword ptr [eax]
	mov ecx, dword ptr [eax+4]
	mov dword ptr [ebp-8], ebx
	mov dword ptr [ebp-4], ecx
	push dword ptr [ebp-4]
	push dword ptr [ebp-8]
	push dword ptr [ebp+16]
	push dword ptr [ebp+12]
	push eax
	call __InterlockedCompareExchange64
	add esp, 20
	cmp eax, dword ptr[ebp-8]
	jne @F
	cmp edx, dword ptr[ebp-4]
	je $248_
@@:
	jmp $247_
$248_:
	mov eax, dword ptr [ebp-8]
	mov edx, dword ptr [ebp-4]
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function _InlineInterlockedExchange64 end
;function _InlineInterlockedExchangeAdd64 begin
__InlineInterlockedExchangeAdd64: 
	push ebp
	mov ebp, esp
	sub esp, 8
	push ebx
	push esi
	push edi
	mov eax, dword ptr [ebp+12]
	mov ebx, dword ptr [ebp+16]
	mov dword ptr [ebp+12], eax
	mov dword ptr [ebp+16], ebx
$253_:
	mov eax, dword ptr [ebp+8]
	mov ebx, dword ptr [eax]
	mov ecx, dword ptr [eax+4]
	mov dword ptr [ebp-8], ebx
	mov dword ptr [ebp-4], ecx
	push dword ptr [ebp-4]
	push dword ptr [ebp-8]
	mov ebx, dword ptr [ebp-8]
	mov ebx, dword ptr [ebp-4]
	add ebx, dword ptr [ebp+12] 
	adc ebx, dword ptr [ebp+16] 
	push ebx
	push ebx
	push eax
	call __InterlockedCompareExchange64
	add esp, 20
	cmp eax, dword ptr[ebp-8]
	jne @F
	cmp edx, dword ptr[ebp-4]
	je $254_
@@:
	jmp $253_
$254_:
	mov eax, dword ptr [ebp-8]
	mov edx, dword ptr [ebp-4]
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function _InlineInterlockedExchangeAdd64 end
;function MemoryBarrier begin
_MemoryBarrier: 
	push ebp
	mov ebp, esp
	sub esp, 4
	push ebx
	push esi
	push edi
	push 0
	lea eax, dword ptr [ebp-4]
	push eax
	call __InterlockedOr
	add esp, 8
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function MemoryBarrier end
;function GetFiberData begin
_GetFiberData: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	push 16
	call ___readfsdword
	add esp, 4
	mov eax, dword ptr [eax]
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function GetFiberData end
;function GetCurrentFiber begin
_GetCurrentFiber: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	push 16
	call ___readfsdword
	add esp, 4
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function GetCurrentFiber end
;function ReadAcquire8 begin
_ReadAcquire8: 
	push ebp
	mov ebp, esp
	sub esp, 4
	push ebx
	push esi
	push edi
	mov eax, dword ptr [ebp+8]
	mov bl, byte ptr [eax]
	mov byte ptr [ebp-4], bl
	mov al, byte ptr [ebp-4]
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function ReadAcquire8 end
;function ReadNoFence8 begin
_ReadNoFence8: 
	push ebp
	mov ebp, esp
	sub esp, 4
	push ebx
	push esi
	push edi
	mov eax, dword ptr [ebp+8]
	mov bl, byte ptr [eax]
	mov byte ptr [ebp-4], bl
	mov al, byte ptr [ebp-4]
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function ReadNoFence8 end
;function WriteRelease8 begin
_WriteRelease8: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	mov al, byte ptr[ebp+12]
	mov byte ptr [ebp+12], al
	mov eax, dword ptr [ebp+8]
	mov bl, byte ptr [ebp+12]
	mov byte ptr [eax], bl
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function WriteRelease8 end
;function WriteNoFence8 begin
_WriteNoFence8: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	mov al, byte ptr[ebp+12]
	mov byte ptr [ebp+12], al
	mov eax, dword ptr [ebp+8]
	mov bl, byte ptr [ebp+12]
	mov byte ptr [eax], bl
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function WriteNoFence8 end
;function ReadAcquire16 begin
_ReadAcquire16: 
	push ebp
	mov ebp, esp
	sub esp, 4
	push ebx
	push esi
	push edi
	mov eax, dword ptr [ebp+8]
	mov bx, word ptr [eax]
	mov word ptr [ebp-4], bx
	mov ax, word ptr [ebp-4]
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function ReadAcquire16 end
;function ReadNoFence16 begin
_ReadNoFence16: 
	push ebp
	mov ebp, esp
	sub esp, 4
	push ebx
	push esi
	push edi
	mov eax, dword ptr [ebp+8]
	mov bx, word ptr [eax]
	mov word ptr [ebp-4], bx
	mov ax, word ptr [ebp-4]
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function ReadNoFence16 end
;function WriteRelease16 begin
_WriteRelease16: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	mov ax, word ptr[ebp+12]
	mov word ptr [ebp+12], ax
	mov eax, dword ptr [ebp+8]
	mov bx, word ptr [ebp+12]
	mov word ptr [eax], bx
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function WriteRelease16 end
;function WriteNoFence16 begin
_WriteNoFence16: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	mov ax, word ptr[ebp+12]
	mov word ptr [ebp+12], ax
	mov eax, dword ptr [ebp+8]
	mov bx, word ptr [ebp+12]
	mov word ptr [eax], bx
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function WriteNoFence16 end
;function ReadAcquire begin
_ReadAcquire: 
	push ebp
	mov ebp, esp
	sub esp, 4
	push ebx
	push esi
	push edi
	mov eax, dword ptr [ebp+8]
	mov ebx, dword ptr [eax]
	mov dword ptr [ebp-4], ebx
	mov eax, dword ptr [ebp-4]
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function ReadAcquire end
;function ReadNoFence begin
_ReadNoFence: 
	push ebp
	mov ebp, esp
	sub esp, 4
	push ebx
	push esi
	push edi
	mov eax, dword ptr [ebp+8]
	mov ebx, dword ptr [eax]
	mov dword ptr [ebp-4], ebx
	mov eax, dword ptr [ebp-4]
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function ReadNoFence end
;function WriteRelease begin
_WriteRelease: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	mov eax, dword ptr [ebp+8]
	mov ebx, dword ptr [ebp+12]
	mov dword ptr [eax], ebx
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function WriteRelease end
;function WriteNoFence begin
_WriteNoFence: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	mov eax, dword ptr [ebp+8]
	mov ebx, dword ptr [ebp+12]
	mov dword ptr [eax], ebx
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function WriteNoFence end
;function ReadAcquire64 begin
_ReadAcquire64: 
	push ebp
	mov ebp, esp
	sub esp, 8
	push ebx
	push esi
	push edi
	mov eax, dword ptr [ebp+8]
	mov ebx, dword ptr [eax]
	mov ecx, dword ptr [eax+4]
	mov dword ptr [ebp-8], ebx
	mov dword ptr [ebp-4], ecx
	mov eax, dword ptr [ebp-8]
	mov edx, dword ptr [ebp-4]
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function ReadAcquire64 end
;function ReadNoFence64 begin
_ReadNoFence64: 
	push ebp
	mov ebp, esp
	sub esp, 8
	push ebx
	push esi
	push edi
	mov eax, dword ptr [ebp+8]
	mov ebx, dword ptr [eax]
	mov ecx, dword ptr [eax+4]
	mov dword ptr [ebp-8], ebx
	mov dword ptr [ebp-4], ecx
	mov eax, dword ptr [ebp-8]
	mov edx, dword ptr [ebp-4]
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function ReadNoFence64 end
;function WriteRelease64 begin
_WriteRelease64: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	mov eax, dword ptr [ebp+12]
	mov ebx, dword ptr [ebp+16]
	mov dword ptr [ebp+12], eax
	mov dword ptr [ebp+16], ebx
	mov eax, dword ptr [ebp+8]
	mov ebx, dword ptr [ebp+12]
	mov ecx, dword ptr [ebp+16]
	mov dword ptr [eax], ebx
	mov dword ptr [eax+4], ecx
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function WriteRelease64 end
;function WriteNoFence64 begin
_WriteNoFence64: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	mov eax, dword ptr [ebp+12]
	mov ebx, dword ptr [ebp+16]
	mov dword ptr [ebp+12], eax
	mov dword ptr [ebp+16], ebx
	mov eax, dword ptr [ebp+8]
	mov ebx, dword ptr [ebp+12]
	mov ecx, dword ptr [ebp+16]
	mov dword ptr [eax], ebx
	mov dword ptr [eax+4], ecx
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function WriteNoFence64 end
;function ReadRaw8 begin
_ReadRaw8: 
	push ebp
	mov ebp, esp
	sub esp, 4
	push ebx
	push esi
	push edi
	mov eax, dword ptr [ebp+8]
	mov bl, byte ptr [eax]
	mov byte ptr [ebp-4], bl
	mov al, byte ptr [ebp-4]
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function ReadRaw8 end
;function WriteRaw8 begin
_WriteRaw8: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	mov al, byte ptr[ebp+12]
	mov byte ptr [ebp+12], al
	mov eax, dword ptr [ebp+8]
	mov bl, byte ptr [ebp+12]
	mov byte ptr [eax], bl
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function WriteRaw8 end
;function ReadRaw16 begin
_ReadRaw16: 
	push ebp
	mov ebp, esp
	sub esp, 4
	push ebx
	push esi
	push edi
	mov eax, dword ptr [ebp+8]
	mov bx, word ptr [eax]
	mov word ptr [ebp-4], bx
	mov ax, word ptr [ebp-4]
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function ReadRaw16 end
;function WriteRaw16 begin
_WriteRaw16: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	mov ax, word ptr[ebp+12]
	mov word ptr [ebp+12], ax
	mov eax, dword ptr [ebp+8]
	mov bx, word ptr [ebp+12]
	mov word ptr [eax], bx
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function WriteRaw16 end
;function ReadRaw begin
_ReadRaw: 
	push ebp
	mov ebp, esp
	sub esp, 4
	push ebx
	push esi
	push edi
	mov eax, dword ptr [ebp+8]
	mov ebx, dword ptr [eax]
	mov dword ptr [ebp-4], ebx
	mov eax, dword ptr [ebp-4]
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function ReadRaw end
;function WriteRaw begin
_WriteRaw: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	mov eax, dword ptr [ebp+8]
	mov ebx, dword ptr [ebp+12]
	mov dword ptr [eax], ebx
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function WriteRaw end
;function ReadRaw64 begin
_ReadRaw64: 
	push ebp
	mov ebp, esp
	sub esp, 8
	push ebx
	push esi
	push edi
	mov eax, dword ptr [ebp+8]
	mov ebx, dword ptr [eax]
	mov ecx, dword ptr [eax+4]
	mov dword ptr [ebp-8], ebx
	mov dword ptr [ebp-4], ecx
	mov eax, dword ptr [ebp-8]
	mov edx, dword ptr [ebp-4]
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function ReadRaw64 end
;function WriteRaw64 begin
_WriteRaw64: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	mov eax, dword ptr [ebp+12]
	mov ebx, dword ptr [ebp+16]
	mov dword ptr [ebp+12], eax
	mov dword ptr [ebp+16], ebx
	mov eax, dword ptr [ebp+8]
	mov ebx, dword ptr [ebp+12]
	mov ecx, dword ptr [ebp+16]
	mov dword ptr [eax], ebx
	mov dword ptr [eax+4], ecx
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function WriteRaw64 end
;function ReadUCharAcquire begin
_ReadUCharAcquire: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	push dword ptr [ebp+8]
	call _ReadAcquire8
	add esp, 4
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function ReadUCharAcquire end
;function ReadUCharNoFence begin
_ReadUCharNoFence: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	push dword ptr [ebp+8]
	call _ReadNoFence8
	add esp, 4
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function ReadUCharNoFence end
;function ReadBooleanAcquire begin
_ReadBooleanAcquire: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	push dword ptr [ebp+8]
	call _ReadAcquire8
	add esp, 4
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function ReadBooleanAcquire end
;function ReadBooleanNoFence begin
_ReadBooleanNoFence: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	push dword ptr [ebp+8]
	call _ReadNoFence8
	add esp, 4
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function ReadBooleanNoFence end
;function ReadUCharRaw begin
_ReadUCharRaw: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	push dword ptr [ebp+8]
	call _ReadRaw8
	add esp, 4
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function ReadUCharRaw end
;function WriteUCharRelease begin
_WriteUCharRelease: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	mov al, byte ptr[ebp+12]
	mov byte ptr [ebp+12], al
	mov al, byte ptr [ebp+12]
	movzx ebx, al
	push ebx
	push dword ptr [ebp+8]
	call _WriteRelease8
	add esp, 8
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function WriteUCharRelease end
;function WriteUCharNoFence begin
_WriteUCharNoFence: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	mov al, byte ptr[ebp+12]
	mov byte ptr [ebp+12], al
	mov al, byte ptr [ebp+12]
	movzx ebx, al
	push ebx
	push dword ptr [ebp+8]
	call _WriteNoFence8
	add esp, 8
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function WriteUCharNoFence end
;function WriteBooleanRelease begin
_WriteBooleanRelease: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	mov al, byte ptr[ebp+12]
	mov byte ptr [ebp+12], al
	mov al, byte ptr [ebp+12]
	movzx ebx, al
	push ebx
	push dword ptr [ebp+8]
	call _WriteRelease8
	add esp, 8
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function WriteBooleanRelease end
;function WriteBooleanNoFence begin
_WriteBooleanNoFence: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	mov al, byte ptr[ebp+12]
	mov byte ptr [ebp+12], al
	mov al, byte ptr [ebp+12]
	movzx ebx, al
	push ebx
	push dword ptr [ebp+8]
	call _WriteNoFence8
	add esp, 8
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function WriteBooleanNoFence end
;function WriteUCharRaw begin
_WriteUCharRaw: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	mov al, byte ptr[ebp+12]
	mov byte ptr [ebp+12], al
	mov al, byte ptr [ebp+12]
	movzx ebx, al
	push ebx
	push dword ptr [ebp+8]
	call _WriteRaw8
	add esp, 8
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function WriteUCharRaw end
;function ReadUShortAcquire begin
_ReadUShortAcquire: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	push dword ptr [ebp+8]
	call _ReadAcquire16
	add esp, 4
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function ReadUShortAcquire end
;function ReadUShortNoFence begin
_ReadUShortNoFence: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	push dword ptr [ebp+8]
	call _ReadNoFence16
	add esp, 4
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function ReadUShortNoFence end
;function ReadUShortRaw begin
_ReadUShortRaw: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	push dword ptr [ebp+8]
	call _ReadRaw16
	add esp, 4
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function ReadUShortRaw end
;function WriteUShortRelease begin
_WriteUShortRelease: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	mov ax, word ptr[ebp+12]
	mov word ptr [ebp+12], ax
	mov ax, word ptr [ebp+12]
	movsx ebx, ax
	push ebx
	push dword ptr [ebp+8]
	call _WriteRelease16
	add esp, 8
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function WriteUShortRelease end
;function WriteUShortNoFence begin
_WriteUShortNoFence: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	mov ax, word ptr[ebp+12]
	mov word ptr [ebp+12], ax
	mov ax, word ptr [ebp+12]
	movsx ebx, ax
	push ebx
	push dword ptr [ebp+8]
	call _WriteNoFence16
	add esp, 8
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function WriteUShortNoFence end
;function WriteUShortRaw begin
_WriteUShortRaw: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	mov ax, word ptr[ebp+12]
	mov word ptr [ebp+12], ax
	mov ax, word ptr [ebp+12]
	movsx ebx, ax
	push ebx
	push dword ptr [ebp+8]
	call _WriteRaw16
	add esp, 8
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function WriteUShortRaw end
;function ReadULongAcquire begin
_ReadULongAcquire: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	push dword ptr [ebp+8]
	call _ReadAcquire
	add esp, 4
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function ReadULongAcquire end
;function ReadULongNoFence begin
_ReadULongNoFence: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	push dword ptr [ebp+8]
	call _ReadNoFence
	add esp, 4
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function ReadULongNoFence end
;function ReadULongRaw begin
_ReadULongRaw: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	push dword ptr [ebp+8]
	call _ReadRaw
	add esp, 4
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function ReadULongRaw end
;function WriteULongRelease begin
_WriteULongRelease: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	mov eax, dword ptr [ebp+12]
	push eax
	push dword ptr [ebp+8]
	call _WriteRelease
	add esp, 8
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function WriteULongRelease end
;function WriteULongNoFence begin
_WriteULongNoFence: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	mov eax, dword ptr [ebp+12]
	push eax
	push dword ptr [ebp+8]
	call _WriteNoFence
	add esp, 8
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function WriteULongNoFence end
;function WriteULongRaw begin
_WriteULongRaw: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	mov eax, dword ptr [ebp+12]
	push eax
	push dword ptr [ebp+8]
	call _WriteRaw
	add esp, 8
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function WriteULongRaw end
;function ReadULong64Acquire begin
_ReadULong64Acquire: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	push dword ptr [ebp+8]
	call _ReadAcquire64
	add esp, 4
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function ReadULong64Acquire end
;function ReadULong64NoFence begin
_ReadULong64NoFence: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	push dword ptr [ebp+8]
	call _ReadNoFence64
	add esp, 4
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function ReadULong64NoFence end
;function ReadULong64Raw begin
_ReadULong64Raw: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	push dword ptr [ebp+8]
	call _ReadRaw64
	add esp, 4
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function ReadULong64Raw end
;function WriteULong64Release begin
_WriteULong64Release: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	mov eax, dword ptr [ebp+12]
	mov ebx, dword ptr [ebp+16]
	mov dword ptr [ebp+12], eax
	mov dword ptr [ebp+16], ebx
	mov eax, dword ptr [ebp+12]
	mov ebx, dword ptr [ebp+16]
	push ebx
	push eax
	push dword ptr [ebp+8]
	call _WriteRelease64
	add esp, 12
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function WriteULong64Release end
;function WriteULong64NoFence begin
_WriteULong64NoFence: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	mov eax, dword ptr [ebp+12]
	mov ebx, dword ptr [ebp+16]
	mov dword ptr [ebp+12], eax
	mov dword ptr [ebp+16], ebx
	mov eax, dword ptr [ebp+12]
	mov ebx, dword ptr [ebp+16]
	push ebx
	push eax
	push dword ptr [ebp+8]
	call _WriteNoFence64
	add esp, 12
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function WriteULong64NoFence end
;function WriteULong64Raw begin
_WriteULong64Raw: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	mov eax, dword ptr [ebp+12]
	mov ebx, dword ptr [ebp+16]
	mov dword ptr [ebp+12], eax
	mov dword ptr [ebp+16], ebx
	mov eax, dword ptr [ebp+12]
	mov ebx, dword ptr [ebp+16]
	push ebx
	push eax
	push dword ptr [ebp+8]
	call _WriteRaw64
	add esp, 12
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function WriteULong64Raw end
;function ReadPointerAcquire begin
_ReadPointerAcquire: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	push dword ptr [ebp+8]
	call _ReadAcquire
	add esp, 4
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function ReadPointerAcquire end
;function ReadPointerNoFence begin
_ReadPointerNoFence: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	push dword ptr [ebp+8]
	call _ReadNoFence
	add esp, 4
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function ReadPointerNoFence end
;function ReadPointerRaw begin
_ReadPointerRaw: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	push dword ptr [ebp+8]
	call _ReadRaw
	add esp, 4
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function ReadPointerRaw end
;function WritePointerRelease begin
_WritePointerRelease: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	mov eax, dword ptr [ebp+12]
	push eax
	push dword ptr [ebp+8]
	call _WriteRelease
	add esp, 8
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function WritePointerRelease end
;function WritePointerNoFence begin
_WritePointerNoFence: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	mov eax, dword ptr [ebp+12]
	push eax
	push dword ptr [ebp+8]
	call _WriteNoFence
	add esp, 8
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function WritePointerNoFence end
;function WritePointerRaw begin
_WritePointerRaw: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	mov eax, dword ptr [ebp+12]
	push eax
	push dword ptr [ebp+8]
	call _WriteRaw
	add esp, 8
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function WritePointerRaw end
;function HEAP_MAKE_TAG_FLAGS begin
_HEAP_MAKE_TAG_FLAGS: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	mov eax, dword ptr [ebp+12]
	shl eax, 18
	mov ebx, dword ptr [ebp+8]
	add ebx, eax
	mov eax, ebx
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function HEAP_MAKE_TAG_FLAGS end
;function RtlSecureZeroMemory begin
_RtlSecureZeroMemory: 
	push ebp
	mov ebp, esp
	sub esp, 12
	push ebx
	push esi
	push edi
	mov eax, dword ptr [ebp+8]
	mov dword ptr [ebp-4], eax
$598_:
	mov eax, dword ptr [ebp+12]
	cmp eax, 0
	je $600_
	mov eax, dword ptr [ebp-4]
	mov byte ptr [eax], 0
	mov eax, dword ptr [ebp-4]
	mov dword ptr [ebp-8], eax
	add eax, 1
	mov dword ptr [ebp-4], eax
	mov eax, dword ptr [ebp+12]
	mov dword ptr [ebp-12], eax
	sub eax, 1
	mov dword ptr [ebp+12], eax
	jmp $598_
$600_:
	mov eax, dword ptr [ebp+8]
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function RtlSecureZeroMemory end
;function CUSTOM_SYSTEM_EVENT_TRIGGER_INIT begin
_CUSTOM_SYSTEM_EVENT_TRIGGER_INIT: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	push 8
	push 0
	push dword ptr [ebp+8]
	call _memset
	add esp, 12
	mov eax, dword ptr [ebp+8]
	add eax, 0
	mov dword ptr [eax], 8
	mov eax, dword ptr [ebp+8]
	add eax, 4
	mov ebx, dword ptr [ebp+12]
	mov dword ptr [eax], ebx
$607_:
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function CUSTOM_SYSTEM_EVENT_TRIGGER_INIT end
;function TpInitializeCallbackEnviron begin
_TpInitializeCallbackEnviron: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	mov eax, dword ptr [ebp+8]
	add eax, 0
	mov dword ptr [eax], 3
	mov eax, dword ptr [ebp+8]
	add eax, 4
	mov dword ptr [eax], 0
	mov eax, dword ptr [ebp+8]
	add eax, 8
	mov dword ptr [eax], 0
	mov eax, dword ptr [ebp+8]
	add eax, 12
	mov dword ptr [eax], 0
	mov eax, dword ptr [ebp+8]
	add eax, 16
	mov dword ptr [eax], 0
	mov eax, dword ptr [ebp+8]
	add eax, 20
	mov dword ptr [eax], 0
	mov eax, dword ptr [ebp+8]
	add eax, 24
	mov dword ptr [eax], 0
	mov eax, dword ptr [ebp+8]
	add eax, 28
	mov dword ptr [eax], 0
	mov eax, dword ptr [ebp+8]
	add eax, 32
	mov dword ptr [eax], 1
	mov eax, dword ptr [ebp+8]
	add eax, 36
	mov dword ptr [eax], 40
$623_:
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function TpInitializeCallbackEnviron end
;function TpSetCallbackThreadpool begin
_TpSetCallbackThreadpool: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	mov eax, dword ptr [ebp+8]
	add eax, 4
	mov ebx, dword ptr [ebp+12]
	mov dword ptr [eax], ebx
$641_:
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function TpSetCallbackThreadpool end
;function TpSetCallbackCleanupGroup begin
_TpSetCallbackCleanupGroup: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	mov eax, dword ptr [ebp+8]
	add eax, 8
	mov ebx, dword ptr [ebp+12]
	mov dword ptr [eax], ebx
	mov eax, dword ptr [ebp+8]
	add eax, 12
	mov ebx, dword ptr [ebp+16]
	mov dword ptr [eax], ebx
$644_:
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function TpSetCallbackCleanupGroup end
;function TpSetCallbackActivationContext begin
_TpSetCallbackActivationContext: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	mov eax, dword ptr [ebp+8]
	add eax, 20
	mov ebx, dword ptr [ebp+12]
	mov dword ptr [eax], ebx
$647_:
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function TpSetCallbackActivationContext end
;function TpSetCallbackNoActivationContext begin
_TpSetCallbackNoActivationContext: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	mov eax, dword ptr [ebp+8]
	add eax, 20
	mov dword ptr [eax], -1
$650_:
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function TpSetCallbackNoActivationContext end
;function TpSetCallbackLongFunction begin
_TpSetCallbackLongFunction: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	mov eax, dword ptr [ebp+8]
	add eax, 28
	mov ebx, dword ptr [eax]
	and ebx, -2
	mov ecx, 1
	or ecx, ebx
	mov dword ptr [eax], ecx
$654_:
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function TpSetCallbackLongFunction end
;function TpSetCallbackRaceWithDll begin
_TpSetCallbackRaceWithDll: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	mov eax, dword ptr [ebp+8]
	add eax, 16
	mov ebx, dword ptr [ebp+12]
	mov dword ptr [eax], ebx
$659_:
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function TpSetCallbackRaceWithDll end
;function TpSetCallbackFinalizationCallback begin
_TpSetCallbackFinalizationCallback: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	mov eax, dword ptr [ebp+8]
	add eax, 24
	mov ebx, dword ptr [ebp+12]
	mov dword ptr [eax], ebx
$662_:
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function TpSetCallbackFinalizationCallback end
;function TpSetCallbackPriority begin
_TpSetCallbackPriority: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	mov eax, dword ptr [ebp+8]
	add eax, 32
	mov dword ptr [eax], 0
$665_:
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function TpSetCallbackPriority end
;function TpSetCallbackPersistent begin
_TpSetCallbackPersistent: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	mov eax, dword ptr [ebp+8]
	add eax, 28
	mov ebx, dword ptr [eax]
	and ebx, -3
	mov ecx, 2
	or ecx, ebx
	mov dword ptr [eax], ecx
$668_:
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function TpSetCallbackPersistent end
;function TpDestroyCallbackEnviron begin
_TpDestroyCallbackEnviron: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
$673_:
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function TpDestroyCallbackEnviron end
;function NtCurrentTeb begin
_NtCurrentTeb: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	push 24
	call ___readfsdword
	add esp, 4
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function NtCurrentTeb end
;function GetCurrentProcessToken begin
_GetCurrentProcessToken: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	mov eax, -4
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function GetCurrentProcessToken end
;function GetCurrentThreadToken begin
_GetCurrentThreadToken: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	mov eax, -5
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function GetCurrentThreadToken end
;function GetCurrentThreadEffectiveToken begin
_GetCurrentThreadEffectiveToken: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	mov eax, -6
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function GetCurrentThreadEffectiveToken end
;function MapViewOfFile2 begin
_MapViewOfFile2: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	mov eax, dword ptr [ebp+16]
	mov ebx, dword ptr [ebp+20]
	mov dword ptr [ebp+16], eax
	mov dword ptr [ebp+20], ebx
	push -1
	push dword ptr [ebp+36]
	push dword ptr [ebp+32]
	push dword ptr [ebp+28]
	push dword ptr [ebp+24]
	push dword ptr [ebp+20]
	push dword ptr [ebp+16]
	push dword ptr [ebp+12]
	push dword ptr [ebp+8]
	call _MapViewOfFileNuma2
	add esp, 36
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function MapViewOfFile2 end
;function InitializeThreadpoolEnvironment begin
_InitializeThreadpoolEnvironment: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	push dword ptr [ebp+8]
	call _TpInitializeCallbackEnviron
	add esp, 4
$717_:
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function InitializeThreadpoolEnvironment end
;function SetThreadpoolCallbackPool begin
_SetThreadpoolCallbackPool: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	push dword ptr [ebp+12]
	push dword ptr [ebp+8]
	call _TpSetCallbackThreadpool
	add esp, 8
$720_:
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function SetThreadpoolCallbackPool end
;function SetThreadpoolCallbackCleanupGroup begin
_SetThreadpoolCallbackCleanupGroup: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	push dword ptr [ebp+16]
	push dword ptr [ebp+12]
	push dword ptr [ebp+8]
	call _TpSetCallbackCleanupGroup
	add esp, 12
$723_:
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function SetThreadpoolCallbackCleanupGroup end
;function SetThreadpoolCallbackRunsLong begin
_SetThreadpoolCallbackRunsLong: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	push dword ptr [ebp+8]
	call _TpSetCallbackLongFunction
	add esp, 4
$726_:
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function SetThreadpoolCallbackRunsLong end
;function SetThreadpoolCallbackLibrary begin
_SetThreadpoolCallbackLibrary: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	push dword ptr [ebp+12]
	push dword ptr [ebp+8]
	call _TpSetCallbackRaceWithDll
	add esp, 8
$729_:
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function SetThreadpoolCallbackLibrary end
;function SetThreadpoolCallbackPriority begin
_SetThreadpoolCallbackPriority: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	push 0
	push dword ptr [ebp+8]
	call _TpSetCallbackPriority
	add esp, 8
$732_:
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function SetThreadpoolCallbackPriority end
;function DestroyThreadpoolEnvironment begin
_DestroyThreadpoolEnvironment: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	push dword ptr [ebp+8]
	call _TpDestroyCallbackEnviron
	add esp, 4
$735_:
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function DestroyThreadpoolEnvironment end
;function SetThreadpoolCallbackPersistent begin
_SetThreadpoolCallbackPersistent: 
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	push dword ptr [ebp+8]
	call _TpSetCallbackPersistent
	add esp, 4
$738_:
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function SetThreadpoolCallbackPersistent end
;function HRESULT_FROM_WIN32 begin
_HRESULT_FROM_WIN32: 
	push ebp
	mov ebp, esp
	sub esp, 4
	push ebx
	push esi
	push edi
	mov eax, dword ptr [ebp+8]
	cmp eax, 0
	jg $743_
	mov dword ptr [ebp-4], eax
	jmp $744_
$743_:
	mov eax, dword ptr [ebp+8]
	and eax, 65535
	or eax, 458752
	or eax, -2147483648
	mov dword ptr [ebp-4], eax
$744_:
	mov eax, dword ptr [ebp-4]
	pop edi
	pop esi
	pop ebx
	mov esp, ebp
	pop ebp
	ret
;function HRESULT_FROM_WIN32 end
