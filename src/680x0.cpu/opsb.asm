
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; OPCODESB.ASM
;;
;; 68000 opcodes $B000 thru $BFFF (CMP, CMPA, EOR, CMPM)
;;
;; Copyright (C) 1991-2008 by Darek Mihocka. All Rights Reserved.
;; Branch Always Software. http://www.emulators.com/
;;
;; 01/06/1997 darekm        last update
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	INCLUDE 68000.inc

;;
;;  CMP <ea>,Dn
;;  EOR Dn,<ea>
;;  CMPA <ea>,An
;;  CMPM -(Ay),-(Ax)
;;
;;  bits 11..9 - Dn or An
;;  bit  8     - 0 = <ea>,Dn   1 = Dn,<ea>
;;  bits 7..6  - size: 00=byte  01=word  02=long
;;  bits 5..3  - mode
;;  bits 2..0  - register
;;

	opcode = 0B000h
	REPEAT 01000h

	Decode %opcode

	IF (opsize EQ 3)

		IF FAny(opsmod,opsreg,opsize)

			IF (opbit8 EQ 0)

				;; CMPA.W <ea>,An
				Valid 3C, 3F, 1

				IF (fEmit NE 0)
				ReadEA %opsmod, %opsreg, %SIZE_W
				SignExtendWord
				DoCMP  1, %opdreg, %SIZE_L
				FastFetch
				ENDIF

			ELSE

				;; CMPA.L <ea>,An
				Valid 3C, 3F, 1

				IF (fEmit NE 0)
				ReadEA %opsmod, %opsreg, %SIZE_L
				DoCMP  1, %opdreg, %SIZE_L
				FastFetch
				ENDIF

			ENDIF

		ENDIF

	ELSEIF (opbit8 EQ 0)

		IF FAny(opsmod,opsreg,opsize)

			;; CMP <ea>,Dn
			Valid 3C, 3F

			IF (fEmit NE 0)
			ReadEA %opsmod, %opsreg, %opsize
			DoCMP  0, %opdreg, %opsize
			FastFetch %opsize
			ENDIF

		ENDIF

	ELSE

		IF FData(opsmod,opsreg,opsize) AND FAlt(opsmod,opsreg,opsize)

			;; EOR Dn,<ea>
			Valid 39, 3F

			IF (fEmit NE 0)
			ReadEA %opsmod, %opsreg, %opsize
			DoEOR %opdreg, %opsize
			WriteEA %opsmod, %opsreg, %opsize, 0
			FastFetch %opsize
			ENDIF

		ELSEIF (opsmod EQ 1)

			;; CMPM (Ay)+,(Ax)+
			Valid 7, 7

			IF (fEmit NE 0)
			ReadEAMode 3, %opsreg, %opsize
			DoCMP  3, %opdreg, %opsize
			FastFetch %opsize
			ENDIF

		ENDIF

	ENDIF

	ENDM


 .CONST

 opcode = 0B000h

	PUBLIC _opsB

_opsB   LABEL DWORD
opcB000 LABEL DWORD

 REPEAT 01000h
   DefOp %opcode
 ENDM

 END

