
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; X6502.ASM
;;
;; 6502 interpreter engine for Atari 800 emulator
;;
;; Copyright (C) 1986-2012 by Darek Mihocka. All Rights Reserved.
;; Branch Always Software. http://www.emulators.com/
;;
;; 08/18/2012   darekm      optimize for 32-bit
;; 11/30/2008   darekm      open source release
;; 10/10/1999   darekm      last update
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

IFDEF HWIN32
    .486
    .MODEL FLAT,C
    OPTION NOSCOPED
    OPTION NOOLDMACROS

_si TEXTEQU <esi>
_di TEXTEQU <edi>
_bp TEXTEQU <ebp>
_ax TEXTEQU <eax>
_bx TEXTEQU <ebx>
_cx TEXTEQU <ecx>
_dx TEXTEQU <edx>
_mem TEXTEQU <rgbMem>
ENDIF

IFDEF HDOS32
    .486
    DOSSEG
    .MODEL FLAT
    OPTION NOSCOPED
    OPTION NOOLDMACROS

_si TEXTEQU <esi>
_di TEXTEQU <edi>
_bp TEXTEQU <ebp>
_ax TEXTEQU <eax>
_bx TEXTEQU <ebx>
_cx TEXTEQU <ecx>
_dx TEXTEQU <edx>
_mem TEXTEQU <rgbMem>
ENDIF

IFDEF HDOS16
    DOSSEG
    .MODEL   compact,c
    .286

_si TEXTEQU <si>
_di TEXTEQU <di>
_bp TEXTEQU <bp>
_ax TEXTEQU <ax>
_bx TEXTEQU <bx>
_cx TEXTEQU <cx>
_dx TEXTEQU <dx>
_mem TEXTEQU <>
ENDIF

    PUBLIC fTrace, fSIO, mdXLXE, cntTick, ramtop
    PUBLIC regPC, regSP, regA, regX, regY, regP, op00
    PUBLIC wLeft

    PUBLIC rgbRainbow

    PUBLIC rgbMem6502, pfnPokeB

    PUBLIC jump_tab

    EXTERNDEF SwapMem:NEAR


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Constants
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; exit codes from interpreter
EXIT_BREAK      EQU  0
EXIT_INVALID    EQU  1
EXIT_ALTKEY     EQU  2

; status bits of the P register: (6502)  NV_BDIZC 

NBIT EQU 080h
VBIT EQU 040h
BBIT EQU 010h
DBIT EQU 008h
IBIT EQU 004h
ZBIT EQU 002h
CBIT EQU 001h


; register usage
;
; AX      scratch
; BX      pointer to op00 + offset into opcodes
; CL      Y register
; CH      Z flag
; DL      A register
; DH      X register
; DI      EA register
; SI      6502 PC
; BP      tick counter
; DS      6502 address space
; ES
; SS      DATA segment (since SS == DS)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Macro definitions
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

opinit MACRO
cbFetch = 1    ; assume a one or 2 byte instruction
    ENDM

unused TEXTEQU <opEA>

dispatch MACRO

   IFNDEF HDOS16
    xor   edi,edi
   ENDIF

   IF cbFetch EQ 1
    mov   bl,byte ptr [_si]
    inc   _si
   ENDIF

    dec   _bp
   IFDEF HDOS16
    mov   di,bx
   ENDIF
    je    done
   IFDEF HDOS16
    jmp   word ptr [di+bx]
   ELSE
    jmp   dword ptr [edi+ebx*4]
   ENDIF
    ENDM

setorg MACRO label,count
    align 2
    ENDM

; Effective Address calculation macros (EA usually ends up in DI)

EAimm EQU 0
EAzp  EQU 1
EAabs EQU 2

EA_imm MACRO
mdEA = EAimm
cbFetch = 2
    movzx _ax,word ptr [_si]
    add   _si,2         ; AL = operand    AH = next opcode
    mov   bl,ah
    ENDM

EA_zp MACRO
mdEA = EAzp
cbFetch = 2
    movzx _ax,word ptr [_si]
    add   _si,2         ; AL = operand    AH = next opcode
    mov   bl,ah
    movzx _ax,al
    movzx _di,al
    ENDM

EA_zpX MACRO            ; zp,X
mdEA = EAzp
cbFetch = 2
    movzx _ax,word ptr [_si]
    add   _si,2         ; EA = [pc].b
    mov   bl,ah
    movzx _ax,al
    add   al,dh         ; EA += X.b
    mov   _di,_ax
    ENDM

EA_zpY MACRO            ; zp,Y
mdEA = EAzp
cbFetch = 2
    movzx _ax,word ptr [_si]
    add   _si,2         ; EA = [pc].b
    mov   bl,ah
    movzx _ax,al
    add   al,cl         ; EA += Y.b
    mov   _di,_ax
    ENDM

EA_zpXind MACRO         ; (zp,X)
mdEA = EAabs
    movzx _ax,byte ptr [_si]       ; EA = [pc].b
    inc   _si
    add   al,dh         ; EA = [pc].b + x.b
    mov   _di,_ax
    movzx _di,word ptr _mem[_di]   ; EA = [[pc].b + x.b]
    ENDM

EA_zpYind MACRO         ; (zp),Y
mdEA = EAabs
    movzx _ax,byte ptr [_si]       ; EA = [pc].b
    inc   _si
    mov   _di,_ax
    movzx _di,word ptr _mem[_di]       ; EA = [[pc].b]
    xor   _ax,_ax
    mov   al,cl
    add   di,ax         ; EA = [pc].b] + y
    ENDM

EA_abs MACRO
mdEA = EAabs
    movzx _ax,word ptr [_si]
    add   _si,2
    mov   _di,_ax
    ENDM

EA_absX MACRO
mdEA = EAabs
    movzx _ax,word ptr [_si]
    add   _si,2         ; EA = [pc]+X
    add   al,dh
    adc   ah,0
    mov   _di,_ax
    ENDM

EA_absY MACRO
mdEA = EAabs
    movzx _ax,word ptr [_si]
    add   _si,2         ; EA = [pc]+Y
    add   al,cl
    adc   ah,0
    mov   _di,_ax
    ENDM


; Common opcode macros. Requires that EA is already in DI

ORA_com MACRO
   IF mdEA EQ EAimm
    or    dl,al
   ELSE
    or    dl,byte ptr _mem[_di]
   ENDIF
   update_NZ dl
   ENDM

EOR_com MACRO
   IF mdEA EQ EAimm
    xor   dl,al
   ELSE
    xor   dl,byte ptr _mem[_di]
   ENDIF
   update_NZ dl
   ENDM

AND_com MACRO
   IF mdEA EQ EAimm
    and   dl,al
   ELSE
    and   dl,byte ptr _mem[_di]
   ENDIF
   update_NZ dl
   ENDM

LD_com MACRO reg        ; used by LDA, LDX, LDY
   IF mdEA EQ EAimm
    mov   reg,al
   ELSE
    mov   reg,byte ptr _mem[_di]
   ENDIF
   update_NZ reg
   ENDM

ST_com MACRO reg        ; used by STA, STX, STY
    WriteService reg
    ENDM

CMP_com MACRO reg
    mov   ch,reg        ; compare must not affect A, X, or Y
   IF mdEA EQ EAimm
    sub   ch,al
   ELSE
    sub   ch,byte ptr _mem[_di]       ; Z flag
   ENDIF
    mov   srN,ch        ; N flag
    update_CC
    ENDM

SBC_com MACRO
    mov   ah,srC
    not   ah           ; reverse carry bit
    add   ah,ah        ; set carry flag
   IF mdEA EQ EAimm
    sbb   dl,al
   ELSE
    sbb   dl,byte ptr _mem[_di]      ; subtract operand from A
   ENDIF
    update_NZ dl
    update_VCC
    ENDM

SBC_dec MACRO
    mov   ah,srC
    not   ah           ; reverse carry bit
    add   ah,ah        ; set carry flag
   IF mdEA EQ EAimm
    sbb   dl,al
   ELSE
    sbb   dl,byte ptr _mem[_di]      ; subtract operand from A
   ENDIF
    mov   al,dl
    das
    mov   dl,al
    update_NZ dl
    update_VCC
    ENDM

ADC_com MACRO
    mov   ah,srC
    add   ah,ah        ; set carry flag
   IF mdEA EQ EAimm
    adc   dl,al
   ELSE
    adc   dl,byte ptr _mem[_di]      ; add operand to A
   ENDIF
    update_NZ dl
    update_VC
    ENDM
    
ADC_dec MACRO
    mov   ah,srC
    add   ah,ah        ; set carry flag
   IF mdEA EQ EAimm
    adc   dl,al
   ELSE
    adc   dl,byte ptr _mem[_di]      ; add operand to A
   ENDIF
    mov   al,dl
    daa
    mov   dl,al
    update_NZ dl
    update_VC
    ENDM

ASLA_com MACRO
    add   dl,dl        ; shift A left
    update_NZ dl
    update_C
    ENDM
    
LSRA_com MACRO
    shr   dl,1         ; shift A right
    update_NZ dl
    update_C
    ENDM
    
ASL_com MACRO
    mov   ch,byte ptr _mem[_di]
    add   ch,ch        ; shift value left
    mov   srN,ch
    update_C
    WriteService ch
    ENDM
    
LSR_com MACRO
    mov   ch,byte ptr _mem[_di]
    shr   ch,1         ; shift value right
    mov   srN,ch
    update_C
    WriteService ch
    ENDM
    
ROL_com MACRO
    mov   ah,srC
    add   ah,ah        ; set carry flag
    mov   ch,byte ptr _mem[_di]
    rcl   ch,1         ; rotate value left
    mov   srN,ch
    update_C
    WriteService ch
    ENDM
    
ROR_com MACRO
    mov   ah,srC
    add   ah,ah        ; set carry flag
    mov   ch,byte ptr _mem[_di]
    rcr   ch,1         ; rotate value right
    mov   srN,ch
    update_C
    WriteService ch
    ENDM
    
INC_com MACRO
    mov   ch,byte ptr _mem[_di]
    inc   ch           ; increment value
    mov   srN,ch
    WriteService ch
    ENDM
    
DEC_com MACRO
    mov   ch,byte ptr _mem[_di]
    dec   ch           ; decrement value
    mov   srN,ch
    WriteService ch
    ENDM
    
DEX_com MACRO
    dec   dh
    update_NZ dh
    ENDM

DEY_com MACRO
    dec   cl
    update_NZ cl
    ENDM

INX_com MACRO
    inc   dh
    update_NZ dh
    ENDM

INY_com MACRO
    inc   cl
    update_NZ cl
    ENDM

; Macro to update 6502 Z and N flags (used by ORA, AND, LDA, INX, DEY, etc)

update_NZ MACRO reg
    mov   ch,reg        ; Z flag
    mov   srN,reg       ; N flag
    ENDM

; Macro to update 6502 C flag (used by ADC, SBC, ASL, LSR, etc)

update_C MACRO
    sbb   al,al         ; set to 0 or -1 depending on value of C
    mov   srC,al        ; C flag
    ENDM


; Macro to update 6502 C flag (used by SBC and CMP)

update_CC MACRO
    sbb   al,al         ; set to 0 or -1 depending on value of C
    not   al
    mov   srC,al        ; C flag (opposite)
    ENDM


; Macro to update 6502 V and C flags (used by ADC)

update_VC MACRO
    seto  al            ; isolate V flag
    mov   srV,al        ; V flag
    sbb   al,al         ; set to 0 or -1 depending on value of C
    mov   srC,al        ; C flag
    ENDM


; Macro to update 6502 V and C flags (used by SBC)

update_VCC MACRO
    seto  al            ; isolate V flag
    mov   srV,al        ; V flag
    sbb   al,al         ; set to 0 or -1 depending on value of C
    not   al
    mov   srC,al        ; C flag
    ENDM


; Write byte macros (used by STA, STX, STY, INC, ASL, LSR, etc)

WriteService MACRO reg

  IF mdEA EQ EAabs
    cmp   di,ramtop     ; is EA in RAM?
    mov   al,reg
    jae   writehw       ; no, it's not RAM
  ;; UNCOMMENT THE NEXT LINE TO LOG ALL ABSOLUTE MEMORY ACCESSES INCLUDING RAM
  ; jmp   writehw       ; no, it's not RAM
  ENDIF
    mov   _mem[_di],reg      ; write to RAM

    ENDM


; used by Bcc opcodes to skip byte and fetch when condition succeeds

AddOffset MACRO
   IFDEF HDOS16
    mov   al,byte ptr [_si]
    cbw                     ; sign extend
   ELSE
    movsx _ax,byte ptr [_si]
   ENDIF
    add   _si,_ax           ; add to PC
    ENDM

; used by Bcc opcodes to skip byte and fetch when condition fails

SkipByte MACRO
    mov   bl,byte ptr [_si+1]
    add   _si,2
    dec   _bp
   IFDEF HDOS16
    movzx _di,bx
   ENDIF
    je    done
   IFDEF HDOS16
    jmp   word ptr [di+bx]
   ELSE
    jmp   dword ptr [edi+ebx*4]
   ENDIF
    ENDM

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; 6502 memory segment
;;
;; Linker doesn't allow for 65536 byte segments, so we make 2
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

   IFDEF HDOS16
    .FARDATA? memseg
   ELSE
    .DATA?
   ENDIF

rgbMem6502 LABEL BYTE
rgbMem LABEL BYTE
    DB 32768 DUP (?)    ; 32K of RAM

mem8000 LABEL BYTE
    DB 8192 DUP (?)     ; 8K of RAM or 8K cartridge

memA000 LABEL BYTE
    DB 8192 DUP (?)     ; 8K of RAM or 8K cartridge

memC000 LABEL BYTE
    DB 4096 DUP (?)     ; 4K unused block

memD000 LABEL BYTE      ; CTIA/GTIA chip

MXPF LABEL DWORD
M0PF    DB ?
M1PF    DB ?
M2PF    DB ?
M3PF    DB ?
PXPF LABEL DWORD
P0PF    DB ?
P1PF    DB ?
P2PF    DB ?
P3PF    DB ?
MXPL LABEL DWORD
M0PL    DB ?
M1PL    DB ?
M2PL    DB ?
M3PL    DB ?
PXPL LABEL DWORD
P0PL    DB ?
P1PL    DB ?
P2PL    DB ?
P3PL    DB ?
TRIG0   DB ?
TRIG1   DB ?
TRIG2   DB ?
TRIG3   DB ?
PAL     DB ?
        DB ?
        DB ?
        DB ?
        DB ?
        DB ?
        DB ?
        DB ?
        DB ?
        DB ?
        DB ?
CONSOL    DB ?

    DB 224 DUP (?)      ; remainder of CTIA/GTIA address space

memD100 LABEL BYTE

; the 8 status bits must be together and in the same order as in the 6502
srN     DB ?
srV     DB ?
srB     DB ?
srD     DB ?
srI     DB ?
srZ     DB ?  ; unused, in CH
srC     DB ?
        DB ?  ; padding

        DB 8 DUP (?)

; write registers

memD110 LABEL BYTE
writeGTIA LABEL BYTE

HPOSPX  LABEL DWORD
HPOSP0    DB ?
HPOSP1    DB ?
HPOSP2    DB ?
HPOSP3    DB ?
HPOSMX  LABEL DWORD
HPOSM0    DB ?
HPOSM1    DB ?
HPOSM2    DB ?
HPOSM3    DB ?
SIZEPX  LABEL DWORD
SIZEP0    DB ?
SIZEP1    DB ?
SIZEP2    DB ?
SIZEP3    DB ?
SIZEM    DB ?
GRAFPX  LABEL DWORD
GRAFP0    DB ?
GRAFP1    DB ?
GRAFP2    DB ?
GRAFP3    DB ?
GRAFM    DB ?
COLPMX  LABEL DWORD
COLPM0    DB ?
COLPM1    DB ?
COLPM2    DB ?
COLPM3    DB ?
COLPFX  LABEL DWORD
COLPF0    DB ?
COLPF1    DB ?
COLPF2    DB ?
COLPF3    DB ?
COLBK    DB ?
PRIOR    DB ?
VDELAY    DB ?
GRACTL    DB ?
HITCLR    DB ?
wCONSOL    DB ?

memD130 LABEL BYTE
writePOKEY LABEL BYTE

AUDF1    DB ?
AUDC1    DB ?
AUDF2    DB ?
AUDC2    DB ?
AUDF3    DB ?
AUDC3    DB ?
AUDF4    DB ?
AUDC4    DB ?
AUDCTL    DB ?
STIMER    DB ?
SKRES    DB ?
POTGO    DB ?
        DB ?
SEROUT    DB ?
IRQEN    DB ?
SKCTLS    DB ?

memD140 LABEL BYTE
writePIA LABEL BYTE

wPORTA    DB ?
wPORTB    DB ?
wPACTL    DB ?
wPBCTL    DB ?

wPADDIR DB ?
wPBDDIR DB ?
wPADATA DB ?
wPBDATA DB ?
rPADATA DB ?
rPBDATA DB ?

        DB 6 DUP (?)

memD150 LABEL BYTE
writeANTIC LABEL BYTE

DMACTL    DB ?
CHACTL    DB ?
DLISTL    DB ?
DLISTH    DB ?
HSCROL    DB ?
VSCROL    DB ?
        DB ?
PMBASE    DB ?
        DB ?
CHBASE    DB ?
WSYNC    DB ?
        DB ?
        DB ?
        DB ?
NMIEN    DB ?
NMIRES    DB ?

DLPC LABEL WORD
DLPCL   DB ?
DLPCH   DB ?

    DB 158 DUP (?)      ; remainder of unused $D1xx range

memD200 LABEL BYTE        ; POKEY chip

POT0    DB ?
POT1    DB ?
POT2    DB ?
POT3    DB ?
POT4    DB ?
POT5    DB ?
POT6    DB ?
POT7    DB ?
ALLPOT    DB ?
KBCODE    DB ?
RANDOM    DB ?
        DB ?
        DB ?
        DB ?
IRQST    DB ?
SKSTAT    DB ?

    DB 240 DUP (?)      ; remainder of $D2xx range

memD300 LABEL BYTE        ; unused

PORTA    DB ?
PORTB    DB ?
PACTL    DB ?
PBCTL    DB ?

    DB 252 DUP (?)      ; remainder of $D400 range

memD400 LABEL BYTE        ; ANTIC chip

        DB ?
        DB ?
        DB ?
        DB ?
        DB ?
        DB ?
        DB ?
        DB ?
        DB ?
        DB ?
        DB ?
VCOUNT    DB ?
PENH    DB ?
PENV    DB ?
        DB ?
NMIST    DB ?

    DB 240 DUP (?)      ; remainder of $D4xx range

memD500 LABEL BYTE
    DB 254 DUP (?)      ; unused page - 2 bytes for ramtop
ramtop  DW ?            ; size of RAM ($A000 or $C000)

IFNDEF HDOS16
    DB 512 DUP (?)
ELSE
jump_tab LABEL BYTE
    DW    op00
    DW    op01
    DW    unused
    DW    unused
    DW    unused
    DW    op05
    DW    op06
    DW    unused
    DW    op08
    DW    op09
    DW    op0A
    DW    unused
    DW    unused
    DW    op0D
    DW    op0E
    DW    unused
    DW    op10
    DW    op11
    DW    unused
    DW    unused
    DW    unused
    DW    op15
    DW    op16
    DW    unused
    DW    op18
    DW    op19
    DW    unused
    DW    unused
    DW    unused
    DW    op1D
    DW    op1E
    DW    unused
    DW    op20
    DW    op21
    DW    unused
    DW    unused
    DW    op24
    DW    op25
    DW    op26
    DW    unused
    DW    op28
    DW    op29
    DW    op2A
    DW    unused
    DW    op2C
    DW    op2D
    DW    op2E
    DW    unused
    DW    op30
    DW    op31
    DW    unused
    DW    unused
    DW    unused
    DW    op35
    DW    op36
    DW    unused
    DW    op38
    DW    op39
    DW    unused
    DW    unused
    DW    unused
    DW    op3D
    DW    op3E
    DW    unused
    DW    op40
    DW    op41
    DW    unused
    DW    unused
    DW    unused
    DW    op45
    DW    op46
    DW    unused
    DW    op48
    DW    op49
    DW    op4A
    DW    unused
    DW    op4C
    DW    op4D
    DW    op4E
    DW    unused
    DW    op50
    DW    op51
    DW    unused
    DW    unused
    DW    unused
    DW    op55
    DW    op56
    DW    unused
    DW    op58
    DW    op59
    DW    unused
    DW    unused
    DW    unused
    DW    op5D
    DW    op5E
    DW    unused
    DW    op60
    DW    op61
    DW    unused
    DW    unused
    DW    unused
    DW    op65
    DW    op66
    DW    unused
    DW    op68
    DW    op69
    DW    op6A
    DW    unused
    DW    op6C
    DW    op6D
    DW    op6E
    DW    unused
    DW    op70
    DW    op71
    DW    unused
    DW    unused
    DW    unused
    DW    op75
    DW    op76
    DW    unused
    DW    op78
    DW    op79
    DW    unused
    DW    unused
    DW    unused
    DW    op7D
    DW    op7E
    DW    unused
    DW    unused
    DW    op81
    DW    unused
    DW    unused
    DW    op84
    DW    op85
    DW    op86
    DW    unused
    DW    op88
    DW    unused
    DW    op8A
    DW    unused
    DW    op8C
    DW    op8D
    DW    op8E
    DW    unused
    DW    op90
    DW    op91
    DW    unused
    DW    unused
    DW    op94
    DW    op95
    DW    op96
    DW    unused
    DW    op98
    DW    op99
    DW    op9A
    DW    unused
    DW    unused
    DW    op9D
    DW    unused
    DW    unused
    DW    opA0
    DW    opA1
    DW    opA2
    DW    unused
    DW    opA4
    DW    opA5
    DW    opA6
    DW    unused
    DW    opA8
    DW    opA9
    DW    opAA
    DW    unused
    DW    opAC
    DW    opAD
    DW    opAE
    DW    unused
    DW    opB0
    DW    opB1
    DW    unused
    DW    unused
    DW    opB4
    DW    opB5
    DW    opB6
    DW    unused
    DW    opB8
    DW    opB9
    DW    opBA
    DW    unused
    DW    opBC
    DW    opBD
    DW    opBE
    DW    unused
    DW    opC0
    DW    opC1
    DW    unused
    DW    unused
    DW    opC4
    DW    opC5
    DW    opC6
    DW    unused
    DW    opC8
    DW    opC9
    DW    opCA
    DW    unused
    DW    opCC
    DW    opCD
    DW    opCE
    DW    unused
    DW    opD0
    DW    opD1
    DW    unused
    DW    unused
    DW    unused
    DW    opD5
    DW    opD6
    DW    unused
    DW    opD8
    DW    opD9
    DW    unused
    DW    unused
    DW    unused
    DW    opDD
    DW    opDE
    DW    opDF
    DW    opE0
    DW    opE1
    DW    unused
    DW    unused
    DW    opE4
    DW    opE5
    DW    opE6
    DW    unused
    DW    opE8
    DW    opE9
    DW    opEA
    DW    unused
    DW    opEC
    DW    opED
    DW    opEE
    DW    unused
    DW    opF0
    DW    opF1
    DW    unused
    DW    unused
    DW    unused
    DW    opF5
    DW    opF6
    DW    unused
    DW    opF8
    DW    opF9
    DW    unused
    DW    unused
    DW    unused
    DW    opFD
    DW    opFE
    DW    unused
ENDIF

memD800 LABEL BYTE
    DB 2048 DUP (?)     ; 2K of FP ROM

memE000 LABEL BYTE
    DB 8192 DUP (?)     ; 8K of OS


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; DATA segment
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

    .DATA?

fTrace  DB ?  ; non-zero for single opcode execution
fSIO    DB ?  ; non-zero for SIO call
mdXLXE    DB ?  ; 0 = Atari 400/800, 1 = 800XL, 2 = 130XE
cntTick DB ?  ; mode display countdown timer (18 Hz ticks)

IFDEF HDOS16
pfnPokeB DW ?
ELSE
pfnPokeB DD ?
ENDIF

; 6502 context. These variables are global and used by the C code

    EVEN

regPC   DW ?  ; si
regSP   DW ?
regA    DB ?  ; dl
regX    DB ?  ; dx
regY    DB ?  ; cl
regP    DB ?

wLeft   DW ?  ; clock ticks left on scan line (counts down)

; These variables are used only within the interpreter

mdVideo DB ?  ; video mode

    .CONST

rgbRainbow LABEL BYTE
    DB    0,    0,    0                ; $00
    DB    8,    8,    8
    DB    16,    16,    16
    DB    24,    24,    24
    DB    31,    31,    31
    DB    36,    36,    36
    DB    40,    40,    40
    DB    44,    44,    44
    DB    47,    47,    47
    DB    50,    50,    50
    DB    53,    53,    53
    DB    56,    56,    56
    DB    58,    58,    58
    DB    60,    60,    60
    DB    62,    62,    62
    DB    63,    63,    63
    DB    6,    5,    0                ; $10
    DB    12,    10,    0
    DB    24,    15,    1
    DB    30,    20,    2
    DB    35,    27,    3
    DB    39,    33,    6
    DB    44,    42,    12
    DB    47,    44,    17
    DB    49,    48,    20
    DB    52,    51,    23
    DB    54,    53,    26
    DB    55,    55,    29
    DB    56,    56,    33
    DB    57,    57,    36
    DB    58,    58,    39
    DB    59,    59,    41
    DB    9,    5,    0                ; $20
    DB    14,    9,    0
    DB    20,    13,    0
    DB    28,    20,    0
    DB    36,    28,    1
    DB    43,    33,    1
    DB    47,    39,    10
    DB    49,    43,    17
    DB    51,    46,    24
    DB    53,    47,    26
    DB    55,    49,    28
    DB    57,    50,    30
    DB    59,    51,    32
    DB    60,    53,    36
    DB    61,    55,    39 
    DB    62,    56,    40
    DB    11,    3,    1                ; $30
    DB    18,    5,    2
    DB    27,    7,    4
    DB    36,    11,    8
    DB    44,    20,    13
    DB    46,    24,    16
    DB    49,    28,    21
    DB    51,    30,    25
    DB    53,    35,    30
    DB    54,    38,    34
    DB    55,    42,    37
    DB    56,    43,    38
    DB    57,    44,    39
    DB    57,    46,    40
    DB    58,    48,    42
    DB    59,    49,    44
    DB    11,    1,    3                ; $40
    DB    22,    6,    9
    DB    37,    10,    17
    DB    42,    15,    22
    DB    45,    21,    28
    DB    48,    24,    30
    DB    50,    26,    32
    DB    52,    28,    34
    DB    53,    30,    36
    DB    54,    33,    38
    DB    55,    35,    40
    DB    56,    37,    42
    DB    57,    39,    44
    DB    58,    41,    45
    DB    59,    42,    46
    DB    60,    43,    47
    DB    12,    0,    11                ; $50
    DB    20,    2,    18
    DB    28,    4,    26
    DB    39,    8,    37
    DB    48,    18,    49
    DB    53,    24,    53
    DB    55,    29,    55
    DB    56,    32,    56
    DB    57,    35,    57
    DB    58,    37,    58
    DB    59,    39,    59
    DB    59,    41,    59
    DB    59,    42,    59
    DB    59,    43,    59
    DB    59,    44,    59
    DB    60,    45,    60
    DB    5,    1,    16                ; $60
    DB    10,    2,    32
    DB    22,    10,    46
    DB    27,    15,    49
    DB    32,    21,    51
    DB    35,    25,    52
    DB    38,    28,    53
    DB    40,    32,    54
    DB    42,    35,    55
    DB    44,    37,    56
    DB    46,    38,    57
    DB    47,    40,    57
    DB    48,    41,    58
    DB    49,    43,    58
    DB    50,    44,    59
    DB    51,    45,    59
    DB    0,    0,    13                ; $70
    DB    4,    4,    26
    DB    10,    10,    46
    DB    18,    18,    49
    DB    24,    24,    53
    DB    27,    27,    54
    DB    30,    30,    55
    DB    33,    33,    56
    DB    36,    36,    57
    DB    39,    39,    57
    DB    41,    41,    58
    DB    43,    43,    58
    DB    44,    44,    59
    DB    46,    46,    60
    DB    48,    48,    61
    DB    49,    49,    62
    DB    1,    7,    18                ; $80
    DB    2,    13,    30
    DB    3,    19,    42
    DB    4,    24,    42
    DB    9,    28,    45
    DB    14,    32,    48
    DB    17,    35,    51
    DB    20,    37,    53
    DB    24,    39,    55
    DB    28,    41,    56
    DB    31,    44,    57
    DB    34,    46,    57
    DB    37,    47,    58
    DB    39,    48,    58
    DB    41,    49,    59
    DB    42,    50,    60
    DB    1,    4,    12                ; $90
    DB    2,    6,    22
    DB    3,    10,    32
    DB    5,    15,    36
    DB    8,    20,    38
    DB    15,    25,    44
    DB    21,    30,    47
    DB    24,    34,    49
    DB    27,    38,    52
    DB    29,    42,    54
    DB    31,    44,    55
    DB    33,    46,    56
    DB    36,    47,    57
    DB    38,    49,    58
    DB    40,    50,    59
    DB    42,    51,    60
    DB    0,    9,    7                ; $A0
    DB    1,    18,    14
    DB    2,    26,    20
    DB    3,    35,    27
    DB    4,    42,    33
    DB    6,    47,    38
    DB    14,    51,    44
    DB    18,    53,    46
    DB    22,    55,    49
    DB    25,    56,    51
    DB    28,    57,    52
    DB    32,    58,    53
    DB    36,    59,    55
    DB    40,    60,    56
    DB    44,    61,    57
    DB    45,    62,    58
    DB    0,    10,    1                ; $B0
    DB    0,    16,    3
    DB    1,    22,    5
    DB    5,    33,    7
    DB    9,    44,    16
    DB    14,    48,    21
    DB    19,    51,    24
    DB    22,    52,    28
    DB    24,    53,    31
    DB    30,    55,    35
    DB    36,    57,    38
    DB    39,    58,    41
    DB    41,    59,    44
    DB    43,    59,    47
    DB    46,    59,    49
    DB    47,    60,    50
    DB    3,    10,    0                ; $C0
    DB    6,    20,    0
    DB    9,    30,    1
    DB    14,    37,    4
    DB    18,    44,    7
    DB    22,    46,    12
    DB    26,    48,    17
    DB    29,    50,    22
    DB    33,    52,    26
    DB    36,    54,    28
    DB    38,    55,    30
    DB    40,    56,    33
    DB    42,    57,    36
    DB    45,    58,    39
    DB    48,    59,    42
    DB    49,    60,    43
    DB    5,    9,    0                ; $D0
    DB    11,    22,    0
    DB    17,    35,    1
    DB    23,    42,    2
    DB    29,    48,    8
    DB    34,    50,    12
    DB    38,    51,    17
    DB    40,    52,    21
    DB    42,    53,    24
    DB    44,    54,    27
    DB    46,    55,    29
    DB    47,    56,    31
    DB    48,    57,    34
    DB    50,    58,    37
    DB    52,    59,    40
    DB    53,    60,    42
    DB    8,    7,    0                ; $E0
    DB    19,    16,    0
    DB    28,    24,    4
    DB    33,    31,    6
    DB    48,    38,    8
    DB    52,    44,    12
    DB    55,    50,    15
    DB    57,    52,    19
    DB    58,    54,    22
    DB    58,    56,    24
    DB    59,    57,    26
    DB    59,    58,    29
    DB    60,    58,    33
    DB    61,    59,    35
    DB    61,    59,    36
    DB    62,    60,    38
    DB    8,    5,    0                ; $F0
    DB    13,    9,    0
    DB    22,    14,    1
    DB    32,    21,    3
    DB    42,    29,    5
    DB    45,    33,    7
    DB    48,    36,    12
    DB    50,    39,    18
    DB    53,    42,    24
    DB    54,    45,    27
    DB    55,    46,    30
    DB    56,    47,    33
    DB    57,    49,    36
    DB    58,    50,    38
    DB    58,    53,    39
    DB    59,    54,    40


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; CODE segment for video functions
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

    .CODE

   IFDEF HDOS16
    ASSUME ds:@data
   ENDIF

    align 2

  IFNDEF HWIN32

SaveVideo PROC
    mov   ah,0Fh
    int   10h           ; get video mode
    mov   mdVideo,al

    mov   ax,13h
    int   10h           ; set 320x200x256 video mode

    mov      dx,3c8h         ;Color palette write mode index reg.
    xor   ax,ax
    out      dx,al

    mov   dx,3c9h         ;Color palette data reg.
    lea   _di,rgbRainbow
    xor   bl,bl
@@:
    mov   al,[_di]      ; R
    inc   _di
    out   dx,al

    mov   al,[_di]        ; G
    inc   _di
    out   dx,al

    mov   al,[_di]        ; B
    inc   _di
    out   dx,al

    inc   bl
    jne   @B

    ret
SaveVideo ENDP


    align 2

RestoreVideo PROC
    mov   ax,2
    int   10h           ; set 80x25 text mode
    ret
RestoreVideo ENDP


VPORT    equ 3dah
VTRACE  equ 8


WaitForVRetrace PROC
    mov     dx,VPORT        ;status reg. port on color card
@@:
    in        al,dx        ;read the status
    and     al,VTRACE        ;test whether beam is currently in retrace
    jz        @B
    ret
WaitForVRetrace ENDP


WaitForNotVRetrace PROC
    mov     dx,VPORT       ;status reg. port on color card
@@:
    in        al,dx       ;read the status
    and     al,VTRACE       ;test whether beam is currently in retrace
    jnz     @B          ;still in retrace
WaitForNotVRetrace ENDP

  ENDIF

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Helper routines that don't fit in interp segment
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

   IFDEF HDOS16
    ASSUME ds:memseg
   ENDIF

    align 2

PackP PROC
    mov   ah,20h          ; we'll generate P in AH
    cmp   srN,0
    jns   @F
    or    ah,80h
@@:
    cmp   srV,0
    je    @F
    or    ah,40h
@@:
    cmp   srD,0
    je    @F
    or    ah,08h
@@:
    or    ch,ch             ; Z flag (CH is the last result, so Z = !CH)
    jne   @F
    or    ah,02h
@@:
    cmp   srC,0
    je    @F
    or    ah,01h
@@:
    or    ah,srI
    or    ah,srB
    mov   regP,ah
    ret
PackP ENDP


    align 2

UnpackP PROC
    mov   ah,regP
    
    mov   al,ah
    and   al,04h
    mov   srI,al

    mov   al,ah
    and   al,10h
    mov   srB,al

    mov   al,ah
    and   al,80h
    mov   srN,al

    mov   al,ah
    and   al,40h
    mov   srV,al

    mov   al,ah
    and   al,08h
    mov   srD,al

    mov   al,ah
    and   al,01h
    neg   al        ; if al = 1, al = $FF (sets high bit in srC)
    mov   srC,al

    and   ah,02h
    xor   ah,02h
    mov   ch,ah        ; ch = !Z

    ret
UnpackP ENDP

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; CODE segment for 6502 opcodes
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

   IFDEF HDOS16
    ASSUME ds:memseg
   ENDIF

op00:                   ; BRK
    opinit

    movzx _ax,regSP
    dec   al
    mov   _di,_ax
    dec   al
    mov   byte ptr regSP,al      ; decrement stack pointer
;    inc   _si

   IFDEF HDOS16
    mov   [di],si       ; push PC

    mov   si,0FFFEh
    mov   si,[si]       ; pc = [$206]
   ELSE
    lea   eax,rgbMem
    sub   esi,eax
    mov   word ptr rgbMem[edi],si       ; push PC

    mov   si,0FFFAh
    movzx esi,word ptr rgbMem[esi]      ; pc = [$206]
    lea   esi,rgbMem[esi]
   ENDIF

    mov   srB,010h      ; set B flag ; HACK?!?!?
    call  PackP
    movzx _di,regSP
    mov   al,regP        
    mov   _mem[_di],al    ; store P to stack
    dec   byte ptr regSP  ; then decrement stack pointer

    mov   srI,4         ; set I flag
    mov   srB,010h      ; set B flag
    dispatch


    ; C level entry point into interpreter

    align 2

Go6502 PROC
    push  _bp
    push  _si
    push  _di

   IFDEF HWIN32
    push  _bx
   ENDIF

   IFDEF HDOS16
    push  ds            ; save current DATA segment
    mov   ax,memseg
    mov   ds,ax         ; DS points to 6502 memory

    mov   si,regPC      ; load 6502 registers
   ELSE
    movzx esi,regPC      ; load 6502 registers
    lea   esi,rgbMem[esi]
   ENDIF

    mov   dl,regA
    mov   dh,regX
    mov   cl,regY
    call  UnpackP

   IFDEF HDOS16
    mov   bp,wLeft
    mov   bx,6B00h      ; $D600 / 2
   ELSE
    movzx ebp,wLeft
    lea   ebx,jump_tab
    add   ebx,1023t
    and   ebx,0FFFFFC00h
    shr   ebx,2
    xor   edi,edi
   ENDIF

    cmp   byte ptr fTrace,0
    je    short @F
    mov   _bp,1
@@:
    sub   wLeft,bp      ; decrement clock ahead of time

    mov   bl,byte ptr [_si]
    inc   _si

   IFDEF HDOS16
    mov   di,bx
    jmp   word ptr [di+bx]
   ELSE
    jmp   dword ptr [edi+ebx*4]
   ENDIF
Go6502 ENDP


    setorg  op00,256
op01:                   ; ORA (zp,X)
    opinit
    EA_zpXind
    ORA_com
next:
    dispatch

    opinit
    ; exit interpreter

    align 2

done:
   IFDEF HDOS16
    dec   si
    mov   regPC,si      ; save 6502 registers
   ELSE
    lea   eax,rgbMem
    dec   esi
    sub   esi,eax
    mov   regPC,si      ; save 6502 registers
   ENDIF

    mov   regA,dl
    mov   regX,dh
    mov   regY,cl
    call  PackP ; save P register

   IFDEF HDOS16
    pop   ds            ; restore DATA segment
   ENDIF

   IFDEF HWIN32
    pop   _bx
   ENDIF

    pop   _di
    pop   _si
    pop   _bp

    ret


    ; write handler when EA > $A000

writehw:
 IFDEF ASM_VERSIONS
    push  _ax

    mov   ax,di
    cmp   ah,0D0h
    je      pokeGTIA

    cmp   ah,0D2h
    je      pokePOKEY

    cmp   ah,0D3h
    je      pokePIA

    cmp   ah,0D4h
    je      pokeANTIC

    cmp   byte ptr mdXLXE,0
    jz    short @F

    cmp   ax,0D1FFh
    je    pokeD1FF

    test  byte ptr PORTB,1
    jnz   short @F              ; ROM is enabled, don't write

    cmp   di,0C000h
    jae   writeXLRAM

@@:
    pop   _ax
    dispatch

pokeGTIA:
    pop   _ax
    and   _di,31
    mov   ah,[writeGTIA+_di]
    mov   [writeGTIA+_di],al

    cmp   _di,30
    jne   short @F

    ; HITCLR - clear collision registers

    xor   ax,ax
    mov   word ptr M0PF,ax
    mov   word ptr M2PF,ax
    mov   word ptr P0PF,ax
    mov   word ptr P2PF,ax
    mov   word ptr M0PL,ax
    mov   word ptr M2PL,ax
    mov   word ptr P0PL,ax
    mov   word ptr P2PL,ax
    jmp   short gtiax

@@:
    cmp   _di,31
    jne   short @F

    ; CONSOL - internal speaker (bit 3)

    xor   ah,al
    and   ah,8

   IFNDEF HWIN32
    .if (ah != 0)
        in    al,61h           ; read current speaker value
        and   al,0FEh           ; mask bottom bit (why???)
        xor   al,2               ; toggle speaker state
        out   61h,al
    .endif
   ENDIF

    jmp   short gtiax

gtiax:
    dispatch

pokePOKEY:
    pop   _ax
    and   _di,15
    mov   ah,[writePOKEY+_di]
    mov   [writePOKEY+_di],al

    cmp   _di,10
    jne   short @F

    ; SKRES - set bits 5 6 7 of SKSTAT to 1

    or    SKSTAT,0E0h
    jmp   short pokeyx

@@:
    cmp   _di,13
    jne   short @F

    ; SEROUT - generate output ready and xmit finished interrupts

    mov   al,IRQEN
    and   al,018h
    not   al
    and   IRQST,al
    jmp   short pokeyx

@@:
    cmp   _di,14
    jne   short @F

    ; IRQEN - mask with IRQST to clear interrupts

    not   al
    or    IRQST,al
    jmp   short pokeyx

@@:
pokeyx:
    dispatch

pokeANTIC:
    pop   _ax
    and   _di,15
    mov   ah,[writeANTIC+_di]
    mov   [writeANTIC+_di],al

    cmp   _di,10
    jne   short @F

    ; WSYNC - wait until start of next scan line

    mov   _bp,1
    jmp   short anticx

@@:
    cmp   _di,15
    jne   short @F

    ; NMIRES - reset NMI status (NMIST)

    mov   NMIST,01Fh
    jmp   short anticx
@@:
anticx:
    dispatch

 ELSE ;; !ASM_VERSIONS

   IFDEF HDOS16
    pop   ds
    ASSUME ds:@data
   ENDIF

    push  _bx
    push  _cx
    push  _dx
    
    push  _ax           ; byte to poke
    push  _di           ; address

   IFDEF HDOS16
    mov   ax,si
    dec   ax
    mov   regPC,ax
   ELSE
    lea   eax,rgbMem
    mov   ebx,esi
    dec   ebx
    sub   ebx,eax
    mov   regPC,bx
   ENDIF

    mov   wLeft,bp      ; reload in case changed
   IFDEF HDOS16
    call  word ptr pfnPokeB
   ELSE
    call  dword ptr pfnPokeB
   ENDIF
    mov   bp,wLeft      ; reload in case changed

   IFNDEF HWIN32
    pop   _ax
    pop   _ax
   ELSE
    pop   _ax
    pop   _ax
   ENDIF

    pop   _dx
    pop   _cx
    pop   _bx

IFDEF HDOS16
    push  ds
    mov   ax,memseg
    mov   ds,ax         ; DS points to 6502 memory

    ASSUME ds:memseg
ENDIF
    dispatch

 ENDIF ;; ASM_VERSIONS

    setorg  op04,256
op05:                     ; ORA zp
    opinit
    EA_zp
    ORA_com
    dispatch

; ASL zp
    setorg  op05,256
op06:
    opinit
    EA_zp
    ASL_com
    dispatch

    opinit
pokePIA:
    ; The PIA is funky in that it has 8 registers in a window of 4:
    ;
    ; D300 - PORTA data register (read OR write) OR data direction register
    ;      - the data register reads the first two joysticks
    ;      - the direction register just behaves as RAM
    ;        for each of the 8 bits: 1 = write, 0 = read
    ;      - when reading PORTA, write bits behave as RAM
    ;
    ; D301 - PACTL, controls PORTA
    ;      - bits 7 and 6 are ignored, always read as 0
    ;      - bits 5 4 3 1 and 0 are ignored and just behave as RAM
    ;      - bit 2 selects: 1 = data register, 0 = direction register
    ;
    ; D302 - PORTB, similar to PORTB, on the XL/XE it controls memory banks
    ;
    ; D303 - PBCTL, controls PORTB
    ;
    ; When we get a write to PACTL or PBCTL, strip off the upper two bits and
    ; stuff the result into the appropriate read location. The only bit we care
    ; about is bit 2 (data/direction select). When this bit toggles we need to
    ; update the PORTA or PORTB read location appropriately.
    ;
    ; When we get a write to PORTA or PORTB, if the appropriate select bit
    ; selects the data direction register, write to wPADDIR or wPBDDIR.
    ; Otherwise, write to wPADATA or wPBDATA using ddir as a mask.
    ;
    ; When reading PORTA or PORTB, return (writereg ^ ddir) | (readreg ^ ~ddir)
    ;

    pop   _ax
    and   _di,3
    mov   [writePIA+_di],al

    .if (_di & 2)
        ; it is a control register

        and   _di,1
        and   al,03Ch
        mov   [PACTL+_di],al
    .else
        ; it is either a data or ddir register. Check the control bit

        .if   ([PACTL+_di] & 4)
            ; it is a data register. Update only bits that are marked as write.

            mov   ah,[wPADDIR+_di]
            and   al,ah
            not   ah
            and   ah,[wPADATA+_di]
            or    al,ah
            mov   ah,[wPADATA+_di]    ; AH = old value in data register
            mov   [wPADATA+_di],al    ; AL = new value in data register

            .if (al != ah) && (_di == 1) && (mdXLXE != 0)

                ; Writing a new value to PORT B on the XL/XE.

                .if (mdXLXE != 2)
                    or    al,07Ch   ; in 800XL mode, mask bank select bits
                    or    wPBDATA,07Ch
                .endif

               IFDEF HDOS16
                pop   ds            ; restore DATA segment
               ENDIF

                push  _bx
                push  _cx
                push  _dx

                push  _si
                push  _ax            ; flags
                xor   al,ah
                push  _ax            ; mask

                call  SwapMem

                pop   _ax
                pop   _ax
                pop   _ax

                pop   _dx
                pop   _cx
                pop   _bx

               IFDEF HDOS16
                push  ds            ; save current DATA segment
                mov   ax,memseg
                mov   ds,ax         ; DS points to 6502 memory
               ENDIF
            .endif
        .else
            ; it is a data direction register.

            mov   [wPADDIR+_di],al
        .endif
    .endif

    ; update PORTA and PORTB read registers

    .if   (PACTL & 4)
        mov   al,wPADATA
        mov   ah,rPADATA
        xor   al,ah
        and   al,wPADDIR
        xor   al,ah
    .else
        mov   al,wPADDIR
    .endif
    mov   PORTA,al

    .if   (PBCTL & 4)
        mov   al,wPBDATA
        mov   ah,rPBDATA
        xor   al,ah
        and   al,wPBDDIR
        xor   al,ah
    .else
        mov   al,wPBDDIR
    .endif
    mov   PORTB,al

    dispatch


; PHP - do a 68000 to 6502 flag conversion, like in the init code
    setorg  op07,256
op08:
    opinit
    call  PackP
    mov   di,regSP
    mov   al,regP        
    mov   _mem[_di],al    ; store P to stack
    dec   byte ptr regSP  ; then decrement stack pointer
    dispatch

; ORA #
    setorg op08,256
op09:
    opinit
    EA_imm
    ORA_com
    dispatch

; ASL A
    setorg op09,256
op0A:
    opinit
    ASLA_com
    dispatch

    opinit
writeXLRAM:
    ; Called from writehw.
    ; Write value on stack to memory location DI.
    ; DI >= $C000, but still need to check for hardware range

    pop   _ax
    .if   ((di < 0D000h) || (di >= 0D800h))
        mov   _mem[_di],al  ; write to RAM
    .endif
    dispatch

; ORA abs
    setorg op0C,256
op0D:
    opinit
    EA_abs
    ORA_com
    dispatch                ; and go do another opcode

; ASL abs
    setorg op0D,256
op0E:
    opinit
    EA_abs
    ASL_com
    dispatch

; BPL
    setorg op0F,256
op10:
    opinit
    cmp   srN,0             ; test N flag
    js    short @F

    AddOffset
@@:
    SkipByte

; ORA (zp),Y
    setorg op10,256
op11:
    opinit
    EA_zpYind
    ORA_com
    dispatch

    opinit
pokeD1FF:
    pop   _ax
    mov   _mem[_di],al

   IFDEF HDOS16
    mov   di,0d800h
   ELSE
    lea   _di,rgbMem+0d800h
   ENDIF

 if 1
    .if (al == 1)

        ; swap in XE BUS device 1 (R: handler)

        mov   byte ptr [_di+00h],00h ; bus device header
        mov   byte ptr [_di+01h],00h
        mov   byte ptr [_di+02h],04h
        mov   byte ptr [_di+03h],80h
        mov   byte ptr [_di+04h],00h
        mov   byte ptr [_di+05h],4Ch ; JMP to SIO vector
        mov   word ptr [_di+06h],0D770h
        mov   byte ptr [_di+08h],4Ch ; JMP to interrupt vector
        mov   word ptr [_di+09h],0D780h
        mov   byte ptr [_di+0Bh],91h
        mov   byte ptr [_di+0Ch],00h
        mov   word ptr [_di+0Dh],0D700h ; SIO vectors
        mov   word ptr [_di+0Fh],0D710h
        mov   word ptr [_di+11h],0D720h
        mov   word ptr [_di+13h],0D730h
        mov   word ptr [_di+15h],0D740h
        mov   word ptr [_di+17h],0D750h
        mov   byte ptr [_di+19h],4Ch ; JMP to init vector
        mov   word ptr [_di+1Ah],0D760h
    .else

        ; swap out XE BUS device 1 (R: handler)

        mov   byte ptr [_di+00h],020h ; original ROM code at $D800
        mov   byte ptr [_di+01h],0A1h
        mov   byte ptr [_di+02h],0DBh
        mov   byte ptr [_di+03h],020h
        mov   byte ptr [_di+04h],0BBh
        mov   byte ptr [_di+05h],0DBh
        mov   byte ptr [_di+06h],0B0h
        mov   byte ptr [_di+07h],039h
        mov   byte ptr [_di+08h],0A2h
        mov   byte ptr [_di+09h],0EDh
        mov   byte ptr [_di+0Ah],0A0h
        mov   byte ptr [_di+0Bh],004h
        mov   byte ptr [_di+0Ch],020h
        mov   byte ptr [_di+0Dh],048h
        mov   byte ptr [_di+0Eh],0DAh
        mov   byte ptr [_di+0Fh],0A2h

        mov   byte ptr [_di+10h],0FFh
        mov   byte ptr [_di+11h],086h
        mov   byte ptr [_di+12h],0F1h
        mov   byte ptr [_di+13h],020h
        mov   byte ptr [_di+14h],044h
        mov   byte ptr [_di+15h],0DAh
        mov   byte ptr [_di+16h],0F0h
        mov   byte ptr [_di+17h],004h
        mov   byte ptr [_di+18h],0A9h
        mov   byte ptr [_di+19h],0FFh
        mov   byte ptr [_di+1Ah],085h
        mov   byte ptr [_di+1Bh],0F0h

    .endif
 endif

    dispatch

; ORA zp,X
    setorg op14,256
op15:
    opinit
    EA_zpX
    ORA_com
    dispatch

; ASL zp,X
    setorg op15,256
op16:
    opinit
    EA_zpX
    ASL_com
    dispatch

; CLC
    setorg op17,256
op18:
    opinit
    xor   al,al
    mov   srC,al
    dispatch

; ORA abs,Y
    setorg op18,256
op19:
    opinit
    EA_absY
    ORA_com
    dispatch

; ORA abs,X
    setorg op1C,256
op1D:
    opinit
    EA_absX
    ORA_com
    dispatch

; ASL abs,X
    setorg op1D,256
op1E:
    opinit
    EA_absX
    ASL_com
    dispatch

; JSR abs
    setorg op1F,256
op20:
    opinit
    movzx _ax,regSP
    dec   al
    mov   _di,_ax
    dec   al
    mov   byte ptr regSP,al      ; decrement stack pointer
    inc   _si           ; next instruction - 1

   IFDEF HDOS16
    mov   [di],si       ; push PC
    mov   si,[si-1]       ; pc =  [pc]
   ELSE
    lea   eax,rgbMem
    sub   esi,eax
    mov   word ptr rgbMem[edi],si       ; push PC
    movzx esi,word ptr rgbMem[esi-1]       ; pc =  [pc]
   ENDIF

    .if (si == 0E459h) && ((mdXLXE == 0) || (wPBDATA & 1))
        ; this is our SIO hook!

        mov   fSIO,1
        mov   _bp,1
    .endif

    .if (mdXLXE != 0) && (si >= 0D700h) && (si <= 0D7FFh)

        ; this is our XE BUS hook!

        mov   fSIO,1
        mov   _bp,1

    .endif

   IFNDEF HDOS16
    lea    esi,rgbMem[esi]
   ENDIF

    dispatch

; AND (zp,X)
    setorg op20,256
op21:
    opinit
    EA_zpXind
    AND_com
    dispatch

; BIT zp
    setorg op23,256
op24:
    opinit
    EA_zp
    mov   al,byte ptr _mem[_di]
    cbw             ; sign extend
    mov   srN,ah    ; and store in N flag
    mov   ch,al
    and   al,040h   ; isolate bit 6
    mov   srV,al    ; and store in V flag
    and   ch,dl     ; Z bit
    dispatch

; AND zp
    setorg op24,256
op25:
    opinit
    EA_zp
    AND_com
    dispatch

; ROL zp
    setorg op25,256
op26:
    opinit
    EA_zp
    ROL_com
    dispatch

; PLP
    setorg op27,256
op28:
    opinit
    inc   byte ptr regSP  ; increment stack pointer
    mov   di,regSP
    mov   al,byte ptr _mem[_di]         ; read P from stack
    mov   regP,al
    call  UnpackP ; update status registers
    dispatch

; AND #
    setorg op28,256
op29:
    opinit
    EA_imm
    AND_com
    dispatch

; ROL A
    setorg op29,256
op2A:
    opinit
    mov   ah,srC
    add   ah,ah        ; set carry flag
    rcl   dl,1         ; rotate value left
    update_NZ dl
    update_C
    dispatch

; BIT abs
    setorg op2B,256
op2C:
    opinit
    EA_abs
    mov   al,byte ptr _mem[_di]
    cbw             ; sign extend
    mov   srN,ah    ; and store in N flag
    mov   ch,al
    and   al,040h   ; isolate bit 6
    mov   srV,al    ; and store in V flag
    and   ch,dl     ; Z bit
    dispatch

; AND abs
    setorg op2C,256
op2D:
    opinit
    EA_abs
    AND_com
    dispatch

; ROL abs
    setorg op2D,256
op2E:
    opinit
    EA_abs
    ROL_com
    dispatch

; BMI
    setorg op2F,256
op30:
    opinit
    cmp   srN,0             ; test N flag
    jns   short @F

    AddOffset
@@:
    SkipByte

; AND (zp),Y
    setorg op30,256
op31:
    opinit
    EA_zpYind
    AND_com
    dispatch

; AND zp,X
    setorg op34,256
op35:
    opinit
    EA_zpX
    AND_com
    dispatch

; ROL zp,X
    setorg op35,256
op36:
    opinit
    EA_zpX
    ROL_com
    dispatch

; SEC
    setorg op37,256
op38:
    opinit
    mov   al,0ffh
    mov   srC,al
    dispatch

; AND abs,Y
    setorg op38,256
op39:
    opinit
    EA_absY
    AND_com
    dispatch

; AND abs,X
    setorg op3C,256
op3D:
    opinit
    EA_absX
    AND_com
    dispatch

; ROL abs,X
    setorg op3D,256
op3E:
    opinit
    EA_absX
    ROL_com
    dispatch

; RTI
    setorg op3F,256
op40:
    opinit

    ; first do a PLP

    inc   byte ptr regSP  ; increment stack pointer
    movzx _di,regSP
    mov   al,byte ptr _mem[_di]         ; read P from stack
    mov   regP,al
    call  UnpackP ; update status registers
    mov   srB,000h      ; set B flag ; HACK?!?!?

    ; then do an RTS but do not increment the PC

    inc   byte ptr regSP  ; increment stack pointer
    movzx _di,regSP
   IFDEF HDOS16
    mov   si,[di]         ; read PC from stack
   ELSE
    movzx  esi,word ptr rgbMem[edi]
    lea    esi,rgbMem[esi]
   ENDIF
    inc   byte ptr regSP  ; increment stack pointer

    dispatch


; EOR (zp,X)
    setorg op40,256
op41:
    opinit
    EA_zpXind
    EOR_com
    dispatch

; EOR zp
    setorg op44,256
op45:
    opinit
    EA_zp
    EOR_com
    dispatch

; LSR zp
    setorg op45,256
op46:
    opinit
    EA_zp
    LSR_com
    dispatch

; PHA
    setorg op47,256
op48:
    opinit
    movzx _di,regSP
    mov   _mem[_di],dl         ; store A to stack
    dec   byte ptr regSP  ; then decrement stack pointer
    dispatch

; EOR #
    setorg op48,256
op49:
    opinit
    EA_imm
    EOR_com
    dispatch

; LSR A
    setorg op49,256
op4A:
    opinit
    LSRA_com
    dispatch

; JMP abs
    setorg op4B,256
op4C:
    opinit
   IFDEF HDOS16
    mov   si,[si]       ; pc =  [pc]
   ELSE
    movzx  esi,word ptr [esi]
   ENDIF

    .if (si == 0E459h) && ((mdXLXE == 0) || (wPBDATA & 1))
        ; this is our SIO hook!

        mov   fSIO,1
        mov   _bp,1
    .endif

    .if (mdXLXE != 0) && (si >= 0D700h) && (si <= 0D7FFh)

        ; this is our XE BUS hook!

        mov   fSIO,1
        mov   _bp,1

    .endif

   IFNDEF HDOS16
    lea    esi,rgbMem[esi]
   ENDIF

    dispatch

; EOR abs
    setorg op4C,256
op4D:
    opinit
    EA_abs
    EOR_com
    dispatch

; LSR abs
    setorg op4D,256
op4E:
    opinit
    EA_abs
    LSR_com
    dispatch

; BVC
    setorg op4F,256
op50:
    opinit
    cmp   srV,0             ; test V flag
    jne   short @F

    AddOffset
@@:
    SkipByte

; EOR (zp),Y
    setorg op50,256
op51:
    opinit
    EA_zpYind
    EOR_com
    dispatch

; EOR zp,X
    setorg op54,256
op55:
    opinit
    EA_zpX
    EOR_com
    dispatch

; LSR zp,X
    setorg op55,256
op56:
    opinit
    EA_zpX
    LSR_com
    dispatch

; CLI
    setorg op57,256
op58:
    opinit
    mov   srI,0

  ; check if other interrupts pending

    dispatch

; EOR abs,Y
    setorg op58,256
op59:
    opinit
    EA_absY
    EOR_com
    dispatch

; EOR abs,X
    setorg op5C,256
op5D:
    opinit
    EA_absX
    EOR_com
    dispatch

; LSR abs,X
    setorg op5D,256
op5E:
    opinit
    EA_absX
    LSR_com
    dispatch

; RTS
    setorg op5F,256
op60:
    opinit
  
    inc   byte ptr regSP  ; increment stack pointer
    movzx _di,regSP
   IFDEF HDOS16
    mov   si,[di]         ; read PC from stack
   ELSE
    movzx  esi,word ptr rgbMem[_di]
   ENDIF
    inc   byte ptr regSP  ; increment stack pointer
    inc   _si          ; increment PC
    
    .if (mdXLXE != 0) && (si >= 0D700h) && (si <= 0D7FFh)

        ; this is our XE BUS hook!

        mov   fSIO,1
        mov   _bp,1

    .endif

   IFNDEF HDOS16
    lea    esi,rgbMem[esi]
   ENDIF

    dispatch

; ADC (zp,X)
    setorg op60,256
op61:
    opinit

    EA_zpXind
    cmp   srD,0
    jne   short @F

    ADC_com
    dispatch

@@:
    ADC_dec
    dispatch

; ADC zp
    setorg op64,256
op65:
    opinit

    EA_zp
    cmp   srD,0
    jne   short @F

    ADC_com
    dispatch

@@:
    ADC_dec
    dispatch

; ROR zp
    setorg op65,256
op66:
    opinit
    EA_zp
    ROR_com
    dispatch

; PLA
    setorg op67,256
op68:
    opinit
    inc   byte ptr regSP  ; increment stack pointer
    movzx _di,regSP
    mov   dl,_mem[_di]         ; read A from stack
    update_NZ dl
    dispatch

; ADC #
    setorg op68,256
op69:
    opinit

    EA_imm
    cmp   srD,0
    jne   short @F

    ADC_com
    dispatch

@@:
    ADC_dec
    dispatch

; ROR A
    setorg op69,256
op6A:
    opinit
    mov   ah,srC
    add   ah,ah        ; set carry flag
    rcr   dl,1         ; rotate value right
    update_NZ dl
    update_C
    dispatch

; JMP (abs)
    setorg op6B,256
op6C:
    opinit
    movzx _di,word ptr [_si]       ; pc =  [[pc]]
   IFDEF HDOS16
    mov   si,[di]
   ELSE
    movzx  esi,word ptr rgbMem[edi]
    lea    esi,rgbMem[esi]
   ENDIF
    dispatch

; ADC abs
    setorg op6C,256
op6D:
    opinit

    EA_abs
    cmp   srD,0
    jne   short @F

    ADC_com
    dispatch

@@:
    ADC_dec
    dispatch

; ROR abs
    setorg op6D,256
op6E:
    opinit
    EA_abs
    ROR_com
    dispatch

; BVS
    setorg op6F,256
op70:
    opinit
    cmp   srV,0             ; test V flag
    je    short @F

    AddOffset
@@:
    SkipByte

; ADC (zp),Y
    setorg op70,256
op71:
    opinit
    
    EA_zpYind
    cmp   srD,0
    jne   short @F

    ADC_com
    dispatch

@@:
    ADC_dec
    dispatch

; ADC zp,X
    setorg op74,256
op75:
    opinit

    EA_zpX
    cmp   srD,0
    jne   short @F

    ADC_com
    dispatch

@@:
    ADC_dec
    dispatch

; ROR zp,X
    setorg op75,256
op76:
    opinit
    EA_zpX
    ROR_com
    dispatch

; SEI
    setorg op77,256
op78:
    opinit
    mov   srI,4
    dispatch

; ADC abs,Y
    setorg op78,256
op79:
    opinit

    EA_absY
    cmp   srD,0
    jne   short @F

    ADC_com
    dispatch

@@:    
    ADC_dec
    dispatch

    setorg op7C,256
; ADC abs,X
op7D:
    opinit

    EA_absX
    cmp   srD,0
    jne   short @F

    ADC_com
    dispatch

@@:
    ADC_dec
    dispatch

; ROR abs,X
    setorg op7D,256
op7E:
    opinit
    EA_absX
    ROR_com
    dispatch

; STA (zp,X)
    setorg op80,256
op81:
    opinit
    EA_zpXind
    ST_com dl
    dispatch

; STY zp
    setorg op83,256
op84:
    opinit
    EA_zp
    mov   _mem[_di],cl
    dispatch

; STA zp
    setorg op84,256
op85:
    opinit
    EA_zp
    mov   _mem[_di],dl
    dispatch

; STX zp
    setorg op85,256
op86:
    opinit
    EA_zp
    mov   _mem[_di],dh
    dispatch

; DEY
    setorg op87,256
op88:                       ; DEY
    opinit
    DEY_com
    dispatch

; TXA
    setorg op89,256
op8A:
    opinit
    mov   dl,dh
    update_NZ dl
    dispatch

; STY abs
    setorg op8B,256
op8C:
    opinit
    EA_abs
    ST_com cl
    dispatch

; STA abs
    setorg op8C,256
op8D:
    opinit
    EA_abs
    ST_com dl
    dispatch

; STX abs
    setorg op8D,256
op8E:
    opinit
    EA_abs
    ST_com dh
    dispatch
 
; BCC
    setorg op8F,256
op90:
    opinit
    cmp   srC,0             ; test C flag
    jne   short @F

    AddOffset
@@:
    SkipByte

; STA (zp),Y
    setorg op90,256
op91:
    opinit
    EA_zpYind
    ST_com dl
    dispatch

; STY zp,X
    setorg op93,256
op94:
    opinit
    EA_zpX
    mov   _mem[_di],cl
    dispatch

; STA zp,X
    setorg op94,256
op95:
    opinit
    EA_zpX
    mov   _mem[_di],dl
    dispatch

; STX zp,Y
    setorg op95,256
op96:
    opinit
    EA_zpY
    mov   _mem[_di],dh
    dispatch

; TYA
    setorg op97,256
op98:
    opinit
    mov   dl,cl
    update_NZ dl
    dispatch

; STA abs,Y
    setorg op98,256
op99:
    opinit
    EA_absY
    ST_com dl
    dispatch

; TXS
    setorg op99,256
op9A:
    opinit
    mov   byte ptr regSP,dh
    dispatch

; STA abs,X
    setorg op9C,256
op9D:
    opinit
    EA_absX
    ST_com dl
    dispatch

; LDY #
    setorg op9F,256
opA0:                       ; LDY #
    opinit
    EA_imm
    LD_com cl
    dispatch

; LDA (zp,X)
    setorg opA0,256
opA1:
    opinit
    EA_zpXind
    LD_com dl
    dispatch

; LDX #
    setorg opA1,256
opA2:                       ; LDX #
    opinit
    EA_imm
    LD_com dh
    dispatch

; LDY zp
    setorg opA3,256
opA4:
    opinit
    EA_zp
    LD_com cl
    dispatch

; LDA zp
    setorg opA4,256
opA5:                       ; LDA #
    opinit
    EA_zp
    LD_com dl
    dispatch

; LDX zp
    setorg opA5,256
opA6:
    opinit
    EA_zp
    LD_com dh
    dispatch

; TAY
    setorg opA7,256
opA8:
    opinit
    mov   cl,dl
    update_NZ cl
    dispatch

    setorg opA8,256
opA9:                       ; LDA #
    opinit
    EA_imm
    LD_com dl
    dispatch

; TAX
    setorg opA9,256
opAA:
    opinit
    mov   dh,dl
    update_NZ dh
    dispatch

; LDY abs
    setorg opAB,256
opAC:
    opinit
    EA_abs
    LD_com cl
    dispatch

; LDA abs
    setorg opAC,256
opAD:
    opinit
    EA_abs
    LD_com dl
    dispatch

; LDX abs
    setorg opAD,256
opAE:
    opinit
    EA_abs
    LD_com dh
    dispatch

; BCS
    setorg opAF,256
opB0:
    opinit
    cmp   srC,0             ; test C flag
    je    short @F

    AddOffset
@@:
    SkipByte

; LDA (zp),Y
    setorg opB0,256
opB1:
    opinit
    EA_zpYind
    LD_com dl
    dispatch

; LDY zp,X
    setorg opB3,256
opB4:
    opinit
    EA_zpX
    LD_com cl
    dispatch

; LDA zp,X
    setorg opB4,256
opB5:
    opinit
    EA_zpX
    LD_com dl
    dispatch

; LDX zp,Y
    setorg opB5,256
opB6:
    opinit
    EA_zpY
    LD_com dh
    dispatch

    setorg opB7,256
opB8:                      ; CLV
    opinit
    mov   srV,0
    dispatch

; LDA abs,Y
    setorg opB8,256
opB9:
    opinit
    EA_absY
    LD_com dl
    dispatch

; TSX
    setorg opB9,256
opBA:
    opinit
    mov   dh,byte ptr regSP
    update_NZ dh
    dispatch

; LDY abs,X
    setorg opBB,256
opBC:
    opinit
    EA_absX
    LD_com cl
    dispatch

; LDA abs,X
    setorg opBC,256
opBD:
    opinit
    EA_absX
    LD_com dl
    dispatch

; LDX abs,Y
    setorg opBD,256
opBE:
    opinit
    EA_absY
    LD_com dh
    dispatch

; CPY #
    setorg opBF,256
opC0:
    opinit
    EA_imm
    CMP_com cl
    dispatch


; CMP (zp,X)
    setorg opC0,256
opC1:
    opinit
    EA_zpXind
    CMP_com dl
    dispatch

; CPY zp
    setorg opC3,256
opC4:
    opinit
    EA_zp
    CMP_com cl
    dispatch

; CMP zp
    setorg opC4,256
opC5:
    opinit
    EA_zp
    CMP_com dl
    dispatch

; DEC zp
    setorg opC5,256
opC6:
    opinit
    EA_zp
    DEC_com
    dispatch

; INY
    setorg opC7,256
opC8:
    opinit
    INY_com
    dispatch

; CMP #
    setorg opC8,256
opC9:
    opinit
    EA_imm
    CMP_com dl
    dispatch

; DEX
    setorg opC9,256
opCA:                       ; DEX
    opinit
    DEX_com
    dispatch

; CPY abs
    setorg opCB,256
opCC:
    opinit
    EA_abs
    CMP_com cl
    dispatch

; CMP abs
    setorg opCC,256
opCD:
    opinit
    EA_abs
    CMP_com dl
    dispatch

; DEC abs
    setorg opCD,256
opCE:
    opinit
    EA_abs
    DEC_com
    dispatch

; BNE
    setorg opCF,256
opD0:                       ; BNE
    opinit
    or    ch,ch             ; test Z flag
    je    short @F

    AddOffset
@@:
    SkipByte

; CMP (zp),Y
    setorg opD0,256
opD1:
    opinit
    EA_zpYind
    CMP_com dl
    dispatch

; CMP zp,X
    setorg opD4,256
opD5:
    opinit
    EA_zpX
    CMP_com dl
    dispatch

; DEC zp,X
    setorg opD5,256
opD6:
    opinit
    EA_zpX
    DEC_com
    dispatch

; CLD
    setorg opD7,256
opD8:
    opinit

    mov   srD,0         ; clear D flag
    dispatch

; CMP abs,Y
    setorg opD8,256
opD9:
    opinit
    EA_absY
    CMP_com dl
    dispatch

; CMP abs,X
    setorg opDC,256
opDD:
    opinit
    EA_absX
    CMP_com dl
    dispatch

; DEC abs,X
    setorg opDD,256
opDE:
    opinit
    EA_absX
    DEC_com
    dispatch

    setorg opDE,256
opDF:
    opinit
;    unused
    EA_imm
    cmp   al,0DFh
    jne   short @F

    ; this is our SIO hook!
    mov   fSIO,1
    jmp   done
@@:
    dispatch

; CPX #
    setorg opDF,256
opE0:
    opinit
    EA_imm
    CMP_com dh
    dispatch

; SBC (zp,X)
    setorg opE0,256
opE1:
    opinit

    EA_zpXind
    cmp   srD,0
    jne   short @F

    SBC_com
    dispatch
@@:
    SBC_dec
    dispatch

; CPX zp
    setorg opE3,256
opE4:
    opinit
    EA_zp
    CMP_com dh
    dispatch

; SBC zp
    setorg opE4,256
opE5:
    opinit

    EA_zp
    cmp   srD,0
    jne   short @F

    SBC_com
    dispatch
@@:
    SBC_dec
    dispatch

; INC zp
    setorg opE5,256
opE6:
    opinit
    EA_zp
    INC_com
    dispatch

; INX
    setorg opE7,256
opE8:
    opinit
    INX_com
    dispatch

; SBC #
    setorg opE8,256
opE9:
    opinit

    EA_imm
    cmp   srD,0
    jne   short @F

    SBC_com
    dispatch
@@:
    SBC_dec
    dispatch

; NOP
    setorg opE9,256
opEA:
    opinit
    dispatch

; CPX abs
    setorg opEB,256
opEC:
    opinit
    EA_abs
    CMP_com dh
    dispatch

; SBC abs
    setorg opEC,256
opED:
    opinit

    EA_abs
    cmp   srD,0
    jne   short @F

    SBC_com
    dispatch
@@:
    SBC_dec
    dispatch

; INC abs
    setorg opED,256
opEE:
    opinit
    EA_abs
    INC_com
    dispatch

; BEQ
    setorg opEF,256
opF0:
    opinit
    or    ch,ch             ; test Z flag
    jne   short @F

    AddOffset
@@:
    SkipByte

; SBC (zp),Y
    setorg opF0,256
opF1:
    opinit

    EA_zpYind
    cmp   srD,0
    jne   short @F

    SBC_com
    dispatch

@@:
    SBC_dec
    dispatch

; SBC zp,X
    setorg opF4,256
opF5:
    opinit

    EA_zpX
    cmp   srD,0
    jne   short @F

    SBC_com
    dispatch
@@:
    SBC_dec
    dispatch

; INC zp,X
    setorg opF5,256
opF6:
    opinit
    EA_zpX
    INC_com
    dispatch

; SED
    setorg opF7,256
opF8:
    opinit
    
    mov   srD,1         ; set D flag
    dispatch

; SBC abs,Y
    setorg opF8,256
opF9:
    opinit

    EA_absY
    cmp   srD,0
    jne   short @F

    SBC_com
    dispatch
@@:
    SBC_dec
    dispatch

; SBC abs,X
    setorg opFC,256
opFD:
    opinit

    EA_absX
    cmp   srD,0
    jne   short @F

    SBC_com
    dispatch

@@:
    SBC_dec
    dispatch

; INC abs,X
    setorg opFD,256
opFE:
    opinit
    EA_absX
    INC_com
    dispatch

    ret

IFNDEF HDOS16

    .DATA?

    align 4

jump_tab LABEL DWORD
    DD    512t DUP (?)


    .CONST

    PUBLIC jump_tab_RO

jump_tab_RO LABEL DWORD
    DD    op00
    DD    op01
    DD    unused
    DD    unused
    DD    unused
    DD    op05
    DD    op06
    DD    unused
    DD    op08
    DD    op09
    DD    op0A
    DD    unused
    DD    unused
    DD    op0D
    DD    op0E
    DD    unused
    DD    op10
    DD    op11
    DD    unused
    DD    unused
    DD    unused
    DD    op15
    DD    op16
    DD    unused
    DD    op18
    DD    op19
    DD    unused
    DD    unused
    DD    unused
    DD    op1D
    DD    op1E
    DD    unused
    DD    op20
    DD    op21
    DD    unused
    DD    unused
    DD    op24
    DD    op25
    DD    op26
    DD    unused
    DD    op28
    DD    op29
    DD    op2A
    DD    unused
    DD    op2C
    DD    op2D
    DD    op2E
    DD    unused
    DD    op30
    DD    op31
    DD    unused
    DD    unused
    DD    unused
    DD    op35
    DD    op36
    DD    unused
    DD    op38
    DD    op39
    DD    unused
    DD    unused
    DD    unused
    DD    op3D
    DD    op3E
    DD    unused
    DD    op40
    DD    op41
    DD    unused
    DD    unused
    DD    unused
    DD    op45
    DD    op46
    DD    unused
    DD    op48
    DD    op49
    DD    op4A
    DD    unused
    DD    op4C
    DD    op4D
    DD    op4E
    DD    unused
    DD    op50
    DD    op51
    DD    unused
    DD    unused
    DD    unused
    DD    op55
    DD    op56
    DD    unused
    DD    op58
    DD    op59
    DD    unused
    DD    unused
    DD    unused
    DD    op5D
    DD    op5E
    DD    unused
    DD    op60
    DD    op61
    DD    unused
    DD    unused
    DD    unused
    DD    op65
    DD    op66
    DD    unused
    DD    op68
    DD    op69
    DD    op6A
    DD    unused
    DD    op6C
    DD    op6D
    DD    op6E
    DD    unused
    DD    op70
    DD    op71
    DD    unused
    DD    unused
    DD    unused
    DD    op75
    DD    op76
    DD    unused
    DD    op78
    DD    op79
    DD    unused
    DD    unused
    DD    unused
    DD    op7D
    DD    op7E
    DD    unused
    DD    unused
    DD    op81
    DD    unused
    DD    unused
    DD    op84
    DD    op85
    DD    op86
    DD    unused
    DD    op88
    DD    unused
    DD    op8A
    DD    unused
    DD    op8C
    DD    op8D
    DD    op8E
    DD    unused
    DD    op90
    DD    op91
    DD    unused
    DD    unused
    DD    op94
    DD    op95
    DD    op96
    DD    unused
    DD    op98
    DD    op99
    DD    op9A
    DD    unused
    DD    unused
    DD    op9D
    DD    unused
    DD    unused
    DD    opA0
    DD    opA1
    DD    opA2
    DD    unused
    DD    opA4
    DD    opA5
    DD    opA6
    DD    unused
    DD    opA8
    DD    opA9
    DD    opAA
    DD    unused
    DD    opAC
    DD    opAD
    DD    opAE
    DD    unused
    DD    opB0
    DD    opB1
    DD    unused
    DD    unused
    DD    opB4
    DD    opB5
    DD    opB6
    DD    unused
    DD    opB8
    DD    opB9
    DD    opBA
    DD    unused
    DD    opBC
    DD    opBD
    DD    opBE
    DD    unused
    DD    opC0
    DD    opC1
    DD    unused
    DD    unused
    DD    opC4
    DD    opC5
    DD    opC6
    DD    unused
    DD    opC8
    DD    opC9
    DD    opCA
    DD    unused
    DD    opCC
    DD    opCD
    DD    opCE
    DD    unused
    DD    opD0
    DD    opD1
    DD    unused
    DD    unused
    DD    unused
    DD    opD5
    DD    opD6
    DD    unused
    DD    opD8
    DD    opD9
    DD    unused
    DD    unused
    DD    unused
    DD    opDD
    DD    opDE
    DD    opDF
    DD    opE0
    DD    opE1
    DD    unused
    DD    unused
    DD    opE4
    DD    opE5
    DD    opE6
    DD    unused
    DD    opE8
    DD    opE9
    DD    opEA
    DD    unused
    DD    opEC
    DD    opED
    DD    opEE
    DD    unused
    DD    opF0
    DD    opF1
    DD    unused
    DD    unused
    DD    unused
    DD    opF5
    DD    opF6
    DD    unused
    DD    opF8
    DD    opF9
    DD    unused
    DD    unused
    DD    unused
    DD    opFD
    DD    opFE
    DD    unused

ENDIF

    END

