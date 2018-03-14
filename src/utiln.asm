
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; UTILN.ASM
;;
;; Helper routines in assembly
;;
;; Copyright (C) 1991-2008 by Darek Mihocka. All Rights Reserved.
;; Branch Always Software. http://www.emulators.com/
;;
;; 08/26/2007   darekm      last update
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


    .486
    .MODEL FLAT
    OPTION SCOPED
    OPTION NOOLDMACROS
    .RADIX 16

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;                                                                        ;;
;;  High level routines called from C                                     ;;
;;                                                                        ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

    .DATA

temp DD 0

    .CODE

;;
;;  F486
;;
;;  Returns non-zero if running on a 486 or higher
;;

_F486 PROC
    mov   edx,esp
    and   esp,not 3          ; dword align stack

    pushfd
    pop   eax              ; get original EFLAGS
    mov   ecx,eax
    xor   eax,040000h      ; flip AC bit
    push  eax
    popfd
    pushfd
    pop   eax
    xor   eax,ecx          ; 0 = 386, non-zero = 486 and up

IF 1
    push  ecx             ; restore original EFLAGS
    popfd
ELSE
    mov   ecx,cr0
    or    ecx,040000h      ; flip AC bit
    mov   cr0,ecx
ENDIF

    mov   esp,edx          ; restore original unaligned stack pointer
    ret
_F486 ENDP


;;
;;  Blit[Mono|4|16][X][Inv](BYTE *pbSrc, BYTE *pbDst, LONG cbScanSrc);
;;
;;  Routines to blit interleaved video data from Atari ST memory
;;  to either the bitmap buffer or DirectX surface.
;;
;;  pbSrc     - pointer to Atari ST memory. Data goes backwards
;;  pbDst     - pointer to screen surface. Goes forwards.
;;  cbScanSrc - # of bytes of data to transfer. Should be a multiple of 8
;;  cbScanDst - # of bytes per scanline. Used for DirectX modes
;;

    align 4

_BlitMono PROC
    mov   edx,ss:[esp+8t]   ; pbDst
    mov   ecx,ss:[esp+12t]  ; cbScanSrc

    push  ebx
    mov   ebx,ss:[esp+8t]   ; pbSrc

    align 4
bm_loop:
    mov   eax,[ebx]
    add   edx,4
    lea   ebx,[ebx+4]
    not   eax
    sub   ecx,4
    mov   [edx-4],eax
    jne   bm_loop

    pop   ebx
    ret
_BlitMono ENDP

    align 4

_BlitMonoInv PROC
    mov   edx,[esp+8t]      ; pbDst
    mov   ecx,[esp+12t]     ; cbScanSrc

    push  ebx
    mov   ebx,[esp+8t]      ; pbSrc

    align 4
bmi_loop:
    mov   eax,[ebx]
    add   edx,4
    lea   ebx,[ebx+4]
    sub   ecx,4
    mov   [edx-4],eax
    jne   bmi_loop

    pop   ebx
    ret
_BlitMonoInv ENDP


    align 4

_BlitMonoX PROC
IF 1
    sub   esp,16t

    mov   [esp+4],ebx
    mov   [esp+8],edi
    mov   [esp+12t],esi
ELSE
    push  esi
    push  edi
    push  ebx
    push  ebp
ENDIF

    mov   esi,[esp+20t]     ; pbSrc
    mov   edi,[esp+24t]     ; pbDst
    mov   ecx,[esp+28t]     ; cbScanSrc

    align 4

bmx_loop:
    mov   edx,[esi]
    add   esi,4
    ror   edx,2

    ib = 0

    REPEAT 4
    mov   eax,edx
    rol   edx,4
    and   eax,60t
    mov   ebx,edx
    mov   eax,dword ptr [mpntol+eax]
    and   ebx,60t
    mov   [edi+ib],eax
    mov   eax,dword ptr [mpntol+ebx]
    IF (ib NE 24t)
        ror   edx,12t
    ENDIF
    mov   [edi+ib+4],eax

    ib = ib + 8
    ENDM

    sub   ecx,4
    lea   edi,[edi+32t]
    jne   bmx_loop

    mov   ebx,[esp+4]
    mov   edi,[esp+8]
    mov   esi,[esp+12t]

    add   esp,16t
    ret
_BlitMonoX ENDP

    align 4

_BlitMonoXInv PROC
    push  ebx
    push  edi
    push  esi

    mov   esi,ss:[esp+16t]  ; pbSrc
    mov   edi,ss:[esp+20t]  ; pbDst
    mov   ecx,ss:[esp+24t]  ; cbScanSrc

    align 4

bmxi_loop:
    mov   edx,[esi]
    add   esi,4
    ror   edx,2
    not   edx

    ib = 0

    REPEAT 4
    mov   eax,edx
    rol   edx,4
    and   eax,60t
    mov   ebx,edx
    mov   eax,dword ptr [mpntol+eax]
    and   ebx,60t
    mov   [edi+ib],eax
    mov   eax,dword ptr [mpntol+ebx]
    IF (ib NE 24t)
        ror   edx,12t
    ENDIF
    mov   [edi+ib+4],eax

    ib = ib + 8
    ENDM

    sub   ecx,4
    lea   edi,[edi+32t]
    jne   bmxi_loop

    pop   esi
    pop   edi
    pop   ebx
    ret
_BlitMonoXInv ENDP


_Blit4 PROC
    push  ebx
    push  edi
    push  ebp

    mov   ebx,ss:[esp+16t]  ; pbSrc
    mov   edx,ss:[esp+20t]  ; pbDst
    mov   ecx,ss:[esp+24t]  ; cbScanSrc
    mov   ebp,0             ; cbScanDst

b4_loop::
    mov   edi,[ebx]         ; plane 0 | plane 1
    bswap edi
    ror   edi,1

    REPEAT 4
    REPEAT 4
    shl   eax,6
    ror   edi,15t
    adc   eax,eax
    ror   edi,16t
    adc   eax,eax
    ENDM

    bswap eax
    lea   edx,[edx+4]
    and   eax,03030303h
    add   eax,0A0A0A0Ah
    mov   [edx-4],eax
    mov   [edx+ebp-4],eax
    ENDM

    sub   ecx,4
    lea   ebx,[ebx+4]
    jne   b4_loop

    pop   ebp
    pop   edi
    pop   ebx
    ret
_Blit4 ENDP


_Blit4X PROC
    push  ebx
    push  edi
    push  ebp

    mov   ebx,ss:[esp+16t]  ; pbSrc
    mov   edx,ss:[esp+20t]  ; pbDst
    mov   ecx,ss:[esp+24t]  ; cbScanSrc
    mov   ebp,ss:[esp+28t]  ; cbScanDst

    jmp   b4_loop
_Blit4X ENDP


_Blit16 PROC
    push  ebx
    push  esi
    push  edi

    mov   ebx,ss:[esp+16t]  ; pbSrc
    mov   edx,ss:[esp+20t]  ; pbDst
    mov   ecx,ss:[esp+24t]  ; cbScanSrc

b16_loop:
    mov   edi,[ebx]         ; plane 0 | plane 1
    bswap edi
    ror   edi,1
    mov   esi,[ebx+4]       ; plane 2 | plane 3
    bswap esi
    ror   esi,1

    REPEAT 4
    REPEAT 4
    shl   eax,4
    ror   esi,15t
    adc   eax,eax
    ror   esi,16t
    adc   eax,eax
    ror   edi,15t
    adc   eax,eax
    ror   edi,16t
    adc   eax,eax
    ENDM

    bswap eax
    lea   edx,[edx+4]
    and   eax,0F0F0F0Fh
    add   eax,0A0A0A0Ah
    mov   [edx-4],eax
    ENDM

    sub   ecx,8
    lea   ebx,[ebx+8]
    jg    b16_loop

    pop   edi
    pop   esi
    pop   ebx
    ret
_Blit16 ENDP


_Blit16X PROC
    push  ebx
    push  esi
    push  edi
    push  ebp

    mov   ebx,ss:[esp+20t]  ; pbSrc
    mov   edx,ss:[esp+24t]  ; pbDst
    mov   ecx,ss:[esp+28t]  ; cbScanSrc
    mov   ebp,ss:[esp+32t]  ; cbScanDst

b16x_loop:
    mov   edi,[ebx]         ; plane 0 | plane 1
    bswap edi
    ror   edi,1
    mov   esi,[ebx+4]       ; plane 2 | plane 3
    bswap esi
    ror   esi,1

    REPEAT 8
    REPEAT 2
    shl   eax,12t
    ror   esi,15t
    adc   eax,eax
    ror   esi,16t
    adc   eax,eax
    ror   edi,15t
    adc   eax,eax
    ror   edi,16t
    adc   eax,eax
    mov   ah,al
    ENDM

    bswap eax
    lea   edx,[edx+4]
    and   eax,0F0F0F0Fh
    add   eax,0A0A0A0Ah
    mov   [edx-4],eax
    mov   [edx+ebp-4],eax
    ENDM

    sub   ecx,8
    lea   ebx,[ebx+8]
    jg    b16x_loop

    pop   ebp
    pop   edi
    pop   esi
    pop   ebx
    ret
_Blit16X ENDP

    .CONST

    align 4

mpntol LABEL DWORD
    DD 0FFFFFFFFh
    DD 000FFFFFFh
    DD 0FF00FFFFh
    DD 00000FFFFh
    DD 0FFFF00FFh
    DD 000FF00FFh
    DD 0FF0000FFh
    DD 0000000FFh
    DD 0FFFFFF00h
    DD 000FFFF00h
    DD 0FF00FF00h
    DD 00000FF00h
    DD 0FFFF0000h
    DD 000FF0000h
    DD 0FF000000h
    DD 000000000h


    END


