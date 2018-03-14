
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; 68000.ASM
;;
;; 68000 emulator main entry module
;;
;; Copyright (C) 1991-2008 by Darek Mihocka. All Rights Reserved.
;; Branch Always Software. http://www.emulators.com/
;;
;; 11/30/2008 darekm        last update
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


    INCLUDE 68000.inc

    ECHO
    ECHO BUILD FLAGS...
    ECHO

IF DEBUG
    ECHO DEBUG   ON
ELSE
    ECHO DEBUG   off
ENDIF

    PUBLIC _cpi68K
    PUBLIC _lAccessAddr
    PUBLIC _DIVoverflow

    .DATA?

    align 4

; value of ESP when at main level of Go68000

espMain   DD ?

; access address when generating bus error exception frame

_lAccessAddr DD ?
_lFaultAddr  DD ?

; during address and bus errors, flags say which error it is

          DB ?
          DB ?

; TRUE if handling group 0 or group 1 exception

fException DB ?

_traced    DB ?   ; This byte is set when we dispatch through Step68000.
                  ; It means that it's ok to check T and gen trace excp.

_cntHit     DD ?
            DD ?
_cntMiss    DD ?
            DD ?

; temporary used during interrupts

_temp	DD ?

; bit vector of exceptions to pass to VM

rgfExcptHook DD 2 DUP (?)

vm_PollIntrpt  DD ?
vm_InitHW      DD ?

pfnDecodeInstr DD ?

pduop_current  DD ?


    .CONST

_cpi68K  DD 0
         DD _GetCpus68  ;; returns CPUs supported
         DD _GetCaps68  ;; returns interpreter capabilities
         DD _GetVers68  ;; returns intepreter version
         DD 0           ;; returns page size in bits
         DD _Init68000  ;; initialization routine
         DD _Reset68000 ;; processor reset routine
         DD _Go68000    ;; interpreter entry point
         DD _Step68000  ;; interpreter entry point (step)
         DD rgfExcptHook

pfnBUSADDR DD _xcptBUSADDR

    .CODE

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


    BigAlign

_GetCpus68 PROC
    mov   eax,cpu68000 OR cpu68010 OR cpu68020 OR cpu68030 OR cpu68040
    ret
_GetCpus68 ENDP


    BigAlign

_GetCaps68 PROC
    mov   eax,0
    ret
_GetCaps68 ENDP


    BigAlign

_GetVers68 PROC
    mov   eax,00085001h   ;; version 8.50.01
    ret
_GetVers68 ENDP


      BigAlign

externdef _PidefFrom68K:NEAR     ;; routine to find microcode for given instr
externdef _PidefFromPPC:NEAR     ;; routine to find microcode for given instr

_Init68000 PROC
    ;; int 3

    Enter68k

    mov   eax,[esp+40t]
    mov   vm_PollIntrpt,eax
    mov   eax,[esp+44t]
    mov   vm_InitHW,eax
    mov   eax,[esp+48t]
    mov   cpuCur,eax

    xor   eax,eax
    mov   fException,al

    Exit68k

    ret
_Init68000 ENDP


    BigAlign

_Reset68000 PROC
    ret
_Reset68000 ENDP

    BigAlign

_Step68000 PROC
    ;; int 3

    Enter68k

    ; initial CCR (must be done before PC in case of page table xcpt)

    UnpackFlags

    ; initial PC

    PC2Flat regPC

    ; remember ESP

    mov   espMain,esp

    mov   counter,1     ;; only execute one instruction

    ; go fetch first opcode through the good table
 ;; int 3

dispgood:
    mov   _traced,1
    jmp   _dispatch
_Step68000 ENDP


    BigAlign

_Go68000 PROC

; REGISTER USAGE
;
; ESI - 68000 program counter
; EDI - 68000 address space size - 1
; EBP - 68000 effective address, offset into large memory block
; EAX - scratch
; EBX - accumulator, opcode fetch
; ECX - countlo : counthi : SR : CCR
; EDX - pointer to globals

; INITIALIZE

    ;; int 3

    Enter68k

 	mov   eax,[esp+20t]
    mov   counter,eax

    ; initial CCR (must be done before PC in case of page table xcpt)

    UnpackFlags

    ; initial PC

    PC2Flat regPC

    ; remember ESP

    mov   espMain,esp

    mov   _traced,1

    jmp   PollInterrupts


exitFATALERR:
    mov   esp,espMain

    mov   eax,14t ; exit code 14 = bus error during exception processing
    jmp   exit


_xcptTRACE::
    ; dump stack frame for trace exception

    mov   esp,espMain

    Exception
    Vector 9      ; Trace Exception is vector $9 (address $24)


_xcptPRIVERR::
    ; privlidge violation exception

    mov   esp,espMain

    DecrementPCBy2 ; move back PC

    Exception 8
    Vector 8      ; Priviledge Violation is vector $8 (address $20)


_xcptTRAPV::
    ; TRAPV exception

    mov   esp,espMain

    Exception 7
    Vector 7      ; TRAPV Error is vector $7 (address $1C)


_xcptCHKERR::
    ; CHK exception

    mov   esp,espMain

    Exception 6
    Vector 6      ; CHK Error is vector $6 (address $18)


_xcptDIVZERO:
    ; division by zero error

    mov   esp,espMain

    Exception 5
    Vector 5      ; Division By Zero Error is vector $5 (address $14)


opsILLEGAL:
_xcptILLEGAL:
    ; illegal instruction error

    ; refetch the opcode and move back the PC

    mov   esp,espMain
    mov   fException,1

    RefetchOpcodeImm16zx    rDA
    DecrementPCBy2

    Exception 4
    Vector 4      ; Illegal Instruction is vector $4 (address $10)


    PUBLIC _xcptBUSADDR

_xcptBUSADDR:
    ; Address error - odd memory reference

IF REGBASE
    call  _GetPregs
    mov   rGR,eax
ENDIF
    mov   esp,espMain
    mov   rPC,regPCLv

    test  fException,0FF
    jnz   exitFATALERR

    mov   fException,1

    .if (cpuCur >= cpu68020)
        Exception 3, 24t, 10t

        ; SP+16.l = access address

    	mov   rDA,_lFaultAddr
        mov   rGA,regA7
        add   rGA,16t
        WriteMemAtEA %SIZE_L

        ; SP+8.w = special status register

    	mov   rDA,0
        mov   rGA,regA7
        add   rGA,8
        WriteMemAtEA %SIZE_W

        ; SP+10.w = R/W=1 ($0040)

    	mov   rDA,0040h
        mov   rGA,regA7
        add   rGA,10t
        WriteMemAtEA %SIZE_W

    .elseif (cpuCur == cpu68010)
        Exception 3, 50t, 8

    .else
        Exception 3

        sub   dword ptr regA7,8  ; Address Error exception frame has 4 more words
        mov   rGA,regA7

        ; SP+2.l = access address

        mov   rDA,_lFaultAddr
        add   rGA,2
        WriteMemAtEA %SIZE_L

        ; SP+0.w = (R/W=1, N/I=1, func code=6)

        mov   rDA16,0FFFEh
        sub   rGA,2
        WriteMemAtEA %SIZE_W

        ; SP+6.w = instruction register (just stuff $FFFF)

        mov   rDA16,0FFFFh
        add   rGA,6
        WriteMemAtEA %SIZE_W

        ; get PC written out by exception at SP+10.l

        add   rGA,4
        ReadMemAtEA %SIZE_L

        cmp   rDA,0400h          ; check if PC >= $400
        jc    @F
    
        mov   rDA16,0FFFEh
        sub   rGA,4
        WriteMemAtEA %SIZE_W

        mov   rDA16,0FFFFh       ; fetch instruction that caused bus exception
        and   rDA8L,0E0h
        or    rDA8L,016h         ; R/W=1, N/I=0, func code=1
        sub   rGA,6
        WriteMemAtEA %SIZE_W

@@:
    .endif

    Vector 3    ; Address Error is vector $3 (address $0C)


    PUBLIC _xcptBUSREAD

_xcptBUSREAD:
    ; Bus error on read

IF REGBASE
    call  _GetPregs
    mov   rGR,eax
ENDIF
    mov   esp,espMain
    mov   rPC,regPCLv

    test  fException,0FF
    jnz   exitFATALERR

    mov   fException,1

    .if (cpuCur >= cpu68020)
        Exception 2, 24t, 10t

        ; SP+16.l = access address

    	mov   rDA,_lFaultAddr
        mov   rGA,regA7
        add   rGA,16t
        WriteMemAtEA %SIZE_L

        ; SP+8.w = special status register

    	mov   rDA,0
        mov   rGA,regA7
        add   rGA,8
        WriteMemAtEA %SIZE_W

        ; SP+10.w = R/W=1 ($0040)

    	mov   rDA,0040h
        mov   rGA,regA7
        add   rGA,10t
        WriteMemAtEA %SIZE_W

    .elseif (cpuCur == cpu68010)
        Exception 2, 50t, 8

    .else
        Exception 2

        sub   dword ptr regA7,8  ; Bus Error exception frame has 4 more words

        ; SP+2.l = access address

    	mov   rDA,_lFaultAddr
        mov   rGA,regA7
        add   rGA,2
        WriteMemAtEA %SIZE_L

        ; SP+0.w = $FFFE (R/W=1, N/I=1, func code=6)

        mov   rDA16,0FFFEh
        mov   rGA,regA7
        WriteMemAtEA %SIZE_W

        ; SP+6.w = instruction register (just stuff $FFFF)

        mov   rDA16,0FFFFh
        mov   rGA,regA7
        add   rGA,6
        WriteMemAtEA %SIZE_W

    .endif

    Vector 2    ; Bus Error is vector $2 (address $08)


    PUBLIC _xcptBUSWRITE

_xcptBUSWRITE PROC

    ; Bus error on write

IF REGBASE
    call  _GetPregs
    mov   rGR,eax
ENDIF
    mov   esp,espMain
    mov   rPC,regPCLv

    test  fException,0FF
    jnz   exitFATALERR

    mov   fException,1

    .if (cpuCur >= cpu68020)
        Exception 2, 24t, 10t

        ; SP+16.l = access address

    	mov   rDA,_lFaultAddr
        mov   rGA,regA7
        add   rGA,16t
        WriteMemAtEA %SIZE_L

        ; SP+8.w = special status register

    	mov   rDA,0
        mov   rGA,regA7
        add   rGA,8
        WriteMemAtEA %SIZE_L

        ; SP+10.w = R/W=1 ($0040)

    	mov   rDA,0040h
        mov   rGA,regA7
        add   rGA,10t
        WriteMemAtEA %SIZE_L

    .elseif (cpuCur == cpu68010)
        Exception 2, 50t, 8

    .else
        Exception 2

        sub   dword ptr regA7,8  ; Bus Error exception frame has 4 more words

        ; SP+2.l = access address

    	mov   rDA,_lFaultAddr
        mov   rGA,regA7
        add   rGA,2
        WriteMemAtEA %SIZE_L

        ; SP+0.w = $FFEE (R/W=0, N/I=1, func code=6)

        mov   rDA16,0FFEEh
        sub   rGA,2
        WriteMemAtEA %SIZE_W

        ; SP+6.w = instruction register (just stuff $FFFF)

        mov   rDA16,0FFFFh
        add   rGA,6
        WriteMemAtEA %SIZE_W

    .endif

    Vector 2    ; Bus Error is vector $2 (address $08)

_xcptBUSWRITE ENDP

;;
;; Exception vector, cb, format
;;
;; EAX = vector
;; EBX = cb (additional bytes in addition to the usual 6/8 byte frame)
;; EBP = format*1000h + vector*4 (frame descriptor word)
;;
;; Generate an exception stack frame for exception #vector.
;;
;; First check rgfExcptHook to see if VM has hooked exception.
;; If it is hooked, call the VM. The VM will then return:
;;
;;   1 - generate the exception as normal
;;  -1 - generate the exception but rewind the PC by 2 bytes
;;   0 - exception was handled, punt it
;;
;; If punting the exception, jump to _dispatch.
;;
;; If generating the exception, create a 6 or 8 byte exception
;; frame with cb bytes of padding on top (to be filled in later).
;;

_exception PROC
    mov   vecnum,eax

    ; copy the access address to the fault address before it gets trashed

    push  _lAccessAddr
    pop   _lFaultAddr

    bt    rgfExcptHook,eax
    jnc   doexcpt

    PrepareForCall
    push  vecnum
    call  _TrapHook
    AfterCall 1

    test  eax,eax
    je    _clrxnfetch
    jns   doexcpt
    DecrementPCBy2	        ; backtrack the PC

doexcpt:

    test  regSR,02000h

    mov   vecnum,rGA        ; format:vector

    mov   rGA,regA7

    jne   @F

    ;; switching from user mode to supervisor mode
    mov   regUSP,rGA
    mov   rGA,regSSP

@@:

    .if (cpuCur > cpu68000)
        sub   rGA,2       ; make room for exception type
    .endif
    sub   rGA,6           ; room for the usual 6 byte exception frame
    sub   rGA,rDA         ; room for extra bytes in frame
    mov   regA7,rGA

    .if (cpuCur > cpu68000)
        ;; Fill in the frame type and vector #

        mov     rDA16,word ptr vecnum
        add     rGA,6
        WriteMemAtEA %SIZE_W
    .endif

    ; snapshot the temporary flags state to CCR but keep temp state valid

    PackFlags

    Flat2PC rDA

    ; push the return address

    mov     rGA,regA7
    add     rGA,2
    WriteMemAtEA %SIZE_L

    ; push the SR

    mov     rDA16,regSR
    sub     rGA,2
    WriteMemAtEA %SIZE_W

    ;; set S bit and clear T bits
    or      rDA16,02000h
    and     rDA16,03FFFh
    mov     regSR,rDA16

    ret

_exception ENDP

_vector PROC

    .if (cpuCur > cpu68000)
        ;; Fill in the true vector # in the exception frame

        mov   rGA,regA7
        add   rGA,6
        ReadMemAtEA %SIZE_W

        or    rDA16,temp16
        WriteMemAtEA %SIZE_W
    .endif

    mov   rGA,dword ptr regVBR
    add   rGA,temp32

    ReadMemAtEA %SIZE_L
    PC2Flat rDA

_clrxnfetch:
    mov   esp,espMain               ; clear any nested return addresses
    mov   fException,0              ; clear exception processing flag
    jmp   _dispatch

_vector ENDP

exitBREAK:
    mov   eax,1  ; exit code 1 = break
    jmp   exit


    BigAlign

exit:
    mov   esp,espMain
    Flat2PC
    mov   regPC,rGA

    PackFlags

    Exit68k

    ret

    .CONST

    align 4

dispfullx LABEL DWORD
    DD    full0
    DD    full1
    DD    full2
    DD    full3
    DD    _xcptILLEGAL
    DD    full5
    DD    full6
    DD    full7


    .CODE

externdef @pvGetHashedPC@4:NEAR

    BigAlign

_dispatch PROC

IF DEBUG
    PrepareForCall
    push   rPC
    call  _StepHook
    AfterCall 1

    test  rPC,1
    jz    @F

    int   3
@@:
ENDIF

    ; Get everything in Sync

    Flat2PC

    ; intepret with a known guest PC in rGA
    ; also entered from any instruction that changes the guest PC

_dispatch_newPC::
    mov   eax,[rPC]
    mov   esp,espMain

    ; check if rPC is already pointing to correct duop

    cmp   rGA,[rPC+4t]
    je    good

    ; nope, look it up

    mov   ecx,rGA
    call  @pvGetHashedPC@4

    ; See if this is an uninitialized entry

    cmp   eax,offset _fetch_empty
    je    empty

    ; Check if out instructions in this time span
    ; OK to underflow

good::
    sub   counter,1
    js    exitNORM

    ; check if a breakpoint is set and go to slower dispatcher

    cmp   _lBreakPoint,0
    jne   call_table_slow

    ; Dispatch via corresponding entry in call table

call_table_fast:

    ; This is a software pipelined dispatch loop which tries
    ; to give the CALL EAX as much time to set up and predict
    ;
    ; ECX = pointer to pduop destriptor
    ; EDX = first opcode for this descriptor
    ; EAX = handler for this opcode

    mov   edx,dword ptr [rPC+8]
    mov   ecx,rPC
    IncrementPCBy2
    call  eax

    sub   counter,1                 ; decrement instr count but don't check

    ; EAX now contains the pointer to the next opcode handler

    jmp   call_table_fast

call_table_slow:

    ; Same dispatch as fast loop, but with breakpoint check

    mov   edx,dword ptr [rPC+8]
    mov   ecx,rPC
    IncrementPCBy2
    call  eax

    sub   counter,1                 ; decrement instr count but don't check

    Flat2PC
    cmp   _lBreakPoint,rGA
    je    exitBREAK

    jmp   call_table_slow


empty:
    ; Have to pre-increment the PC as if dispatched from call table

    IncrementPCBy2
    call  _fetch_empty

    Flat2PC
    jmp   _dispatch_newPC


_dispatch ENDP


externdef @pvAddToHashedPCs@8:NEAR

    BigAlign

_fetch_empty PROC
    ; clear the write TLB entry corresponding to this PC

    DecrementPCBy2

    Flat2PC
    TransHash
    xor   rGA,dword ptr [memtlbRW+edx*8]
    test  rGA,dwBaseMask
    jne   @F
    not   dword ptr [memtlbRW+edx*8]
@@:

IF PERFCOUNT
    add   _cntCodeMiss,1
ENDIF

try_again:
    Flat2PC                         ; this syncs rPC with regPC
    PC2Flat                         ; and normalizes duop array overflow

    Imm16zxNoIncPC                  ; get instruction opcode
    mov   edx,dword ptr [opcodes+eax*4]

    mov   ecx,regPC
    mov [rPC+8],edx
    call  @pvAddToHashedPCs@8

    ; do a sanity check, and if EAX is in sync with duop entry, jump in!

    cmp   eax,dword ptr [rPC]
    je    good

    jmp   try_again                 ; can happen when wrap around call table
    int   3                         ; if this triggers, major trouble!

_fetch_empty ENDP


exitNORM:
exit2:
    mov   esp,espMain               ; fix stack pointer just in case

    Flat2PC
    mov   regPC,rGA

    PackFlags

    Exit68k

    xor   eax,eax ; exit code 0 = normal exit
    ret


;
; DecodeExtWordRn (base register)
;

    BigAlign

;; given the (An,Rn) extension word in EAX, generate EA in EBP
;; The base register (An or PC) is on the stack, so this routine
;; MUST exit with a RET 4.

_DecodeExtWordRn:
    push  rGA
    mov   rGA,eax
    shr   rGA,12t   ;; sets C if long, clears if short
    mov   rGA,[regD0+rGA*4]
    jc    @F
    movsx rGA,rGA16
@@:

    ;; EBP contains the index register
    ;; now scale it

    .if (cpuCur >= cpu68020)
        .if (ah & 4)
            add   rGA,rGA
            add   rGA,rGA
        .endif

        .if (ah & 2)
            add   rGA,rGA
        .endif

        .if (ah & 1)
            jmp   fullx
        .endif
    .endif

    ;; EBP now contains index register * scale

    ;; short extension word, just add 8-bit signed displacement

    movsx eax,al
    add   eax,rGA           ;; 8-bit signed displacement
    pop   rGA
    add   rGA,eax           ;; add base register
    ret


    BigAlign

_DecodeExtWordAn:
    push  eax
    Imm16zx

    call  _DecodeExtWordRn
    pop   eax

    ret


    BigAlign

_DecodeExtWordPC:
    push  eax
    Imm16zx

    Flat2PC
    sub   rGA,2     ;; PC is 2 bytes too far
    call  _DecodeExtWordRn
    pop   eax

    ret


    BigAlign

fullx:
; int 3

    xor   edx,edx

    .if (al & 020h)
        ;; there is a base displacement

        .if (al & 010h)
            ;; 32-bit displacement

            push    eax
            Imm32   edx
            pop     eax
        .else
            push    eax
            Imm16sx edx
            pop     eax
        .endif
    .endif

    add    al,al                ;; if (al & 080h)
    jc     @F                   ;; BS bit, base register is 0
;;  .else
        add   edx,[esp]         ;; base register
;;  .endif

@@:
    add    al,al                ;; .if (al & 040h)
    jnc    @F

        xor   rGA,rGA           ;; IS bit, doh!
;;  .endif

@@:
    and   eax,28t               ;; mask what was the lowest 7 bits (*4)

    ;; at this point, EBP = index or 0, EDI/stack = base or 0

    xchg  rGA,edx

    ;; and now stack = index or 0, EBP = base or 0

    jmp   dword ptr [dispfullx+eax]


    BigAlign
full0:
    add   rGA,edx   
    pop   eax
    ret

full1:
    mov   [esp],rDA
    add   rGA,edx   
    ReadMemAtEA %SIZE_L
    mov   rGA,rDA
    pop   rDA
    ret

full2:
    mov   [esp],rDA
    add   rGA,edx   
    ReadMemAtEA %SIZE_L
    mov   rGA,rDA
    Imm16sx
    pop   rDA
    add   rGA,eax
    ret

full3:
    mov   [esp],rDA
    add   rGA,edx   
    ReadMemAtEA %SIZE_L
    mov   rGA,rDA
    Imm32
    pop   rDA
    add   rGA,eax
    ret

    align 4t
full5:
    mov   [esp],edx
    push  rDA
    ReadMemAtEA %SIZE_L
    mov   rGA,[esp+4]
    add   rGA,rDA
    mov   rDA,[esp]
    add   esp,8
    ret

    align 4t

full6:
    mov   [esp],edx
    push  rDA
    ReadMemAtEA %SIZE_L
    mov   rGA,rDA
    pop   rDA
    Imm16sx
    add   rGA,eax
    add   rGA,[esp]
    pop   eax
    ret

    align 4t

full7:
    mov   [esp],edx
    push  rDA
    ReadMemAtEA %SIZE_L
    mov   rGA,rDA
    pop   rDA
    Imm32
    add   rGA,eax
    add   rGA,[esp]
    pop   eax
    ret

_Go68000 ENDP


;;
;; DIVS and DIVU don't generate overflow exceptions.
;; They simply set the V flag and go to the next instruction.
;;
;; However, the x86 generates an integer overflow exception,
;; so this is the target from the exception handler.

_DIVoverflow:
    or    flagsVC,2   ;; quotient > 16 bits. Overflow.
    FastFetch


IF 1

;;
;;	Hardware WRITE routines
;;
;;  Assumes 32-bit effective address is in EBP. Value to write is in EBX.
;;	Only byte and word accesses need handling. Other modes call byte or word.
;;	Each routine exits to the caller on success or ends up at _xcptBUSWRITE.
;;

    BigAlign

WriteEA0::
    TranslatLookupForWrite 1
    jne   WriteLU0

    TranslateAddr
    WriteMemFlat %SIZE_B
    ret

WriteLU0:
    WriteMemAtEA %SIZE_B, 1
    ret


    ; word access

    BigAlign

WriteEA1::
    TranslatLookupForWrite 2
    jne   WriteLU1

    TranslateAddr
    WriteMemFlat %SIZE_W
    ret

WriteLU1:
    WriteMemAtEA %SIZE_W, 1
    ret


    BigAlign

WriteEA2:
    TranslatLookupForWrite 4
    jne   WriteHW2

    TranslateAddr
    WriteMemFlat %SIZE_L
    ret

WriteHW2:
    rol     rDA,16t
    call    WriteEA1
    rol     rDA,16t
    add     rGA,2
    call    WriteEA1
    sub     rGA,2
    ret


    ; peripheral word access = 2 byte accesses

    BigAlign

WriteEA4:
    rol     rDA16,8
    call    WriteEA0
    add     rGA,2
    rol     rDA16,8
    call    WriteEA0
    sub     rGA,2
    ret


    ; peripheral long access = 4 byte accesses

    BigAlign

WriteEA5:
    rol     rDA,8
    call    WriteEA0
    add     rGA,2
    rol     rDA,8
    call    WriteEA0
    add     rGA,2
    rol     rDA,8
    call    WriteEA0
    add     rGA,2
    rol     rDA,8
    call    WriteEA0
    sub     rGA,6
    ret

ENDIF

IF 1

;;
;;	CPU READ routines
;;
;;  Input:
;;      32-bit effective address is in rGA and must be preserved.
;;
;;  Output:
;;      Zero extended read data in rDA.
;;
;;  Uses: EAX EDX EFLAGS, preserves all other host registers
;;
;;  Byte and word accesses try to map effective address to physical address and
;;  failing a mapping to readabable RAM/ROM will call the hardware handler.
;;
;;  Larger accesses are treated as composites of byte and word accesses.
;;
;;	Each routine exits to the caller on success or ends up at _xcptBUSREAD.
;;

    ;
    ; Byte access
    ;

    BigAlign

    ; First try a fast TLB lookup

ReadEA0::
    TranslatLookupForRead 1
    jne   ReadLU0

    TranslateAddr
    ReadMemFlat %SIZE_B
    ret

    BigAlign

    ; Now try a slower lookup which may involve a page table walk

ReadLU0:
    ReadMemAtEA %SIZE_B, 1
    ret


    ;
    ; word access
    ;

    BigAlign

ReadEA1::
    TranslatLookupForRead 2
    jne   ReadLU1

    TranslateAddr
    ReadMemFlat %SIZE_w
    ret

    BigAlign

ReadLU1:
    ReadMemAtEA %SIZE_W, 1
    ret


    ; long access = 2 word accesses

    BigAlign

ReadEA2:
    TranslatLookupForRead 4
    jne   ReadHW2

    TranslateAddr
    ReadMemFlat %SIZE_L
    ret

    BigAlign

ReadHW2:
    call    ReadEA1
    shl     rDA,16t
    push    rDA
    add     rGA,2
    call    ReadEA1
    sub     rGA,2
    pop     edx
    or      rDA,edx
    ret


    ; peripheral word access = 2 byte accesses

    BigAlign

ReadEA4:
    call    ReadEA0
    shl     rDA,8
    push    rDA
    add     rGA,2
    call    ReadEA0
    sub     rGA,2
    pop     edx
    or      rDA,edx
    ret


    ; peripheral long access = 4 byte accesses

    BigAlign

ReadEA5:
    call    ReadEA0
    shl     rDA,8
    push    rDA
    add     rGA,2
    call    ReadEA0
    pop     edx
    or      rDA,edx
    shl     rDA,8
    push    rDA
    add     rGA,2
    call    ReadEA0
    pop     edx
    or      rDA,edx
    shl     rDA,8
    push    rDA
    add     rGA,2
    call    ReadEA0
    pop     edx
    or      rDA,edx
    sub     rGA,6
    ret

ENDIF


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;                                                                        ;;
;;  Low level routines called from ASM                                    ;;
;;                                                                        ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;
;;  putsr
;;
;;  Put a new value into the SR and check if the Trace bit got turned on
;;

externdef _fStepMode:BYTE

    BigAlign

_putsr:
    mov   regSR,rDA16
    UnpackFlags

    test  rDA16,2000h       ; check new S bit
    jnz   @F

    ;; switching from supervisor to user mode
    mov   eax,regA7
    mov   rDA,regUSP
    mov   regSSP,eax
    mov   regA7,rDA

@@:
    test  regSR,08000h      ; check new T bit
    jz    @F

    cmp   _fStepMode,0      ; are we already stepping?
    jnz   @F

    or    _fStepMode,2      ; nope
    mov   _traced,0

@@:
    test  regSR,08000h      ; check new T bit again
    jnz   @F

    and   _fStepMode,0FDh   ; T bit is clear, so clear the bit in fStepMode
    mov   _traced,0

@@:
    jmp   PollInterrupts  


;;
;;  PollInterrupts
;;
;;  Called by PutSR or after any instruction to check for pending interrupts.
;;  Jumps to _dispatch routine after setting up any necessary exceptions.
;;

    BigAlign

PollInterrupts:
    movzx rDA,regSR
    and   rDA16,0700h
    cmp   rDA16,0600h
    jae   _dispatch      ;; if IPL >= 6, interrupts masked

    PrepareForCall
    call  dword ptr vm_PollIntrpt  ;; returns vector number if any pending
    AfterCall 0,0,0

    ;; test for interrupt
;	test  eax,eax       ;; AfterCall does the test
    jz    _dispatch

    mov   _temp,eax      ;; save away vector number

    Exception

    mov   eax,_temp
    shr   eax,24t
    SetIPL al

    mov   eax,_temp
    and   eax,00FFFFFFh
    mov   temp32,eax
    Vector


    END

