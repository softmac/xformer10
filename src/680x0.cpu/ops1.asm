
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; OPCODES1.ASM
;;
;; 68000 opcodes $1000 thru $1FFF
;;
;; Copyright (C) 1991-2008 by Darek Mihocka. All Rights Reserved.
;; Branch Always Software. http://www.emulators.com/
;;
;; Instructions in this range:
;;
;; MOVE.B - bits 11..9 - opdreg
;;        - bits 8..6  - opdmod
;;        - bits 5..3  - opsmod
;;        - bits 2..0  - opsreg
;;
;; 07/26/2008 darekm        last update
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	INCLUDE 68000.inc

    BigAlign

MovB_Rn_Dn::
    mov     eax,[ecx+12t]
    movsx   rDA,byte ptr [regD0+edx]

    UpdateNZ_ClearVC_Test32
    mov     byte ptr [regD0+eax],rDA8L

    FastFetch


MovB_Rn_D0::
    movsx   rDA,byte ptr [regD0+edx]

    UpdateNZ_ClearVC_Test32
    mov     byte ptr [regD0],rDA8L

    FastFetch


	opcode = 01000h
	REPEAT 01000h

	Decode %opcode
	IF FAny(opsmod,opsreg,SIZE_B) AND FAlt(opdmod,opdreg,SIZE_B)

		Valid 3C, 3F

		IF (fEmit NE 0)

		  IF 1 AND (opdmod EQ 0) AND (opsmod LT 2)    ; moving Rn into Dn

        ;;  int 3
            IF (opdreg EQ 0)
                mov     dword ptr [ecx], offset MovB_Rn_D0
            ELSE
                mov     dword ptr [ecx], offset MovB_Rn_Dn
            ENDIF

            mov     eax,((opthis AND 15t) * 4)
            mov     [ecx+8t],eax

            mov     eax,(opdreg * 4)
            mov     [ecx+12t],eax

            mov     edx,dword ptr [ecx+8t]

            IF (opdreg EQ 0)
                jmp     MovB_Rn_D0
            ELSE
                jmp     MovB_Rn_Dn
            ENDIF

          ELSE

			ReadEA %opsmod, %opsreg, %SIZE_B

            UpdateNZ_ClearVC_Test8
			WriteEA %opdmod, %opdreg, %SIZE_B

			FastFetch 0

          ENDIF

		ENDIF

	ENDIF

	ENDM


	.CONST

	PUBLIC _ops1

_ops1   LABEL DWORD
opc1000 LABEL DWORD

	opcode = 01000h
	REPEAT 01000h
		DefOp %opcode
	ENDM

 END

