;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; OPSF.INC, included by OPSF.ASM
;;
;; Contains FPU instructions
;;
;; Note: Due to the use of GenEA, this file must be in pass 0 only!
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


;;
;;  68030 MMU traps
;;
;;  All 68030 MMU instructions are coded for id 0 of type 0, and all
;;  use the 6-bit effective address field and 16-bit extension word.
;;

	opcode = 0F000h
	REPEAT 00040h

	Decode %opcode
	IF FControl(opsmod,opsreg,SIZE_L) AND FAlt(opdmod,opdreg,SIZE_L)

        Valid

        cmp cpuCur,cpu68030
        jne LINEF_ok

        call CheckPrivAndSetFPIAR

        Imm16zx

        GenEA %opsmod, %opsreg, %SIZE_L

        mov   opXword,ax
        mov   _lAccessAddr,rGA
        jmp   LINEF2

	ENDIF

	ENDM


;;
;;  FPU coprocessor traps
;;
;;  Most FPU instructions have an effective address encoded in the
;;  bottom 6 bits. This code decodes that address, stores it in
;;  the global _lAccessAddr, then calls the LINE F exception as usual.
;;
;;  For instructions that don't require decoding an effective address
;;  we do nothing. Those are handled in the exception handler.
;;

    ;; FPU TYPE 0 - arithmetic operations, allow all addressing modes
    ;; Takes one 16-bit extension word

	opcode = 0F200h
	REPEAT 00040h

	Decode %opcode
	IF FAny(opsmod,opsreg,SIZE_L)

        Valid

        Imm16zx
        mov   opXword,ax

        ;; check the R/M bit regardless of what's in the address mode field

        test  eax,0E000h
        jz    FPUreg

        ;; Definitely a memory-to-register operation

        Flat2PC
        sub   rGA,2
        mov   regFPIAR,rGA

        IF (opsmod EQ 0)
            SetiDreg %opsreg
            lea   rGA,[iDreg]
        ELSEIF (opsmod EQ 1)
            SetiAreg %opsreg
            lea   rGA,[iAreg]
        ELSEIF (opsmod EQ 7) AND (opsreg EQ 4)
            lea   rGA,[rPC-2]
        ELSE
            GenEA %opsmod, %opsreg, %SIZE_L
        ENDIF

        jmp   FPUaddr

	ENDIF

	ENDM


    ;; FPU TYPE 1 - conditionals
    ;; Takes one 16-bit extension word

	opcode = 0F240h
	REPEAT 00040h

	Decode %opcode
	IF (opsmod EQ 7) AND (opsreg GE 2)

        ;; FTRAPcc

        Valid

        jmp   FPU

	ELSEIF (opsmod EQ 1)

        ;; FDBcc

        Valid 7, 7

        IF (fEmit NE 0)

        jmp   FPU

        ENDIF

	ELSE
     IF FAny(opsmod,opsreg,SIZE_B)

        ;; FScc

        Valid

        Flat2PC
        sub   rGA,2
        mov   regFPIAR,rGA

        Imm16zx
        mov   opXword,ax

        IF (opsmod EQ 0)
            SetiDreg %opsreg
            lea   rGA,[iDreg]
        ELSE
            GenEA %opsmod, %opsreg, %SIZE_B
        ENDIF

        jmp   FPUaddr

	ENDIF
	ENDIF

	ENDM


    ;; FPU TYPES 2 and 3 - branches, no address to decode

	opcode = 0F280h
	REPEAT 00080h

	Decode %opcode

        Valid 3F, 3F, 1

        IF (fEmit NE 0)

        IF (opsize EQ 3)
            Imm32               ; fetch 32-bit offset
            lea   rGA,[eax-4]
        ELSE
            Imm16sx             ; fetch 16-bit offset
            lea   rGA,[eax-2]
        ENDIF

        and   rDA,63t       ; extract conditinal predicate

        PrepareForCall
        push  rDA
        call  _Fcc
        AfterCall 1
        je    @F

        sub   rPC,rGA
@@:
        Fetch

        ENDIF

	ENDM


    ;; FPU TYPES 4 and 5 - FSAVE and FRESTORE

	opcode = 0F300h
	REPEAT 00080h

	Decode %opcode
	IF FAny(opsmod,opsreg,SIZE_L)

		Valid

        call CheckPrivAndSetFPIAR

        IF (opsmod EQ 0)
            SetiDreg %opsreg
            lea   rGA,[iDreg]
        ELSEIF (opsmod EQ 1)
            SetiAreg %opsreg
            lea   rGA,[iAreg]
        ELSEIF (opsmod EQ 7) AND (opsreg EQ 4)
            lea   rGA,[rPC-2]
        ELSE
            GenEA %opsmod, %opsreg, %SIZE_L
        ENDIF

        jmp   FPUaddr

	ENDIF

	ENDM


    ;; 68040 MOVE16

	opcode = 0F600h
	REPEAT 00028h

	Decode %opcode

    Valid 7, 7

    IF (fEmit NE 0)

    mov   eax,[rPC-2]       ; eax = xword or xlong, EBX already has opcode

    IF (opsmod EQ 0)

        ;; MOVE16 (Ay)+,xxx.L

        jmp   MOVE16

    ELSEIF (opsmod EQ 1)

        ;; MOVE16 xxx.L,(Ay)+

        jmp   MOVE16

    ELSEIF (opsmod EQ 2)

        ;; MOVE16 (Ay),xxx.L

        jmp   MOVE16

    ELSEIF (opsmod EQ 3)

        ;; MOVE16 xxx.L,(Ay)

        jmp   MOVE16

    ELSEIF (opsmod EQ 4)

        ;; MOVE16 (Ax)+,(Ay)+

        shr   eax,26t               ; mask for Ay (high bit set so go off D0)
        and   rDA,7                 ; mask for Ax
        sub   rPC,2                 ; fetched 16-bit extension word
        and   eax,60t               ; mask for Ay

        lea   rDA,[regA0+rDA*4]     ; Ax
        lea   eax,[regD0+eax]       ; Ay

        mov   rGA,[rDA]             ; source
        add   dword ptr [rDA],16t

        mov   rDA,[eax]             ; destination
        add   dword ptr [eax],16t

        and   rGA,0FFFFFFF0h
        call  cpuTranslateRead
        fild  qword ptr [eax-7] 
        fild  qword ptr [eax-15t] 

        mov   rGA,rDA
        and   rGA,0FFFFFFF0h
        call  cpuTranslateWrite
        fistp qword ptr [eax-15t] 
        fistp qword ptr [eax-7] 

        FastFetch

    ENDIF

    ENDIF ; fEmit

	ENDM


