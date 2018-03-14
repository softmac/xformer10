;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; 601.INC, included by 601.ASM
;;
;; Contains PowerPC 601 opcodes that require two passes.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


	opcode = 010000h
	REPEAT 010000h/020h

	PpcDecode %opcode

    IF (opinst EQ 0)

        ;; dummy

    ELSEIF (opinst EQ 3t)

        ;; TWI

    ELSEIF (opinst EQ 7t)

        ;; MULLI

        Valid

        STARTBLOCK

        PpcFetch

        ENDBLOCK

    ELSEIF (opinst EQ 19t)

        ;; extended opcode 19

        Valid

        STARTBLOCK

        mov   ebp,eax
        shr   eax,11t
        and   ebp,2047t
        jmp   dword ptr [ppc19+ebp*4]

        ENDBLOCK

    ELSEIF (opinst EQ 31t)

        ;; extended opcode 31

        Valid

        STARTBLOCK

        mov   ebp,eax
        shr   eax,11t
        and   ebp,2047t
        jmp   dword ptr [ppc31+ebp*4]

        ENDBLOCK

    ELSEIF (opinst EQ 32t)

        ;; LWZ

        Valid
        STARTBLOCK
        PpcLdSt %LS_LD, %LS_NOU, %LS_NOM, %LS_WZ, %opregD, %opregA, %LS_IMM
        PpcFetch
        ENDBLOCK

    ELSEIF (opinst EQ 33t)

        ;; LWZU

        Valid
        STARTBLOCK
        PpcLdSt %LS_LD, %LS_UPD, %LS_NOM, %LS_WZ, %opregD, %opregA, %LS_IMM
        PpcFetch
        ENDBLOCK

    ELSEIF (opinst EQ 34t)

        ;; LBZ

        Valid
        STARTBLOCK
        PpcLdSt %LS_LD, %LS_NOU, %LS_NOM, %LS_BZ, %opregD, %opregA, %LS_IMM
        PpcFetch
        ENDBLOCK

    ELSEIF (opinst EQ 35t)

        ;; LBZU

        Valid
        STARTBLOCK
        PpcLdSt %LS_LD, %LS_UPD, %LS_NOM, %LS_BZ, %opregD, %opregA, %LS_IMM
        PpcFetch
        ENDBLOCK

    ELSEIF (opinst EQ 36t)

        ;; STW

        Valid
        STARTBLOCK
        PpcLdSt %LS_ST, %LS_NOU, %LS_NOM, %LS_WZ, %opregD, %opregA, %LS_IMM
        PpcFetch
        ENDBLOCK

    ELSEIF (opinst EQ 37t)

        ;; STWU

        Valid
        STARTBLOCK
        PpcLdSt %LS_ST, %LS_UPD, %LS_NOM, %LS_WZ, %opregD, %opregA, %LS_IMM
        PpcFetch
        ENDBLOCK

    ELSEIF (opinst EQ 38t)

        ;; STB

        Valid
        STARTBLOCK
        PpcLdSt %LS_ST, %LS_NOU, %LS_NOM, %LS_BZ, %opregD, %opregA, %LS_IMM
        PpcFetch
        ENDBLOCK

    ELSEIF (opinst EQ 39t)

        ;; STBU

        Valid
        STARTBLOCK
        PpcLdSt %LS_ST, %LS_UPD, %LS_NOM, %LS_BZ, %opregD, %opregA, %LS_IMM
        PpcFetch
        ENDBLOCK

    ELSEIF (opinst EQ 40t)

        ;; LHZ

        Valid
        STARTBLOCK
        PpcLdSt %LS_LD, %LS_NOU, %LS_NOM, %LS_HZ, %opregD, %opregA, %LS_IMM
        PpcFetch
        ENDBLOCK

    ELSEIF (opinst EQ 41t)

        ;; LHZU

        Valid
        STARTBLOCK
        PpcLdSt %LS_LD, %LS_UPD, %LS_NOM, %LS_HZ, %opregD, %opregA, %LS_IMM
        PpcFetch
        ENDBLOCK

    ELSEIF (opinst EQ 42t)

        ;; LHA

        Valid
        STARTBLOCK
        PpcLdSt %LS_LD, %LS_NOU, %LS_NOM, %LS_HA, %opregD, %opregA, %LS_IMM
        PpcFetch
        ENDBLOCK

    ELSEIF (opinst EQ 43t)

        ;; LHAU

        Valid
        STARTBLOCK
        PpcLdSt %LS_LD, %LS_UPD, %LS_NOM, %LS_HA, %opregD, %opregA, %LS_IMM
        PpcFetch
        ENDBLOCK

    ELSEIF (opinst EQ 44t)

        ;; STH

        Valid
        STARTBLOCK
        PpcLdSt %LS_ST, %LS_NOU, %LS_NOM, %LS_HZ, %opregD, %opregA, %LS_IMM
        PpcFetch
        ENDBLOCK

    ELSEIF (opinst EQ 45t)

        ;; STHU

        Valid
        STARTBLOCK
        PpcLdSt %LS_ST, %LS_UPD, %LS_NOM, %LS_HZ, %opregD, %opregA, %LS_IMM
        PpcFetch
        ENDBLOCK

    ELSEIF (opinst EQ 46t)

        ;; LMW

        Valid
        STARTBLOCK
        PpcLdSt %LS_LD, %LS_NOU, %LS_MUL, %LS_WZ, %opregD, %opregA, %LS_IMM
        PpcFetch
        ENDBLOCK

    ELSEIF (opinst EQ 47t)

        ;; STMW

        Valid
        STARTBLOCK
        PpcLdSt %LS_ST, %LS_NOU, %LS_MUL, %LS_WZ, %opregD, %opregA, %LS_IMM
        PpcFetch
        ENDBLOCK

    ELSEIF (opinst EQ 48t)

        ;; LFS

        Valid
        STARTBLOCK
        PpcLdSt %LS_LD, %LS_NOU, %LS_NOM, %LS_SF, %opregD, %opregA, %LS_IMM
        PpcFetch
        ENDBLOCK

    ELSEIF (opinst EQ 49t)

        ;; LFSU

        Valid
        STARTBLOCK
        PpcLdSt %LS_LD, %LS_UPD, %LS_NOM, %LS_SF, %opregD, %opregA, %LS_IMM
        PpcFetch
        ENDBLOCK

    ELSEIF (opinst EQ 50t)

        ;; LFD

        Valid
        STARTBLOCK
        PpcLdSt %LS_LD, %LS_NOU, %LS_NOM, %LS_DF, %opregD, %opregA, %LS_IMM
        PpcFetch
        ENDBLOCK

    ELSEIF (opinst EQ 51t)

        ;; LFDU

        Valid
        STARTBLOCK
        PpcLdSt %LS_LD, %LS_UPD, %LS_NOM, %LS_DF, %opregD, %opregA, %LS_IMM
        PpcFetch
        ENDBLOCK

    ELSEIF (opinst EQ 52t)

        ;; STFS

        Valid
        STARTBLOCK
        PpcLdSt %LS_ST, %LS_NOU, %LS_NOM, %LS_SF, %opregD, %opregA, %LS_IMM
        PpcFetch
        ENDBLOCK

    ELSEIF (opinst EQ 53t)

        ;; STFSU

        Valid
        STARTBLOCK
        PpcLdSt %LS_ST, %LS_UPD, %LS_NOM, %LS_SF, %opregD, %opregA, %LS_IMM
        PpcFetch
        ENDBLOCK

    ELSEIF (opinst EQ 54t)

        ;; STFD

        Valid
        STARTBLOCK
        PpcLdSt %LS_ST, %LS_NOU, %LS_NOM, %LS_DF, %opregD, %opregA, %LS_IMM
        PpcFetch
        ENDBLOCK

    ELSEIF (opinst EQ 55t)

        ;; STFDU

        Valid
        STARTBLOCK
        PpcLdSt %LS_ST, %LS_UPD, %LS_NOM, %LS_DF, %opregD, %opregA, %LS_IMM
        PpcFetch
        ENDBLOCK

    ELSEIF (opinst EQ 59t)

        ;; extended opcode 59

        Valid

        STARTBLOCK

        mov   ebp,eax
        shr   eax,11t
        and   ebp,2047t
        jmp   dword ptr [ppc59+ebp*4]

        ENDBLOCK

    ELSEIF (opinst EQ 63t)

        ;; extended opcode 63

        Valid

        STARTBLOCK

        mov   ebp,eax
        shr   eax,11t
        and   ebp,2047t
        jmp   dword ptr [ppc63+ebp*4]

        ENDBLOCK

    ELSE

    ENDIF

    opcode = opcode + 01Fh

	ENDM

