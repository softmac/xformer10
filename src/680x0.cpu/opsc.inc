;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; OPSC.INC, included by OPSC.ASM
;;
;; Contains opcodes that require two passes.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


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


