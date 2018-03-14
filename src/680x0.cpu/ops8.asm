
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; OPCODES8.ASM
;;
;; 68000 opcodes $8000 thru $8FFF (OR, DIVU, DIVS, SBCD)
;;
;; Copyright (C) 1991-2008 by Darek Mihocka. All Rights Reserved.
;; Branch Always Software. http://www.emulators.com/
;;
;; 03/21/2002 darekm        last update
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	INCLUDE 68000.inc


; NOTE: This file is virtually identical to OPSC.ASM (AND, MUL, ABCD).
;       If you change one you better change the other.

opinline = 1				; force inline fetches

externdef _DIVoverflow:NEAR

DoDIV MACRO ireg:REQ, fSigned:REQ

    SetiDreg %ireg

    lea   rGA,iDreg

    IF (fSigned NE 0)
          jmp   _DIVS
    ELSE
          jmp   _DIVU
    ENDIF
ENDM

; finished off a DIVS instruction, calls UpdateNZ, and calls FastFetch
;
; EAX = address of a data register

	BigAlign

_DIVS:
    ClearVC
	movsx rDA,rDA16

	test  rDA,rDA
	je    _xcptDIVZERO

	mov   eax,[rGA]
	cdq
	idiv  rDA        ;; EAX = quotient. EDX = remainder

	movsx rDA,ax     ;; Make sure that high word of quotient matches
	cmp   rDA,eax    ;; the sign bit of the quotient

_DIVcom:
	mov   rDA,edx   ;; remainder

	jne   _DIVoverflow

	shl   rDA,16t   ;; shift up remainder
	mov   rDA16,ax  ;; merge in quotient
	mov   [rGA],rDA

	UpdateNZ_Test16 ;; flags based on 16-bit quotient

	FastFetch

; finished off a DIVU instruction, calls UpdateNZ, and calls FastFetch
;
; EAX = address of a data register

	BigAlign

_DIVU:
    ClearVC
	movzx rDA,rDA16
	test  rDA,rDA
	je    _xcptDIVZERO
	mov   eax,[rGA]
	xor   edx,edx
	div   rDA        ;; EAX = quotient. EDX = remainder

	test  eax,0FFFF0000h  ;; make sure high word of quotient is all 0's
	jmp   _DIVcom


;;
;;  OR   <ea>,Dn or Dn,<ea>
;;  DIVx <ea>,Dn
;;  SBCD Dy,Dx or -(Ay),-(Ax)
;;
;;  bits 11..9 - Dn or An
;;  bit  8     - 0 = <ea>,Dn   1 = Dn,<ea>
;;  bits 7..6  - size: 00=byte  01=word  02=long
;;  bits 5..3  - mode
;;  bits 2..0  - register
;;

	opcode = 08000h
	REPEAT 01000h

	Decode %opcode

	IF (opsize EQ 3)

		IF FData(opsmod,opsreg,opsize)

			IF (opbit8 EQ 0)

				;; DIVU <ea>,Dn
				Valid 3C, 3F

				IF (fEmit NE 0)
				ReadEA %opsmod, %opsreg, %SIZE_W
				DoDIV  %opdreg, 0
				FastFetch
				ENDIF

			ELSE

				;; DIVS <ea>,Dn
				Valid 3C, 3F

				IF (fEmit NE 0)
				ReadEA %opsmod, %opsreg, %SIZE_W
				DoDIV  %opdreg, 1
				FastFetch
				ENDIF

			ENDIF

		ENDIF

	ELSEIF (opbit8 EQ 0)

		IF FData(opsmod,opsreg,opsize)

			;; OR <ea>,Dn
			Valid 3C, 3F

			IF (fEmit NE 0)
			ReadEA %opsmod, %opsreg, %opsize
			DoOR %opdreg, %opsize, 0
			FastFetch %opsize
			ENDIF

		ENDIF

	ELSE

		IF FMem(opsmod,opsreg,opsize) AND FAlt(opsmod,opsreg,opsize)

			;; OR Dn,<ea>
			Valid 39, 3F

			IF (fEmit NE 0)
			ReadEA %opsmod, %opsreg, %opsize
			DoOR %opdreg, %opsize, 1
			WriteEA %opsmod, %opsreg, %opsize, 0
			FastFetch %opsize
			ENDIF

		ELSEIF (opdmod EQ 4)

			IF (opsmod EQ 0)

				;; SBCD Dy,Dx
				Valid 7, 7

				IF (fEmit NE 0)
				ReadEAMode 0, %opsreg, %SIZE_B
				MoveAccToSrc
				ReadEAMode 0, %opdreg, %SIZE_B

				DoBCD 1

				WriteEA 0, %opdreg, %SIZE_B
				FastFetch 0
				ENDIF

			ELSEIF (opsmod EQ 1)

				;; SBCD -(Ay),-(Ax)
				Valid 7, 7

				IF (fEmit NE 0)
				ReadEAMode 4, %opsreg, %SIZE_B
                MoveAccToSrc
				ReadEAMode 4, %opdreg, %SIZE_B

				DoBCD 1

				WriteEA 2, %opdreg, %SIZE_B, 0
				FastFetch 0
				ENDIF

			ENDIF

		ENDIF

	ENDIF

	ENDM


	opcode = 08000h
	REPEAT 01000h

	Decode %opcode

	IF (opsize EQ 3)

		IF FAny(opsmod,opsreg,opsize)

			IF (opbit8 EQ 0)

			ELSE

			ENDIF

		ENDIF

	ELSEIF (opbit8 EQ 0)

		IF FAny(opsmod,opsreg,opsize)

		ENDIF

	ELSE

		IF FMem(opsmod,opsreg,opsize) AND FAlt(opsmod,opsreg,opsize)

		ELSEIF (opdmod EQ 4)

			IF (opsmod EQ 0)

			ELSEIF (opsmod EQ 1)

			ENDIF

		ENDIF

	ENDIF

	ENDM


 .CONST

	PUBLIC _ops8

_ops8   LABEL DWORD
opc8000 LABEL DWORD

 opcode = 08000h

 REPEAT 01000h
   DefOp %opcode
 ENDM

 END

