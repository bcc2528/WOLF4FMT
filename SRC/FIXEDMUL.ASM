section .text
align 4

global FixedMul

FixedMul:
	mov eax, [esp+4]
	imul dword [esp+8]
	add eax, 8000h
	adc edx, 0h
	shrd eax, edx, 16
	ret

