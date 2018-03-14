;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; OPS2.INC, included by OPS2.ASM
;;
;; Contains opcodes that require two passes.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


	opcode = 02000h
	REPEAT 01000h

	Decode %opcode
	IF FAny(opsmod,opsreg,SIZE_L) AND FAlt(opdmod,opdreg,SIZE_L)

		Valid 3C, 3F, 1

		IF (fEmit NE 0)

			ReadEA %opsmod, %opsreg, %SIZE_L

			IF (opdmod NE 1)
                TestLong
                UpdateNZ_ClearVC
            ELSEIF (opdreg EQ 7)
                and rDA,0FFFFFFFEh
			ENDIF

			WriteEA %opdmod, %opdreg, %SIZE_L

			FastFetch

		ENDIF

	ENDIF

	ENDM

