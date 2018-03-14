
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; OPCODES5.ASM
;;
;; 68000 opcodes $5000 thru $5FFF (ADDQ, SUBQ, DBcc, Scc)
;;
;; Copyright (C) 1991-2008 by Darek Mihocka. All Rights Reserved.
;; Branch Always Software. http://www.emulators.com/
;;
;; 10/23/1993 darekm        last update
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	INCLUDE 68000.inc

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

			;; Condition false, so decrement data register and loop.

            Imm16sxNoIncPC

            SetiDreg %opsreg
			sub   word ptr [iDreg],1
			jc    DBtrue

            Flat2PC
			add   rGA,eax
            PC2Flat

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

		    Imm16zx
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


;;
;; JMP here to complete a DBcc that does not branch
;;

	BigAlign
DBtrue:
	IncrementPCBy2
	FastFetch 0


	.CONST

	PUBLIC _ops5

_ops5   LABEL DWORD
opc5000 LABEL DWORD

	opcode = 05000h
	REPEAT 01000h
		DefOp %opcode
	ENDM

 END

