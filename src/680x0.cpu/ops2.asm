
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; OPCODES2.ASM
;;
;; 68000 opcodes $2000 thru $2FFF
;;
;; Copyright (C) 1991-2008 by Darek Mihocka. All Rights Reserved.
;; Branch Always Software. http://www.emulators.com/
;;
;; Instructions in this range:
;;
;; MOVE.L - bits 11..9 - opdreg
;;        - bits 8..6  - opdmod
;;        - bits 5..3  - opsmod
;;        - bits 2..0  - opsreg
;;
;; 07/26/2008 darekm        last update
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	INCLUDE 68000.inc

    BigAlign

MovL_Rn_Dn::
    mov     eax,[ecx+12t]
    mov     rDA,[regD0+edx]

    UpdateNZ_ClearVC_Test32
    mov     [regD0+eax],rDA

    FastFetch


MovL_Rn_D0::
    mov     rDA,[regD0+edx]

    UpdateNZ_ClearVC_Test32
    mov     [regD0],rDA

    FastFetch


    BigAlign

MovL_Rn_An_NotA7::
    mov     eax,[ecx+12t]
    mov     rDA,[regD0+edx]
    mov     [regA0+eax],rDA

    FastFetch


    BigAlign

MovL_Rn_A7::
    mov     rDA,[regD0+edx]
    and     rDA,0FFFFFFFEh
    mov     [regA7],rDA

    FastFetch


	opcode = 02000h
	REPEAT 01000h

	Decode %opcode
	IF FAny(opsmod,opsreg,SIZE_L) AND FAlt(opdmod,opdreg,SIZE_L)

		Valid 3C, 3F, 1

		IF (fEmit NE 0)

		  IF 1 AND (opdmod EQ 0) AND (opsmod LT 2)    ; moving Rn into Dn

        ;;  int 3
            IF (opdreg EQ 0)
                mov     dword ptr [ecx], offset MovL_Rn_D0
            ELSE
                mov     dword ptr [ecx], offset MovL_Rn_Dn
            ENDIF

            mov     edx,((opthis AND 15t) * 4)
            mov     [ecx+8t],edx

            mov     eax,(opdreg * 4)
            mov     [ecx+12t],eax

            IF (opdreg EQ 0)
                jmp     MovL_Rn_D0
            ELSE
                jmp     MovL_Rn_Dn
            ENDIF

		  ELSEIF 1 AND (opdmod EQ 1) AND (opsmod LT 2)    ; moving Rn into An

        ;;  int 3
            IF (opdreg EQ 7)
                mov     dword ptr [ecx], offset MovL_Rn_A7
            ELSE
                mov     dword ptr [ecx], offset MovL_Rn_An_NotA7
            ENDIF

            mov     edx,((opthis AND 15t) * 4)
            mov     [ecx+8t],edx

            mov     eax,(opdreg * 4)
            mov     [ecx+12t],eax

            IF (opdreg EQ 7)
                jmp     MovL_Rn_A7
            ELSE
                jmp     MovL_Rn_An_NotA7
            ENDIF

          ELSE

			ReadEA %opsmod, %opsreg, %SIZE_L

			IF (opdmod NE 1)
                UpdateNZ_ClearVC_Test32
            ELSEIF (opdreg EQ 7)
                and rDA,0FFFFFFFEh
			ENDIF

			WriteEA %opdmod, %opdreg, %SIZE_L

			FastFetch

          ENDIF

		ENDIF

	ENDIF

	ENDM


	.CONST

	PUBLIC _ops2

_ops2   LABEL DWORD
opc2000 LABEL DWORD

	opcode = 02000h
	REPEAT 01000h
		DefOp %opcode
	ENDM

 END

