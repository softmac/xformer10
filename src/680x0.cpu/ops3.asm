
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; OPCODES3.ASM
;;
;; 68000 opcodes $3000 thru $3FFF
;;
;; Copyright (C) 1991-2008 by Darek Mihocka. All Rights Reserved.
;; Branch Always Software. http://www.emulators.com/
;;
;; Instructions in this range:
;;
;; MOVE.W - bits 11..9 - opdreg
;;        - bits 8..6  - opdmod
;;        - bits 5..3  - opsmod
;;        - bits 2..0  - opsreg
;;
;; 07/26/2008 darekm        last update
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	INCLUDE 68000.inc

    BigAlign

MovW_Rn_Dn::
    mov     eax,[ecx+12t]
    movsx   rDA,word ptr [regD0+edx]

    UpdateNZ_ClearVC_Test32
    mov     word ptr [regD0+eax],rDA16

    FastFetch


MovW_Rn_D0::
    movsx   rDA,word ptr [regD0+edx]

    UpdateNZ_ClearVC_Test32
    mov     word ptr [regD0],rDA16

    FastFetch


    BigAlign

MovW_Rn_An_NotA7::
    mov     eax,[ecx+12t]
    movsx   rDA,word ptr [regD0+edx]
    mov     [regA0+eax],rDA

    FastFetch


    BigAlign

MovW_Rn_A7::
    movsx   rDA,word ptr [regD0+edx]
    mov     [regA7],rDA

    FastFetch


	opcode = 03000h
	REPEAT 01000h

	Decode %opcode
	IF FAny(opsmod,opsreg,SIZE_W) AND FAlt(opdmod,opdreg,SIZE_W)

		Valid 3C, 3F, 1

		IF (fEmit NE 0)

		  IF 1 AND (opdmod EQ 0) AND (opsmod LT 2)    ; moving Rn into Dn

        ;;  int 3
            IF (opdreg EQ 0)
                mov     dword ptr [ecx], offset MovW_Rn_D0
            ELSE
                mov     dword ptr [ecx], offset MovW_Rn_Dn
            ENDIF

            mov     edx,((opthis AND 15t) * 4)
            mov     [ecx+8t],edx

            mov     eax,(opdreg * 4)
            mov     [ecx+12t],eax

            IF (opdreg EQ 0)
                jmp     MovW_Rn_D0
            ELSE
                jmp     MovW_Rn_Dn
            ENDIF

		  ELSEIF 1 AND (opdmod EQ 1) AND (opsmod LT 2)    ; moving Rn into An

        ;;  int 3
            IF (opdreg EQ 7)
                mov     dword ptr [ecx], offset MovW_Rn_A7
            ELSE
                mov     dword ptr [ecx], offset MovW_Rn_An_NotA7
            ENDIF

            mov     edx,((opthis AND 15t) * 4)
            mov     [ecx+8t],edx

            mov     eax,(opdreg * 4)
            mov     [ecx+12t],eax

            IF (opdreg EQ 7)
                jmp     MovW_Rn_A7
            ELSE
                jmp     MovW_Rn_An_NotA7
            ENDIF

          ELSE

			ReadEA %opsmod, %opsreg, %SIZE_W

			IF (opdmod NE 1)
				UpdateNZ_ClearVC_Test16
				WriteEA %opdmod, %opdreg, %SIZE_W
			ELSE
                SignExtendWord
				WriteEA %opdmod, %opdreg, %SIZE_L
			ENDIF

			FastFetch 0

          ENDIF

		ENDIF

	ENDIF

	ENDM


	.CONST

	PUBLIC _ops3

_ops3   LABEL DWORD
opc3000 LABEL DWORD

	opcode = 03000h
	REPEAT 01000h
		DefOp %opcode
	ENDM

 END

