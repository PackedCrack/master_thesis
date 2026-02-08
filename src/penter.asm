EXTERN printf:PROC


.data
str1		db "Random func does nothing!\n",0
str2		db "But it counted time to do nothing!\n",0
format		db "Time to do nothing: %llu",0


.code
_penter PROC
	PUSHFQ
	PUSH	RAX
	PUSH	RBX
	PUSH	RCX
	PUSH	RDX
	PUSH	RSI
	PUSH	RDI
	PUSH	R8
	PUSH	R9
	PUSH	R10
	PUSH	R11
	PUSH	R12
	SUB		RSP, 8d							; Align stack to 16


	SUB		RSP, 96d
	movdqu	XMMWORD PTR [RSP+00h],	XMM0	; XMM registers - 128 bit
    movdqu	XMMWORD PTR [RSP+10h],	XMM1
    movdqu	XMMWORD PTR [RSP+20h],	XMM2
    movdqu	XMMWORD PTR [RSP+30h],	XMM3
    movdqu	XMMWORD PTR [RSP+40h],	XMM4
    movdqu	XMMWORD PTR [RSP+50h],	XMM5


	RDTSC					; R10 = start
	SHL		RDX,  32d
	MOV		R10D, EAX
	OR		R10,  RDX

	SUB		RSP,  32d		; Shadow space of 32 bytes
	LEA		RCX,  str1
	CALL	printf
	ADD		RSP,  32d		; Free Shadow space


	SUB		RSP,  32d		; Shadow space of 32 bytes
	LEA		RCX,  str2
	CALL	printf
	ADD		RSP,  32d		; Free Shadow space


	RDTSC					; R11 = end
	SHL		RDX,  32d
	MOV		R11d, EAX
	OR		R11,  RDX
	SUB		R11,  R10		; R11 = delta

	SUB		RSP,  32d		; Shadow space of 32 bytes
	LEA		RCX,  format
	MOV		RDX,  R11
	CALL	printf
	ADD		RSP,  32d		; Free Shadow space


    MOVDQU	XMM5, XMMWORD PTR [RSP+50h]
    MOVDQU	XMM4, XMMWORD PTR [RSP+40h]
    MOVDQU	XMM3, XMMWORD PTR [RSP+30h]
    MOVDQU	XMM2, XMMWORD PTR [RSP+20h]
    MOVDQU	XMM1, XMMWORD PTR [RSP+10h]
	MOVDQU	XMM0, XMMWORD PTR [RSP+00h]
	ADD		RSP, 96d

	
	ADD		RSP, 8d
	POP		R12
	POP		R11
	POP		R10
	POP		R9
	POP		R8
	POP		RDI
	POP		RSI
	POP		RDX
	POP		RCX
	POP		RBX
	POP		RAX
	POPFQ

	RET
_penter ENDP

END