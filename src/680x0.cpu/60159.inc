;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; 60159.INC, included by 601.ASM
;;
;; Contains PowerPC 601 opcodes that require two passes.
;; This file handles extended opcode 59 (FPU single precision operations)
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


	opcode = 590000h
	REPEAT 2048t

	PpcDecodeExt %opcode

    IF (opext2 EQ 18t)

        ;; FDIVS
        ;; FDIVS.

        Valid

        STARTBLOCK

        PpcFetch

        ENDBLOCK

    ELSE

    ENDIF

	ENDM

