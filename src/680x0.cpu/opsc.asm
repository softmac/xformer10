
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; OPCODESC.ASM
;;
;; 68000 opcodes $C000 thru $CFFF (AND, MULU, MULS, ABCD)
;;
;; Copyright (C) 1991-2008 by Darek Mihocka. All Rights Reserved.
;; Branch Always Software. http://www.emulators.com/
;;
;; 03/21/2002 darekm        last update
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	INCLUDE 68000.inc


DoMUL MACRO ireg:REQ, fSigned:REQ

    SetiDreg %ireg

    lea   rGA,iDreg

    IF (fSigned NE 0)
          jmp   _MULS
    ELSE
          jmp   _MULU
    ENDIF

ENDM

; finished off a MULS instruction, calls UpdateNZ, and calls FastFetch
;
; EAX = address of a data register

	BigAlign

_MULS:
	movsx eax,rDA16
	movsx rDA,word ptr [rGA]

	imul  rDA,eax
	mov   [rGA],rDA

	UpdateNZ_ClearVC_Test32
	FastFetch


; finished off a MULU instruction, calls UpdateNZ, and calls FastFetch
;
; EAX = address of a data register

	BigAlign

_MULU:
	movzx eax,rDA16
	movzx rDA,word ptr [rGA]

	imul  rDA,eax
	mov   [rGA],rDA

	UpdateNZ_ClearVC_Test32
	FastFetch


;;
;;  AND  <ea>,Dn or Dn,<ea>
;;  MULx <ea>,Dn
;;  ABCD Dy,Dx or -(Ay),-(Ax)
;;  EXG  Rn,Rn
;;
;;  bits 11..9 - Dn or An
;;  bit  8     - 0 = <ea>,Dn   1 = Dn,<ea>
;;  bits 7..6  - size: 00=byte  01=word  02=long
;;  bits 5..3  - mode
;;  bits 2..0  - register
;;

	opcode = 0C000h
	REPEAT 01000h

	Decode %opcode

	IF (opsize EQ 3)

		IF FData(opsmod,opsreg,opsize)

			IF (opbit8 EQ 0)

				;; MULU <ea>,Dn
				Valid 3C, 3F

				if (fEmit NE 0)
				ReadEA %opsmod, %opsreg, %SIZE_W
				DoMUL  %opdreg, 0
				FastFetch
				ENDIF

			ELSE

				;; MULS <ea>,Dn
				Valid 3C, 3F

				if (fEmit NE 0)
				ReadEA %opsmod, %opsreg, %SIZE_W
				DoMUL  %opdreg, 1
				FastFetch
				ENDIF

			ENDIF

		ENDIF

	ELSEIF (opbit8 EQ 0)

		IF FData(opsmod,opsreg,opsize)

			;; AND <ea>,Dn
			Valid 3C, 3F

			if (fEmit NE 0)
			ReadEA %opsmod, %opsreg, %opsize
			DoAND %opdreg, %opsize, 0
			FastFetch %opsize
			ENDIF

		ENDIF

	ELSE  ;; opsize < 3 && opbit8 == 1

		IF FMem(opsmod,opsreg,opsize) AND FAlt(opsmod,opsreg,opsize)

			;; AND Dn,<ea>
			Valid 39, 3F

			if (fEmit NE 0)
			ReadEA %opsmod, %opsreg, %opsize
			DoAND %opdreg, %opsize, 1
			WriteEA %opsmod, %opsreg, %opsize, 0
			FastFetch %opsize
			ENDIF

		ELSEIF (opdmod EQ 4)

			IF (opsmod EQ 0)

				;; ABCD Dy,Dx
				Valid 7, 7

				if (fEmit NE 0)
				ReadEAMode 0, %opsreg, %SIZE_B
				MoveAccToSrc
				ReadEAMode 0, %opdreg, %SIZE_B
				
				DoBCD 0

				WriteEA 0, %opdreg, %SIZE_B
				FastFetch 0
				ENDIF

			ELSEIF (opsmod EQ 1)

				;; ABCD -(Ay),-(Ax)
				Valid 7, 7

				if (fEmit NE 0)
				ReadEAMode 4, %opsreg, %SIZE_B
                MoveAccToSrc
				ReadEAMode 4, %opdreg, %SIZE_B

				DoBCD 0

				WriteEA 2, %opdreg, %SIZE_B, 0
				FastFetch 0
				ENDIF

			ENDIF

		ELSEIF (opdmod EQ 5)

		ENDIF

	ENDIF

	ENDM


	opcode = 0C000h
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

	ELSE  ;; opsize < 3 && opbit8 == 1

		IF FMem(opsmod,opsreg,opsize) AND FAlt(opsmod,opsreg,opsize)

		ELSEIF (opdmod EQ 4)

			IF (opsmod EQ 0)

			ELSEIF (opsmod EQ 1)

			ENDIF

		ELSEIF (opdmod EQ 5)

			IF (opsmod EQ 0)

				;; EXG Dn,Dn
				Valid

				SetiDreg %opdreg
				mov   rDA,iDreg

				SetiDreg %opsreg
				mov   eax,iDreg
				mov   iDreg,rDA

				SetiDreg %opdreg
				mov   iDreg,eax

				FastFetch

			ELSEIF (opsmod EQ 1)

				;; EXG An,An
				Valid

				SetiAreg %opdreg
				mov   rDA,iAreg

				SetiAreg %opsreg
				mov   eax,iAreg
				mov   iAreg,rDA

				SetiAreg %opdreg
				mov   iAreg,eax

				FastFetch

			ENDIF

		ELSEIF (opdmod EQ 6)

			IF (opsmod EQ 1)

				;; EXG Dn,An
				Valid

				SetiDreg %opdreg
				mov   rDA,iDreg

				SetiAreg %opsreg
				mov   eax,iAreg
				mov   iAreg,rDA
				mov   iDreg,eax

				FastFetch

			ENDIF

		ENDIF

	ENDIF

	ENDM


 .CONST

	PUBLIC _opsC

_opsC   LABEL DWORD
opcC000 LABEL DWORD

 opcode = 0C000h

 REPEAT 01000h
   DefOp %opcode
 ENDM

 END

