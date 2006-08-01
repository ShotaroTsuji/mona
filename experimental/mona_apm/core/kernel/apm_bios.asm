BITS 32
%include "macro.asm"

cglobal apm_bios_call
extern _apm_eip

section .text

; word apm_bios_call(word fn, apm_bios_regs *regs);
_apm_bios_call:
	push ebx
	push ecx
	push edi
	push esi
	push ebp

	xor eax, eax
	xor ebx, ebx
	xor ecx, ecx
	xor edx, edx
	xor edi, edi
	xor esi, esi

	mov ax, [esp+28]
	mov bx, [eax+2]
	mov cx, [eax+4]
	mov dx, [eax+6]
	mov si, [eax+8]
	mov di, [eax+10]

	mov eax, 0x50
	mov ds, eax
	mov es, eax
	mov eax, [esp+24]
	mov ah, 0x53

	call far [cs:_apm_eip]
	mov ebp, 0x10
	mov ds, ebp
	mov es, ebp
	jc err

	mov eax, [esp+28]
	mov [eax], ax
	mov [eax+2], bx
	mov [eax+4], cx
	mov [eax+6], dx
	mov [eax+8], si
	mov [eax+10], di
	xor eax, eax

err:
	cbw
	cwde

	pop ebp
	pop esi
	pop edi
	pop ecx
	pop ebx
	ret
