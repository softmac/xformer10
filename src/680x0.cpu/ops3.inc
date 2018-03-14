;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; OPS3.INC, included by OPS3.ASM
;;
;; Contains opcodes that require two passes.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


	opcode = 03000h
	REPEAT 01000h

	Decode %opcode
	IF FAny(opsmod,opsreg,SIZE_W) AND FAlt(opdmod,opdreg,SIZE_W)

		Valid 3C, 3F, 1

		IF (fEmit NE 0)

			ReadEA %opsmod, %opsreg, %SIZE_W

			IF (opdmod NE 1)
				TestWord
				UpdateNZ_ClearVC
				WriteEA %opdmod, %opdreg, %SIZE_W
			ELSE
                SignExtendWord
				WriteEA %opdmod, %opdreg, %SIZE_L
			ENDIF

			FastFetch 0

		ENDIF

	ENDIF

	ENDM

