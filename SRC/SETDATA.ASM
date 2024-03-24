section .text
		align 4
global setdata_b
setdata_b:
	push	ebp
	mov ebp,esp
	push	edi
	push	es		;esをスタックに対比
	mov ax,word [ebp+8]	; es = セグメント
	mov es,ax
	mov edi, [ebp+12]	;es:[edi] = 転送先
	mov al,byte [ebp+16]	;eax = 書き込みデータ
	mov ecx, [ebp+20]	;ecx = 書き込み数
	cld
	rep stosb		;書き込み
	pop	es		;esを元に戻す
	xor eax,eax		;戻り値 = 常に0
	pop edi
	leave
	ret

		align 4
global setdata_w
setdata_w:
	push	ebp
	mov ebp,esp
	push	edi
	push	es		;esをスタックに対比
	mov ax,word [ebp+8]	; es = セグメント
	mov es,ax
	mov edi, [ebp+12]	;es:[edi] = 転送先
	mov ax, word[ebp+16]	;eax = 書き込みデータ
	mov ecx, [ebp+20]	;ecx = 書き込み数
	cld
	rep stosw		;書き込み
	pop	es		;esを元に戻す
	xor eax,eax		;戻り値 = 常に0
	pop edi
	leave
	ret

		align 4
global setdata_d
setdata_d:
	push	ebp
	mov ebp,esp
	push	edi
	push	es		;esをスタックに対比
	mov ax,word [ebp+8]	; es = セグメント
	mov es,ax
	mov edi, [ebp+12]	;es:[edi] = 転送先
	mov eax, [ebp+16]	;eax = 書き込みデータ
	mov ecx, [ebp+20]	;ecx = 書き込み数
	cld
	rep stosd		;書き込み
	pop	es		;esを元に戻す
	xor eax,eax		;戻り値 = 常に0
	pop edi
	leave
	ret
