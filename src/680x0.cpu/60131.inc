;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; 60131.INC, included by 601.ASM
;;
;; Contains PowerPC 601 opcodes that require two passes.
;; This file handles extended opcode 31 (integer operations)
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


	opcode = 310000h
	REPEAT 2048t

	PpcDecodeExt %opcode

    IF (opext  EQ 0)

        ;; CMP

        Valid

        STARTBLOCK

        PpcFetch

        ENDBLOCK

    ELSE

    ENDIF

	ENDM

