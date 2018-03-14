;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; OPS4.INC, included by OPS4.ASM
;;
;; Contains opcodes that require two passes.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


;;
;;  NEGX.B <ea>
;;
	opcode = 04000h

	REPEAT 00040h

	Decode %opcode
	IF FDataAlt(opsmod,opsreg,SIZE_B)

		Valid 39, 3F

		IF (fEmit NE 0)
		ReadEA %opsmod, %opsreg, %SIZE_B
		SignExtendByteToSrc

        ClearLong
		MoveXToCarry
        sbb     rDA8L,temp8
		UpdateNEGX

		WriteEA %opsmod, %opsreg, %SIZE_B, 0

		FastFetch 0
		ENDIF

	ENDIF

	ENDM


;;
;;  NEGX.W <ea>
;;
	opcode = 04040h

	REPEAT 00040h

	Decode %opcode
	IF FDataAlt(opsmod,opsreg,SIZE_W)

		Valid 39, 3F

		IF (fEmit NE 0)
		ReadEA %opsmod, %opsreg, %SIZE_W
		SignExtendWordToSrc

        ClearLong
		MoveXToCarry
        sbb     rDA16,temp16
		UpdateNEGX

		WriteEA %opsmod, %opsreg, %SIZE_W, 0

		FastFetch 0
		ENDIF

	ENDIF

	ENDM


;;
;;  NEGX.L <ea>
;;
	opcode = 04080h

	REPEAT 00040h

	Decode %opcode
	IF FDataAlt(opsmod,opsreg,SIZE_L)

		Valid 39, 3F

		IF (fEmit NE 0)
		ReadEA %opsmod, %opsreg, %SIZE_L
		MoveAccToSrc

        ClearLong
		MoveXToCarry
        sbb     rDA,temp32
		UpdateNEGX

		WriteEA %opsmod, %opsreg, %SIZE_L, 0

		FastFetch
		ENDIF

	ENDIF

	ENDM


;;
;;  CHK <ea>
;;
	indexan = 0
	REPEAT 8

	opcode = 04180h + indexan*0200h
	REPEAT 00040h

	Decode %opcode
	IF FData(opsmod,opsreg,SIZE_L)

		Valid 3C, 3F

		IF (fEmit NE 0)
		ReadEA %opsmod, %opsreg, %SIZE_W
		SetiDreg %opdreg
		mov   ax,word ptr iDreg
		test  rDA16,rDA16
		js    _xcptCHKERR ;; Dn < 0

		cmp   ax,rDA16
		jg    _xcptCHKERR ;; Dn > upper bound

		FastFetch 0
		ENDIF

	ENDIF

	ENDM

	indexan = indexan + 1

	ENDM


;;
;;  CLR.B <ea>
;;
	opcode = 04200h

	REPEAT 00040h

	Decode %opcode
	IF FDataAlt(opsmod,opsreg,SIZE_B)

		Valid 39, 3F

		IF (fEmit NE 0)
		ReadEA %opsmod, %opsreg, %SIZE_B
        ClearLong
        ClearVC
		mov   flagsNZ,40h
		WriteEA %opsmod, %opsreg, %SIZE_B, 0

		FastFetch 0
		ENDIF

	ENDIF

	ENDM


;;
;;  CLR.W <ea>
;;
	opcode = 04240h

	REPEAT 00040h

	Decode %opcode
	IF FDataAlt(opsmod,opsreg,SIZE_W)

		Valid 39, 3F

		IF (fEmit NE 0)
		ReadEA %opsmod, %opsreg, %SIZE_W
        ClearLong
        ClearVC
		mov   flagsNZ,40h
		WriteEA %opsmod, %opsreg, %SIZE_W, 0

		FastFetch 0
		ENDIF

	ENDIF

	ENDM


;;
;;  CLR.L <ea>
;;
	opcode = 04280h

	REPEAT 00040h

	Decode %opcode
	IF FDataAlt(opsmod,opsreg,SIZE_L)

		Valid 39, 3F

		IF (fEmit NE 0)
		ReadEA %opsmod, %opsreg, %SIZE_L
        ClearLong
        ClearVC
		mov   flagsNZ,40h
		WriteEA %opsmod, %opsreg, %SIZE_L, 0

		FastFetch 0
		ENDIF

	ENDIF

	ENDM


;;
;;  NEG.B <ea>
;;
	opcode = 04400h

	REPEAT 00040h

	Decode %opcode
	IF FDataAlt(opsmod,opsreg,SIZE_B)

		Valid 39, 3F

		IF (fEmit NE 0)
		ReadEA %opsmod, %opsreg, %SIZE_B

		NegByte
		UpdateCCR

		WriteEA %opsmod, %opsreg, %SIZE_B, 0

		FastFetch 0
		ENDIF

	ENDIF

	ENDM


;;
;;  NEG.W <ea>
;;
	opcode = 04440h

	REPEAT 00040h

	Decode %opcode
	IF FDataAlt(opsmod,opsreg,SIZE_W)

		Valid 39, 3F

		IF (fEmit NE 0)
		ReadEA %opsmod, %opsreg, %SIZE_W

		NegWord
		UpdateCCR

		WriteEA %opsmod, %opsreg, %SIZE_W, 0

		FastFetch 0
		ENDIF

	ENDIF

	ENDM


;;
;;  NEG.L <ea>
;;
	opcode = 04480h

	REPEAT 00040h

	Decode %opcode
	IF FDataAlt(opsmod,opsreg,SIZE_L)

		Valid 39, 3F

		IF (fEmit NE 0)
		ReadEA %opsmod, %opsreg, %SIZE_L

		NegLong
		UpdateCCR

		WriteEA %opsmod, %opsreg, %SIZE_L, 0

		FastFetch
		ENDIF

	ENDIF

	ENDM


;;
;;  MOVE <ea>,CCR
;;
	opcode = 044C0h

	REPEAT 00040h

	Decode %opcode
	IF FData(opsmod,opsreg,SIZE_W)

		Valid 3C, 3F

		IF (fEmit NE 0)
		ReadEA %opsmod, %opsreg, %SIZE_W
		and   rDA8L,1Fh
		PutCCR

		FastFetch 0
		ENDIF

	ENDIF

	ENDM


;;
;;  NOT.B <ea>
;;
	opcode = 04600h

	REPEAT 00040h

	Decode %opcode
	IF FDataAlt(opsmod,opsreg,SIZE_B)

		Valid 39, 3F

		IF (fEmit NE 0)
		ReadEA %opsmod, %opsreg, %SIZE_B

		NotByte
		TestByte
		UpdateNZ_ClearVC

		WriteEA %opsmod, %opsreg, %SIZE_B, 0

		FastFetch 0
		ENDIF

	ENDIF

	ENDM


;;
;;  NOT.W <ea>
;;
	opcode = 04640h

	REPEAT 00040h

	Decode %opcode
	IF FDataAlt(opsmod,opsreg,SIZE_W)

		Valid 39, 3F

		IF (fEmit NE 0)
		ReadEA %opsmod, %opsreg, %SIZE_W

		NotWord
		TestWord
		UpdateNZ_ClearVC

		WriteEA %opsmod, %opsreg, %SIZE_W, 0

		FastFetch 0
		ENDIF

	ENDIF

	ENDM


;;
;;  NOT.L <ea>
;;
	opcode = 04680h

	REPEAT 00040h

	Decode %opcode
	IF FDataAlt(opsmod,opsreg,SIZE_L)

		Valid 39, 3F

		IF (fEmit NE 0)
		ReadEA %opsmod, %opsreg, %SIZE_L

		NotLong
		TestLong
		UpdateNZ_ClearVC

		WriteEA %opsmod, %opsreg, %SIZE_L, 0

		FastFetch
		ENDIF

	ENDIF

	ENDM


;;
;;  MOVE <ea>,SR
;;
	opcode = 046C0h

	REPEAT 00040h

	Decode %opcode
	IF FData(opsmod,opsreg,SIZE_W)

		Valid 3C, 3F

		IF (fEmit NE 0)
		CheckPriv
		ReadEA %opsmod, %opsreg, %SIZE_W
		PutSR
		ENDIF

	ENDIF

	ENDM


;;
;;  NBCD.B <ea>
;;
	opcode = 04800h

	REPEAT 00040h

	Decode %opcode
	IF FDataAlt(opsmod,opsreg,SIZE_B)

		Valid 39, 3F

		IF (fEmit NE 0)
		ReadEA %opsmod, %opsreg, %SIZE_B

        MoveXToCarry
		mov   al,0
		sbb   al,rDA8L
		das
		mov   rDA8L,al
		UpdateBCD

		WriteEA %opsmod, %opsreg, %SIZE_B, 0

		FastFetch 0
		ENDIF

	ENDIF

	ENDM


;;
;;  TST.B <ea>
;;
	opcode = 04A00h

	REPEAT 00040h

	Decode %opcode
	IF FDataAlt(opsmod,opsreg,SIZE_B)

		Valid 39, 3F

		IF (fEmit NE 0)
		ReadEA %opsmod, %opsreg, %SIZE_B

		TestByte
		UpdateNZ_ClearVC

		FastFetch 0
		ENDIF

	ENDIF

	ENDM


;;
;;  TST.W <ea>
;;
	opcode = 04A40h

	REPEAT 00040h

	Decode %opcode
	;; IF FDataAlt(opsmod,opsreg,SIZE_W)    ;; this is 68000 only
	IF FAny(opsmod,opsreg,SIZE_W)           ;; HACK! HACK! works on 68020+

		;; Valid 39, 3F                     ;; this is 68000 only
		Valid 3B, 3F                        ;; HACK! HACK! works on 68020+

		IF (fEmit NE 0)
		ReadEA %opsmod, %opsreg, %SIZE_W

		TestWord
		UpdateNZ_ClearVC

		FastFetch 0
		ENDIF

	ENDIF

	ENDM


;;
;;  TST.L <ea>
;;
	opcode = 04A80h

	REPEAT 00040h

	Decode %opcode
	;; IF FDataAlt(opsmod,opsreg,SIZE_W)    ;; this is 68000 only
	IF FAny(opsmod,opsreg,SIZE_W)           ;; HACK! HACK! works on 68020+

		;; Valid 39, 3F                     ;; this is 68000 only
		Valid 3B, 3F                        ;; HACK! HACK! works on 68020+

		IF (fEmit NE 0)
		ReadEA %opsmod, %opsreg, %SIZE_L

		TestLong
		UpdateNZ_ClearVC

		FastFetch
		ENDIF

	ENDIF

	ENDM


;;
;;  TAS <ea>
;;
	opcode = 04AC0h

	REPEAT 00040h

	Decode %opcode
	IF FDataAlt(opsmod,opsreg,SIZE_B)

		Valid 39, 3F

		IF (fEmit NE 0)
		ReadEA %opsmod, %opsreg, %SIZE_B

		TestByte
		UpdateNZ_ClearVC
		or    rDA8L,080h

		WriteEA %opsmod, %opsreg, %SIZE_B, 0

		FastFetch 0
		ENDIF

	ENDIF

	ENDM



;;
;; 68020 long multiply, MULU, MULS
;;

	opcode = 04C00h

	REPEAT 00040h

	Decode %opcode

    IF FData(opsmod,opsreg,SIZE_L)

        Valid 3C, 3F

        if (fEmit NE 0)

        CheckCpu cpu68020

        Imm16zx
        mov   opXword,ax

        ReadEA %opsmod, %opsreg, %SIZE_L
        jmp   _MULL

        ENDIF
    ENDIF

	ENDM


;;
;; 68020 long divide, DIVU, DIVS
;;

	opcode = 04C40h

	REPEAT 00040h

	Decode %opcode

    IF FData(opsmod,opsreg,SIZE_L)

        Valid 3C, 3F

        if (fEmit NE 0)

        CheckCpu cpu68020

        Imm16zx
        mov   opXword,ax

        ReadEA %opsmod, %opsreg, %SIZE_L
        jmp   _DIVL

        ENDIF
    ENDIF

	ENDM


