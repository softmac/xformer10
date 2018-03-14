
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; OPCODESD.ASM
;;
;; 68000 opcodes $D000 thru $DFFF (ADD, ADDA, ADDX)
;;
;; Copyright (C) 1991-2008 by Darek Mihocka. All Rights Reserved.
;; Branch Always Software. http://www.emulators.com/
;;
;; 01/06/1997 darekm        last update
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	INCLUDE 68000.inc

;;
;;  ADD <ea>,Dn or Dn,<ea>
;;  ADDA <ea>,Dn
;;  ADDX Dy,Dx or -(Ay),-(Ax)
;;
;;  bits 11..9 - Dn or An
;;  bit  8     - 0 = <ea>,Dn   1 = Dn,<ea>
;;  bits 7..6  - size: 00=byte  01=word  02=long
;;  bits 5..3  - mode
;;  bits 2..0  - register
;;

	opcode = 0D000h
	REPEAT 01000h

	Decode %opcode

	IF (opsize EQ 3)

		IF FAny(opsmod,opsreg,opsize)

			IF (opbit8 EQ 0)

				;; ADDA.W <ea>,An
				Valid 3C, 3F, 1

				IF (fEmit NE 0)
				ReadEA %opsmod, %opsreg, %SIZE_W
				SignExtendWord
				DoAdd  %opdreg, %SIZE_L, 2
				FastFetch
				ENDIF

			ELSE

				;; ADDA.W <ea>,An
				Valid 3C, 3F, 1

				IF (fEmit NE 0)
				ReadEA %opsmod, %opsreg, %SIZE_L
				DoAdd  %opdreg, %SIZE_L, 2
				FastFetch
				ENDIF

			ENDIF

		ENDIF

	ELSEIF (opbit8 EQ 0)

		IF FAny(opsmod,opsreg,opsize)

			;; ADD <ea>,Dn
			Valid 3C, 3F

			IF (fEmit NE 0)
			ReadEA %opsmod, %opsreg, %opsize
			DoAdd %opdreg, %opsize, 0
			FastFetch %opsize
			ENDIF

		ENDIF

	ELSE

		IF FMem(opsmod,opsreg,opsize) AND FAlt(opsmod,opsreg,opsize)

			;; ADD Dn,<ea>
			Valid 39, 3F

			IF (fEmit NE 0)
			ReadEA %opsmod, %opsreg, %opsize
			DoAdd %opdreg, %opsize, 1
			WriteEA %opsmod, %opsreg, %opsize, 0
			FastFetch %opsize
			ENDIF

		ELSEIF (opsmod EQ 0)

			;; ADDX Dy,Dx
			Valid 7, 7

			IF (fEmit NE 0)
			ReadEAMode 0, %opsreg, %opsize
			DoAdd %opdreg, %opsize, 0, 1
			FastFetch %opsize
			ENDIF

		ELSEIF (opsmod EQ 1)

			;; ADDX -(Ay),-(Ax)
			Valid 7, 7

			IF (fEmit NE 0)
			ReadEAMode 4, %opsreg, %opsize
            MoveAccToSrc
			ReadEAMode 4, %opdreg, %opsize

			DoAddxSubx %opsize

			WriteEA 2, %opdreg, %opsize, 0
			FastFetch %opsize
			ENDIF

		ENDIF

	ENDIF

	ENDM

 .CONST

	PUBLIC _opsD

_opsD   LABEL DWORD
opcD000 LABEL DWORD

 opcode = 0D000h

 REPEAT 01000h
   DefOp %opcode
 ENDM

 END

