
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; OPCODESE.ASM
;;
;; 68000 opcodes $E000 thru $EFFF (shifts)
;;
;; Copyright (C) 1991-2008 by Darek Mihocka. All Rights Reserved.
;; Branch Always Software. http://www.emulators.com/
;;
;; 03/24/2002 darekm        last update
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	INCLUDE 68000.inc

Typ0 TEXTEQU <SA>
Typ1 TEXTEQU <SH>
Typ2 TEXTEQU <RC>
Typ3 TEXTEQU <RO>

Dir0 TEXTEQU <R>
Dir1 TEXTEQU <L>


;
; DoShift type, dr, size, count
;
; DoShift - shift iDreg according to the type, direction, size, and count
;
; dr: 0 = right  1 = left
; type: 00=AS  01=LS  02=ROX 03=RO
; size: 0=byte  1=word  2=long  3=memory (rDA16)
; count: 1..8 or cl
;
; Mapping from 68000 commands to 386 commands:
; 68000: ASL  LSL  ROXL  ROL  ASR  LSR  ROXR  ROR
; 386:   SAL  SHL  RCL   ROL  SAR  SHR  RCR   ROR
;

DoShift MACRO type, dr, size, count, fCL:=<0>

	IF (type EQ 2)
        MoveXToCarry
	ELSE
		IF (type EQ 0) AND (dr EQ 1)	;; ASL
			mov   RAX&size,Reg&size     ;; original value
		ENDIF
	ENDIF


foo TEXTEQU <Typ&type>
bar TEXTEQU <Dir&dr>

    IF (fCL NE 0)
        ;; The 68K shifts modulo 64 bits!
        ;; Check for shift count >31 bits
        ;; and if so, shift 32 bits extra
        ;; since the x86 shifts modulo 32

        .if (rDA8L & 32t)
%	        &foo&bar Reg&size,1
%	        &foo&bar Reg&size,31t
        .endif

        xchg  cl,rDA8L

%	    &foo&bar Reg&size,count

        xchg  cl,rDA8L
    ELSE
%	    &foo&bar Reg&size,count
    ENDIF

	IF (type EQ 0) AND (dr EQ 1) ;; ASL - update all flags, V is wierd
        setc  flagsVC
        setc  flagX

        IF (fCL NE 0)
            xchg  cl,rDA8L
        ENDIF
		sar   Reg&size,count
        IF (fCL NE 0)
            xchg  cl,rDA8L
        ENDIF
		cmp   RAX&size,Reg&size
		seto  dl           ;; sign bit changed sometime during shift
        add   dl,dl
        or    flagsVC,dl

        IF (fCL NE 0)
            xchg  cl,rDA8L
        ENDIF
		sal   Reg&size,count
        IF (fCL NE 0)
            xchg  cl,rDA8L
        ENDIF

	ELSEIF (type NE 3)     ;; ASR LSL LSR ROXL ROXR - update XNZC, clear V
        setc  flagsVC
        setc  flagX
	ELSE                   ;; ROL ROR - update NZC, V cleared
        setc  flagsVC
	ENDIF

@@:
%   mov   Acc&size,Reg&size ;; set N Z (shift op didn't)
	UpdateNZ_Test&size

ENDM

StoreDAImmToXword MACRO
    LOCAL   here

;;  int 3
    mov     [ecx+12t],rDA
    mov     [ecx+8],eax
    mov     [ecx], offset here
    mov     edx,[ecx+8]
    DecrementPCBy2

here:
    IncrementPCBy2
    mov     opXword,dx
    mov     rDA,[ecx+12t]
ENDM

;;
;; DoBitfieldOp
;;
;; read the extension word containing the bitfield information
;; and then generate the effective address of the operand.
;; Then branch to the common code for handling bitfields

DoBitfieldOp MACRO
    RefetchOpcodeImm16zx    rDA

    Imm16zx
    StoreDAImmToXword

    IF (opsmod EQ 0)
        SetiDreg %opsreg
        lea   rGA,[iDreg]
    ELSE
        GenEA %opsmod, %opsreg, %SIZE_L
    ENDIF

    jmp   _BF
ENDM

;;
;;  SHIFT/ROTATE Dn,Dn or #imm,Dn or Dn,<ea>
;;
;;  bits 11..9 - register or shift count
;;  bit  8     - direction 0 = right 1 = left
;;  bits 7..6  - size: 00=byte  01=word  02=long
;;  bit  5     - 0 = immediate shift count  1 = register shift count
;;  bits 4..3  - type:  00=AS  01=LS  02=ROX 03=RO
;;  bits 2..0  - data register being shifted
;;
;;
;;  BF*** <ea>{offset:width}
;;
;;  bit  11    - 1
;;  bit  10..8 - one of 8 bitfield options (0, 1, 3, 5 are read only)
;;  bits 7..6  - 11
;;  bit  5..3  - mode
;;  bits 2..0  - register
;;

	opcode = 0E000h
	REPEAT 01000h

	Decode %opcode

	IF (opsize EQ SIZE_X)

	    IF ((opdreg AND 4) NE 0)       ;; bit 11 set means BF*

          ;; 68020 bitfield instructions

        ENDIF

		IF FMem(opsmod,opsreg,SIZE_W) AND FAlt(opsmod,opsreg,SIZE_W)

		  IF ((opdreg AND 4) EQ 0)     ;; bit 11 must be clear!!

			;; shift/rotate <ea>

			Valid 39, 3F

			if (fEmit NE 0)
			ReadEA %opsmod, %opsreg, %SIZE_W

iDreg TEXTEQU <rDA16>
			DoShift %(opdreg AND 3), %opbit8, %SIZE_W, 1

			WriteEA %opsmod, %opsreg, %SIZE_W, 0

			FastFetch 0
			ENDIF

		  ENDIF

		ENDIF

	ELSEIF (opbit5 EQ 0)

		; #imm,Dn

		Valid 7, 7

		if (fEmit NE 0)
        SetiDreg %opsreg
        lea   rGA,[iDreg]

iDreg TEXTEQU <![rGA!]>
		IF (opdreg EQ 0)
			DoShift %(opsmod AND 3), %opbit8, %opsize, 8
		ELSE
			DoShift %(opsmod AND 3), %opbit8, %opsize, %opdreg
		ENDIF

		FastFetch %opsize
		ENDIF

	ELSE

		;; Dn,Dn

		Valid 7, 7

		IF (fEmit NE 0)

			;;ReadEA %0, %opsreg, %opsize
			; calculate Dn based on low 3 bytes of opcode in EBX
            SetiDreg %opsreg
            lea   rGA,[iDreg]

			SetiDreg %opdreg
			mov   rDA,dword ptr iDreg
			and   rDA,03fh

			;; special case - shift count of 0
			;;
			;; X never gets affected. V is always cleared. C is cleared except
			;; for ROX where it is set to the value of X. N and Z updated.

			IF ((opsmod AND 3) EQ 2)
				IF (opsize EQ 0)
					jz    zero_shiftX0
				ELSEIF (opsize EQ 1)
					jz    zero_shiftX1
				ELSE
					jz    zero_shiftX2
				ENDIF
			ELSE
				IF (opsize EQ 0)
					jz    zero_shift0
				ELSEIF (opsize EQ 1)
					jz    zero_shift1
				ELSE
					jz    zero_shift2
				ENDIF
			ENDIF

iDreg TEXTEQU <![rGA!]>
			DoShift %(opsmod AND 3), %opbit8, %opsize, cl, 1

			FastFetch %opsize
		ENDIF

	ENDIF

	ENDM


	opcode = 0E000h
	REPEAT 01000h

	Decode %opcode

	IF (opsize EQ SIZE_X) AND ((opdreg AND 4) NE 0)       ;; bit 11 set means BF*

        ;; 68020 bitfield instructions

        bfi = (opcode/100h) AND 7

        ;; 0 = BFTST
        ;; 1 = BFEXTU
        ;; 2 = BFCHG
        ;; 3 = BFEXTS
        ;; 4 = BFCLR
        ;; 5 = BFFFO
        ;; 6 = BFSET
        ;; 7 = BFINS

        IF (bfi EQ 0) OR (bfi EQ 1) OR (bfi EQ 3) OR (bfi EQ 5)

            IF (opsmod EQ 0) OR FControl(opsmod,opsreg,SIZE_W)

                Valid

                CheckCpu cpu68020

                DoBitfieldOp

            ENDIF

        ELSE

            IF (opsmod EQ 0) OR (FControl(opsmod,opsreg,SIZE_W) AND FAlt(opsmod,opsreg,SIZE_W))

                Valid

                CheckCpu cpu68020

                DoBitfieldOp

            ENDIF

        ENDIF

    ENDIF

	ENDM

externdef _BitfieldHelper:NEAR

_BF:
    PrepareForCall 0,1          ;; flags need to be saved
    movzx eax,opXword
    push  eax                   ;; extension word
    push  rDA                   ;; instruction word
    push  rGA                   ;; effective address
    call  _BitfieldHelper
    AfterCall 3,0,1             ;; read back affected flags

    FastFetch 0

zero_shiftX0:
	movsx rDA,byte ptr [rGA]
    MoveXToCarry
	setc  flagsVC

	UpdateNZ_Test32
	FastFetch %0

zero_shiftX1:
	movsx rDA,word ptr [rGA]
    MoveXToCarry
	setc  flagsVC

	UpdateNZ_Test32
	FastFetch %0

zero_shiftX2:
	mov   rDA,dword ptr [rGA]
    MoveXToCarry
	setc  flagsVC

	UpdateNZ_Test32
	FastFetch

zero_shift0:
	movsx rDA,byte ptr [rGA]
	UpdateNZ_ClearVC_Test32
	FastFetch %0

zero_shift1:
	movsx rDA,word ptr [rGA]
	UpdateNZ_ClearVC_Test32
	FastFetch %0

zero_shift2:
	mov   rDA,dword ptr [rGA]
	UpdateNZ_ClearVC_Test32
	FastFetch


 .CONST

	PUBLIC _opsE

_opsE   LABEL DWORD
opcE000 LABEL DWORD

 opcode = 0E000h

 REPEAT 01000h
   DefOp %opcode
 ENDM

 END

