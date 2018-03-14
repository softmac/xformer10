
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; OPCODES7.ASM
;;
;; 68000 opcodes $7000 thru $7FFF
;;
;; Copyright (C) 1991-2008 by Darek Mihocka. All Rights Reserved.
;; Branch Always Software. http://www.emulators.com/
;;
;; Instructions in this range:
;;
;; MOVEQ - bit 8 == 0
;;       - bits 11..9 specify data register
;;       - bits 7..0 contain immediate 8-bit data
;;
;; 07/30/2008 darekm        last update
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


	INCLUDE 68000.inc

	opcode = 07000h
	REPEAT 8t

	Decode %opcode

	BigAlign              ; force alignment since only 24 routines

	Valid

    RefetchOpcodeImm8sx rDA
	SignExtendByte

    mov     [ecx], offset @F
    mov     [ecx+8],rDA
    mov     edx,rDA

@@:
    mov     rDA,edx
    UpdateNZ_ClearVC_Test32

    WriteEA 0, %opdreg, %SIZE_L

	FastFetch 0

	opcode = opcode + 511t   ;; 255 + 256

	ENDM


	.CONST

	PUBLIC _ops7

_ops7   LABEL DWORD
opc7000 LABEL DWORD

 REPEAT 256t
	DefOp 7000
 ENDM
	DD (256t) DUP (opsILLEGAL)

 REPEAT 256t
	DefOp 7200
 ENDM
	DD (256t) DUP (opsILLEGAL)

 REPEAT 256t
	DefOp 7400
 ENDM
	DD (256t) DUP (opsILLEGAL)

 REPEAT 256t
	DefOp 7600
 ENDM
	DD (256t) DUP (opsILLEGAL)

 REPEAT 256t
	DefOp 7800
 ENDM
	DD (256t) DUP (opsILLEGAL)

 REPEAT 256t
	DefOp 7A00
 ENDM
	DD (256t) DUP (opsILLEGAL)

 REPEAT 256t
	DefOp 7C00
 ENDM
	DD (256t) DUP (opsILLEGAL)

 REPEAT 256t
	DefOp 7E00
 ENDM
	DD (256t) DUP (opsILLEGAL)

	END

