;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; OPS1.INC, included by OPS1.ASM
;;
;; Contains opcodes that require two passes.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


	opcode = 01000h
	REPEAT 01000h

	Decode %opcode
	IF FAny(opsmod,opsreg,SIZE_B) AND FAlt(opdmod,opdreg,SIZE_B)

		Valid 3C, 3F

		IF (fEmit NE 0)

			ReadEA %opsmod, %opsreg, %SIZE_B

            TestByte
            UpdateNZ_ClearVC
			WriteEA %opdmod, %opdreg, %SIZE_B

			FastFetch 0

		ENDIF

	ENDIF

	ENDM

