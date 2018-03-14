
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; OPCODES6.ASM
;;
;; 68000 opcodes $6000 thru $6FFF (Bcc)
;;
;; Copyright (C) 1991-2008 by Darek Mihocka. All Rights Reserved.
;; Branch Always Software. http://www.emulators.com/
;;
;; 07/13/2008 darekm        last update
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	INCLUDE 68000.inc


;;
;; JMP to a BRAnn label to complete a Bcc that branches, or,
;; CALL to new guest PC to complete a BSR
;;
;; ops = return address (fallthrough address)
;; d32 = guest address of target
;;

complete_bsr PROC
    push  ecx
    mov   rDA,[ecx+8]
	PushLong
    pop   ecx

BRA_taken::
complete_bra::
    mov   rGA,[ecx+12t]
    PC2Flat
	Fetch 0

complete_bsr ENDP


	BigAlign

xopsBSR16 PROC

opsBSR16:
  ;; int 3
    mov   [ecx], offset complete_bsr

    mov   eax,[ecx+4]       ; current instruction PC
    add   eax,4
    mov   [ecx+8],eax       ; fallthrough address

	Imm16sxNoIncPC
    add   eax,[ecx+4]
    add   eax,2
    mov   [ecx+12t],eax     ; target address

    jmp   complete_bsr

xopsBSR16 ENDP

	BigAlign

xopsBRA16 PROC

opsBRA16:
  ;; int 3
    mov   [ecx], offset complete_bra

	Imm16sxNoIncPC          ; signed offset
    add   eax,[ecx+4]       ; add current instruction PC
    add   eax,2             ; fudge factor for size of opcode
    mov   [ecx+12t],eax     ; target address

    jmp   complete_bra

xopsBRA16 ENDP

	BigAlign

xopsBSR8 PROC

opsBSR8:
  ;; int 3
    mov   [ecx], offset complete_bsr

    mov   eax,[ecx+4]       ; current instruction PC
    add   eax,2
    mov   [ecx+8],eax       ; fallthrough address

	RefetchOpcodeImm8sx     ; signed offset
    add   eax,[ecx+4]
    add   eax,2
    mov   [ecx+12t],eax     ; target address

    jmp   complete_bsr

xopsBSR8 ENDP

xopsBRA8 PROC

	BigAlign
opsBRA8:
  ;; int 3
    mov   [ecx], offset complete_bra

	RefetchOpcodeImm8sx     ; signed offset
    add   eax,[ecx+4]       ; add current instruction PC
    add   eax,2             ; fudge factor for size of opcode
    mov   [ecx+12t],eax     ; target address

    jmp   complete_bra

xopsBRA8 ENDP


	BigAlign

xopsBSR32 PROC

opsBSR32:
    CheckCpu cpu68020

    Flat2PC rDA
    add   rDA,4
	PushLong

	Imm32NoIncPC
    Flat2PC
    add   rGA,eax
    PC2Flat
	Fetch 0

xopsBSR32 ENDP

	BigAlign

xopsBRA32 PROC

opsBRA32:
    CheckCpu cpu68020

BRA32_ok:
	Imm32NoIncPC
    Flat2PC
    add   rGA,eax
    PC2Flat
	Fetch 0

xopsBRA32 ENDP


;;
;;  Bcc <ea>
;;
;;  Note: we start at $6200. BRA and BSR are handled in the jump table.
;;

	opcode = 06200h
	REPEAT 56t

	Decode %opcode

	Valid

	IF (oplow EQ 0)

  ;; int 3
        Imm16sxNoIncPC          ; signed offset
        add   eax,[ecx+4]       ; add current instruction PC
        add   eax,2             ; fudge factor for size of opcode
        mov   [ecx+12t],eax     ; target address
        mov   [ecx], offset @F
@@:

		;; normal branch
		TestCond <BRA_taken> ; if condition is true, branch
		IncrementPCBy2
	   	FastFetch 0

		opcode = opcode+1    ;; skip odd offsets

	ELSEIF (oplow EQ 2)

  ;; int 3

        RefetchOpcodeImm8sx     ; signed offset
        add   eax,[ecx+4]       ; add current instruction PC
        add   eax,2             ; fudge factor for size of opcode
        mov   [ecx+12t],eax     ; target address
        mov   [ecx], offset @F
@@:

		;; short branch forwards
		TestCond <BRA_taken> ; if condition is true, branch
	   	FastFetch 0

		opcode = opcode + 125t

	ELSEIF (oplow EQ 0FFh)

        CheckCpu cpu68020

		;; long branch
		TestCond <BRA32_ok> ;; if condition is true, branch to BRA32
		IncrementPCBy4
	   	FastFetch 0

	ELSE

  ;; int 3
        RefetchOpcodeImm8sx     ; signed offset
        add   eax,[ecx+4]       ; add current instruction PC
        add   eax,2             ; fudge factor for size of opcode
        mov   [ecx+12t],eax     ; target address
        mov   [ecx], offset @F
@@:

		;; short branch backwards
		TestCond <BRA_taken> ; if condition is true, branch
	   	FastFetch 0

		opcode = opcode + 126t

	ENDIF

	ENDM


	.CONST

	PUBLIC _ops6

_ops6   LABEL DWORD
opc6000 LABEL DWORD

	DD opsBRA16
	DD opsILLEGAL

	REPEAT 126t
		DD opsBRA8
		DD opsILLEGAL
	ENDM

    DD opsBRA8
    DefOp BRA32

	opcode = 06100h

	DD opsBSR16
	DD opsILLEGAL

	REPEAT 126t
		DD opsBSR8
		DD opsILLEGAL
	ENDM

    DD opsBSR8
    DefOp BSR32

	opcode = 06200h

	REPEAT 14t
		DefOp %opcode            ; $6x00
		DD opsILLEGAL
		opcode = opcode + 1

		REPEAT 63t
			DefOp %opcode        ; $6x02
			opcode = opcode-1
			DD opsILLEGAL
		ENDM

		opcode = opcode + 126t

		REPEAT 63t
			DefOp %opcode        ; $6x80
			opcode = opcode-1
			DD opsILLEGAL
		ENDM

        DefOp %opcode            ; $6x80
		opcode = opcode + 126t
        DefOp %opcode            ; $6xFF

	ENDM

 END

