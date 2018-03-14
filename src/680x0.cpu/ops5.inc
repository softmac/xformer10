;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; OPS5.INC, included by OPS5.ASM
;;
;; Contains opcodes that require two passes.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


;;
;;  ADDQ #n,<ea>
;;  SUBQ #n,<ea>
;;  Scc  <ea>
;;  DBcc Dn,<ea>
;;

	opcode = 05000h
	REPEAT 01000h

	Decode %opcode

	IF (opsize EQ SIZE_X)

		IF (opsmod EQ 1)

			;; DBcc Dn,offset
			Valid 7,7

			IF (fEmit NE 0)

			TestCond <DBtrue>  ;; if condition is true, will branch to DBtrue

            and   rDA,7t
			lea   rGA,[regD0+rDA*4]

			;; Condition false, so decrement data register and loop.
			;; Don't do Imm16sx because that decrements rPC.
			movsx eax,word ptr [rPC]

			sub   word ptr [rGA],1
			jc    DBtrue

			sub   rPC,eax
			Fetch 0
			ENDIF

		ELSE
		IF (FAlt(opsmod,opsreg,opsize) NE 0)

			;; Scc <ea>
			Valid 39, 3F

			IF (fEmit NE 0)
			ReadEA %opsmod, %opsreg, %SIZE_B
		    mov   rDA8L,0FFh ;; assume condition true, set to $FF
			TestCond <@F>    ;; if condition is true, will branch to @@
		    inc   rDA8L      ;; condition is false, set to $00
@@:
			WriteEA %opsmod, %opsreg, %SIZE_B, 0
			FastFetch
			ENDIF

		ELSEIF (opsmod EQ 7) AND (opsreg EQ 4)

			;; TRAPcc
			Valid

		    CheckCpu cpu68020

			TestCond <_xcptTRAPV>
			FastFetch

		ELSEIF (opsmod EQ 7) AND (opsreg EQ 2)

			;; TRAPcc.W
			Valid

		    CheckCpu cpu68020

		    Imm16nx
			TestCond <_xcptTRAPV>
			FastFetch

		ELSEIF (opsmod EQ 7) AND (opsreg EQ 3)

			;; TRAPcc.L
			Valid

		    CheckCpu cpu68020

		    Imm32
			TestCond <_xcptTRAPV>
			FastFetch

		ENDIF
		ENDIF

	ELSEIF (opbit8 EQ 0)

		IF FAlt(opsmod,opsreg,opsize)

			;; ADDQ #n,<ea>
			Valid 39, 3F

			IF (fEmit NE 0)
				AddqSubq <Add>, %opdreg, %opsmod, %opsreg, %opsize
				FastFetch %opsize
			ENDIF

		ENDIF

	ELSE

		IF FAlt(opsmod,opsreg,opsize)

			;; SUBQ #n,<ea>
			Valid 39, 3F

			IF (fEmit NE 0)
				AddqSubq <Sub>, %opdreg, %opsmod, %opsreg, %opsize
				FastFetch %opsize
			ENDIF

		ENDIF

	ENDIF

	ENDM


