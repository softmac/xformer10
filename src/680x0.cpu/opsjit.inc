;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; OPSJIT.INC, included by OPSJIT.ASM
;;
;; Contains definitions for jitter micro-ops
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


uopFetch        = 00h
uopFastFetch    = 01h
uopFetchXword   = 02h

uopUpdateNZ     = 10h
uopUpdateNZVC   = 12h
uopUpdateNZX    = 13h
uopUpdateCCR    = 14h

uopNegByte      = 30h
uopNegWord      = 31h
uopNegLong      = 32h
uopAddByte      = 33h
uopAddWord      = 34h
uopAddLong      = 35h
uopSubByte      = 36h
uopSubWord      = 37h
uopSubLong      = 38h
uopAdcByte      = 39h
uopAdcWord      = 3Ah
uopAdcLong      = 3Bh
uopSbbByte      = 3Ch
uopSbbWord      = 3Dh
uopSbbLong      = 3Eh

uopXorByte      = 40h
uopXorWord      = 41h
uopXorLong      = 42h
uopAndByte      = 43h
uopAndWord      = 44h
uopAndLong      = 45h
uopOrByte       = 46h
uopOrWord       = 47h
uopOrLong       = 48h
uopXorByte      = 49h
uopXorWord      = 4Ah
uopXorLong      = 4Bh
uopNotByte      = 4Ch
uopNotWord      = 4Dh
uopNotLong      = 4Eh

uopTestByte     = 50h
uopTestWord     = 51h
uopTestLong     = 52h
uopSignExtendByte = 53h
uopSignExtendWord = 54h
uopSignExtendByteToEax = 55h
uopSignExtendWordToEax = 56h

uopImm32        = 60h
uopImm16sx      = 61h
uopImm16sxNoIncPC = 62h
uopImm16nx      = 63h
uopImm16zx      = 64h
uopImm16gx      = 65h
uopImm8sx       = 66h
uopImm8nx       = 67h
uopImm8zx       = 68h
uopImm8gx       = 69h

uopPC2Flat      = 70h
uopFlat2PC      = 71h
uopFlat2PCRel   = 72h

uopGenEA        = 80h
uopReadEA       = 81h
uopWriteEA      = 82h
uopWriteEASame  = 83h



