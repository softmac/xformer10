;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; 60163.INC, included by 601.ASM
;;
;; Contains PowerPC 601 opcodes that require two passes.
;; This file handles extended opcode 63 (FPU operations)
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


	opcode = 630000h
	REPEAT 2048t

	PpcDecodeExt %opcode

    IF (opext2 EQ 0)

        ;; FCMPU

        Valid

        STARTBLOCK

        PpcFetch

        ENDBLOCK

    ELSE

    ENDIF

	ENDM

