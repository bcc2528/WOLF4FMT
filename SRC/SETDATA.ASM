section .text
		align 4
global setdata_b
setdata_b:
	push	ebp
	mov ebp,esp
	push	edi
	push	es		;es���X�^�b�N�ɑΔ�
	mov ax,word [ebp+8]	; es = �Z�O�����g
	mov es,ax
	mov edi, [ebp+12]	;es:[edi] = �]����
	mov al,byte [ebp+16]	;eax = �������݃f�[�^
	mov ecx, [ebp+20]	;ecx = �������ݐ�
	cld
	rep stosb		;��������
	pop	es		;es�����ɖ߂�
	xor eax,eax		;�߂�l = ���0
	pop edi
	leave
	ret

		align 4
global setdata_w
setdata_w:
	push	ebp
	mov ebp,esp
	push	edi
	push	es		;es���X�^�b�N�ɑΔ�
	mov ax,word [ebp+8]	; es = �Z�O�����g
	mov es,ax
	mov edi, [ebp+12]	;es:[edi] = �]����
	mov ax, word[ebp+16]	;eax = �������݃f�[�^
	mov ecx, [ebp+20]	;ecx = �������ݐ�
	cld
	rep stosw		;��������
	pop	es		;es�����ɖ߂�
	xor eax,eax		;�߂�l = ���0
	pop edi
	leave
	ret

		align 4
global setdata_d
setdata_d:
	push	ebp
	mov ebp,esp
	push	edi
	push	es		;es���X�^�b�N�ɑΔ�
	mov ax,word [ebp+8]	; es = �Z�O�����g
	mov es,ax
	mov edi, [ebp+12]	;es:[edi] = �]����
	mov eax, [ebp+16]	;eax = �������݃f�[�^
	mov ecx, [ebp+20]	;ecx = �������ݐ�
	cld
	rep stosd		;��������
	pop	es		;es�����ɖ߂�
	xor eax,eax		;�߂�l = ���0
	pop edi
	leave
	ret
