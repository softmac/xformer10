;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; OPS8.INC, included by OPS8.ASM
;;
;; Contains opcodes that require two passes.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


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

