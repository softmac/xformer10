;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; OPSE.INC, included by OPSE.ASM
;;
;; Contains opcodes that require two passes.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

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
		and   rDA,7
		lea   rGA,[regD0+rDA*4]

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
			and   rDA,7
			lea   rGA,[regD0+rDA*4]

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

