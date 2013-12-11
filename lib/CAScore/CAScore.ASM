format MS COFF
extrn '__imp__free' as free:dword
extrn '__imp__printf' as printf:dword ;debug
extrn '__imp__strlen' as strlen:dword
extrn '__imp__strcat' as strcat:dword
extrn '__imp__strcpy' as strcpy:dword
extrn '__imp__malloc' as malloc:dword
extrn '__imp__strcmp' as strcmp:dword

section '.text' code
dbg1: db "1[%d] ",0
dbg2: db "2[%d] ",0
dbg3: db "3[%d] ",0
 public _cas_strLen
 ; @desc returns string length
 ; @arg1 valid cas string ptr
 _cas_strLen:
	push ebp
	mov ebp,esp
	push esi
	push edi

	push dword[ebp+8]
	call [strlen]
	add esp,4

	pop edi
	pop esi
	mov esp,ebp
	pop ebp
	ret

 public _cas_strConcat
; @desc returns a pointer to new string
 _cas_strConcat:
	push ebp
	mov ebp,esp
	push esi
	push edi

	mov ebx,0

	push dword[ebp+8]
	call _cas_strLen
	add esp,4
	add ebx,eax
	push dword[ebp+12]
	call _cas_strLen
	add esp,4
	add ebx,eax

	add ebx,1
	push ebx
	call [malloc]
	add esp,4
	push dword[ebp+8] ;source
	push eax ;destination
	call [strcpy]
	add esp,8
	push dword[ebp+12] ;source
	push eax ;destination
	call [strcat]
	add esp,8


	pop edi
	pop esi
	mov esp,ebp
	pop ebp
	ret

public _cas_intToStr
; @desc returns a pointer to new string created from int
_cas_intToStr:
	push ebp
	mov ebp,esp
	push esi
	push edi
	sub esp,12

	mov dword[ebp-12],1
	cmp dword[ebp+8],0
	jge @f
	mov dword[ebp-12],0 ; 0 - minus value, 1 - plus value
	@@:
	mov ebx,0 ; 0 digits
	mov esi,dword[ebp+8]
	mov dword[ebp-20],esi
	@@:
	mov edx,0
	mov eax,dword[ebp+8]
	mov ecx,0Ah
	idiv ecx
	mov dword[ebp+8],eax
	add ebx,1
	cmp dword[ebp+8],0
	jne @b

	add ebx,1
	mov edi,ebx
	push ebx
	call [malloc]
	add esp,4
	mov dword[ebp-16],eax

	mov esi,dword[ebp-20]
	mov dword[ebp+8],esi
	mov ebx,0
	@@:
	mov edx,0
	mov eax,dword[ebp+8]
	mov ecx,0Ah
	idiv ecx
	mov dword[ebp+8],eax
	mov esi,dword[ebp-16]
	add esi,edi
	sub esi,2
	sub esi,ebx
	add edx,48
	mov eax,edx
	and eax,0xff
	mov byte[esi],al
	add ebx,1
	cmp dword[ebp+8],0
	jne @b

	mov eax,dword[ebp-16]
	add eax,edi
	sub eax,1
	mov byte[eax],0
	mov eax,dword[ebp-16]
	mov esi,dword[ebp-20]
	mov dword[ebp+8],esi

	add esp,12
	pop edi
	pop esi
	mov esp,ebp
	pop ebp
	ret

public _cas_strCompare
; @desc sets eax to 1 if strings equal, 0 otherwise
_cas_strCompare:
	push ebp
	mov ebp,esp
	push esi
	push edi

	push dword[ebp+12]
	push dword[ebp+8]
	call [strcmp]
	cmp eax,0
	je .streq
	mov eax,0
	jmp .strend
      .streq:
	mov eax,1
      .strend:

	pop edi
	pop esi
	mov esp,ebp
	pop ebp
	ret