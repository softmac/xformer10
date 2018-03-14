
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; PPC.INC, included by 68000.INC
;;
;; PowerPC emulator include file
;;
;; by Darek Mihocka, (C) 1999 Emulators Inc.
;;
;; 11/06/99 last update
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; PPC registers

regGPR0  TEXTEQU <![edx+80!]>
regGPR1  TEXTEQU <![edx+84!]>
regGPR2  TEXTEQU <![edx+88!]>
regGPR3  TEXTEQU <![edx+8C!]>
regGPR4  TEXTEQU <![edx+90!]>
regGPR5  TEXTEQU <![edx+94!]>
regGPR6  TEXTEQU <![edx+98!]>
regGPR7  TEXTEQU <![edx+9C!]>
regGPR8  TEXTEQU <![edx+A0!]>
regGPR9  TEXTEQU <![edx+A4!]>
regGPRA  TEXTEQU <![edx+A8!]>
regGPRB  TEXTEQU <![edx+AC!]>
regGPRC  TEXTEQU <![edx+B0!]>
regGPRD  TEXTEQU <![edx+B4!]>
regGPRE  TEXTEQU <![edx+B8!]>
regGPRF  TEXTEQU <![edx+BC!]>
regGPR10 TEXTEQU <![edx+C0!]>
regGPR11 TEXTEQU <![edx+C4!]>
regGPR12 TEXTEQU <![edx+C8!]>
regGPR13 TEXTEQU <![edx+CC!]>
regGPR14 TEXTEQU <![edx+D0!]>
regGPR15 TEXTEQU <![edx+D4!]>
regGPR16 TEXTEQU <![edx+D8!]>
regGPR17 TEXTEQU <![edx+DC!]>
regGPR18 TEXTEQU <![edx+E0!]>
regGPR19 TEXTEQU <![edx+E4!]>
regGPR1A TEXTEQU <![edx+E8!]>
regGPR1B TEXTEQU <![edx+EC!]>
regGPR1C TEXTEQU <![edx+F0!]>
regGPR1D TEXTEQU <![edx+F4!]>
regGPR1E TEXTEQU <![edx+F8!]>
regGPR1F TEXTEQU <![edx+FC!]>
regGPRFP TEXTEQU <![edx+FC!]>

regFPR0 TEXTEQU <![edx+100!]>
regFPR1 TEXTEQU <![edx+108!]>
regFPR2 TEXTEQU <![edx+110!]>
regFPR3 TEXTEQU <![edx+118!]>

regMQ   TEXTEQU <![edx+200!]>
regXER  TEXTEQU <![edx+204!]>
;
;
regGPRTCU TEXTEQU <![edx+210!]>
regGPRTCL TEXTEQU <![edx+214!]>
;
;
regLR   TEXTEQU <![edx+220!]>
regCTR  TEXTEQU <![edx+224!]>
;
;
regCR   TEXTEQU <![edx+230!]>
regFPCR TEXTEQU <![edx+234!]>


;; Give the PpcLdSt enumerations unique values so we don't mix them up

LS_LD       = 012h
LS_ST       = 013h
LS_NOU      = 024h
LS_UPD      = 025h
LS_NOM      = 036h
LS_MUL      = 037h
LS_BZ       = 048h
LS_HZ       = 049h
LS_HA       = 04Ah
LS_WZ       = 04Bh
LS_SF       = 04Ch
LS_DF       = 04Dh
LS_IMM      = 050h
LS_IND      = 051h

;
; PpcLdSt kLdSt, kUpd, kMul, kSiz, rD, rA, kOfs
;
; Generic handler for all PPC load and store instructions
;
; Specify whether a load or store, whether to do an update,
; whether to do multiple registers, the load/store register rD,
; and effective address register rA, and whether the offset
; is immediate (in EAX) or indexed (rB encoded in EAX)

PpcLdSt MACRO kLdSt:REQ, kUpd:REQ, kMul:REQ, kSiz:REQ, rD:REQ, rA:REQ, kOfs:REQ

    local notR0, mul_loop

	IF ((kLdSt NE LS_LD) AND (kLdSt NE LS_ST))
		.ERR
		ECHO PpcLdSt: Invalid load/store flag
	ENDIF

	IF ((kUpd NE LS_NOU) AND (kUpd NE LS_UPD))
		.ERR
		ECHO PpcLdSt: Invalid update flag
	ENDIF

	IF ((kMul NE LS_NOM) AND (kMul NE LS_MUL))
		.ERR
		ECHO PpcLdSt: Invalid multiple flag
	ENDIF

	IF ((kSiz NE LS_BZ) AND (kSiz NE LS_HZ) AND (kSiz NE LS_HA))
	IF ((kSiz NE LS_WZ) AND (kSiz NE LS_SF) AND (kSiz NE LS_DF))
		.ERR
		ECHO PpcLdSt: Invalid size
	ENDIF
	ENDIF

	IF ((kOfs NE LS_IMM) AND (kOfs NE LS_IND))
		.ERR
		ECHO PpcLdSt: Invalid offset flag
	ENDIF

    ; at this point EBX contains the upper 16 bits of the instruction
    ; containing the opcode, rD, and rA. EAX contains the 16-bit
    ; immediate offset or rB encoded in bits 16..20
    ;
    ; we want to end up so that EAX ends up with rD, EBP contains the EA,
    ; and EBX is used for the load/store value

    ; REVIEW: right now don't do any inlining optimizations with rD and rA

	IF (kOfs EQ LS_IND)
        shr   eax,11t           ; rB
        mov   eax,[regGPR0+eax*4] ; (rB)
    ENDIF

    mov   ebp,ebx
    shr   ebx,5                 ; opcode | rD
    and   ebp,31t               ; rA
    jz    short notR0

    add   eax,[regGPR0+ebp*4]   ; add (rA)
notR0:
    and   ebx,31t               ; rD

	IF (kUpd EQ LS_UPD)
        mov   [regGPR0+ebp*4],eax
    ENDIF

    mov   ebp,MEMSIZE
    sub   ebp,eax

    mov   eax,ebx               ; rD

mul_loop:
	IF (kUpd EQ LS_ST)

        ;; store register to memory

        IF ((kSiz GE LS_BZ) AND (kSiz LE LS_WZ))
            mov   ebx,[regGPR0+eax*4] ; (rD)

            IF (kSiz EQ LS_BZ)
                mov   byte ptr [edx+ebp],bl
            ELSEIF (kSiz EQ LS_WZ)
                mov   dword ptr [edx+ebp-3],ebx
            ELSE
                mov   word ptr [edx+ebp-1],bx
            ENDIF
        ELSE
            fld   qword ptr [regFPR0+eax*8] ; (fprD)

            IF (kSiz EQ LS_SF)
                fstp  dword ptr [edx+ebp-3]
            ELSE
                fstp  qword ptr [edx+ebp-7]
            ENDIF
        ENDIF

    ELSE
        ;; load register from memory

        IF ((kSiz GE LS_BZ) AND (kSiz LE LS_WZ))
            IF (kSiz EQ LS_BZ)
                movzx ebx,byte ptr [edx+ebp]
            ELSEIF (kSiz EQ LS_WZ)
                mov   ebx,dword ptr [edx+ebp-3]
            ELSEIF (kSiz EQ LS_HZ)
                movzx ebx,word ptr [edx+ebp-1]
            ELSE
                movsx ebx,word ptr [edx+ebp-1]
            ENDIF

            mov   [regGPR0+eax*4],ebx ; (rD)
        ELSE
            IF (kSiz EQ LS_SF)
                fld   dword ptr [edx+ebp-3]
            ELSE
                fld   qword ptr [edx+ebp-7]
            ENDIF

            fstp  qword ptr [regFPR0+eax*8] ; (fprD)
        ENDIF
    ENDIF

	IF (kMul EQ LS_MUL)
        inc   eax
        cmp   eax,20h
        lea   ebp,[ebp-4]
        jne   short mul_loop
    ENDIF

ENDM


IF 0

EA_RnImm    = 0
EA_RnRi     = 1
EA_RnImmUpd = 2
EA_RnRiUpd  = 3
EA_RnMax    = 4

;
; PpcLoadEA mode, reg, offset, size
;
; Generates effective address in EBP and reads contents into EBX.
;

PpcLoadEA MACRO mode, reg, offset, size

	LOCAL x, x3

ireg  TEXTEQU <regGPR&reg>
iregi TEXTEQU <regGPR&offset>

	IF (mode EQ EA_RnImm)
		mov   ebp,MEMSIZE
		sub   ebp,ireg
		sub   ebp,0&offset
    ELSEIF (mode EQ EA_RnImmUpd)
		mov   ebp,MEMSIZE
		sub   ebp,ireg
		sub   ebp,0&offset
		add   dword ptr ireg,0&offset
    ELSEIF (mode EQ EA_RnRi)
		mov   ebp,MEMSIZE
		sub   ebp,ireg
		sub   ebp,iregi
    ELSEIF (mode EQ EA_RnRiUpd)
		mov   ebp,MEMSIZE
		sub   ebp,ireg
		sub   ebp,iregi
    ELSE
		.ERR
		ECHO PpcLoadEA: Invalid addressing mode
	ENDIF

    IF (size EQ SIZE_B OR BOUNDCHECK)
        jnc   short @F
%		    call  ReadHW&size
        jmp   short x3
@@:
    ENDIF
    ReadMem size

x3:
x:

ENDM


;
; PpcStoreEA mode, reg, offset, size
;
; Generates effective address and writes EBX into contents.
; Special cases for Dn and An addressing modes.
; If fGenEA == 0 then EBP already has the effective address.
;

PpcStoreEA MACRO mode, reg, offset, size

	LOCAL x, x3

ireg  TEXTEQU <regGPR&reg>
iregi TEXTEQU <regGPR&offset>

	IF (mode GE EA_RnMax)
		.ERR
		ECHO PpcStoreEA: Invalid addressing mode
	ENDIF

x3:
x:

ENDM

ENDIF ; 0


;
; PpcDecode op
;
; Decodes the fields in the instruction word
;

PpcDecode MACRO op:REQ
	opthis = opcode
	opcode = opcode + 1
	opinst = (0&op/0400h) AND 03Fh
	opcrfD = (0&op/0080h) AND 007h
	opregD = (0&op/0020h) AND 01Fh
	opregA =  0&op        AND 01Fh
	opimm  =  0&op        AND 0FFFFh
	fWriteFetched = 0
	fReaded = 0
ENDM

PpcDecodeExt MACRO op:REQ
	opthis = opcode
	opcode = opcode + 1
	opext  = (0&op/2)     AND 01FFh
	opext2 = (0&op/2)     AND 03FFh
	opext5 = (0&op/2)     AND 01Fh
	opregB = (0&op/0800h) AND 01Fh
	opregC = (0&op/0040h) AND 01Fh
	opOE   = (0&op/0400h) AND 01h
	opRc   =  0&op        AND 01h
	fWriteFetched = 0
	fReaded = 0
ENDM


;
; PpcFetch
;
; Fetch 16-bit opcode and jump through opc_ppc[] jump table
;

PpcFetch  MACRO

	IF (fWriteFetched EQ 0)

IF DEBUG
            PrepareForCall
            call  _StepHookPpc
            AfterCall 0
ENDIF
            movzx ebx,word ptr [esi]
            movzx eax,word ptr [esi-2]
			lea   esi,[esi-4]
			jmp   [opc_ppc+ebx*4]

	ELSE ;; fWriteFetched

		fWriteFetched = 0

	ENDIF

ENDM

