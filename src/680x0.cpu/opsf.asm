
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; OPCODESF.ASM
;;
;; 68000 opcodes $F000 thru $FFFF
;;
;; Copyright (C) 1991-2008 by Darek Mihocka. All Rights Reserved.
;; Branch Always Software. http://www.emulators.com/
;;
;; 11/28/2008 darekm        last update
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	INCLUDE 68000.inc

externdef _lAccessAddr:DWORD

externdef @F68KFPU@4:NEAR
externdef @F68KFPUreg@4:NEAR
externdef _FTryMove16:NEAR
externdef _Fcc:NEAR

;;
;;  68030 MMU traps
;;
;;  All 68030 MMU instructions are coded for id 0 of type 0, and all
;;  use the 6-bit effective address field and 16-bit extension word.
;;

	opcode = 0F000h
	REPEAT 00040h

	Decode %opcode
	IF FControl(opsmod,opsreg,SIZE_L) AND FAlt(opdmod,opdreg,SIZE_L)

        Valid

        cmp cpuCur,cpu68030
        jne LINEF_ok

        call CheckPrivAndSetFPIAR

        Imm16zx

        GenEA %opsmod, %opsreg, %SIZE_L

        mov   opXword,ax
        mov   _lAccessAddr,rGA
        jmp   LINEF2

	ENDIF

	ENDM


StoreDAImmToXword MACRO
    LOCAL   here

;;  int 3
    mov     [ecx+12t],rDA
    mov     [ecx+8],eax
    mov     [ecx], offset here
    mov     edx,eax
    DecrementPCBy2

here:
    IncrementPCBy2
    mov     eax,edx
    mov     opXword,dx
    mov     rDA,[ecx+12t]
ENDM

;;
;;  FPU coprocessor traps
;;
;;  Most FPU instructions have an effective address encoded in the
;;  bottom 6 bits. This code decodes that address, stores it in
;;  the global _lAccessAddr, then calls the LINE F exception as usual.
;;
;;  For instructions that don't require decoding an effective address
;;  we do nothing. Those are handled in the exception handler.
;;

    ;; FPU TYPE 0 - arithmetic operations, allow all addressing modes
    ;; Takes one 16-bit extension word

	opcode = 0F200h
	REPEAT 00040h

	Decode %opcode
	IF FAny(opsmod,opsreg,SIZE_L)

        Valid

        cmp cpuCur,cpu68020
        jc  LINEF_ok

        RefetchOpcodeImm16zx    rDA

        Imm16zx

        ;; check the R/M bit regardless of what's in the address mode field

        test  eax,0E000h
        jz    FPUreg

        ;; Definitely a memory-to-register operation

        StoreDAImmToXword

        mov   rGA,[ecx+4]
        add   rGA,2
        mov   regFPIAR,rGA

        IF (opsmod EQ 0)
            SetiDreg %opsreg
            lea   rGA,[iDreg]
        ELSEIF (opsmod EQ 1)
            SetiAreg %opsreg
            lea   rGA,[iAreg]
        ELSEIF (opsmod EQ 7) AND (opsreg EQ 4)
            lea   rGA,[rGA+2]
        ELSE
            GenEA %opsmod, %opsreg, %SIZE_L
        ENDIF

        jmp   FPUaddr

	ENDIF

	ENDM


    ;; FPU TYPE 1 - conditionals
    ;; Takes one 16-bit extension word

	opcode = 0F240h
	REPEAT 00040h

	Decode %opcode
	IF (opsmod EQ 7) AND (opsreg GE 2)

        ;; FTRAPcc

        Valid

        IF (fEmit NE 0)
        jmp   FPU
        ENDIF

	ELSEIF (opsmod EQ 1)

        ;; FDBcc

        Valid 7, 7

        IF (fEmit NE 0)
        jmp   FPU
        ENDIF

	ELSE
     IF FAny(opsmod,opsreg,SIZE_B)

        ;; FScc

        Valid

        cmp cpuCur,cpu68020
        jc  LINEF_ok

        RefetchOpcodeImm16zx    rDA

        Flat2PC
        sub   rGA,2
        mov   regFPIAR,rGA

        Imm16zx
        mov   opXword,ax

        IF (opsmod EQ 0)
            SetiDreg %opsreg
            lea   rGA,[iDreg]
        ELSE
            GenEA %opsmod, %opsreg, %SIZE_B
        ENDIF

        jmp   FPUaddr

	ENDIF
	ENDIF

	ENDM


    ;; FPU TYPES 2 and 3 - branches, no address to decode

	opcode = 0F280h
	REPEAT 00080h

	Decode %opcode

        Valid 3F, 3F, 1

        cmp cpuCur,cpu68020
        jc  LINEF_ok

        RefetchOpcodeImm16zx    rDA

        IF (fEmit NE 0)

        IF (opsize EQ 3)
            Imm32               ; fetch 32-bit offset
            lea   rGA,[eax-4]
        ELSE
            Imm16sx             ; fetch 16-bit offset
            lea   rGA,[eax-2]
        ENDIF

        and   rDA,63t       ; extract conditinal predicate

        PrepareForCall
        push  rDA
        call  _Fcc
        AfterCall 1
        je    @F

        Flat2PC eax
        add   rGA,eax
        PC2Flat

@@:
        Flat2PC
        PC2Flat
        Fetch

        ENDIF

	ENDM


    ;; FPU TYPES 4 and 5 - FSAVE and FRESTORE

	opcode = 0F300h
	REPEAT 00080h

	Decode %opcode
	IF FAny(opsmod,opsreg,SIZE_L)

		Valid

        cmp cpuCur,cpu68020
        jc  LINEF_ok

        RefetchOpcodeImm16zx    rDA

        call CheckPrivAndSetFPIAR

        IF (opsmod EQ 0)
            SetiDreg %opsreg
            lea   rGA,[iDreg]
        ELSEIF (opsmod EQ 1)
            SetiAreg %opsreg
            lea   rGA,[iAreg]
        ELSEIF (opsmod EQ 7) AND (opsreg EQ 4)
            lea   rGA,[rGA+2]
        ELSE
            GenEA %opsmod, %opsreg, %SIZE_L
        ENDIF

        jmp   FPUaddr

	ENDIF

	ENDM


    ;; 68040 MOVE16

	opcode = 0F600h
	REPEAT 00028h

	Decode %opcode

    Valid 7, 7

    IF (fEmit NE 0)

    RefetchOpcodeImm16zx    rDA

;   CheckCpu cpu68030
    cmp cpuCur,cpu68030
    jc  LINEF_ok

    IF (opsmod EQ 0)

        ;; MOVE16 (Ay)+,xxx.L

        jmp   MOVE16

    ELSEIF (opsmod EQ 1)

        ;; MOVE16 xxx.L,(Ay)+

        jmp   MOVE16

    ELSEIF (opsmod EQ 2)

        ;; MOVE16 (Ay),xxx.L

        jmp   MOVE16

    ELSEIF (opsmod EQ 3)

        ;; MOVE16 xxx.L,(Ay)

        jmp   MOVE16

    ELSEIF (opsmod EQ 4)

        ;; MOVE16 (Ax)+,(Ay)+

        jmp   MOVE16

    ELSE

        ;; REVIEW: illegal?

    ENDIF

    ENDIF ; fEmit

	ENDM



    PUBLIC MOVE16

    BigAlign

CheckPrivAndSetFPIAR PROC
    CheckPriv

    Flat2PC
    sub   rGA,2
    mov   regFPIAR,rGA
    ret

CheckPrivAndSetFPIAR ENDP

    BigAlign

FPU PROC
    cmp cpuCur,cpu68020
    jc  LINEF_ok

    Flat2PC
    sub   rGA,2
    mov   regFPIAR,rGA

    RefetchOpcodeImm16zx    rDA

    Imm16zx
    mov   opXword,ax

FPUaddr::
    mov   _lAccessAddr,rGA

    PrepareForCall 1,0
    mov   ecx,rDA
    call  @F68KFPU@4
    AfterCall 0,1,0
    jne   LINEF2

    FastFetch 0

FPU ENDP

    BigAlign

FPUreg PROC
    StoreDAImmToXword

;;  PrepareForCall 1,0
    movzx ecx,word ptr opXword
    call  @F68KFPUreg@4
;;  AfterCall 0,1,0

    FastFetch 0

FPUreg ENDP

MOVE16 PROC
    Imm32NoIncPC            ; eax = xword or xlong, EBX already has opcode
    DecrementPCBy2 ; move PC back to line F instruction

 ; int 3
    PrepareForCall
    push  eax
    push  rDA
    call  _FTryMove16
    AfterCall 2
    jne   LINEF2
    FastFetch

MOVE16 ENDP


opsLINEF::
;   DecrementPCBy2 ; move PC back to line F instruction

LINEFok PROC

LINEF_ok::
    mov     [ecx], offset here
here:
    DecrementPCBy2 ; move PC back to line F instruction
    Flat2PC
    mov   regFPIAR,rGA
LINEF2::

	Exception 0Bh
	Vector 0Bh     ; LineF is vector $B (address $2C)

LINEFok ENDP

 .CONST

	PUBLIC _opsF

_opsF   LABEL DWORD
opcF000 LABEL DWORD

 opcode = 0F000h

  REPEAT 01000h
    DefOp %opcode, opsLINEF
  ENDM

 END

