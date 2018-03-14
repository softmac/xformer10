
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; 68000.INC
;;
;; 68000 emulator include file
;;
;; Copyright (C) 1991-2008 by Darek Mihocka. All Rights Reserved.
;; Branch Always Software. http://www.emulators.com/
;;
;; 11/30/2008 darekm        last update
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; target minimum of a 386

    .386

    .MODEL FLAT
    OPTION NOSCOPED
    OPTION NOOLDMACROS
    .RADIX 16

POWERPC = 0

; read-only globals set by the 68000 monitor

externdef _vregs:DWORD
externdef _lBreakpoint:DWORD

externdef _DecodeExtWordAn:NEAR
externdef _DecodeExtWordPC:NEAR

; these flags are set during __F*HW calls (read/write hardware layer)

externdef fException:BYTE

; 68000 opcode address table - 65536 longword entries

externdef opcodes:DWORD

; VM callback routines

externdef vm_InitHW:DWORD
externdef _TrapHook:NEAR
externdef _StepHook:NEAR

externdef _PrintSz:NEAR
externdef _PrintLineSz:NEAR
externdef _PrintDword:NEAR
externdef _PrintLineDword:NEAR

externdef _exception:NEAR
externdef _vector:NEAR

externdef _traced:BYTE
externdef doTRACE:NEAR

externdef _cntInstr:DWORD
externdef _cntCodeHit:DWORD
externdef _cntCodeMiss:DWORD

externdef _cntAccess:DWORD
externdef _cntDataHit:DWORD
externdef _cntDataMiss:DWORD

externdef rgfExcptHook:DWORD

externdef opc1000:DWORD
externdef opc2000:DWORD
externdef opc3000:DWORD
externdef opc4000:DWORD
externdef opc5000:DWORD
externdef opc6000:DWORD
externdef opc7000:DWORD
externdef opc8000:DWORD
externdef opc9000:DWORD
externdef opcA000:DWORD
externdef opcB000:DWORD
externdef opcC000:DWORD
externdef opcD000:DWORD
externdef opcE000:DWORD
externdef opcF000:DWORD

;;
;; Host register usage
;;
;;
;; Non-volatiles (valid across helper calls):
;;
;; ESP is host stack
;; EBP is host frame pointer / guest register state
;; ESI is translated guest PC (to be depracated eventually)
;; EDI is guest source register (usually an immediate operand or second source)
;; EBX is guest effective address
;;
;; Volatiles (short lived or must be preserved):
;;
;; EDX is scratch, never needs to be saved
;; ECX is guest data accumulator
;; EAX is temp for flags, misc., needs to be saved across calls

externdef _vregs:DWORD

REGBASE = 1
PERFCOUNT = 0

IF REGBASE
rGR     TEXTEQU <edi>
ELSE
flagZ   TEXTEQU <edi>
ENDIF

rPC     TEXTEQU <esi>
rPC16   TEXTEQU <si>

rGA     TEXTEQU <ebp>
rGA16   TEXTEQU <bp>

rDA     TEXTEQU <ebx>
rDA16   TEXTEQU <bx>
rDA8L   TEXTEQU <bl>
rDA8H   TEXTEQU <bh>

IF REGBASE
regState TEXTEQU <rGR>
ELSE
regState TEXTEQU <_vregs>
ENDIF

; interrupt checking routine called every 256 instructions emulated

externdef PollInterrupts:NEAR

; global 68000 registers, referenced by positive EDX offsets

flagN   TEXTEQU <dword ptr ![regState!]>
regUSP  TEXTEQU <dword ptr ![regState+04!]>
regSSP  TEXTEQU <dword ptr ![regState+08!]>
regVBR  TEXTEQU <dword ptr ![regState+0C!]>
regPC   TEXTEQU <dword ptr ![regState+10!]>

opXword TEXTEQU <word ptr ![regState+14!]>

; status and condition register

regSR   TEXTEQU <word ptr ![regState+16!]>
regCCR  TEXTEQU <byte ptr ![regState+16!]>
regSRHi TEXTEQU <byte ptr ![regState+17!]>

regFPIAR TEXTEQU <![regState+1C!]>

ofsD0   TEXTEQU <regState+20>

regD0   TEXTEQU <![ofsD0!]>
regD1   TEXTEQU <![ofsD0+04!]>
regD2   TEXTEQU <![ofsD0+08!]>
regD3   TEXTEQU <![ofsD0+0C!]>
regD4   TEXTEQU <![ofsD0+10!]>
regD5   TEXTEQU <![ofsD0+14!]>
regD6   TEXTEQU <![ofsD0+18!]>
regD7   TEXTEQU <![ofsD0+1C!]>

regA0   TEXTEQU <![ofsD0+20!]>
regA1   TEXTEQU <![ofsD0+24!]>
regA2   TEXTEQU <![ofsD0+28!]>
regA3   TEXTEQU <![ofsD0+2C!]>
regA4   TEXTEQU <![ofsD0+30!]>
regA5   TEXTEQU <![ofsD0+34!]>
regA6   TEXTEQU <![ofsD0+38!]>
regA7   TEXTEQU <![ofsD0+3C!]>

; temporaries

IF REGBASE
flagZ   TEXTEQU <dword ptr ![regState+60!]>
ENDIF

regPCLv TEXTEQU <dword ptr ![regState+64!]>
regPCSv TEXTEQU <dword ptr ![regState+68!]>

temp32  TEXTEQU <dword ptr ![regState+6C!]>
temp16  TEXTEQU <word ptr  ![regState+6C!]>
temp8   TEXTEQU <byte ptr  ![regState+6C!]>

counter TEXTEQU <dword ptr ![regState+70!]>
cpuCur  TEXTEQU <dword ptr ![regState+74!]>

vecnum  TEXTEQU <dword ptr ![regState+78!]>
flagsVC TEXTEQU <byte ptr  ![regState+7A!]>
flagX   TEXTEQU <byte ptr  ![regState+7B!]>

; keep these defines in sync with 68K.H

bitsSpan    EQU 6t
cbSpan      EQU 1 SHL bitsSpan
dwBaseMask  EQU ((0FFFFFFFFh SHL bitsSpan) AND 0FFFFFFFFh)
dwIndexMask EQU 07FFh

; instruction descriptor and dispatch table used during execution
duops      TEXTEQU <dword ptr ![regState+00007C!]>

cbDuop      EQU 16t

; memory translation table (the "TLB")
memtlbRW   TEXTEQU <dword ptr ![regState+10047C!]>
memtlbRO   TEXTEQU <dword ptr ![regState+10447C!]>

IF POWERPC

rgGPR   TEXTEQU <dword ptr ![rGR+08007C!]>
regR1   TEXTEQU <dword ptr ![rGR+080080!]>
rgFPR   TEXTEQU <dword ptr ![rGR+0800FC!]>
rgSPR   TEXTEQU <dword ptr ![rGR+0801FC!]>

; each special purpose register
regMQ   TEXTEQU <dword ptr ![rGR+0801FC!]>
regXER  TEXTEQU <dword ptr ![rGR+080200!]>
regLR   TEXTEQU <dword ptr ![rGR+08021C!]>
regCTR  TEXTEQU <dword ptr ![rGR+080220!]>
regCR   TEXTEQU <dword ptr ![rGR+08022C!]>
regFPCR TEXTEQU <dword ptr ![rGR+080230!]>

ENDIF ;; POWERPC


; exit handlers

externdef exitNORM:NEAR
externdef exitBREAK:NEAR
externdef _xcptBUSREAD:NEAR
externdef _xcptBUSWRITE:NEAR
externdef _xcptBUSADDR:NEAR
externdef _xcptILLEGAL:NEAR
externdef _xcptDIVZERO:NEAR
externdef _xcptCHKERR:NEAR
externdef _xcptTRAPV:NEAR
externdef _xcptPRIVERR:NEAR
externdef _xcptTRACE:NEAR

; opcode to handle all illegal instruction encodings

externdef opsILLEGAL:NEAR

IF 1

; read address handlers, CALL entry points
; enter with EBP = 68000 effective address
; exits with EBP = page number, EDI = flat address

externdef ReadEA0:NEAR
externdef ReadEA1:NEAR
externdef ReadEA2:NEAR
externdef ReadEA4:NEAR
externdef ReadEA5:NEAR

ENDIF

; write address handlers, CALL entry points
; enter with EBP = 68000 effective address

IF 1

externdef WriteEA0:NEAR
externdef WriteEA1:NEAR
externdef WriteEA2:NEAR
externdef WriteEA4:NEAR
externdef WriteEA5:NEAR

ENDIF

; CPU bit flags

cpu68000 EQU 00000002h
cpu68010 EQU 00000004h
cpu68020 EQU 00000008h
cpu68030 EQU 00000010h
cpu68040 EQU 00000020h

cpu68k32 EQU 00000038h

cpu601   EQU 00000040h


; valid operand sizes

SIZE_B  = 0         ; 8-bit byte
SIZE_W  = 1         ; 16-bit word
SIZE_L  = 2         ; 32-bit long
SIZE_X  = 3         ; invalid
SIZE_PW = 4         ; MOVEP.W
SIZE_PL = 5         ; MOVEP.L
SIZE_16 = 6         ; 16-byte line
SIZE_FX = 7         ; 96-bit extended float
SIZE_FD = 8         ; 64-bit double float
SIZE_FS = 9         ; 32-bit single float
SIZE_FP = 10        ; 96-bit binary coded packed decimal


;; align on 4-byte boundary

BigAlign TEXTEQU <align 4t>

;;
;; Enter68k/Exit68k
;;
;; Sets up 80386 register when entering and exiting 68000 mode.
;; Used by Peek and Poke functions, and Go68000.
;;

externdef _GetPregs:NEAR

SetGuestState MACRO
IF REGBASE
    call  _GetPregs
    mov   rGR,eax
ENDIF
ENDM


Enter68k MACRO
    push  ebp
    push  esi
    push  edi
    push  ebx

    SetGuestState
ENDM


Exit68k MACRO
    pop   ebx
    pop   edi
    pop   esi
    pop   ebp
ENDM


;
; PrepareForCall / AfterCall
;
; Push important registers before calling C function.
; Restore important registers after a call to C.
;
; Assumes EBX, ESI, EDI, and EBP will be preserved by C caller.
; EAX contains return value from call. Z flag set if zero.
;

; Non-destructively packs flagX/flagsVC/flagN/flagZ to regCCR

PackFlags MACRO
    cmp   flagN,0
    sets  dl                ; 0000000N
    add   dl,flagX
    add   dl,flagX          ; ------XN
    cmp   flagZ,1
    adc   dl,dl             ; -----XNZ
    add   dl,dl             ; ----XNZ0
    add   dl,dl             ; ---XNZ00
    xor   dl,flagsVC        ; ---XNZVC
    and   dl,0FCh
    xor   dl,flagsVC        ; ---XNZVC
    mov   regCCR,dl
ENDM

; Non-destructively unpacks regCCR flags to flagX/flagsVC/flagN/flagZ

UnpackFlags MACRO
    mov   dl,regCCR         ; 000XNZVC
    mov   flagsVC,dl
    rol   dl,4              ; NZVC---X
    movsx edx,dl
    mov   flagX,dl
    mov   flagN,edx
    and   edx,40h
    xor   edx,40h
    mov   flagZ,edx
ENDM

;
; PC2Flat
;
; Convert address in reg to flat address in ESI suitable for PC.
;

externdef @FastTranslatePC@4:NEAR

PC2Flat MACRO reg:=<rGA>
    mov   regPCLv,rPC       ; save away last known good live guest PC
    mov   ecx,reg
    call  @FastTranslatePC@4
    mov   rPC,eax
ENDM


;
; Flat2PC
;
; Convert flat host address to a 68000 guest PC.
;
; Note: we assume that the flat address is in the 68000 address space
; and we fudge the upper 8 bits to equal the last PC jumped to.
;

Flat2PC MACRO reg:=<rGA>
    mov     reg,rPC
    sub     reg,regPCSv
    shr     reg,4
    add     reg,regPC
ENDM


PrepareForCall MACRO fSavePC:=<1>, fSaveFlags:=<1>
    push    ecx

    IF (fSavePC NE 0) OR (DEBUG NE 0)
    Flat2PC edx             ; pass the current guest PC to C/C++ code
    mov     regPC,edx
    mov     regPCSv,rPC     ; update temp in case don't call PC2Flat
    ENDIF

    IF (fSaveFlags NE 0) OR (DEBUG NE 0)
    PackFlags               ; snapshot the temporary flags to CCR
    ENDIF
ENDM

AfterCall MACRO cnt:REQ, fRestorePC:=<1>, fRestoreFlags:=<1>
    IF (cnt NE 0)
        add esp,4*cnt
    ENDIF

    IF (fRestorePC NE 0) OR (DEBUG NE 0)
    push    eax             ; PC2Flat trashes EAX
    mov     edx,regPC
    PC2Flat edx
    pop     eax
    ENDIF

    IF (fRestoreFlags NE 0) OR (DEBUG NE 0)
    UnpackFlags
    ENDIF

    pop     ecx

    test    eax,eax
ENDM


RegDn MACRO reg:REQ
    EXITM regD&reg
ENDM


RegAn MACRO reg:REQ
    EXITM regA&reg
ENDM


;
; DefOp op, label
;
; Generates entry in opcodes[] jump table.
; If instruction op is defined, it generates opsXXXX.
; Otherwise it generates the label.

DefOp  MACRO op:REQ, lbl:=<opsILLEGAL>, count:=<1>
    IFDEF ops&op
        DD count DUP (ops&op)
    ELSE
        DD count DUP (lbl)
    ENDIF
    opcode = opcode + count
ENDM


IncrementPCByK  MACRO cb:REQ
    add   rPC,cb SHL 4
ENDM

IncrementPCBy2  MACRO
    IncrementPCByK  2
ENDM

IncrementPCBy4  MACRO
    IncrementPCByK  4
ENDM

DecrementPCBy2  MACRO
    IncrementPCByK  -2
ENDM


;
; FastFetch
;
; Dispatch after a normal fallthrough instruction.
; Dispatch is keyed off the host's pointer to guest instruction.
;

FastFetch  MACRO size:=<2>
    ; Look up and return address of next instruction's handler

    mov   eax,dword ptr [rPC]
;   mov   ecx,rPC
    ret
ENDM


;
; Fetch
;
; Dispatch after a flow control. Does not assume anything is hashed.
;

externdef _dispatch:NEAR
externdef _dispatch_newPC:NEAR

Fetch  MACRO size:=<2>
    jmp   _dispatch
ENDM


;
; Decode op
;
; Decodes the fields in the instruction word
;

Decode MACRO op:REQ
    opthis = opcode
    opcode = opcode + 1
    opsize = (0&op/040h)  AND 3
    oplow  =  0&op        AND 0FFh
    opdreg = (0&op/0200h) AND 7
    opdmod = (0&op/040h)  AND 7
    opsmod = (0&op/8)     AND 7
    opsreg =  0&op        AND 7
    optrap =  0&op        AND 0Fh
    opbit8 = (0&op/0100h) AND 1
    opbit5 = (0&op/020h)  AND 1
    fReaded = 0
ENDM

;
; Valid op
;
; Common prologue code for all instructions
;

Valid MACRO    last:=<0>, mask:=<0>, fSkip7:=<0>
    Valid2 %opthis, %last, %mask, %fSkip7
ENDM

Valid2 MACRO op:REQ, last:=<0>, mask:=<0>, fSkip7:=<0>
    IF ((0&op AND 0FF) EQ 0)
        ECHO $&op
    ENDIF

    IF ((last GT 0) AND ((0&op AND mask) EQ 0))
        BigAlign
    ENDIF

    IF ((last GT 0) AND ((0&op AND mask) EQ last))
        fEmit = 1
    ELSEIF (last EQ 0)
        fEmit = 1
    ELSE
        fEmit = 1   ;; HACK!
    ENDIF

    IF (fEmit)
        align 4t

ops&op::
    ENDIF

ENDM

;
; Imm16sx/zx
;
; Fetch 16-bit immediate operand into register and increment program counter.
; register is extended appropriately.
;

externdef @FastTranslateCodeWord@4:NEAR
externdef @FastTranslateCodeLong@4:NEAR

Imm16zx  MACRO ireg:=<eax>
    mov   ireg,eax
    push  ecx
    push  edx
    Flat2PC ecx
    call  @FastTranslateCodeWord@4
    pop   edx
    pop   ecx
    movzx eax,word ptr [eax]
    xchg  ireg,eax

    IncrementPCBy2
ENDM

Imm16sx  MACRO ireg:=<eax>
    mov   ireg,eax
    push  ecx
    push  edx
    Flat2PC ecx
    call  @FastTranslateCodeWord@4
    pop   edx
    pop   ecx
    movsx eax,word ptr [eax]
    xchg  ireg,eax

    IncrementPCBy2
ENDM

Imm16zxNoIncPC  MACRO ireg:=<eax>
    mov   ireg,eax
    push  ecx
    push  edx
    Flat2PC ecx
    call  @FastTranslateCodeWord@4
    pop   edx
    pop   ecx
    movzx eax,word ptr [eax]
    xchg  ireg,eax
ENDM

Imm16sxNoIncPC  MACRO ireg:=<eax>
    mov   ireg,eax
    push  ecx
    push  edx
    Flat2PC ecx
    call  @FastTranslateCodeWord@4
    pop   edx
    pop   ecx
    movsx eax,word ptr [eax]
    xchg  ireg,eax
ENDM

Imm8zx   MACRO ireg:=<eax>
    mov   ireg,eax
    push  ecx
    push  edx
    Flat2PC ecx
    call  @FastTranslateCodeWord@4
    pop   edx
    pop   ecx
    movzx eax,byte ptr [eax]
    xchg  ireg,eax

    IncrementPCBy2
ENDM

Imm8sx   MACRO ireg:=<eax>
    mov   ireg,eax
    push  ecx
    push  edx
    Flat2PC ecx
    call  @FastTranslateCodeWord@4
    pop   edx
    pop   ecx
    movsx ireg,byte ptr [eax]
    xchg  ireg,eax

    IncrementPCBy2
ENDM

Imm8sxNoIncPC   MACRO ireg:=<eax>
    mov   ireg,eax
    push  ecx
    push  edx
    Flat2PC ecx
    call  @FastTranslateCodeWord@4
    pop   edx
    pop   ecx
    movsx eax,byte ptr [eax]
    xchg  ireg,eax
ENDM

RefetchOpcodeImm8sx   MACRO ireg:=<eax>
    mov   ireg,eax
    push  ecx
    push  edx
    Flat2PC ecx
    add   ecx,-2
    call  @FastTranslateCodeWord@4
    pop   edx
    pop   ecx
    movsx eax,byte ptr [eax]
    xchg  ireg,eax
ENDM

RefetchOpcodeImm16zx  MACRO ireg:=<eax>
    mov   ireg,eax
    push  ecx
    push  edx
    Flat2PC ecx
    add   ecx,-2
    call  @FastTranslateCodeWord@4
    pop   edx
    pop   ecx
    movzx eax,word ptr [eax]
    xchg  ireg,eax
ENDM

;
; Imm32
;
; Fetch 32-bit immediate operand into register and increment program counter.
;

Imm32    MACRO ireg:=<eax>
    mov   ireg,eax
    push  ecx
    push  edx
    Flat2PC ecx
    call  @FastTranslateCodeLong@4
    pop   edx
    pop   ecx
    mov   eax,dword ptr [eax]
    xchg  ireg,eax

    IncrementPCBy4
ENDM

Imm32NoIncPC    MACRO ireg:=<eax>
    mov   ireg,eax
    push  ecx
    push  edx
    Flat2PC ecx
    call  @FastTranslateCodeLong@4
    pop   edx
    pop   ecx
    mov   eax,dword ptr [eax]
    xchg  ireg,eax
ENDM


;;
;; Exception
;;
;; Generate an exception stack frame for exception #vector.
;;
;; EAX = vector
;; EBX = cb (additional bytes in addition to the usual 6/8 byte frame)
;; EBP = format*1000h + vector*4 (frame descriptor word)
;;

Exception MACRO vector:=<0>, cb:=<0>, format:=<0>

    xor   eax,eax
    IF (vector NE 0)
        mov   al,vector
    ENDIF

    xor   rDA,rDA
    IF (cb NE 0)
        mov   rDA8L,cb
    ENDIF

    mov   rGA,format*1000h+vector*4

    call  _exception

ENDM


;;
;;  Vector vector
;;
;;  Jump through vector # in temp or set temp to the non-zero vector number
;;

Vector MACRO vector:=<0>
    IF (vector NE 0)
        mov   temp32,vector*4
    ELSE
        shl   temp32,2
    ENDIF

    jmp   _vector
ENDM



;
; Guest-to-host address translation macros
;
; Convert 68000 effective address in rGA to host address in EAX.
; Zero flag is set if translation succeeded.
;

TransHash MACRO cb:=<1>, src:=<rGA>, bits:=<bitsSpan>
    mov     edx,src
    shr     edx,bitsSpan
    and     edx,dwIndexMask
ENDM


TranslatLookupForWrite MACRO cb:=<1>, src:=<rGA>
    TransHash cb
    mov     ecx,src
    IF (cb GT 2)
    add     ecx,cb-2
    ENDIF
    mov     eax,dword ptr [memtlbRW+edx*8+4]    ; prefetch XOR value
    xor     ecx,dword ptr [memtlbRW+edx*8]
    xor     eax,src
    IF (cb GT 1)
    test    ecx,dwBaseMask OR 1 ; don't mask lowest bit to check for misalign
    ELSE
    test    ecx,dwBaseMask OR 0 ; byte access can't misalign
    ENDIF
ENDM


TranslatLookupForRead MACRO cb:=<1>, src:=<rGA>
    TransHash cb
    mov     ecx,src
    IF (cb GT 2)
    add     ecx,cb-2
    ENDIF
    mov     eax,dword ptr [memtlbRO+edx*8+4]    ; prefetch XOR value
    xor     ecx,dword ptr [memtlbRO+edx*8]
    xor     eax,src
    IF (cb GT 1)
    test    ecx,dwBaseMask OR 1 ; don't mask lowest bit to check for misalign
    ELSE
    test    ecx,dwBaseMask OR 0 ; byte access can't misalign
    ENDIF
ENDM


TranslateAddr MACRO src:=<rGA>
 ;; xor     eax,src
ENDM




;
; FAny/FAlt/FControl/FMem/FData mode, reg, size
;
; Return 1 if the addressing mode is valid, or 0 if it is not
;

FAny  MACRO mode, reg, size
    IF ((mode EQ 1) AND (size EQ 0))
        EXITM <0>
    ELSEIF ((mode LT 7) OR (reg LT 5))
        EXITM <1>
    ENDIF
    EXITM <0>
ENDM

FAlt  MACRO mode, reg, size
    IF ((mode EQ 1) AND (size EQ 0))
        EXITM <0>
    ELSEIF ((mode LT 7) OR (reg LT 2))
        EXITM <1>
    ENDIF
    EXITM <0>
ENDM

FData MACRO mode, reg, size
    IF (mode EQ 1)
        EXITM %0
    ELSEIF ((mode LT 7) OR (reg LT 5))
        EXITM %1
    ENDIF
    EXITM %0
ENDM

FDataAlt MACRO mode, reg, size
    IF (mode EQ 1)
        EXITM <0>
    ELSEIF ((mode LT 7) OR (reg LT 2))
        EXITM <1>
    ENDIF
    EXITM %0
ENDM

FMem  MACRO mode, reg, size
    IF (mode LT 2)
        EXITM %0
    ELSEIF ((mode LT 7) OR (reg LT 5))
        EXITM %1
    ENDIF
    EXITM %0
ENDM

FControl MACRO mode, reg, size
    IF ((mode LT 2) OR (mode EQ 3) OR (mode EQ 4))
        EXITM %0
    ELSEIF ((mode EQ 2) OR (mode EQ 5) OR (mode EQ 6) OR (reg LT 4))
        EXITM %1
    ENDIF
    EXITM %0
ENDM


;
; ReadReg mode, reg, size
;
; Read 68000 register into EBX
;

ReadReg MACRO mode, reg, size

    IF (mode EQ 0)
ireg TEXTEQU <regD&reg>
    ELSEIF (mode EQ 1)
ireg TEXTEQU <regA&reg>
    ELSE
        .ERR
    ENDIF

    IF (size EQ SIZE_B)
        mov   rDA8L,byte ptr ireg
    ELSEIF (size EQ SIZE_W)
        movzx rDA,word ptr ireg
    ELSEIF (size EQ SIZE_L)
        mov   rDA,ireg
    ELSE
        .ERR
    ENDIF

ENDM

;
; WriteReg mode, reg, size
;
; Write 68000 register from EBX
;

WriteReg MACRO mode, reg, size

    IF (mode EQ 0)
ireg TEXTEQU <regD&reg>
    ELSEIF (mode EQ 1)
ireg TEXTEQU <regA&reg>
    ELSE
        .ERR
    ENDIF

    IF (size EQ SIZE_B)
        mov   byte ptr ireg,rDA8L
    ELSEIF (size EQ SIZE_W)
        mov   word ptr ireg,rDA16
    ELSEIF (size EQ SIZE_L)
        mov   ireg,rDA
    ELSE
        .ERR
    ENDIF

ENDM


;
; GenEA mode, reg, size
;
; Generates effective address and store it in EBP.
; Note: requires alterable addressing mode
;
; NOTE: must preserve EAX except for PC-rel modes
;

GenEA MACRO mode, reg, size

iAreg TEXTEQU <regA&reg>

    IF (size GT 10t) OR (size EQ 3)
        .ERR
        ECHO GenEA: Invalid size
    ENDIF

    IF (mode LT 2)
        .ERR
        ECHO GenEA: Invalid mode
    ENDIF

    IF (mode EQ 2)        ;; (An)
        mov rGA,iAreg

    ELSEIF (mode EQ 3)    ;; (An)+
        mov rGA,iAreg

        IF (size EQ SIZE_FX) OR (size EQ SIZE_FP)
            add dword ptr iAreg,12t
        ELSEIF (size EQ SIZE_FD)
            add dword ptr iAreg,8
        ELSEIF (size EQ SIZE_L) OR (size EQ SIZE_FS)
            add dword ptr iAreg,4
        ELSEIF (size EQ SIZE_W) OR (reg EQ 7)
            add dword ptr iAreg,2
        ELSEIF (size EQ SIZE_B)
            add dword ptr iAreg,1
        ELSE
            .ERR
        ENDIF

    ELSEIF (mode EQ 4)    ;; -(An)
        mov rGA,iAreg
        IF (size EQ SIZE_FX) OR (size EQ SIZE_FP)
            add rGA,-12t
        ELSEIF (size EQ SIZE_FD)
            add rGA,-8
        ELSEIF (size EQ SIZE_L) OR (size EQ SIZE_FS)
            add rGA,-4
        ELSEIF (size EQ 1) OR (reg EQ 7)
            add rGA,-2
        ELSEIF (size EQ SIZE_B)
            add rGA,-1
        ELSE
            .ERR
        ENDIF
        mov   iAreg,rGA

    ELSEIF (mode EQ 5)    ;; nnnn(An)
        Imm16sx rGA
        add rGA,dword ptr iAreg

    ELSEIF (mode EQ 6)    ;; nn(An,Rn)
        mov   rGA,iAreg
        call  _DecodeExtWordAn

    ELSEIF (reg EQ 0)    ;; nnnn.w
        Imm16sx rGA

    ELSEIF (reg EQ 1)    ;; nnnnnnnn.l
        Imm32 rGA

    ELSEIF (reg EQ 2)    ;; nnnn(PC)
        Flat2PC
        Imm16sx edx
        add   rGA,edx   ;; 68000 address

    ELSEIF (reg EQ 3)    ;; nn(PC,Rn)
        call  _DecodeExtWordPC

    ELSE
        .ERR

    ENDIF
ENDM


;
; ReadEAMode mode, reg, size
;
; A special version of ReadEA for use with instructions
; that don't have a 6-bit source EA, only a 3-bit register.
; For use with ADDX, SUBX, ABCD, SBCD.
;

ReadEAMode MACRO mode, reg, size

    ReadEA mode, reg, size

ENDM


;
; ReadMemFlat size
;
; Read a zero-extended operand from guest memory into rDA
;

ReadMemFlat MACRO size:REQ

    ; int 3

    IF (size EQ SIZE_B)
        movzx rDA,byte ptr [eax]
    ELSEIF (size EQ SIZE_W)
        movzx rDA,word ptr [eax-1]
    ELSEIF (size EQ SIZE_L)
        mov   rDA,dword ptr [eax-3]
    ELSE
        .ERR
        ECHO ReadMemFlat: Invalid size
    ENDIF
ENDM

;
; WriteMemFlat size
;
; Write an operand in EBX to rgbMem[reg].
; Assumes EBP/EDI have already been checked for validity.
;

WriteMemFlat MACRO size:REQ

    ; int 3

    IF (size EQ SIZE_B)
        mov   byte ptr [eax],rDA8L
    ELSEIF (size EQ SIZE_W)
        mov   word ptr [eax-1],rDA16
    ELSEIF (size EQ SIZE_L)
        mov   dword ptr [eax-3],rDA
    ELSE
        .ERR
        ECHO WriteMemFlat: Invalid size
    ENDIF
ENDM


;
; ReadEA mode, reg, size
;
; Generates effective address and reads contents into EBX.
; Special cases for Dn, An, and immediate addressing modes.
;

externdef @ReadEA0@4:NEAR
externdef @ReadEA1@4:NEAR
externdef @ReadEA2@4:NEAR
externdef @ReadEA4@4:NEAR
externdef @ReadEA5@4:NEAR

ReadMemAtEA MACRO size:REQ, fForceC:=<0>, fInline:=<1>
IF PERFCOUNT
    add   _cntAccess,1
ENDIF

IF fForceC OR 0
    ;; C version

    mov   regPCLv,rPC
    mov   ecx,rGA
    call  @ReadEA&size&@4
    mov   rDA,eax

ELSE
    ;; ASM version

 ;  mov   ecx,rGA
    call  ReadEA&size
 ;  or    rDA,rDA
ENDIF
ENDM


ReadEA MACRO mode, reg, size

iDreg TEXTEQU <regD&reg>
iAreg TEXTEQU <regA&reg>

    IF (FAny(mode, reg, size) NE 1)
        .ERR
        ECHO ReadEA: Invalid addressing mode
    ENDIF

    IF (mode LT 2)
        ReadReg mode, reg, size

    ELSEIF ((mode EQ 7) AND (reg EQ 4)) ;; immediate #
        IF (size EQ SIZE_L)
            Imm32 rDA
        ELSEIF (size EQ SIZE_W)
            Imm16zx rDA
        ELSE
            Imm8zx rDA
        ENDIF

    ELSEIF (mode EQ 3)    ;; (An)+
        mov rGA,iAreg
        ReadMemAtEA &size

        IF (size EQ SIZE_FX) OR (size EQ SIZE_FP)
            add dword ptr iAreg,12t
        ELSEIF (size EQ SIZE_FD)
            add dword ptr iAreg,8
        ELSEIF (size EQ SIZE_L) OR (size EQ SIZE_FS)
            add dword ptr iAreg,4
        ELSEIF (size EQ SIZE_W) OR (reg EQ 7)
            add dword ptr iAreg,2
        ELSEIF (size EQ SIZE_B)
            add dword ptr iAreg,1
        ELSE
            .ERR
        ENDIF

    ELSEIF (mode EQ 4)    ;; -(An)
        mov rGA,iAreg

        IF (size EQ SIZE_FX) OR (size EQ SIZE_FP)
            add rGA,-12t
        ELSEIF (size EQ SIZE_FD)
            add rGA,-8
        ELSEIF (size EQ SIZE_L) OR (size EQ SIZE_FS)
            add rGA,-4
        ELSEIF (size EQ 1) OR (reg EQ 7)
            add rGA,-2
        ELSEIF (size EQ SIZE_B)
            add rGA,-1
        ELSE
            .ERR
        ENDIF

        ReadMemAtEA &size
        mov   iAreg,rGA

    ELSEIF ((mode EQ 7) AND (reg EQ 4)) ;; immediate #
        IF (size EQ SIZE_L)
            Imm32 rDA
        ELSEIF (size EQ SIZE_W)
            Imm16zx rDA
        ELSE
            Imm8zx rDA
        ENDIF

    ELSE
        GenEA %mode, %reg, %size
        ReadMemAtEA &size
    ENDIF
ENDM



;
; WriteEA mode, reg, size
;
; Generates effective address and writes EBX into contents.
; Special cases for Dn and An addressing modes.
; If fGenEA == 0 then EBP already has the effective address.
;
; NOTE: when fGenEA == 0, the Fetch is done automatically!!!!
; There is no need to follow the WriteEA with a Fetch.
;

externdef @WriteEA0@8:NEAR
externdef @WriteEA1@8:NEAR
externdef @WriteEA2@8:NEAR
externdef @WriteEA4@8:NEAR
externdef @WriteEA5@8:NEAR

WriteMemAtEA MACRO size:REQ, fForceC:=<0>, fInline:=<1>
IF PERFCOUNT
    add   _cntAccess,1
ENDIF

IF fForceC OR 0
    ;; C version

    mov   regPCLv,rPC
    mov   ecx,rGA
    mov   edx,rDA
    call  @WriteEA&size&@8

ELSE
    ;; ASM version

 ;  mov   ecx,rGA
 ;  mov   edx,rDA
    call  WriteEA&size
ENDIF
ENDM


WriteEA MACRO mode, reg, size, fGenEA:=<1>

iDreg TEXTEQU <regD&reg>
iAreg TEXTEQU <regA&reg>

    IF (FAlt(mode, reg, size) NE 1)
        .ERR
        ECHO WriteEA: Invalid addressing mode reg size
    ENDIF

    IF (mode LT 2)
        WriteReg mode, reg, size

    ELSEIF (fGenEA EQ 0)
        WriteMemAtEA size

    ELSEIF (mode EQ 3)    ;; (An)+
        mov rGA,iAreg
        WriteMemAtEA &size

        IF (size EQ SIZE_FX) OR (size EQ SIZE_FP)
            add dword ptr iAreg,12t
        ELSEIF (size EQ SIZE_FD)
            add dword ptr iAreg,8
        ELSEIF (size EQ SIZE_L) OR (size EQ SIZE_FS)
            add dword ptr iAreg,4
        ELSEIF (size EQ SIZE_W) OR (reg EQ 7)
            add dword ptr iAreg,2
        ELSEIF (size EQ SIZE_B)
            add dword ptr iAreg,1
        ELSE
            .ERR
        ENDIF

    ELSEIF (mode EQ 4)    ;; -(An)
        mov rGA,iAreg

        IF (size EQ SIZE_FX) OR (size EQ SIZE_FP)
            add rGA,-12t
        ELSEIF (size EQ SIZE_FD)
            add rGA,-8
        ELSEIF (size EQ SIZE_L) OR (size EQ SIZE_FS)
            add rGA,-4
        ELSEIF (size EQ 1) OR (reg EQ 7)
            add rGA,-2
        ELSEIF (size EQ SIZE_B)
            add rGA,-1
        ELSE
            .ERR
        ENDIF

        WriteMemAtEA &size
        mov   iAreg,rGA

    ELSE
        GenEA mode, reg, size
        WriteMemAtEA size
    ENDIF
ENDM


;
; PushLong
;
; Pushes the long accumulator to the A7 stack.
;

PushLong MACRO
    mov   rGA,regA7
    add   rGA,-4
    WriteMemAtEA %SIZE_L
    mov   regA7,rGA
ENDM

PushWord MACRO
    mov   rGA,regA7
    add   rGA,-2
    WriteMemAtEA %SIZE_W
    mov   regA7,rGA
ENDM


;
; PopLong
;
; Pops the long on the A7 stack into accumulator..
;

PopLong MACRO
    mov   rGA,regA7
    ReadMemAtEA %SIZE_L
    add   rGA,4
    mov   regA7,rGA
ENDM


PopWord MACRO
    mov   rGA,regA7
    ReadMemAtEA %SIZE_W
    add   rGA,2
    mov   regA7,rGA
ENDM


;
; SetiDreg reg
;
; Sets the text constant iDreg equal to Dn where n = reg
;

SetiDreg MACRO reg
iDreg TEXTEQU <regD&reg>
ENDM


;
; SetiAreg reg
;
; Sets the text constant iAreg equal to An where n = reg
;

SetiAreg MACRO reg
iAreg TEXTEQU <regA&reg>
ENDM


;
; Sign extend the current 16-bit value
;

SignExtendByte MACRO
    movsx rDA,rDA8L
ENDM

SignExtendWord MACRO
    movsx rDA,rDA16
ENDM

SignExtendByteToEax MACRO
    movsx eax,rDA8L
ENDM

SignExtendWordToEax MACRO
    movsx eax,rDA16
ENDM

SignExtendByteToSrc MACRO
    movsx rDA,rDA8L
    mov   temp32,rDA
ENDM

SignExtendWordToSrc MACRO
    movsx rDA,rDA16
    mov   temp32,rDA
ENDM

MoveAccToSrc MACRO
    mov   temp32,rDA
ENDM

NotByte MACRO
    not   rDA8L
ENDM

NotWord MACRO
    not   rDA16
ENDM

NotLong MACRO
    not   rDA
ENDM

NegByte MACRO
    neg   rDA8L
ENDM

NegWord MACRO
    neg   rDA16
ENDM

NegLong MACRO
    neg   rDA
ENDM

ClearLong MACRO
    xor   rDA,rDA
ENDM


;
; CCR flags
;

bitmaskC TEXTEQU <01h>
bitmaskV TEXTEQU <02h>
bitmaskZ TEXTEQU <04h>
bitmaskN TEXTEQU <08h>
bitmaskX TEXTEQU <10h>


MoveXToCarry MACRO
    bt    dword ptr flagX,0
ENDM

ClearVC MACRO
    mov   flagsVC,0
ENDM


;
; UpdateNZ
;
; Updates the CCR N and Z flags.
; Typically used after a MOVE, OR, AND, EOR, TST, etc. operation.
; Optimized for positive values.
;

UpdateNZ MACRO size:REQ
    IF (size EQ SIZE_B)
        movsx   rDA,rDA8L
    ELSEIF (size EQ SIZE_W)
        movsx   rDA,rDA16
    ELSEIF (size EQ SIZE_L)
    ELSE
        .ERR UpdateNZ invalid size %size
    ENDIF

    mov     flagZ,rDA
    mov     flagN,rDA
ENDM


UpdateNZ_Test8 MACRO
    UpdateNZ %SIZE_B
ENDM

UpdateNZ_Test16 MACRO
    UpdateNZ %SIZE_W
ENDM

UpdateNZ_Test32 MACRO
    UpdateNZ %SIZE_L
ENDM

UpdateNZ_Test0 MACRO
    UpdateNZ_Test8
ENDM

UpdateNZ_Test1 MACRO
    UpdateNZ_Test16
ENDM

UpdateNZ_Test2 MACRO
    UpdateNZ_Test32
ENDM

UpdateNZ_ClearVC_Test0 MACRO
    ClearVC
    UpdateNZ_Test8
ENDM

UpdateNZ_ClearVC_Test8 MACRO
    ClearVC
    UpdateNZ_Test8
ENDM

UpdateNZ_ClearVC_Test1 MACRO
    ClearVC
    UpdateNZ_Test16
ENDM

UpdateNZ_ClearVC_Test16 MACRO
    ClearVC
    UpdateNZ_Test16
ENDM

UpdateNZ_ClearVC_Test2 MACRO
    ClearVC
    UpdateNZ_Test32
ENDM

UpdateNZ_ClearVC_Test32 MACRO
    ClearVC
    UpdateNZ_Test32
ENDM


;
; UpdateNZVC
;
; Updates the CCR flags C V N and Z.
; Typically used after a CMP operation.
;

UpdateNZVC MACRO size:REQ
    seto  al
    adc   al,al
    mov   flagsVC,al

%   UpdateNZ size
ENDM


;
; UpdateCCR
;
; Updates all the CCR flags C X V N and Z.
; Typically used after a ADD or SUB operation.
;

UpdateCCR MACRO size:REQ
    seto  al
    setc  flagX
    adc   al,al
    mov   flagsVC,al

%   UpdateNZ size
ENDM


UpdateNZ_IfNonZero MACRO size:REQ
    IF (size EQ SIZE_B)
        movsx   rDA,rDA8L
    ELSEIF (size EQ SIZE_W)
        movsx   rDA,rDA16
    ELSEIF (size EQ SIZE_L)
    ELSE
        .ERR UpdateNZ_IfNonZero invalid size %size
    ENDIF

    mov     flagN,rDA
    or      flagZ,rDA
ENDM


UpdateZ_IfNonZero MACRO size:REQ
    IF (size EQ SIZE_B)
        movsx   rDA,rDA8L
    ELSEIF (size EQ SIZE_W)
        movsx   rDA,rDA16
    ELSEIF (size EQ SIZE_L)
    ELSE
        .ERR UpdateNZ_IfNonZero invalid size %size
    ENDIF

    or      flagZ,rDA
ENDM


;
; UpdateNEGX
;
; Updates all the CCR flags C X V N and Z.
; Used after an ADDX SUBX or NEGX operation.
; Same as UpdateCCR, except Z flag is cleared if non-zero, unchanged otherwise.
;

UpdateNEGX MACRO size:REQ
    seto  al
    setc  flagX
    adc   al,al
    mov   flagsVC,al

%   UpdateNZ_IfNonZero    size
ENDM


;
; UpdateBCD
;
; Updates the CCR flags C X and Z.
; Used after ABCD SBCD NBCD operations.
; C X set if borrow. Z flag is cleared if non-zero, unchanged otherwise.
;
; Note: result is in BL
;

UpdateBCD MACRO size:REQ
    setc  flagsVC               ; clear old V bit, set new C bit
    setc  flagX    

%   UpdateZ_IfNonZero    size
ENDM


;
; CheckCpu
;
; Make sure the current CPU is at least as high as cpu
;

CheckCpu MACRO cpu:REQ
    cmp    dword ptr cpuCur,cpu
    jb    _xcptILLEGAL
ENDM


;
; CheckPriv
;
; Check if we are in supervisor mode. If not, trap.
;

CheckPriv MACRO
    test  regSR,02000h
    jz    _xcptPRIVERR
ENDM


;
; GetSR
;
; Get 68000's SR register into rDA16.
;

GetSR     MACRO
    PackFlags
    mov   rDA16,regSR
ENDM


;
; PutSR
;
; Put rDA16 into the 68000's SR register.
; Privlidge has already been checked, so assume S=1.
;

externdef _putsr:NEAR

PutSR     MACRO
    jmp   _putsr
ENDM


;;
;;  SetIPL
;;
;;  Sets the IPL field in regSR
;;

SetIPL    MACRO ipl:REQ
    and   regSRHi,0f8h
    or    regSRHi,ipl
ENDM


;
; GetCCR
;
; Get 68000's CCR register into rDA8L.
;

GetCCR    MACRO
    PackFlags
    mov   rDA8L,regCCR
ENDM


;
; PutCCR
;
; Put rDA8L into the 68000's CCR register.
;

PutCCR    MACRO
    mov   regCCR,rDA8L
    UnpackFlags
ENDM


RAX0 TEXTEQU <al>
RAX1 TEXTEQU <ax>
RAX2 TEXTEQU <eax>

Acc0 TEXTEQU <rDA8L>
Acc1 TEXTEQU <rDA16>
Acc2 TEXTEQU <rDA>

Reg0 TEXTEQU <byte ptr iDreg>
Reg1 TEXTEQU <word ptr iDreg>
Reg2 TEXTEQU <dword ptr iDreg>

AReg0 TEXTEQU <byte ptr iAreg>
AReg1 TEXTEQU <word ptr iAreg>
AReg2 TEXTEQU <dword ptr iAreg>


;
; DoADD/DoSUB reg, size, fAcc, fX
;
; DoADD/DoSUB - add/sub iDreg to accumulator and update CCR
; For flags to work, Accumulator must hold the final result.
;
; size: 0=byte  1=word  2=long
; fAcc: 0=store in iDreg  1=store in Acc  2=store in iAreg
; fX:   0=add/sub   1=adc/sbb (ADDX/SUBX)
;

DoADD MACRO ireg:REQ, size:REQ, fAcc:REQ, fX:=<0>

    IF (fX NE 0)
AddOp TEXTEQU <adc>
        MoveXToCarry
    ELSE
AddOp TEXTEQU <add>
    ENDIF

    SetiDreg %ireg
    SetiAreg %ireg

    IF (fAcc EQ 0)
%        AddOp Acc&size,Reg&size
%        mov   Reg&size,Acc&size
    ELSEIF (fAcc EQ 1)
%        AddOp Acc&size,Reg&size
    ELSE
%        AddOp rDA,iAreg
%        mov   iAreg,rDA
    ENDIF

    IF (fX NE 0)
%       UpdateNEGX &size
    ELSEIF (fAcc NE 2)
%       UpdateCCR &size
    ENDIF
ENDM


DoSUB MACRO ireg:REQ, size:REQ, fAcc:REQ, fX:=<0>

    IF (fX NE 0)
SubOp TEXTEQU <sbb>
        MoveXToCarry
    ELSE
SubOp TEXTEQU <sub>
    ENDIF

    SetiDreg %ireg
    SetiAreg %ireg

    IF (fAcc EQ 0)
%        SubOp Reg&size,Acc&size
%        mov   Acc&size,Reg&size
    ELSEIF (fAcc EQ 1)
%        SubOp Acc&size,Reg&size
    ELSE
%        SubOp iAreg,rDA
%        mov   rDA,iAreg
    ENDIF

    IF (fX NE 0)
%       UpdateNEGX &size
    ELSEIF (fAcc NE 2)
%       UpdateCCR &size
    ENDIF
ENDM


DoAddxSubx MACRO size:REQ, fSubx:=<0>
    MoveXToCarry
    mov   eax,temp32

    IF (fSubx EQ 0)
%        adc Acc&size,RAX&size
    ELSE
%        sbb Acc&size,RAX&size
    ENDIF

%   UpdateNEGX &size
ENDM


;
; DoCMP mode, ireg, size
;
; DoCMP - compare accumulator with effective address
;
; size: 0=byte  1=word  2=long
;

DoCMP MACRO mode:REQ, ireg:REQ, size:REQ
    SetiDreg %ireg
    SetiAreg %ireg

    IF (mode EQ 0)
%        mov   RAX&size,Acc&size
%        mov   Acc&size,Reg&size
    ELSEIF (mode EQ 1)
%        mov   RAX&size,Acc&size
%        mov   Acc&size,AReg&size
    ELSE
        mov   temp32,rDA
        ReadEA %mode, %ireg, %size
        mov   eax,temp32
    ENDIF

%   sub   Acc&size,RAX&size
%   UpdateNZVC &size
ENDM


;
; DoEOR ireg, size
;
; DoEOR - EOR iDreg with accumulator and update CCR
;
; size: 0=byte  1=word  2=long
;

DoEOR MACRO ireg, size
    SetiDreg %ireg

%   xor   Acc&size,Reg&size

    UpdateNZ_ClearVC_Test&size
ENDM

;
; DoOR ireg, size, fAcc
;
; DoOR - add iDreg to accumulator and update CCR
;
; size: 0=byte  1=word  2=long
; fAcc: 0=store in iDreg  1=store in Acc
;

DoOR MACRO ireg, size, fAcc
    SetiDreg %ireg

    IF (fAcc EQ 0)
%        or    Acc&size,Reg&size
%        mov   Reg&size,Acc&size
    ELSE
%        or    Acc&size,Reg&size
    ENDIF

    UpdateNZ_ClearVC_Test&size
ENDM


;
; DoAND ireg, size, fAcc
;
; DoAND - add iDreg to accumulator and update CCR
;
; size: 0=byte  1=word  2=long
; fAcc: 0=store in iDreg  1=store in Acc
;

DoAND MACRO ireg, size, fAcc
    SetiDreg %ireg

    IF (fAcc EQ 0)
%        and   Acc&size,Reg&size
%        mov   Reg&size,Acc&size
    ELSE
%        and   Acc&size,Reg&size
    ENDIF

    UpdateNZ_ClearVC_Test&size
ENDM


;
; AddqSubq op, val, mode, ireg, size
;
; AddqSubq - add/sub immediate value to accumulator and update CCR, or to An
;
; size: 0=byte  1=word  2=long
; val:  0=8  1..7=1..7
;

AddqSubq MACRO op:REQ, val:REQ, mode:REQ, ireg:REQ, size:REQ

    IF (mode NE 1)
        ReadEA %mode, %ireg, %size

        IF (val EQ 0)
%            op    Acc&size,8
        ELSE
%            op    Acc&size,val
        ENDIF

        UpdateCCR %size
        WriteEA %mode, %ireg, %size, 0
    ELSE
        ReadEA %mode, %ireg, %SIZE_L

        IF (val EQ 0)
%            op    rDA,8
        ELSE
%            op    rDA,val
        ENDIF

        WriteEA %mode, %ireg, %SIZE_L, 0
    ENDIF

ENDM


DoBCD MACRO fSub:REQ
    MoveXToCarry
    mov   eax,temp32

    IF (fSUB EQ 0)
        adc   al,rDA8L
        daa
        mov   rDA8L,al
    ELSE
        sbb   rDA8L,al
        mov   al,rDA8L
        das
        mov   rDA8L,al
    ENDIF

    UpdateBCD %SIZE_B

ENDM


Bit0 TEXTEQU <bt>
Bit1 TEXTEQU <btc>
Bit2 TEXTEQU <btr>
Bit3 TEXTEQU <bts>

;;
;; DoBIT mod, ireg, type
;;

DoBIT MACRO mode:REQ, ireg:REQ, type:REQ

foo TEXTEQU <Bit&type>

    IF (mode EQ 0)
        ReadEA %mode, %ireg, %SIZE_L

        mov   eax,temp32
        and   eax,31t
%        &foo  rDA,eax
        sbb   edx,edx
        mov   flagZ,edx

        IF (type NE 0)
            WriteEA %mode, %ireg, %SIZE_L, 0
        ENDIF

    ELSE
        ReadEA %mode, %ireg, %SIZE_B

        mov   eax,temp32
        and   eax,7
%        &foo  rDA,eax
        sbb   edx,edx
        mov   flagZ,edx

        IF (type NE 0)
            WriteEA %mode, %ireg, %SIZE_B, 0
        ENDIF

    ENDIF

ENDM


;;
;; TestCond label
;;
;; TestCond - test condition cc and branch to label if true
;;
;; The VC bits are bits 1:0 in flagsVC.
;; The NZ bits are bits 7:6 in flagsNZ.
;; The two registers can be accessed as a 16 bit quantity flags16.
;;
;; Bit masks to test the bits are:
;;
;;   V = 0200 in flags16, or 02 in flagsVC
;;   C = 0100 in flags16, or 01 in flagsVC
;;   N = 0080 in flags16, or 80 in flagsNZ
;;   Z = 0040 in flags16, or 40 in flagsNZ
;;

TestCond MACRO lbl:REQ
    LOCAL fail

    cc = (opdreg*2) + opbit8

    IF (cc EQ 0)
        jmp   lbl
    ELSEIF (cc EQ 1)
        ;; nop
    ELSEIF (cc EQ 2)   ;; HI (C AND Z) = 0
        cmp   flagZ,0
        jz    fail
        test  flagsVC,1
        jz    lbl
    ELSEIF (cc EQ 3)   ;; LS (C OR Z) = 1
        cmp   flagZ,0
        jz    lbl
        test  flagsVC,1
        jnz   lbl
    ELSEIF (cc EQ 4)   ;; CC (C = 0)
        test  flagsVC,1
        jz    lbl
    ELSEIF (cc EQ 5)   ;; CS (C = 1)
        test  flagsVC,1
        jnz   lbl
    ELSEIF (cc EQ 6)   ;; NE (Z = 0)
        cmp   flagZ,0
        jnz   lbl
    ELSEIF (cc EQ 7)   ;; EQ (Z = 1)
        cmp   flagZ,0
        jz    lbl
    ELSEIF (cc EQ 8)   ;; VC (V = 0)
        test  flagsVC,2
        jz    lbl
    ELSEIF (cc EQ 9)   ;; VS (V = 1)
        test  flagsVC,2
        jnz   lbl
    ELSEIF (cc EQ 0ah) ;; PL (N = 0)
        test  flagN,80000000h
        jz    lbl
    ELSEIF (cc EQ 0bh) ;; MI (N = 1)
        test  flagN,80000000h
        jnz   lbl
    ELSEIF (cc EQ 0ch) ;; GE (N = V)
        mov   edx,flagN
        shr   edx,30t
        xor   dl,flagsVC
        test  dl,2
        jz    lbl
    ELSEIF (cc EQ 0dh) ;; LT (N <> V)
        mov   edx,flagN
        shr   edx,30t
        xor   dl,flagsVC
        test  dl,2
        jnz   lbl
    ELSEIF (cc EQ 0eh) ;; GT (Z = 0) AND (N = V)
        cmp   flagZ,0
        jz    fail     ;; fail if (Z = 1)
        mov   edx,flagN
        shr   edx,30t
        xor   dl,flagsVC
        test  dl,2
        jz    lbl
    ELSEIF (cc EQ 0fh) ;; LE (Z = 1) OR (N <> V)
        cmp   flagZ,0
        jz    lbl
        mov   edx,flagN
        shr   edx,30t
        xor   dl,flagsVC
        test  dl,2
        jnz   lbl
    ELSE
        .ERR
        ECHO TestCond: Invalid cc
    ENDIF

fail:
ENDM


externdef _ProfilePoint@4:NEAR

ProfilePoint MACRO id:REQ
IF DEBUG AND 0
    pusha
    push  id
    call  _ProfilePoint@4
    popa
ENDIF
ENDM



    .CODE

    BigAlign          ;; make sure each OPS* file starts aligned.

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

