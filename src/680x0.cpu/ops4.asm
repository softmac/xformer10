
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; OPCODES4.ASM
;;
;; 68000 opcodes $4000 thru $4FFF
;;
;; Copyright (C) 1991-2008 by Darek Mihocka. All Rights Reserved.
;; Branch Always Software. http://www.emulators.com/
;;
;; 08/13/2007 darekm        last update
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	INCLUDE 68000.inc

;;
;;  DumpRTE
;;
;;  Call C routine to log the RTE instruction
;;

DumpRTE MACRO
IF DEBUG
	pusha
	push  regD0
	Flat2PC
	push  rGA
	call  _DumpRTE
	add   esp,8
	popa
ENDIF
ENDM


StoreImmToXword MACRO
    LOCAL   here

;;  int 3
    mov     [ecx+12t],rPC
    mov     [ecx+8],eax
    mov     [ecx], offset here
    mov     edx,[ecx+8]

here:
    mov     opXword,dx
    mov     rPC,[ecx+12t]
ENDM

;;
;; helper routine for 68020 long divide
;;
;; Instructions are followed by a 16-bit extension word with these fields:
;;
;;  bits 14..12 - Dq
;;  bit  11     - 0 = DIVU      1 = DIVS
;;  bit  10     - 0 = 32-bit    1 = 64-bit
;;  bits 2..0   - Dr (64-bit only)
;;

    public _DIVL

externdef _DIVoverflow:NEAR

    BigAlign

_DIVL:
    movzx eax,opXword           ;; fetch extension word

    test  rDA,rDA               ;; EBX = 32-bit divisor
    je    _xcptDIVZERO          ;; check for divide by 0

    mov   rGA,eax

	ClearVC

    shr   eax,10t
    and   eax,28t
    mov   eax,[regD0+eax]       ;; EAX = Dq

    .if   (word ptr opXword & 00400h)
        ;; 64-bit divide, load the upper 32-bits of the dividend

        and   rGA,7t
        mov   rGA,[regD0+rGA*4] ;; rGA = Dr

        ;; Check for overflow first just as the 68020 does.
        ;; This also avoid an x86 exception!
        ;; We do this by simply checking if the upper 32 bits is
        ;; larger than the divisor.

        .if (word ptr opXword & 00800h)
            ;; signed compare
            ;; HACK! not implemented right now!!
            ;; gets caught by expection handler
        .else
            ;; unsigned compare
            cmp   rGA,rDA
            jnc   _DIVoverflow
        .endif

    .elseif (word ptr opXword & 00800h)
        mov   rGA,eax
        sar   rGA,31t           ;; rGA = sign extend of Dq
    .else
        xor   rGA,rGA           ;; rGA = zero extend of Dq
    .endif

    .if   (word ptr opXword & 00800h)
        mov   edx,rGA
        idiv  rDA
    .else
        mov   edx,rGA
        div   rDA
    .endif

    mov   rGA,edx

    movzx rDA,opXword
    and   rDA,7t
    mov   [regD0+rDA*4],rGA     ;; save Dr

    movzx rDA,opXword
    shr   rDA,10t
    and   rDA,28t
    mov   [regD0+rDA],eax       ;; save Dq

    mov   rDA,eax   ;; N Z were not set by IDIV/DIV!!!
    UpdateNZ %SIZE_L

    FastFetch


;;
;; helper routine for 68020 long multiply
;;
;; Instructions are followed by a 16-bit extension word with these fields:
;;
;;  bits 14..12 - Dl
;;  bit  11     - 0 = MULU      1 = MULS
;;  bit  10     - 0 = 32-bit    1 = 64-bit
;;  bits 2..0   - Dh (64-bit only)
;;

    public _MULL

	BigAlign

_MULL:
    movzx rGA,opXword           ;; extension word

    shr   rGA,10t
    and   rGA,28t
    lea   rGA,[regD0+rGA]       ;; rGA = address of Dl

    mov   eax,rDA               ;; EAX = 32-bit multipler
    mov   rDA,edx

    .if   (word ptr opXword & 00800h)
        imul  dword ptr [rGA]
    .else
        mul   dword ptr [rGA]
    .endif

    seto  flagsVC
    shl   flagsVC,1

    mov   rDA,edx               ;; Dh (if needed)
    mov   dword ptr [rGA],eax   ;; save Dl

    movzx rGA,opXword           ;; extension word

    .if   (rGA & 00400h)
        ;; 64-bit multiply, save high part

        and   rGA,7t
        mov   dword ptr [regD0+rGA*4],rDA

        ;; V and C stay clear for 64-bit operations

        or    eax,rDA
        and   rDA,eax   ;; N Z were not set by IMUL/MUL!!!
        UpdateNZ_ClearVC_Test32

        FastFetch

    .endif

    ;; 32-bit multiply, V flag already updated

    mov   rDA,eax   ;; N Z were not set by IMUL/MUL!!!
    UpdateNZ %SIZE_L

    FastFetch

;;
;;  NEGX.B <ea>
;;
	opcode = 04000h

	REPEAT 00040h

	Decode %opcode
	IF FDataAlt(opsmod,opsreg,SIZE_B)

		Valid 39, 3F

		IF (fEmit NE 0)
		ReadEA %opsmod, %opsreg, %SIZE_B
		SignExtendByteToSrc

        ClearLong
		MoveXToCarry
        sbb     rDA8L,temp8
		UpdateNEGX %SIZE_B

		WriteEA %opsmod, %opsreg, %SIZE_B, 0

		FastFetch 0
		ENDIF

	ENDIF

	ENDM


;;
;;  NEGX.W <ea>
;;
	opcode = 04040h

	REPEAT 00040h

	Decode %opcode
	IF FDataAlt(opsmod,opsreg,SIZE_W)

		Valid 39, 3F

		IF (fEmit NE 0)
		ReadEA %opsmod, %opsreg, %SIZE_W
		SignExtendWordToSrc

        ClearLong
		MoveXToCarry
        sbb     rDA16,temp16
		UpdateNEGX %SIZE_W

		WriteEA %opsmod, %opsreg, %SIZE_W, 0

		FastFetch 0
		ENDIF

	ENDIF

	ENDM


;;
;;  NEGX.L <ea>
;;
	opcode = 04080h

	REPEAT 00040h

	Decode %opcode
	IF FDataAlt(opsmod,opsreg,SIZE_L)

		Valid 39, 3F

		IF (fEmit NE 0)
		ReadEA %opsmod, %opsreg, %SIZE_L
		MoveAccToSrc

        ClearLong
		MoveXToCarry
        sbb     rDA,temp32
		UpdateNEGX %SIZE_L

		WriteEA %opsmod, %opsreg, %SIZE_L, 0

		FastFetch
		ENDIF

	ENDIF

	ENDM


;;
;;  CHK <ea>
;;
	indexan = 0
	REPEAT 8

	opcode = 04180h + indexan*0200h
	REPEAT 00040h

	Decode %opcode
	IF FData(opsmod,opsreg,SIZE_L)

		Valid 3C, 3F

		IF (fEmit NE 0)
		ReadEA %opsmod, %opsreg, %SIZE_W
		SetiDreg %opdreg
		mov   ax,word ptr iDreg
		test  rDA16,rDA16
		js    _xcptCHKERR ;; Dn < 0

		cmp   ax,rDA16
		jg    _xcptCHKERR ;; Dn > upper bound

		FastFetch 0
		ENDIF

	ENDIF

	ENDM

	indexan = indexan + 1

	ENDM


;;
;;  CLR.B <ea>
;;
	opcode = 04200h

	REPEAT 00040h

	Decode %opcode
	IF FDataAlt(opsmod,opsreg,SIZE_B)

		Valid 39, 3F

		IF (fEmit NE 0)
		ReadEA %opsmod, %opsreg, %SIZE_B
        ClearLong
		UpdateNZ_ClearVC_Test32
		WriteEA %opsmod, %opsreg, %SIZE_B, 0

		FastFetch 0
		ENDIF

	ENDIF

	ENDM


;;
;;  CLR.W <ea>
;;
	opcode = 04240h

	REPEAT 00040h

	Decode %opcode
	IF FDataAlt(opsmod,opsreg,SIZE_W)

		Valid 39, 3F

		IF (fEmit NE 0)
		ReadEA %opsmod, %opsreg, %SIZE_W
        ClearLong
		UpdateNZ_ClearVC_Test32
		WriteEA %opsmod, %opsreg, %SIZE_W, 0

		FastFetch 0
		ENDIF

	ENDIF

	ENDM


;;
;;  CLR.L <ea>
;;
	opcode = 04280h

	REPEAT 00040h

	Decode %opcode
	IF FDataAlt(opsmod,opsreg,SIZE_L)

		Valid 39, 3F

		IF (fEmit NE 0)
		ReadEA %opsmod, %opsreg, %SIZE_L
        ClearLong
		UpdateNZ_ClearVC_Test32
		WriteEA %opsmod, %opsreg, %SIZE_L, 0

		FastFetch 0
		ENDIF

	ENDIF

	ENDM


;;
;;  NEG.B <ea>
;;
	opcode = 04400h

	REPEAT 00040h

	Decode %opcode
	IF FDataAlt(opsmod,opsreg,SIZE_B)

		Valid 39, 3F

		IF (fEmit NE 0)
		ReadEA %opsmod, %opsreg, %SIZE_B

		NegByte
		UpdateCCR %SIZE_B

		WriteEA %opsmod, %opsreg, %SIZE_B, 0

		FastFetch 0
		ENDIF

	ENDIF

	ENDM


;;
;;  NEG.W <ea>
;;
	opcode = 04440h

	REPEAT 00040h

	Decode %opcode
	IF FDataAlt(opsmod,opsreg,SIZE_W)

		Valid 39, 3F

		IF (fEmit NE 0)
		ReadEA %opsmod, %opsreg, %SIZE_W

		NegWord
		UpdateCCR %SIZE_W

		WriteEA %opsmod, %opsreg, %SIZE_W, 0

		FastFetch 0
		ENDIF

	ENDIF

	ENDM


;;
;;  NEG.L <ea>
;;
	opcode = 04480h

	REPEAT 00040h

	Decode %opcode
	IF FDataAlt(opsmod,opsreg,SIZE_L)

		Valid 39, 3F

		IF (fEmit NE 0)
		ReadEA %opsmod, %opsreg, %SIZE_L

		NegLong
		UpdateCCR %SIZE_L

		WriteEA %opsmod, %opsreg, %SIZE_L, 0

		FastFetch
		ENDIF

	ENDIF

	ENDM


;;
;;  MOVE <ea>,CCR
;;
	opcode = 044C0h

	REPEAT 00040h

	Decode %opcode
	IF FData(opsmod,opsreg,SIZE_W)

		Valid 3C, 3F

		IF (fEmit NE 0)
		ReadEA %opsmod, %opsreg, %SIZE_W
		and   rDA8L,1Fh
		PutCCR

		FastFetch 0
		ENDIF

	ENDIF

	ENDM


;;
;;  NOT.B <ea>
;;
	opcode = 04600h

	REPEAT 00040h

	Decode %opcode
	IF FDataAlt(opsmod,opsreg,SIZE_B)

		Valid 39, 3F

		IF (fEmit NE 0)
		ReadEA %opsmod, %opsreg, %SIZE_B

		NotByte
		UpdateNZ_ClearVC_Test8

		WriteEA %opsmod, %opsreg, %SIZE_B, 0

		FastFetch 0
		ENDIF

	ENDIF

	ENDM


;;
;;  NOT.W <ea>
;;
	opcode = 04640h

	REPEAT 00040h

	Decode %opcode
	IF FDataAlt(opsmod,opsreg,SIZE_W)

		Valid 39, 3F

		IF (fEmit NE 0)
		ReadEA %opsmod, %opsreg, %SIZE_W

		NotWord
		UpdateNZ_ClearVC_Test16

		WriteEA %opsmod, %opsreg, %SIZE_W, 0

		FastFetch 0
		ENDIF

	ENDIF

	ENDM


;;
;;  NOT.L <ea>
;;
	opcode = 04680h

	REPEAT 00040h

	Decode %opcode
	IF FDataAlt(opsmod,opsreg,SIZE_L)

		Valid 39, 3F

		IF (fEmit NE 0)
		ReadEA %opsmod, %opsreg, %SIZE_L

		NotLong
		UpdateNZ_ClearVC_Test32

		WriteEA %opsmod, %opsreg, %SIZE_L, 0

		FastFetch
		ENDIF

	ENDIF

	ENDM


;;
;;  MOVE <ea>,SR
;;
	opcode = 046C0h

	REPEAT 00040h

	Decode %opcode
	IF FData(opsmod,opsreg,SIZE_W)

		Valid 3C, 3F

		IF (fEmit NE 0)
		CheckPriv
		ReadEA %opsmod, %opsreg, %SIZE_W
		PutSR
		ENDIF

	ENDIF

	ENDM


;;
;;  NBCD.B <ea>
;;
	opcode = 04800h

	REPEAT 00040h

	Decode %opcode
	IF FDataAlt(opsmod,opsreg,SIZE_B)

		Valid 39, 3F

		IF (fEmit NE 0)
		ReadEA %opsmod, %opsreg, %SIZE_B

        MoveXToCarry
		mov   al,0
		sbb   al,rDA8L
		das
		mov   rDA8L,al
		UpdateBCD %SIZE_B

		WriteEA %opsmod, %opsreg, %SIZE_B, 0

		FastFetch 0
		ENDIF

	ENDIF

	ENDM


;;
;;  TST.B <ea>
;;
	opcode = 04A00h

	REPEAT 00040h

	Decode %opcode
	IF FDataAlt(opsmod,opsreg,SIZE_B)

		Valid 39, 3F

		IF (fEmit NE 0)
		ReadEA %opsmod, %opsreg, %SIZE_B

		UpdateNZ_ClearVC_Test8

		FastFetch 0
		ENDIF

	ENDIF

	ENDM


;;
;;  TST.W <ea>
;;
	opcode = 04A40h

	REPEAT 00040h

	Decode %opcode
	;; IF FDataAlt(opsmod,opsreg,SIZE_W)    ;; this is 68000 only
	IF FAny(opsmod,opsreg,SIZE_W)           ;; HACK! HACK! works on 68020+

		;; Valid 39, 3F                     ;; this is 68000 only
		Valid 3B, 3F                        ;; HACK! HACK! works on 68020+

		IF (fEmit NE 0)
		ReadEA %opsmod, %opsreg, %SIZE_W

		UpdateNZ_ClearVC_Test16

		FastFetch 0
		ENDIF

	ENDIF

	ENDM


;;
;;  TST.L <ea>
;;
	opcode = 04A80h

	REPEAT 00040h

	Decode %opcode
	;; IF FDataAlt(opsmod,opsreg,SIZE_W)    ;; this is 68000 only
	IF FAny(opsmod,opsreg,SIZE_W)           ;; HACK! HACK! works on 68020+

		;; Valid 39, 3F                     ;; this is 68000 only
		Valid 3B, 3F                        ;; HACK! HACK! works on 68020+

		IF (fEmit NE 0)
		ReadEA %opsmod, %opsreg, %SIZE_L

		UpdateNZ_ClearVC_Test32

		FastFetch
		ENDIF

	ENDIF

	ENDM


;;
;;  TAS <ea>
;;
	opcode = 04AC0h

	REPEAT 00040h

	Decode %opcode
	IF FDataAlt(opsmod,opsreg,SIZE_B)

		Valid 39, 3F

		IF (fEmit NE 0)
		ReadEA %opsmod, %opsreg, %SIZE_B

		UpdateNZ_ClearVC_Test8
		or    rDA8L,080h

		WriteEA %opsmod, %opsreg, %SIZE_B, 0

		FastFetch 0
		ENDIF

	ENDIF

	ENDM



;;
;; 68020 long multiply, MULU, MULS
;;

	opcode = 04C00h

	REPEAT 00040h

	Decode %opcode

    IF FData(opsmod,opsreg,SIZE_L)

        Valid 3C, 3F

        if (fEmit NE 0)

        CheckCpu cpu68020

        Imm16zx
		StoreImmToXword

        ReadEA %opsmod, %opsreg, %SIZE_L
        jmp   _MULL

        ENDIF
    ENDIF

	ENDM


;;
;; 68020 long divide, DIVU, DIVS
;;

	opcode = 04C40h

	REPEAT 00040h

	Decode %opcode

    IF FData(opsmod,opsreg,SIZE_L)

        Valid 3C, 3F

        if (fEmit NE 0)

        CheckCpu cpu68020

        Imm16zx
		StoreImmToXword

        ReadEA %opsmod, %opsreg, %SIZE_L
        jmp   _DIVL

        ENDIF
    ENDIF

	ENDM



;;
;;  MOVE SR,<ea>
;;
	opcode = 040C0h

	REPEAT 00040h

	Decode %opcode
	IF FDataAlt(opsmod,opsreg,SIZE_W)

		Valid

        .if (cpuCur > cpu68000)
            CheckPriv
        .endif

		GetSR
		WriteEA %opsmod, %opsreg, %SIZE_W

		FastFetch 0

	ENDIF

	ENDM



;;
;;  LEA <ea>,An
;;
	indexan = 0
	REPEAT 8

	opcode = 041C0h + indexan*0200h
	REPEAT 00040h

	Decode %opcode
	IF FControl(opsmod,opsreg,SIZE_L)

		Valid

		GenEA %opsmod, %opsreg, %SIZE_L
		SetiAreg %opdreg
		mov   iAreg,rGA

		FastFetch 0

	ENDIF

	ENDM

	indexan = indexan + 1

	ENDM


;;
;;  MOVE CCR,<ea>
;;
;;  NOTE: this is a 68010 instruction used by TOS 1.6 to determine _longframe.
;;  For 68000 emulation, this must generate an illegal instruction exception.
;;
	opcode = 042C0h

	REPEAT 00040h

	Decode %opcode
	IF FDataAlt(opsmod,opsreg,SIZE_W)

		Valid

        CheckCpu cpu68010

        GetSR
        and   rDA,1Fh
        WriteEA %opsmod, %opsreg, %SIZE_W       ;; this is a word operation!!

        FastFetch 0
	ENDIF

	ENDM


;;
;;  SWAP Dn
;;
	opcode = 04840h

	REPEAT 00008h

	Decode %opcode
	SetiDreg %opsreg

	Valid

	mov   rDA,iDreg
	rol   rDA,16t
	mov   iDreg,rDA
	UpdateNZ_ClearVC_Test32

	FastFetch 0

	ENDM


;;
;;  PEA <ea>
;;
	opcode = 04850h

	REPEAT 00030h

	Decode %opcode
	IF FControl(opsmod,opsreg,SIZE_L)

		Valid

		GenEA %opsmod, %opsreg, %SIZE_L
		mov   rDA,rGA
		PushLong

		FastFetch

	ENDIF

	ENDM


;;
;;  EXT.W Dn
;;
	opcode = 04880

	REPEAT 00008h

	Decode %opcode

	Valid

	SetiDreg %opsreg
	movsx rDA,byte ptr iDreg 
	mov   word ptr iDreg,rDA16
	UpdateNZ_ClearVC_Test32

	FastFetch 0

	ENDM


;;
;;  EXT.L Dn
;;
	opcode = 048C0

	REPEAT 00008h

	Decode %opcode

	Valid

	SetiDreg %opsreg
	movsx rDA,word ptr iDreg 
	mov   dword ptr iDreg,rDA
	UpdateNZ_ClearVC_Test32

	FastFetch 0

	ENDM


;;
;;  EXT.B Dn
;;
	opcode = 049C0

	REPEAT 00008h

	Decode %opcode

	Valid

    CheckCpu cpu68020

	SetiDreg %opsreg
	movsx rDA,byte ptr iDreg 
	mov   dword ptr iDreg,rDA
	UpdateNZ_ClearVC_Test32

	FastFetch 0

	ENDM


;
; This is the new MOVEM code. It is faster but doesn't take care of
; MOVEM.L regs,-(An) and MOVEM.L (An)+,regs where regs includes An!!!
; Such code is used by the AES task switcher. Since that code just
; saves and restores A0, we can use the faster code, but other programs
; may make use of the above side effect and will break.
;

DoMoveM MACRO size:REQ, fDir:REQ, fUpd:REQ

    LOCAL loop, term

    ; size should either be SIZE_W or SIZE_L
    ; fDir is 0 for register to memory, 1 for memory to register
    ; fUpd means whether to update address register

    IF (fUpd NE 0)
        push  rDA           ; pointer to register that gets updated
    ENDIF

    IF (fUpd NE 0) AND (fDir EQ 0)
        lea   eax,regA7+4   ; starting register = A7 (predecrement)
    ELSE
        lea   eax,regD0-4   ; starting register = D0 (control or post inc)
    ENDIF

loop:
    IF (fUpd NE 0) AND (fDir EQ 0)
        add   eax,-4        ; next register
    ELSE
        add   eax,4         ; next register
    ENDIF

	shr   word ptr opXword,1
	jnc   term

    IF (fUpd NE 0) AND (fDir EQ 0)
      IF (size EQ SIZE_L)
        sub   rGA,4         ; next address
      ELSE
        sub   rGA,2         ; next address
      ENDIF
    ENDIF

    IF (fDir EQ 0)
        mov   rDA,[eax]     ; read register
        push  eax
        WriteMemAtEA &size
        pop   eax
    ELSEIF (size EQ SIZE_W)
        push  eax
        ReadMemAtEA &size
        pop   eax
        movsx rDA,rDA16
        mov   [eax],rDA     ; write to register
    ELSE
        push  eax
        ReadMemAtEA &size
        pop   eax
        mov   [eax],rDA     ; write to register
    ENDIF

    IF (fUpd EQ 0) OR (fDir EQ 1)
      IF (size EQ SIZE_L)
        add   rGA,4         ; next address
      ELSE
        add   rGA,2         ; next address
      ENDIF
    ENDIF

	jmp   loop

term:
	jne   loop

    IF (fUpd NE 0)
        pop   eax           ; pointer to register that gets updated
        mov   [eax],rGA     ; save updated address register
    ENDIF

    FastFetch
ENDM


;;
;;  MOVEM.W regs,<ea>
;;
	opcode = 04890

	REPEAT 00030h

	Decode %opcode
	IF (opsmod EQ 2) OR (opsmod EQ 4) OR (opsmod EQ 5) OR (opsmod EQ 6) OR ((opsmod EQ 7) AND (opsreg LT 2))

		Valid

		Imm16zx
		StoreImmToXword

		IF (opsmod EQ 4)
			SetiAreg %opsreg
			lea   rDA,iAreg
			GenEA %2, %opsreg, %SIZE_W
			jmp   movemw_to_mem_pre
		ELSE
			GenEA %opsmod, %opsreg, %SIZE_W
			jmp   movemw_to_mem
		ENDIF

	ENDIF

	ENDM

	BigAlign

movemw_to_mem_pre:
    DoMoveM %SIZE_W, 0, 1

	BigAlign

movemw_to_mem:
    DoMoveM %SIZE_W, 0, 0

;;
;;  MOVEM.L regs,<ea>
;;
	opcode = 048D0

	REPEAT 00030h

	Decode %opcode
	IF (opsmod EQ 2) OR (opsmod EQ 4) OR (opsmod EQ 5) OR (opsmod EQ 6) OR ((opsmod EQ 7) AND (opsreg LT 2))

		Valid

		Imm16zx
		StoreImmToXword

		IF (opsmod EQ 4)
			SetiAreg %opsreg
			lea   rDA,iAreg
			GenEA %2, %opsreg, %SIZE_L
			jmp   moveml_to_mem_pre
		ELSE
			GenEA %opsmod, %opsreg, %SIZE_L
			jmp   moveml_to_mem
		ENDIF

	ENDIF

	ENDM

	;  MOVEM.L regs,-(An)

	BigAlign

moveml_to_mem_pre:
    DoMoveM %SIZE_L, 0, 1

	BigAlign

moveml_to_mem:
    DoMoveM %SIZE_L, 0, 0

;;
;;  MOVEM.W <ea>,regs
;;
	opcode = 04C90

	REPEAT 00030h

	Decode %opcode
	IF (opsmod EQ 2) OR (opsmod EQ 3) OR (opsmod EQ 5) OR (opsmod EQ 6) OR ((opsmod EQ 7) AND (opsreg LT 4))

		Valid

		Imm16zx
		StoreImmToXword

		IF (opsmod EQ 3)
			SetiAreg %opsreg
			lea   rDA,iAreg
			GenEA %2, %opsreg, %SIZE_W
			jmp   movemw_to_reg_post
		ELSE
			GenEA %opsmod, %opsreg, %SIZE_W
			jmp   movemw_to_reg
		ENDIF

	ENDIF

	ENDM

	BigAlign

movemw_to_reg_post:
    DoMoveM %SIZE_W, 1, 1

	BigAlign

movemw_to_reg:
    DoMoveM %SIZE_W, 1, 0

;;
;;  MOVEM.L <ea>,regs
;;
	opcode = 04CD0

	REPEAT 00030h

	Decode %opcode
	IF (opsmod EQ 2) OR (opsmod EQ 3) OR (opsmod EQ 5) OR (opsmod EQ 6) OR ((opsmod EQ 7) AND (opsreg LT 4))

		Valid

		Imm16zx
		StoreImmToXword

		IF (opsmod EQ 3)
			SetiAreg %opsreg
			lea   rDA,iAreg
			GenEA %2, %opsreg, %SIZE_L
			jmp   moveml_to_reg_post
		ELSE
			GenEA %opsmod, %opsreg, %SIZE_L
			jmp   moveml_to_reg
		ENDIF

	ENDIF

	ENDM

	BigAlign

moveml_to_reg_post:
    DoMoveM %SIZE_L, 1, 1

    BigAlign

moveml_to_reg:
    DoMoveM %SIZE_L, 1, 0


;;
;;  ILLEGAL
;;
;;  $4AFA and $4AFB are reserved. Gemulator debugger uses $4AFA for breakpoints
;;

	Valid2 4AFA
	jmp   exitBREAK

	Valid2 4AFB
	jmp   exitBREAK

	Valid2 4AFC
	jmp   _xcptILLEGAL


;;
;;  TRAP #n
;;

	opcode = 04E40h

	REPEAT 00010h

	Decode %opcode

	Valid

    ;; To keep the speed of VDI and BIOS calls fast in Atari mode
    ;; we only hook TRAP #1. Macintosh doesn't use TRAP so not a worry.

    Exception %020h+optrap
    Vector    %020h+optrap

    ENDM



;;
;;  LINK An,#imm
;;

	opcode = 04E50h

	REPEAT 00008h

	Decode %opcode

	Valid

	SetiAreg %opsreg
	mov   rDA,iAreg
	PushLong

	SetiAreg %opsreg
	mov   rDA,regA7
	Imm16sx
	mov   iAreg,rDA
	add   rDA,eax
	mov   regA7,rDA

	FastFetch 0

	ENDM


;;
;;  LINK An,#imm32
;;

	opcode = 04808h

	REPEAT 00008h

	Decode %opcode

	Valid

    CheckCpu cpu68020

	SetiAreg %opsreg
	mov   rDA,iAreg
	PushLong

	; new code
	SetiAreg %opsreg
	mov   rDA,regA7
	Imm32
	mov   iAreg,rDA
	add   rDA,eax
	mov   regA7,rDA

	FastFetch

	ENDM


;;
;;  UNLK An
;;

	opcode = 04E58h

	REPEAT 00008h

	Decode %opcode

	Valid

	SetiAreg %opsreg
	mov   rDA,iAreg
	mov   regA7,rDA

	PopLong
	SetiAreg %opsreg
	mov   iAreg,rDA

	FastFetch 0

	ENDM


;;
;;  MOVE An,USP
;;

	opcode = 04E60h

	REPEAT 00008h

	Decode %opcode

	Valid

	CheckPriv
	SetiAreg %opsreg
	mov   eax,iAreg
	mov   regUSP,eax

	FastFetch 0

	ENDM


;;
;;  MOVE USP,An
;;

	opcode = 04E68h

	REPEAT 00008h

	Decode %opcode

	Valid

	CheckPriv
	SetiAreg %opsreg
	mov   eax,regUSP
	mov   iAreg,eax

	FastFetch 0

	ENDM


;;
;;  RESET
;;

externdef _InitSTHW:NEAR

	Valid2 4E70
	CheckPriv
	or    regSR,0700h       ; set IPL to 7
    PrepareForCall
	call  dword ptr vm_InitHW
    AfterCall 0

    Flat2PC
    PC2Flat
	Fetch


;;
;;  NOP
;;

	Valid2 4E71
	FastFetch 0


;;
;;  STOP
;;

	Valid2 4E72
    Flat2PC
    PC2Flat
	Fetch


;;
;;  RTE
;;

	Valid2 4E73

	CheckPriv

	;; PopWord SR

	mov   rGA,regA7
	ReadMemAtEA %SIZE_W
    push  rDA

	;; PopLong retaddr
	add   rGA,2
	ReadMemAtEA %SIZE_L

    .if (cpuCur > cpu68000)
        push  rDA

        ;; PopWord ax           ;; vector and offset
        add   rGA,4
        ReadMemAtEA %SIZE_W
        movzx eax,rDA16

        ; pop off 8 byte exception frame plus any internal registers

        shr   eax,12t

        cmp   eax,16t
        jc    @F

      ; int 3

@@:
        add   eax,eax
        mov   eax,mpframecb[eax+eax]

        test  eax,eax
        jne   @F

      ; int 3
@@:
        add   dword ptr regA7,eax

        pop   rGA
    .else
        mov   rGA,rDA
        add   dword ptr regA7,6
    .endif

	PC2Flat

;	DumpRTE

    pop   rDA
	PutSR                   ;; may switch us to User mode


    .DATA                   ;; can't use CONST, it would screw up opcodes

	align 4

mpframecb DD 8, 8, 0Ch, 0Ch, 10h, 10h, 10h, 3Ch, 3Ah, 14h, 20h, 5Ch, 18h
          DD 8, 8, 8

    .CODE

;;
;;  RTD (68010 and up)
;;
	Valid2 4E74

    CheckCpu cpu68010

	PopLong
	Imm16sx eax
    mov   rGA,rDA
	add   regA7,eax
	PC2Flat
	Fetch 0


;;
;;  RTS
;;
	Valid2 4E75

	PopLong
    mov   rGA,rDA
	PC2Flat
	Fetch 0


;;
;;  TRAPV
;;

	Valid2 4E76

	test  flagsVC,2         ; test V flag in CCR
	jnz   _xcptTRAPV
	FastFetch

;;
;;  RTR
;;

	Valid2 4E77

	PopWord
	PutCCR

	; finish off with RTS

	PopLong
    mov   rGA,rDA
	PC2Flat
	Fetch 0


;;
;;  MOVEC Rn,Rc or Rc,Rn
;;
;;  NOTE: this is a 68010 instruction used by TOS 1.6 to determine _longframe.
;;  For 68000 emulation, this must generate an illegal instruction exception.
;;
;;  For 68010+ we handle it in the trap handler, so generate nothing either!
;;

	Valid2 4E7A

    CheckCpu cpu68010

    jmp   _xcptILLEGAL


	Valid2 4E7B

    CheckCpu cpu68010

    jmp   _xcptILLEGAL


;;
;;  JSR <ea>
;;
	opcode = 04E90h

	REPEAT 0002Fh

	Decode %opcode
	IF FControl(opsmod,opsreg,SIZE_L)

		Valid

		GenEA %opsmod, %opsreg, %SIZE_L

		Flat2PC rDA
		PC2Flat
		PushLong

        mov rGA,regPC
		Fetch

	ENDIF

	ENDM

;;
;;  JMP <ea>
;;

	opcode = 04ED0h

	REPEAT 0002Fh

	Decode %opcode
	IF FControl(opsmod,opsreg,SIZE_L)

		Valid

		GenEA %opsmod, %opsreg, %SIZE_L
		PC2Flat
		Fetch 0

	ENDIF

	ENDM



 .CONST

	PUBLIC _ops4

_ops4   LABEL DWORD
opc4000 LABEL DWORD

 opcode = 04000h

 REPEAT 01000h
   DefOp %opcode
 ENDM

 END
