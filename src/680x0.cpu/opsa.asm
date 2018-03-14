
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; OPCODESA.ASM
;;
;; 68000 opcodes $A000 thru $AFFF (line A)
;;
;; Copyright (C) 1991-2008 by Darek Mihocka. All Rights Reserved.
;; Branch Always Software. http://www.emulators.com/
;;
;; 10/20/2000 darekm        last update
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	INCLUDE 68000.inc

opinline = 1				; force inline fetches

opsLINEA::
    DecrementPCBy2 ; move PC back to line A instruction
	Exception 0Ah
	Vector 0Ah     ; LineA is vector $A (address $28)


 .CONST

	PUBLIC _opsA

_opsA   LABEL DWORD
opcA000 LABEL DWORD

 opcode = 0A000h

	DD (01000h) DUP (opsLINEA)

 END
