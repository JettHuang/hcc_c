;-----------------------------------------
; F:/GitHub/hcc_c/dist/samples/variables.c
;-----------------------------------------
.486
.model flat
option casemap:none

.data
align 4
_pstr     dd    _1_ 
public _pstr
align 1
_name      db  49
    db  50
    db  51
    db  0
public _name
align 4
_pval     dd    _gs + 4 
public _pval

.data?
align 4
_gs      db  8  dup (?)
public _gs

.const
align 1
_1_     db   49, 50, 51, 52, 53, 54, 0 
end