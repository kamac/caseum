format MS COFF
extrn _cas_strConcat
extrn _cas_intToStr
extrn _cas_strCompare
extrn '__imp__printf' as printf:dword
section '.text' code
_start:
call main
ret
main:
push ebp
mov ebp,esp
push edi
push esi
mov esi,_c32
push esi
call [printf]
add esp,4
._j31:
add esp,0
pop esi
pop edi
mov esp,ebp
pop ebp
ret
_c32:
db 'Hello world',0
