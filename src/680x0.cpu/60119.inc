;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; 60119.INC, included by 601.ASM
;;
;; Contains PowerPC 601 opcodes that require two passes.
;; This file handles extended opcode 19 (CR operations)
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


	opcode = 190000h
	REPEAT 2048t

	PpcDecodeExt %opcode

    IF (opext  EQ 0)

        ;; MCRF

        Valid

        STARTBLOCK

        PpcFetch

        ENDBLOCK

    ELSE

    ENDIF

	ENDM

