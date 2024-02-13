	bits 16
	section .text

	global vid_setmode_
vid_setmode_:
	int 10h
	retf

	global vid_setcolor_
vid_setcolor_:
	mov ah, dl
	mov dx, 3c8h
	out dx, al
	inc dx
	mov al, ah
	shr al, 2
	out dx, al
	mov al, bl
	shr al, 2
	out dx, al
	mov al, cl
	shr al, 2
	out dx, al
	retf

	global vid_setcolors_
vid_setcolors_:
	; TODO
	retf

; vi:set ts=8 sts=8 sw=8 ft=nasm:
