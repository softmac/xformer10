;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; OPS0.INC, included by OPS0.ASM
;;
;; Contains opcodes that require two passes.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


;;
;;  ORI.B #<data>,<ea>
;;

	opcode = 00000h
	REPEAT 00040h

	Decode %opcode
	IF FDataAlt(opsmod,opsreg,SIZE_B)

		Valid 39, 3F

		IF (fEmit NE 0)
			Imm8zx
            mov   temp32,eax

			ReadEA %opsmod, %opsreg, %SIZE_B
			or    rDA8L,temp8
			UpdateNZ_ClearVC

			WriteEA %opsmod, %opsreg, %SIZE_B, 0
			FastFetch 0
		ENDIF

	ENDIF

	ENDM


;;
;;  ORI.W #<data>,<ea>
;;

	opcode = 00040h
	REPEAT 00040h

	Decode %opcode
	IF FDataAlt(opsmod,opsreg,SIZE_W)

		Valid 39, 3F

		IF (fEmit NE 0)
			Imm16zx
            mov   temp32,eax

			ReadEA %opsmod, %opsreg, %SIZE_W
			or    rDA16,temp16
			UpdateNZ_ClearVC

			WriteEA %opsmod, %opsreg, %SIZE_W, 0
			FastFetch 0
		ENDIF

	ENDIF

	ENDM


;;
;;  ORI.L #<data>,<ea>
;;

	opcode = 00080h
	REPEAT 00040h

	Decode %opcode
	IF FDataAlt(opsmod,opsreg,SIZE_L)

		Valid 39, 3F

		IF (fEmit NE 0)
			Imm32 
            mov   temp32,eax

			ReadEA %opsmod, %opsreg, %SIZE_L
			or    rDA,temp32
			UpdateNZ_ClearVC

			WriteEA %opsmod, %opsreg, %SIZE_L, 0
			FastFetch
		ENDIF

	ENDIF

	ENDM


;;
;;  ANDI.B #<data>,<ea>
;;

	opcode = 00200h

	REPEAT 00040h

	Decode %opcode
	IF FDataAlt(opsmod,opsreg,SIZE_B)

		Valid 39, 3F

		IF (fEmit NE 0)
			Imm8zx
            mov   temp32,eax

			ReadEA %opsmod, %opsreg, %SIZE_B
			and   rDA8L,temp8
			UpdateNZ_ClearVC

			WriteEA %opsmod, %opsreg, %SIZE_B, 0
			FastFetch 0
		ENDIF

	ENDIF

	ENDM


;;
;;  ANDI.W #<data>,<ea>
;;
	
	opcode = 00240h
	REPEAT 00040h

	Decode %opcode
	IF FDataAlt(opsmod,opsreg,SIZE_W)

		Valid 39, 3F

		IF (fEmit NE 0)
			Imm16zx
            mov   temp32,eax

			ReadEA %opsmod, %opsreg, %SIZE_W
			and   rDA16,temp16
			UpdateNZ_ClearVC

			WriteEA %opsmod, %opsreg, %SIZE_W, 0
			FastFetch 0
		ENDIF

	ENDIF

	ENDM


;;
;;  ANDI.L #<data>,<ea>
;;

	opcode = 00280h
	REPEAT 00040h

	Decode %opcode
	IF FDataAlt(opsmod,opsreg,SIZE_L)

		Valid 39, 3F

		IF (fEmit NE 0)
			Imm32
            mov   temp32,eax

			ReadEA %opsmod, %opsreg, %SIZE_L
			and   rDA,temp32
			UpdateNZ_ClearVC
	
			WriteEA %opsmod, %opsreg, %SIZE_L, 0
			FastFetch
		ENDIF

	ENDIF

	ENDM


;;
;;  SUBI.B #<data>,<ea>
;;
	opcode = 00400h

	REPEAT 00040h

	Decode %opcode
	IF FDataAlt(opsmod,opsreg,SIZE_B)

		Valid 39, 3F

		IF (fEmit NE 0)
			Imm8zx
            mov   temp32,eax

			ReadEA %opsmod, %opsreg, %SIZE_B
			sub   rDA8L,temp8
			UpdateCCR
		
			WriteEA %opsmod, %opsreg, %SIZE_B, 0
			FastFetch 0
		ENDIF

	ENDIF

	ENDM


;;
;;  SUBI.W #<data>,<ea>
;;
	opcode = 00440h
	REPEAT 00040h

	Decode %opcode
	IF FDataAlt(opsmod,opsreg,SIZE_W)

		Valid 39, 3F

		IF (fEmit NE 0)
			Imm16zx 
            mov   temp32,eax

			ReadEA %opsmod, %opsreg, %SIZE_W
			sub   rDA16,temp16
			UpdateCCR

			WriteEA %opsmod, %opsreg, %SIZE_W, 0
			FastFetch 0
		ENDIF

	ENDIF

	ENDM


;;
;;  SUBI.L #<data>,<ea>
;;
	opcode = 00480h
	REPEAT 00040h

	Decode %opcode
	IF FDataAlt(opsmod,opsreg,SIZE_L)

		Valid 39, 3F

		IF (fEmit NE 0)
			Imm32
            mov   temp32,eax

			ReadEA %opsmod, %opsreg, %SIZE_L
			sub   rDA,temp32
			UpdateCCR

			WriteEA %opsmod, %opsreg, %SIZE_L, 0
			FastFetch
		ENDIF

	ENDIF

	ENDM


;;
;;  ADDI.B #<data>,<ea>
;;
	opcode = 00600h

	REPEAT 00040h

	Decode %opcode
	IF FDataAlt(opsmod,opsreg,SIZE_B)

		Valid 39, 3F

		IF (fEmit NE 0)
			Imm8zx
            mov   temp32,eax

			ReadEA %opsmod, %opsreg, %SIZE_B
			add   rDA8L,temp8
			UpdateCCR

			WriteEA %opsmod, %opsreg, %SIZE_B, 0
			FastFetch 0
		ENDIF

	ENDIF

	ENDM


;;
;;  ADDI.W #<data>,<ea>
;;
	opcode = 00640h
	REPEAT 00040h

	Decode %opcode
	IF FDataAlt(opsmod,opsreg,SIZE_W)

		Valid 39, 3F

		IF (fEmit NE 0)
			Imm16zx
            mov   temp32,eax

			ReadEA %opsmod, %opsreg, %SIZE_W
			add   rDA16,temp16
			UpdateCCR

			WriteEA %opsmod, %opsreg, %SIZE_W, 0
			FastFetch 0
		ENDIF

	ENDIF

	ENDM


;;
;;  ADDI.L #<data>,<ea>
;;
	opcode = 00680h
	REPEAT 00040h

	Decode %opcode
	IF FDataAlt(opsmod,opsreg,SIZE_L)

		Valid 39, 3F

		IF (fEmit NE 0)
			Imm32
            mov   temp32,eax

			ReadEA %opsmod, %opsreg, %SIZE_L
			add   rDA,temp32
			UpdateCCR

			WriteEA %opsmod, %opsreg, %SIZE_L, 0
			FastFetch
		ENDIF

	ENDIF

	ENDM


;;
;;  EORI.B #<data>,<ea>
;;
	opcode = 00A00h

	REPEAT 00040h

	Decode %opcode
	IF FDataAlt(opsmod,opsreg,SIZE_B)

		Valid 39, 3F

		IF (fEmit NE 0)
			Imm8zx
            mov   temp32,eax

			ReadEA %opsmod, %opsreg, %SIZE_B
			xor   rDA8L,temp8
			UpdateNZ_ClearVC

			WriteEA %opsmod, %opsreg, %SIZE_B, 0
			FastFetch 0
		ENDIF

	ENDIF

	ENDM


;;
;;  EORI.W #<data>,<ea>
;;
	opcode = 00A40h
	REPEAT 00040h

	Decode %opcode
	IF FDataAlt(opsmod,opsreg,SIZE_W)

		Valid 39, 3F

		IF (fEmit NE 0)
			Imm16zx
            mov   temp32,eax

			ReadEA %opsmod, %opsreg, %SIZE_W
			xor   rDA16,temp16
			UpdateNZ_ClearVC

			WriteEA %opsmod, %opsreg, %SIZE_W, 0
			FastFetch 0
		ENDIF

	ENDIF

	ENDM


;;
;;  EORI.L #<data>,<ea>
;;
	opcode = 00A80h
	REPEAT 00040h

	Decode %opcode
	IF FDataAlt(opsmod,opsreg,SIZE_L)

		Valid 39, 3F

		IF (fEmit NE 0)
			Imm32
            mov   temp32,eax

			ReadEA %opsmod, %opsreg, %SIZE_L
			xor   rDA,temp32
			UpdateNZ_ClearVC

			WriteEA %opsmod, %opsreg, %SIZE_L, 0
			FastFetch
		ENDIF

	ENDIF

	ENDM


;;
;;  CMPI.B #<data>,<ea>
;;
	opcode = 00C00h

	REPEAT 00040h

	Decode %opcode
	IF FDataAlt(opsmod,opsreg,SIZE_B)

		Valid 39, 3F

		IF (fEmit NE 0)
			Imm8zx
            mov   temp32,eax

			ReadEA %opsmod, %opsreg, %SIZE_B
			sub   rDA8L,temp8
			UpdateNZVC

			FastFetch 0
		ENDIF

	ENDIF

	ENDM


;;
;;  CMPI.W #<data>,<ea>
;;
	opcode = 00C40h
	REPEAT 00040h

	Decode %opcode
	IF FDataAlt(opsmod,opsreg,SIZE_W)

		Valid 39, 3F

		IF (fEmit NE 0)
			Imm16zx
            mov   temp32,eax

			ReadEA %opsmod, %opsreg, %SIZE_W
			sub   rDA16,temp16
			UpdateNZVC

			FastFetch 0
		ENDIF

	ENDIF

	ENDM


;;
;;  CMPI.L #<data>,<ea>
;;
	opcode = 00C80h
	REPEAT 00040h

	Decode %opcode
	IF FDataAlt(opsmod,opsreg,SIZE_L)

		Valid 39, 3F

		IF (fEmit NE 0)
			Imm32
            mov   temp32,eax

			ReadEA %opsmod, %opsreg, %SIZE_L
			sub   rDA,temp32
			UpdateNZVC

			FastFetch
		ENDIF

	ENDIF

	ENDM


;;
;;  CAS.B Dc,Du,<ea>
;;
	opcode = 00AC0h

	REPEAT 00040h

	Decode %opcode
    IF FMem(opsmod,opsreg,SIZE_B) AND FAlt(opsmod,opsreg,SIZE_B)

        Valid 39, 3F

		IF (fEmit NE 0)

            CheckCpu cpu68020

            Imm16zx
            mov   opXword,ax


			ReadEA %opsmod, %opsreg, %SIZE_B

            movzx eax,opXword
            and   eax,7t                 ;; EAX = Dc
            mov   eax,[regD0+eax*4]
			cmp   rDA8L,al
			UpdateNZVC

            .if (al == rDA8L)
                movzx eax,opXword
                shr   eax,6t
                and   eax,7t               ;; EAX = Du
                mov   rDA8L,[regD0+eax*4]
            .else
                movzx eax,opXword
                and   eax,7t               ;; EAX = Dc
                mov   [regD0+eax*4],rDA8L
            .endif

			WriteEA %opsmod, %opsreg, %SIZE_B, 0
			FastFetch 0
		ENDIF

	ENDIF

	ENDM


;;
;;  CAS.W Dc,Du,<ea>
;;
	opcode = 00CC0h

	REPEAT 00040h

	Decode %opcode
    IF FMem(opsmod,opsreg,SIZE_W) AND FAlt(opsmod,opsreg,SIZE_W)

        Valid 39, 3F

		IF (fEmit NE 0)

            CheckCpu cpu68020

            Imm16zx
            mov   opXword,ax

			ReadEA %opsmod, %opsreg, %SIZE_W

            movzx eax,opXword
            and   eax,7t                 ;; EAX = Dc
            mov   eax,[regD0+eax*4]
			cmp   rDA16,ax
			UpdateNZVC

            .if (ax == rDA16)
                movzx eax,opXword
                shr   eax,6t
                and   eax,7t               ;; EAX = Du
                mov   rDA16,[regD0+eax*4]
            .else
                movzx eax,opXword
                and   eax,7t               ;; EAX = Dc
                mov   [regD0+eax*4],rDA16
            .endif

			WriteEA %opsmod, %opsreg, %SIZE_W, 0
			FastFetch 0
		ENDIF

	ENDIF

	ENDM


;;
;;  CAS.L Dc,Du,<ea>
;;
	opcode = 00EC0h

	REPEAT 00040h

	Decode %opcode
    IF FMem(opsmod,opsreg,SIZE_L) AND FAlt(opsmod,opsreg,SIZE_L)

        Valid 39, 3F

		IF (fEmit NE 0)

            CheckCpu cpu68020

            Imm16zx
            mov   opXword,ax

			ReadEA %opsmod, %opsreg, %SIZE_L

            movzx eax,opXword
            and   eax,7t                 ;; EAX = Dc
            mov   eax,[regD0+eax*4]
			cmp   rDA,eax
			UpdateNZVC

            .if (eax == rDA)
                movzx eax,opXword
                shr   eax,6t
                and   eax,7t               ;; EAX = Du
                mov   rDA,[regD0+eax*4]
            .else
                movzx eax,opXword
                and   eax,7t               ;; EAX = Dc
                mov   [regD0+eax*4],rDA
            .endif

			WriteEA %opsmod, %opsreg, %SIZE_L, 0
			FastFetch 0
		ENDIF

	ENDIF

	ENDM


;;
;;  BTST/BCHG/BCLR/BSET #imm,<ea>
;;

	opcode = 00800h
	REPEAT 00100h

	Decode %opcode
	IF FDataAlt(opsmod,opsreg,SIZE_L) OR ((opsize EQ 0) AND FData(opsmod,opsreg,SIZE_L))

		IF (opsize EQ 0) AND FData(opsmod,opsreg,SIZE_L)
			Valid 3B, 3F
		ELSE
			Valid 39, 3F
		ENDIF

		IF (fEmit NE 0)
            Imm8zx
            mov   temp32,eax
			DoBIT %opsmod, %opsreg, %opsize
			FastFetch
		ENDIF

	ENDIF

	ENDM


;;
;;  BTST/BCHG/BCLR/BSET Dn,<ea>
;;

	opcode = 00100h
	REPEAT 00F00h

	Decode %opcode
	IF (opbit8 EQ 1)

		IF FDataAlt(opsmod,opsreg,SIZE_L) OR ((opsize EQ 0) AND FData(opsmod,opsreg,SIZE_L))

			IF (opsize EQ 0) AND FData(opsmod,opsreg,SIZE_L)
				Valid 3C, 3F
			ELSE
				Valid 39, 3F
			ENDIF

			IF (fEmit NE 0)
				SetiDreg %opdreg
				movzx eax,byte ptr iDreg
                mov   temp32,eax
				DoBIT %opsmod, %opsreg, %opsize

				FastFetch
			ENDIF

		ENDIF

	ENDIF

	ENDM


