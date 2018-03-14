
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; OPCODES0.ASM
;;
;; 68000 opcodes $0000 thru $0FFF
;;
;; Copyright (C) 1991-2008 by Darek Mihocka. All Rights Reserved.
;; Branch Always Software. http://www.emulators.com/
;;
;; 10/29/2007 darekm        last update
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	INCLUDE 68000.inc

	PUBLIC _opcodes

externdef _gui_hooks:NEAR
externdef _mac_EFS:NEAR

StoreImmToTemp MACRO
    LOCAL   here

;;  int 3
    mov     [ecx+12t],rPC
    mov     [ecx+8],eax
    mov     [ecx], offset here
    mov     edx,[ecx+8]

here:
    mov     temp32,edx
    mov     rPC,[ecx+12t]
ENDM

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
            StoreImmToTemp

			ReadEA %opsmod, %opsreg, %SIZE_B
			or    rDA8L,temp8
			UpdateNZ_ClearVC_Test8

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
            StoreImmToTemp

			ReadEA %opsmod, %opsreg, %SIZE_W
			or    rDA16,temp16
			UpdateNZ_ClearVC_Test16

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
            StoreImmToTemp

			ReadEA %opsmod, %opsreg, %SIZE_L
			or    rDA,temp32
			UpdateNZ_ClearVC_Test32

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
            StoreImmToTemp

			ReadEA %opsmod, %opsreg, %SIZE_B
			and   rDA8L,temp8
			UpdateNZ_ClearVC_Test8

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
            StoreImmToTemp

			ReadEA %opsmod, %opsreg, %SIZE_W
			and   rDA16,temp16
			UpdateNZ_ClearVC_Test16

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
            StoreImmToTemp

			ReadEA %opsmod, %opsreg, %SIZE_L
			and   rDA,temp32
			UpdateNZ_ClearVC_Test32
	
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
            StoreImmToTemp

			ReadEA %opsmod, %opsreg, %SIZE_B
			sub   rDA8L,temp8
			UpdateCCR %SIZE_B
		
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
            StoreImmToTemp

			ReadEA %opsmod, %opsreg, %SIZE_W
			sub   rDA16,temp16
			UpdateCCR %SIZE_W

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
            StoreImmToTemp

			ReadEA %opsmod, %opsreg, %SIZE_L
			sub   rDA,temp32
			UpdateCCR %SIZE_L

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
            StoreImmToTemp

			ReadEA %opsmod, %opsreg, %SIZE_B
			add   rDA8L,temp8
			UpdateCCR %SIZE_B

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
            StoreImmToTemp

			ReadEA %opsmod, %opsreg, %SIZE_W
			add   rDA16,temp16
			UpdateCCR %SIZE_W

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
            StoreImmToTemp

			ReadEA %opsmod, %opsreg, %SIZE_L
			add   rDA,temp32
			UpdateCCR %SIZE_L

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
            StoreImmToTemp

			ReadEA %opsmod, %opsreg, %SIZE_B
			xor   rDA8L,temp8
			UpdateNZ_ClearVC_Test8

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
            StoreImmToTemp

			ReadEA %opsmod, %opsreg, %SIZE_W
			xor   rDA16,temp16
			UpdateNZ_ClearVC_Test16

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
            StoreImmToTemp

			ReadEA %opsmod, %opsreg, %SIZE_L
			xor   rDA,temp32
			UpdateNZ_ClearVC_Test32

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
            StoreImmToTemp

			ReadEA %opsmod, %opsreg, %SIZE_B
			sub   rDA8L,temp8
			UpdateNZVC %SIZE_B

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
            StoreImmToTemp

			ReadEA %opsmod, %opsreg, %SIZE_W
			sub   rDA16,temp16
			UpdateNZVC %SIZE_w

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
            StoreImmToTemp

			ReadEA %opsmod, %opsreg, %SIZE_L
			sub   rDA,temp32
			UpdateNZVC %SIZE_L

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
			sub   rDA8L,al
			UpdateNZVC %SIZE_B
			add   rDA8L,al

            .if (al == rDA8L)
                movzx eax,opXword
                shr   eax,6t
                and   eax,7t               ;; EAX = Du
                mov   rDA8L,byte ptr [regD0+eax*4]
            .else
                movzx eax,opXword
                and   eax,7t               ;; EAX = Dc
                mov   byte ptr [regD0+eax*4],rDA8L
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
			sub   rDA16,ax
			UpdateNZVC %SIZE_W
			add   rDA16,ax

            .if (ax == rDA16)
                movzx eax,opXword
                shr   eax,6t
                and   eax,7t               ;; EAX = Du
                mov   rDA16,word ptr [regD0+eax*4]
            .else
                movzx eax,opXword
                and   eax,7t               ;; EAX = Dc
                mov   word ptr [regD0+eax*4],rDA16
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
			sub   rDA,eax
			UpdateNZVC %SIZE_L
			add   rDA,eax

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



;;
;;  ORI.B #<data>,CCR
;;
	Valid2 3C

	Imm8zx

    .if (al == 080h)

        ; identify Gemulator and version number

        mov   al,byte ptr regD0

        .if (al == 000h)

            ; return emulator version

            mov   word ptr regD0,0921h

        .else

            PrepareForCall
            call  _gui_hooks
            AfterCall 0

        .endif

    .elseif (al == 0C0h)

        ; Macintosh External File System hooks

        PrepareForCall
        call  _mac_EFS
        AfterCall 0

    .else

        PackFlags
        or    regCCR,al
        and   regCCR,1Fh    ;; only the bottom 5 bits are valid
        UnpackFlags

    .endif

	FastFetch 0


;;
;;  ORI.W #<data>,SR
;;

	Valid2 7C

	CheckPriv
	Imm16zx
	GetSR
	or    rDA,eax
	PutSR             ;; won't switch us to User mode


;;
;;  ANDI.B #<data>,CCR
;;
	Valid2 23C

	Imm16zx

    PackFlags
	and   regCCR,al
    UnpackFlags

	FastFetch 0

;;
;;  ANDI.W #<data>,SR
;;

	Valid2 27C

	CheckPriv
	Imm16zx
	GetSR
	and   rDA,eax
	PutSR             ;; may switch us to User mode


;;
;;  EORI.B #<data>,CCR
;;

	Valid2 A3C

	Imm8zx

    PackFlags
    xor   regCCR,al
    and   regCCR,1Fh  ;; only the bottom 5 bits are valid
    UnpackFlags

	FastFetch 0


;;
;;  EORI.W #<data>,SR
;;

	Valid2 A7C

	CheckPriv
	Imm16zx
	GetSR
	xor   rDA,eax
	PutSR             ;; may switch us to User mode


;;
;;  MOVEP.[WL] (An),Dn
;;  MOVEP.[WL] Dn,(An)
;;

	opcode = 00100h
	REPEAT 00F00h

	Decode %opcode
	IF (opbit8 EQ 1)

		IF (opsmod EQ 1)

			Valid

			IF (opsize EQ 0)

				;; MOVEP.W (An),Dn

				ReadEAMode %5, %opsreg, %SIZE_PW
				WriteReg %0, %opdreg, %SIZE_W
				FastFetch 0

			ELSEIF (opsize EQ 1)

				;; MOVEP.L (An),Dn

				ReadEAMode %5, %opsreg, %SIZE_PL
				WriteReg %0, %opdreg, %SIZE_L
				FastFetch

			ELSEIF (opsize EQ 2)

				;; MOVEP.W Dn,(An)

				ReadReg %0, %opdreg, %SIZE_W
				WriteEA %5, %opsreg, %SIZE_PW
				FastFetch 0

			ELSE

				;; MOVEP.L Dn,(An)

				ReadReg %0, %opdreg, %SIZE_L
				WriteEA %5, %opsreg, %SIZE_PL
				FastFetch

			ENDIF

		ENDIF

	ENDIF

	ENDM


	.CONST

    align 4t

_opcodes LABEL DWORD
opcodes LABEL DWORD
	opcode = 00000h
	REPEAT 01000h
		DefOp %opcode
	ENDM

 END

