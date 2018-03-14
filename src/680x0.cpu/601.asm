;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; 601.ASM
;;
;; PowerPC 601 opcodes
;;
;; by Darek Mihocka, (C) 1999 Emulators Inc.
;;
;; 11/06/99 last update
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	INCLUDE 68000.inc

	Pass      = 1
	INCLUDE 601.inc
	INCLUDE 60119.inc
	INCLUDE 60131.inc
	INCLUDE 60159.inc
	INCLUDE 60163.inc

	Pass      = 2
	INCLUDE 601.inc
	INCLUDE 60119.inc
	INCLUDE 60131.inc
	INCLUDE 60159.inc
	INCLUDE 60163.inc

	Pass      = 0


    .CODE

ppcILLEGAL:
    int 3


ppc_fetch:
	movzx ebx,word ptr [esi]
	movzx eax,word ptr [esi-2]
	lea   esi,[esi-4]
	jmp   [opc_ppc+ebx*4]


	.CONST

    ;; Main jump table

opc_ppc LABEL DWORD

	opcode = 010000h
	REPEAT 010000h/020h
		DefOp %opcode, ppcILLEGAL, 20h
	ENDM


    ;; Extended opcode 19

ppc19 LABEL DWORD

	opcode = 190000h
	REPEAT 2048t
		DefOp %opcode, ppcILLEGAL
	ENDM


    ;; Extended opcode 31

ppc31 LABEL DWORD

	opcode = 310000h
	REPEAT 2048t
		DefOp %opcode, ppcILLEGAL
	ENDM


    ;; Extended opcode 59

ppc59 LABEL DWORD

	opcode = 590000h
	REPEAT 2048t
		DefOp %opcode, ppcILLEGAL
	ENDM


    ;; Extended opcode 63

ppc63 LABEL DWORD

	opcode = 630000h
	REPEAT 2048t
		DefOp %opcode, ppcILLEGAL
	ENDM

 END

