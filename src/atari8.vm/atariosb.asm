
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; ATARIOSB.ASM
;;
;; Hex dump of Atari 800 OS revision B
;;
;; Atari OS and BASIC used with permission. Copyright (C) 1979-1984 Atari Corp.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

IFDEF HWIN32
    .486
    .MODEL FLAT,C
    OPTION NOSCOPED
    OPTION NOOLDMACROS
ENDIF

IFDEF HDOS32
    .486
    DOSSEG
    .MODEL FLAT
    OPTION NOSCOPED
    OPTION NOOLDMACROS
ENDIF

IFDEF HDOS16
    DOSSEG
    .MODEL   large,c
    .286
ENDIF

    .CONST

    PUBLIC rgbAtariOSB, rgbMapScans

rgbAtariOSB LABEL WORD

    DW 0a120h, 020dbh, 0dbbbh, 039b0h, 0eda2h, 004a0h, 04820h, 0a2dah
    DW 086ffh, 020f1h, 0da44h, 004f0h, 0ffa9h, 0f085h, 09420h, 0b0dbh
    DW 04821h, 0d5a6h, 011d0h, 0eb20h, 068dbh, 0d905h, 0d985h, 0f1a6h
    DW 0e630h, 086e8h, 0d0f1h, 068e1h, 0f1a6h, 00210h, 0ede6h, 0184ch
    DW 060d8h, 02ec9h, 014f0h, 045c9h, 019f0h, 0f0a6h, 068d0h, 02bc9h
    DW 0c6f0h, 02dc9h, 000f0h, 0ee85h, 0bef0h, 0f1a6h, 05810h, 086e8h
    DW 0f0f1h, 0a5b5h, 085f2h, 020ech, 0db94h, 037b0h, 0a5aah, 048edh
    DW 0ed86h, 09420h, 0b0dbh, 04817h, 0eda5h, 0850ah, 00aedh, 0650ah
    DW 085edh, 068edh, 06518h, 085edh, 0a4edh, 020f2h, 0db9dh, 0efa5h
    DW 009f0h, 0eda5h, 0ff49h, 06918h, 08501h, 068edh, 06518h, 085edh
    DW 0d0edh, 0c913h, 0f02bh, 0c906h, 0d02dh, 08507h, 020efh, 0db94h
    DW 0ba90h, 0eca5h, 0f285h, 0f2c6h, 0eda5h, 0f1a6h, 00530h, 003f0h
    DW 0e538h, 048f1h, 0682ah, 0856ah, 090edh, 02003h, 0dbebh, 0eda5h
    DW 06918h, 08544h, 020d4h, 0dc00h, 00bb0h, 0eea6h, 006f0h, 0d4a5h
    DW 08009h, 0d485h, 06018h, 05120h, 0a9dah, 08d30h, 0057fh, 0d4a5h
    DW 028f0h, 07f29h, 03fc9h, 02890h, 045c9h, 024b0h, 0e938h, 0203fh
    DW 0dc70h, 0a420h, 009dch, 09d80h, 00580h, 080adh, 0c905h, 0f02eh
    DW 04c03h, 0d988h, 0c120h, 04cdch, 0d99ch, 0b0a9h, 0808dh, 06005h
    DW 001a9h, 07020h, 020dch, 0dca4h, 086e8h, 0a5f2h, 00ad4h, 0e938h
    DW 0ae80h, 00580h, 030e0h, 017f0h, 081aeh, 0ac05h, 00582h, 0828eh
    DW 08c05h, 00581h, 0f2a6h, 002e0h, 002d0h, 0f2e6h, 06918h, 08501h
    DW 0a9edh, 0a445h, 020f2h, 0dc9fh, 0f284h, 0eda5h, 00b10h, 000a9h
    DW 0e538h, 085edh, 0a9edh, 0d02dh, 0a902h, 0202bh, 0dc9fh, 000a2h
    DW 0eda5h, 0e938h, 0900ah, 0e803h, 0f8d0h, 06918h, 0480ah, 0208ah
    DW 0dc9dh, 00968h, 02080h, 0dc9dh, 080adh, 0c905h, 0d030h, 0180dh
    DW 0f3a5h, 00169h, 0f385h, 0f4a5h, 00069h, 0f485h, 0d4a5h, 00910h
    DW 0c120h, 0a0dch, 0a900h, 0912dh, 060f3h, 0d4a5h, 0f885h, 0d5a5h
    DW 0f785h, 04420h, 0f8dah, 010a0h, 0f806h, 0f726h, 003a2h, 0d4b5h
    DW 0d475h, 0d495h, 0d0cah, 088f7h, 0eed0h, 0a9d8h, 08542h, 04cd4h
    DW 0dc00h, 000a9h, 0f785h, 0f885h, 0d4a5h, 06630h, 043c9h, 062b0h
    DW 0e938h, 09040h, 0693fh, 00a00h, 0f585h, 05a20h, 0b0dah, 0a553h
    DW 085f7h, 0a5f9h, 085f8h, 020fah, 0da5ah, 046b0h, 05a20h, 0b0dah
    DW 01841h, 0f8a5h, 0fa65h, 0f885h, 0f7a5h, 0f965h, 0f785h, 032b0h
    DW 0b920h, 018dch, 0f865h, 0f885h, 0f7a5h, 00069h, 024b0h, 0f785h
    DW 0f5c6h, 0c6d0h, 0b920h, 0c9dch, 09005h, 0180dh, 0f8a5h, 00169h
    DW 0f885h, 0f7a5h, 00069h, 0f785h, 0f8a5h, 0d485h, 0f7a5h, 0d585h
    DW 06018h, 06038h, 0d4a2h, 006a0h, 000a9h, 00095h, 088e8h, 0fad0h
    DW 0a960h, 08505h, 0a9f4h, 08580h, 060f3h, 02618h, 026f8h, 060f7h
    DW 0e0a5h, 08049h, 0e085h, 0e0a5h, 07f29h, 0f785h, 0d4a5h, 07f29h
    DW 0e538h, 010f7h, 0a210h, 0b505h, 0b4d4h, 095e0h, 098e0h, 0d495h
    DW 010cah, 030f4h, 0f0e1h, 0c907h, 0b005h, 02019h, 0dc3eh, 0a5f8h
    DW 045d4h, 030e0h, 0a21eh, 01804h, 0d5b5h, 0e175h, 0d595h, 010cah
    DW 0d8f7h, 003b0h, 0004ch, 0a9dch, 02001h, 0dc3ah, 001a9h, 0d585h
    DW 0004ch, 0a2dch, 03804h, 0d5b5h, 0e1f5h, 0d595h, 010cah, 090f7h
    DW 0d804h, 0004ch, 0a5dch, 049d4h, 08580h, 038d4h, 004a2h, 000a9h
    DW 0d5f5h, 0d595h, 010cah, 0d8f7h, 0004ch, 0a5dch, 0f0d4h, 0a545h
    DW 0f0e0h, 0203eh, 0dccfh, 0e938h, 03840h, 0e065h, 03830h, 0e020h
    DW 0a5dch, 029dfh, 0850fh, 0c6f6h, 030f6h, 02006h, 0dd01h, 0f74ch
    DW 0a5dah, 04adfh, 04a4ah, 0854ah, 0c6f6h, 030f6h, 02006h, 0dd05h
    DW 0094ch, 020dbh, 0dc62h, 0f5c6h, 0d7d0h, 0eda5h, 0d485h, 0044ch
    DW 020dch, 0da44h, 06018h, 06038h, 0e0a5h, 0faf0h, 0d4a5h, 0f4f0h
    DW 0cf20h, 038dch, 0e0e5h, 06918h, 03040h, 020ebh, 0dce0h, 0f5e6h
    DW 04e4ch, 0a2dbh, 0b500h, 095d5h, 0e8d4h, 00ce0h, 0f7d0h, 005a0h
    DW 0f838h, 0dab9h, 0f900h, 000e6h, 0da99h, 08800h, 0f410h, 090d8h
    DW 0e604h, 0d0d9h, 020e9h, 0dd0fh, 0d906h, 0d906h, 0d906h, 0d906h
    DW 005a0h, 0f838h, 0dab9h, 0f900h, 000e0h, 0da99h, 08800h, 0f410h
    DW 090d8h, 0e604h, 0d0d9h, 020e9h, 0dd09h, 0f5c6h, 0b5d0h, 06220h
    DW 04cdch, 0db1ah, 0af20h, 0a4dbh, 090f2h, 0b102h, 0c8f3h, 0f284h
    DW 0a460h, 0a9f2h, 0d120h, 0d0f3h, 0c803h, 0f9d0h, 0f284h, 0a460h
    DW 0b1f2h, 038f3h, 030e9h, 01890h, 00ac9h, 0a560h, 048f2h, 09420h
    DW 090dbh, 0c91fh, 0f02eh, 0c914h, 0f02bh, 0c907h, 0f02dh, 06803h
    DW 06038h, 09420h, 090dbh, 0c90bh, 0d02eh, 020f4h, 0db94h, 00290h
    DW 0edb0h, 08568h, 018f2h, 0a260h, 0d0e7h, 0a202h, 0a0d5h, 01804h
    DW 00436h, 00336h, 00236h, 00136h, 00036h, 0ec26h, 0d088h, 060f0h
    DW 000a2h, 0da86h, 004a2h, 0d4a5h, 02ef0h, 0d5a5h, 01ad0h, 000a0h
    DW 0d6b9h, 09900h, 000d5h, 0c0c8h, 09005h, 0c6f5h, 0cad4h, 0ead0h
    DW 0d5a5h, 004d0h, 0d485h, 06018h, 0d4a5h, 07f29h, 071c9h, 00190h
    DW 0c960h, 0b00fh, 02003h, 0da44h, 06018h, 0d4a2h, 002d0h, 0e0a2h
    DW 0f986h, 0f785h, 0f885h, 004a0h, 004b5h, 00595h, 088cah, 0f8d0h
    DW 000a9h, 00595h, 0f9a6h, 0f7c6h, 0ecd0h, 000b5h, 06518h, 095f8h
    DW 06000h, 00aa2h, 0d4b5h, 0d595h, 010cah, 0a9f9h, 08500h, 060d4h
    DW 0f785h, 000a2h, 000a0h, 09320h, 038dch, 001e9h, 0f785h, 0d5b5h
    DW 04a4ah, 04a4ah, 09d20h, 0b5dch, 029d5h, 0200fh, 0dc9dh, 0e0e8h
    DW 09005h, 0a5e3h, 0d0f7h, 0a905h, 0202eh, 0dc9fh, 00960h, 09930h
    DW 00580h, 060c8h, 00aa2h, 080bdh, 0c905h, 0f02eh, 0c907h, 0d030h
    DW 0ca07h, 0f2d0h, 0bdcah, 00580h, 02060h, 0dbebh, 0eca5h, 00f29h
    DW 03860h, 0f3a5h, 001e9h, 0f385h, 0f4a5h, 000e9h, 0f485h, 0a560h
    DW 045d4h, 029e0h, 08580h, 006eeh, 046e0h, 0a5e0h, 029d4h, 0607fh
    DW 0ee05h, 0ed85h, 000a9h, 0d485h, 0e085h, 02820h, 020ddh, 0dbe7h
    DW 0eca5h, 00f29h, 0e685h, 005a9h, 0f585h, 03420h, 020ddh, 0da44h
    DW 0a260h, 0d0d9h, 0a206h, 0d0d9h, 0a208h, 0a0dfh, 0d0e5h, 0a204h
    DW 0a0dfh, 0a9ebh, 08505h, 018f7h, 0b5f8h, 07900h, 00000h, 00095h
    DW 088cah, 0f7c6h, 0f310h, 060d8h, 005a0h, 0e0b9h, 09900h, 000e6h
    DW 01088h, 060f7h, 005a0h, 0d4b9h, 09900h, 000dah, 01088h, 060f7h
    DW 0fe86h, 0ff84h, 0ef85h, 0e0a2h, 005a0h, 0a720h, 020ddh, 0ddb6h
    DW 0fea6h, 0ffa4h, 08920h, 0c6ddh, 0f0efh, 0202dh, 0dadbh, 028b0h
    DW 0a518h, 069feh, 08506h, 090feh, 0a506h, 069ffh, 08500h, 0a6ffh
    DW 0a4feh, 020ffh, 0dd98h, 06620h, 0b0dah, 0c60dh, 0f0efh, 0a209h
    DW 0a0e0h, 02005h, 0dd98h, 0d330h, 08660h, 084fch, 0a0fdh, 0b105h
    DW 099fch, 000d4h, 01088h, 060f8h, 0fc86h, 0fd84h, 005a0h, 0fcb1h
    DW 0e099h, 08800h, 0f810h, 08660h, 084fch, 0a0fdh, 0b905h, 000d4h
    DW 0fc91h, 01088h, 060f8h, 005a2h, 0d4b5h, 0e095h, 010cah, 060f9h
    DW 089a2h, 0dea0h, 09820h, 020ddh, 0dadbh, 07fb0h, 000a9h, 0f185h
    DW 0d4a5h, 0f085h, 07f29h, 0d485h, 0e938h, 03040h, 0c926h, 01004h
    DW 0a26ah, 0a0e6h, 02005h, 0dda7h, 0d220h, 0a5d9h, 085d4h, 0a5f1h
    DW 0d0d5h, 02058h, 0d9aah, 0b620h, 0a2ddh, 0a0e6h, 02005h, 0dd89h
    DW 06020h, 0a9dah, 0a20ah, 0a04dh, 020deh, 0dd40h, 0b620h, 020ddh
    DW 0dadbh, 0f1a5h, 023f0h, 06a18h, 0e085h, 001a9h, 00290h, 010a9h
    DW 0e185h, 004a2h, 000a9h, 0e295h, 010cah, 0a5fbh, 018e0h, 04069h
    DW 019b0h, 01730h, 0e085h, 0db20h, 0a5dah, 010f0h, 0200dh, 0ddb6h
    DW 08fa2h, 0dea0h, 08920h, 020ddh, 0db28h, 03860h, 03d60h, 09417h
    DW 00019h, 03d00h, 03357h, 00005h, 03e00h, 05405h, 06276h, 03e00h
    DW 01932h, 02762h, 03f00h, 06801h, 03060h, 03f36h, 03207h, 02703h
    DW 03f41h, 04325h, 05634h, 03f75h, 02766h, 03037h, 04050h, 01501h
    DW 09212h, 03f55h, 09999h, 09999h, 03f99h, 04243h, 04894h, 04019h
    DW 00001h, 00000h, 08600h, 084feh, 0a2ffh, 0a0e0h, 02005h, 0dda7h
    DW 0fea6h, 0ffa4h, 09820h, 020ddh, 0da66h, 0e6a2h, 005a0h, 0a720h
    DW 0a2ddh, 0a0e0h, 02005h, 0dd89h, 0fea6h, 0ffa4h, 09820h, 020ddh
    DW 0da60h, 0e6a2h, 005a0h, 09820h, 020ddh, 0db28h, 0a960h, 0d001h
    DW 0a902h, 08500h, 0a5f0h, 010d4h, 03802h, 0a560h, 085d4h, 038e0h
    DW 040e9h, 0850ah, 0a5f1h, 029d5h, 0d0f0h, 0a904h, 0d001h, 0e604h
    DW 0a9f1h, 08510h, 0a2e1h, 0a904h, 09500h, 0cae2h, 0fb10h, 02820h
    DW 0a2dbh, 0a066h, 020dfh, 0de95h, 0e6a2h, 005a0h, 0a720h, 020ddh
    DW 0ddb6h, 0db20h, 0a9dah, 0a20ah, 0a072h, 020dfh, 0dd40h, 0e6a2h
    DW 005a0h, 09820h, 020ddh, 0dadbh, 06ca2h, 0dfa0h, 09820h, 020ddh
    DW 0da66h, 0b620h, 0a9ddh, 08500h, 0a5d5h, 085f1h, 010d4h, 04907h
    DW 018ffh, 00169h, 0d485h, 0aa20h, 024d9h, 010f1h, 0a906h, 00580h
    DW 085d4h, 020d4h, 0da66h, 0f0a5h, 00af0h, 089a2h, 0dea0h, 09820h
    DW 020ddh, 0db28h, 06018h, 00340h, 02216h, 06677h, 0503fh, 00000h
    DW 00000h, 0493fh, 05715h, 00811h, 051bfh, 04970h, 00847h, 0393fh
    DW 05720h, 09561h, 004bfh, 06339h, 05503h, 0103fh, 03009h, 06412h
    DW 0093fh, 00839h, 06004h, 0123fh, 05842h, 04247h, 0173fh, 01237h
    DW 00806h, 0283fh, 02995h, 01771h, 0863fh, 08885h, 04496h, 0163eh
    DW 04405h, 00049h, 095beh, 03868h, 00045h, 0023fh, 07968h, 01694h
    DW 004bfh, 07892h, 08090h, 0073fh, 01503h, 00020h, 008bfh, 02992h
    DW 04412h, 0113fh, 04008h, 01109h, 014bfh, 03128h, 00456h, 0193fh
    DW 09899h, 04477h, 033bfh, 03333h, 01331h, 0993fh, 09999h, 09999h
    DW 0783fh, 09853h, 03416h, 01698h, 0fc34h, 032e0h, 0d950h, 01168h
    DW 00000h, 00000h, 00000h, 00000h, 01800h, 01818h, 00018h, 00018h
    DW 06600h, 06666h, 00000h, 00000h, 06600h, 066ffh, 0ff66h, 00066h
    DW 03e18h, 03c60h, 07c06h, 00018h, 06600h, 0186ch, 06630h, 00046h
    DW 0361ch, 0381ch, 0666fh, 0003bh, 01800h, 01818h, 00000h, 00000h
    DW 00e00h, 0181ch, 01c18h, 0000eh, 07000h, 01838h, 03818h, 00070h
    DW 06600h, 0ff3ch, 0663ch, 00000h, 01800h, 07e18h, 01818h, 00000h
    DW 00000h, 00000h, 01800h, 03018h, 00000h, 07e00h, 00000h, 00000h
    DW 00000h, 00000h, 01800h, 00018h, 00600h, 0180ch, 06030h, 00040h
    DW 03c00h, 06e66h, 06676h, 0003ch, 01800h, 01838h, 01818h, 0007eh
    DW 03c00h, 00c66h, 03018h, 0007eh, 07e00h, 0180ch, 0660ch, 0003ch
    DW 00c00h, 03c1ch, 07e6ch, 0000ch, 07e00h, 07c60h, 06606h, 0003ch
    DW 03c00h, 07c60h, 06666h, 0003ch, 07e00h, 00c06h, 03018h, 00030h
    DW 03c00h, 03c66h, 06666h, 0003ch, 03c00h, 03e66h, 00c06h, 00038h
    DW 00000h, 01818h, 01800h, 00018h, 00000h, 01818h, 01800h, 03018h
    DW 00c06h, 03018h, 00c18h, 00006h, 00000h, 0007eh, 07e00h, 00000h
    DW 03060h, 00c18h, 03018h, 00060h, 03c00h, 00c66h, 00018h, 00018h
    DW 03c00h, 06e66h, 0606eh, 0003eh, 01800h, 0663ch, 07e66h, 00066h
    DW 07c00h, 07c66h, 06666h, 0007ch, 03c00h, 06066h, 06660h, 0003ch
    DW 07800h, 0666ch, 06c66h, 00078h, 07e00h, 07c60h, 06060h, 0007eh
    DW 07e00h, 07c60h, 06060h, 00060h, 03e00h, 06060h, 0666eh, 0003eh
    DW 06600h, 07e66h, 06666h, 00066h, 07e00h, 01818h, 01818h, 0007eh
    DW 00600h, 00606h, 06606h, 0003ch, 06600h, 0786ch, 06c78h, 00066h
    DW 06000h, 06060h, 06060h, 0007eh, 06300h, 07f77h, 0636bh, 00063h
    DW 06600h, 07e76h, 06e7eh, 00066h, 03c00h, 06666h, 06666h, 0003ch
    DW 07c00h, 06666h, 0607ch, 00060h, 03c00h, 06666h, 06c66h, 00036h
    DW 07c00h, 06666h, 06c7ch, 00066h, 03c00h, 03c60h, 00606h, 0003ch
    DW 07e00h, 01818h, 01818h, 00018h, 06600h, 06666h, 06666h, 0007eh
    DW 06600h, 06666h, 03c66h, 00018h, 06300h, 06b63h, 0777fh, 00063h
    DW 06600h, 03c66h, 0663ch, 00066h, 06600h, 03c66h, 01818h, 00018h
    DW 07e00h, 0180ch, 06030h, 0007eh, 01e00h, 01818h, 01818h, 0001eh
    DW 04000h, 03060h, 00c18h, 00006h, 07800h, 01818h, 01818h, 00078h
    DW 00800h, 0361ch, 00063h, 00000h, 00000h, 00000h, 00000h, 000ffh
    DW 03600h, 07f7fh, 01c3eh, 00008h, 01818h, 01f18h, 0181fh, 01818h
    DW 00303h, 00303h, 00303h, 00303h, 01818h, 0f818h, 000f8h, 00000h
    DW 01818h, 0f818h, 018f8h, 01818h, 00000h, 0f800h, 018f8h, 01818h
    DW 00703h, 01c0eh, 07038h, 0c0e0h, 0e0c0h, 03870h, 00e1ch, 00307h
    DW 00301h, 00f07h, 03f1fh, 0ff7fh, 00000h, 00000h, 00f0fh, 00f0fh
    DW 0c080h, 0f0e0h, 0fcf8h, 0fffeh, 00f0fh, 00f0fh, 00000h, 00000h
    DW 0f0f0h, 0f0f0h, 00000h, 00000h, 0ffffh, 00000h, 00000h, 00000h
    DW 00000h, 00000h, 00000h, 0ffffh, 00000h, 00000h, 0f0f0h, 0f0f0h
    DW 01c00h, 0771ch, 00877h, 0001ch, 00000h, 01f00h, 0181fh, 01818h
    DW 00000h, 0ff00h, 000ffh, 00000h, 01818h, 0ff18h, 018ffh, 01818h
    DW 00000h, 07e3ch, 07e7eh, 0003ch, 00000h, 00000h, 0ffffh, 0ffffh
    DW 0c0c0h, 0c0c0h, 0c0c0h, 0c0c0h, 00000h, 0ff00h, 018ffh, 01818h
    DW 01818h, 0ff18h, 000ffh, 00000h, 0f0f0h, 0f0f0h, 0f0f0h, 0f0f0h
    DW 01818h, 01f18h, 0001fh, 00000h, 06078h, 06078h, 0187eh, 0001eh
    DW 01800h, 07e3ch, 01818h, 00018h, 01800h, 01818h, 03c7eh, 00018h
    DW 01800h, 07e30h, 01830h, 00000h, 01800h, 07e0ch, 0180ch, 00000h
    DW 01800h, 07e3ch, 03c7eh, 00018h, 00000h, 0063ch, 0663eh, 0003eh
    DW 06000h, 07c60h, 06666h, 0007ch, 00000h, 0603ch, 06060h, 0003ch
    DW 00600h, 03e06h, 06666h, 0003eh, 00000h, 0663ch, 0607eh, 0003ch
    DW 00e00h, 03e18h, 01818h, 00018h, 00000h, 0663eh, 03e66h, 07c06h
    DW 06000h, 07c60h, 06666h, 00066h, 01800h, 03800h, 01818h, 0003ch
    DW 00600h, 00600h, 00606h, 03c06h, 06000h, 06c60h, 06c78h, 00066h
    DW 03800h, 01818h, 01818h, 0003ch, 00000h, 07f66h, 06b7fh, 00063h
    DW 00000h, 0667ch, 06666h, 00066h, 00000h, 0663ch, 06666h, 0003ch
    DW 00000h, 0667ch, 07c66h, 06060h, 00000h, 0663eh, 03e66h, 00606h
    DW 00000h, 0667ch, 06060h, 00060h, 00000h, 0603eh, 0063ch, 0007ch
    DW 01800h, 0187eh, 01818h, 0000eh, 00000h, 06666h, 06666h, 0003eh
    DW 00000h, 06666h, 03c66h, 00018h, 00000h, 06b63h, 03e7fh, 00036h
    DW 00000h, 03c66h, 03c18h, 00066h, 00000h, 06666h, 03e66h, 0780ch
    DW 00000h, 00c7eh, 03018h, 0007eh, 01800h, 07e3ch, 0187eh, 0003ch
    DW 01818h, 01818h, 01818h, 01818h, 07e00h, 07c78h, 0666eh, 00006h
    DW 01808h, 07838h, 01838h, 00008h, 01810h, 01e1ch, 0181ch, 00010h
    DW 0f3fbh, 0f633h, 0f63dh, 0f6a3h, 0f633h, 0f63ch, 0e44ch, 000f3h
    DW 0f3f5h, 0f633h, 0f592h, 0f5b6h, 0f633h, 0fcfbh, 0e44ch, 000f3h
    DW 0f633h, 0f633h, 0f6e1h, 0f63ch, 0f633h, 0f63ch, 0e44ch, 000f3h
    DW 0ee9eh, 0eedbh, 0ee9dh, 0eea6h, 0ee80h, 0ee9dh, 0784ch, 000eeh
    DW 0ef4bh, 0f02ah, 0efd5h, 0f00fh, 0f027h, 0ef4ah, 0414ch, 000efh
    DW 0ea4ch, 04cedh, 0edf0h, 0c44ch, 04ce4h, 0e959h, 0ed4ch, 04ce8h
    DW 0e7aeh, 0054ch, 04ce9h, 0e944h, 0f24ch, 04cebh, 0e6d5h, 0a64ch
    DW 04ce4h, 0f223h, 01b4ch, 04cf1h, 0f125h, 0e94ch, 04cefh, 0ef5dh
    DW 0e790h, 0e78fh, 0e78fh, 0e78fh, 0ffbeh, 0eb0fh, 0ea90h, 0eacfh
    DW 0e78fh, 0e78fh, 0e78fh, 0e706h, 00000h, 00000h, 00000h, 00000h
    DW 00000h, 0e7aeh, 0e905h, 000a2h, 0ffa9h, 0409dh, 0a903h, 09dc0h
    DW 00346h, 0e4a9h, 0479dh, 08a03h, 06918h, 0aa10h, 080c9h, 0e890h
    DW 0a060h, 06085h, 02f85h, 02e86h, 0298ah, 0d00fh, 0e004h, 09080h
    DW 0a005h, 04c86h, 0e61bh, 000a0h, 040bdh, 09903h, 00020h, 0c8e8h
    DW 00cc0h, 0f490h, 084a0h, 022a5h, 003c9h, 02590h, 0c0a8h, 0900eh
    DW 0a002h, 0840eh, 0b917h, 0e6c6h, 00ff0h, 002c9h, 035f0h, 008c9h
    DW 04cb0h, 004c9h, 063f0h, 0c94ch, 0a5e5h, 0c920h, 0f0ffh, 0a005h
    DW 04c81h, 0e61bh, 09e20h, 0b0e6h, 020f8h, 0e63dh, 0f3b0h, 08920h
    DW 0a9e6h, 0850bh, 02017h, 0e63dh, 02ca5h, 02685h, 02da5h, 02785h
    DW 01d4ch, 0a0e6h, 08401h, 02023h, 0e63dh, 003b0h, 08920h, 0a9e6h
    DW 085ffh, 0a920h, 085e4h, 0a927h, 085c0h, 04c26h, 0e61dh, 020a5h
    DW 0ffc9h, 005d0h, 09e20h, 0b0e6h, 020b8h, 0e63dh, 08920h, 0a6e6h
    DW 0bd2eh, 00340h, 02085h, 01d4ch, 0a5e6h, 02522h, 0d02ah, 0a005h
    DW 04c83h, 0e61bh, 03d20h, 0b0e6h, 0a5f8h, 00528h, 0d029h, 02008h
    DW 0e689h, 02f85h, 01d4ch, 020e6h, 0e689h, 02f85h, 03530h, 000a0h
    DW 02491h, 07020h, 0a5e6h, 02922h, 0d002h, 0a50ch, 0c92fh, 0d09bh
    DW 02006h, 0e663h, 0c34ch, 020e5h, 0e663h, 0dbd0h, 022a5h, 00229h
    DW 011d0h, 08920h, 085e6h, 0302fh, 0a50ah, 0c92fh, 0d09bh, 0a9f3h
    DW 08589h, 02023h, 0e677h, 01d4ch, 0a5e6h, 02522h, 0d02ah, 0a005h
    DW 04c87h, 0e61bh, 03d20h, 0b0e6h, 0a5f8h, 00528h, 0d029h, 0a506h
    DW 0e62fh, 0d028h, 0a006h, 0b100h, 08524h, 0202fh, 0e689h, 02530h
    DW 07020h, 0a5e6h, 02922h, 0d002h, 0a50ch, 0c92fh, 0d09bh, 02006h
    DW 0e663h, 0154ch, 020e6h, 0e663h, 0dbd0h, 022a5h, 00229h, 005d0h
    DW 09ba9h, 08920h, 020e6h, 0e677h, 01d4ch, 084e6h, 0a423h, 0b92eh
    DW 00344h, 02485h, 045b9h, 08503h, 0a225h, 0b500h, 09920h, 00340h
    DW 0c8e8h, 00ce0h, 0f590h, 02fa5h, 02ea6h, 023a4h, 0a460h, 0c020h
    DW 09022h, 0a004h, 0b085h, 0b91bh, 0031bh, 02c85h, 01cb9h, 08503h
    DW 0a42dh, 0b917h, 0e6c6h, 0b1a8h, 0aa2ch, 0b1c8h, 0852ch, 0862dh
    DW 0182ch, 0c660h, 0a528h, 0c928h, 0d0ffh, 0c602h, 00529h, 06029h
    DW 024e6h, 002d0h, 025e6h, 0a660h, 0382eh, 048bdh, 0e503h, 08528h
    DW 0bd28h, 00349h, 029e5h, 02985h, 0a060h, 02092h, 0e693h, 02384h
    DW 000c0h, 0aa60h, 02da5h, 0a548h, 0482ch, 0a68ah, 0602eh, 000a0h
    DW 024b1h, 00cf0h, 021a0h, 01ad9h, 0f003h, 0880ah, 08888h, 0f610h
    DW 082a0h, 0b038h, 09813h, 02085h, 0a038h, 0b101h, 0e924h, 0c930h
    DW 0900ah, 0a902h, 08501h, 01821h, 00060h, 00404h, 00404h, 00606h
    DW 00606h, 00802h, 0a90ah, 08d40h, 0d40eh, 038a9h, 0028dh, 08dd3h
    DW 0d303h, 000a9h, 0008dh, 0ead3h, 0eaeah, 03ca9h, 0028dh, 08dd3h
    DW 0d303h, 06c60h, 00216h, 04080h, 00204h, 00801h, 02010h, 00836h
    DW 01214h, 00e10h, 00a0ch, 0ad48h, 0d20eh, 02029h, 00dd0h, 0dfa9h
    DW 00e8dh, 0a5d2h, 08d10h, 0d20eh, 00a6ch, 08a02h, 0a248h, 0bd06h
    DW 0e6f6h, 005e0h, 004d0h, 01025h, 005f0h, 00e2ch, 0f0d2h, 0ca06h
    DW 0ed10h, 0624ch, 049e7h, 08dffh, 0d20eh, 010a5h, 00e8dh, 0bdd2h
    DW 0e6feh, 0bdaah, 00200h, 08c8dh, 0bd02h, 00201h, 08d8dh, 06802h
    DW 06caah, 0028ch, 000a9h, 01185h, 0ff8dh, 08d02h, 002f0h, 04d85h
    DW 04068h, 0aa68h, 0022ch, 010d3h, 0ad06h, 0d300h, 0026ch, 02c02h
    DW 0d303h, 00610h, 001adh, 06cd3h, 00204h, 08d68h, 0028ch, 04868h
    DW 01029h, 007f0h, 08cadh, 04802h, 0066ch, 0ad02h, 0028ch, 06848h
    DW 02c40h, 0d40fh, 00310h, 0006ch, 04802h, 00fadh, 029d4h, 0f020h
    DW 04c03h, 0e474h, 0488ah, 04898h, 00f8dh, 06cd4h, 00222h, 014e6h
    DW 008d0h, 04de6h, 013e6h, 002d0h, 012e6h, 0fea9h, 000a2h, 04da4h
    DW 00610h, 04d85h, 013a6h, 0f6a9h, 04e85h, 04f86h, 000a2h, 0d020h
    DW 0d0e8h, 02003h, 0e8cah, 042a5h, 008d0h, 0bdbah, 00104h, 00429h
    DW 003f0h, 0054ch, 0ade9h, 0d40dh, 0358dh, 0ad02h, 0d40ch, 0348dh
    DW 0ad02h, 00231h, 0038dh, 0add4h, 00230h, 0028dh, 0add4h, 0022fh
    DW 0008dh, 0add4h, 0026fh, 01b8dh, 0a2d0h, 08e08h, 0d01fh, 0bd58h
    DW 002c0h, 04f45h, 04e25h, 0129dh, 0cad0h, 0f210h, 0f4adh, 08d02h
    DW 0d409h, 0f3adh, 08d02h, 0d401h, 002a2h, 0d020h, 0d0e8h, 02003h
    DW 0e8cdh, 002a2h, 0e8e8h, 018bdh, 01d02h, 00219h, 006f0h, 0d020h
    DW 09de8h, 00226h, 008e0h, 0ecd0h, 00fadh, 029d2h, 0f004h, 0ad08h
    DW 002f1h, 003f0h, 0f1ceh, 0ad02h, 0022bh, 017f0h, 00fadh, 029d2h
    DW 0d004h, 0ce60h, 0022bh, 00bd0h, 006a9h, 02b8dh, 0ad02h, 0d209h
    DW 0fc8dh, 0a002h, 0a201h, 0b903h, 0d300h, 04a4ah, 04a4ah, 0789dh
    DW 0ca02h, 000b9h, 029d3h, 09d0fh, 00278h, 088cah, 0e910h, 003a2h
    DW 010bdh, 09dd0h, 00284h, 000bdh, 09dd2h, 00270h, 004bdh, 09dd2h
    DW 00274h, 010cah, 08debh, 0d20bh, 006a2h, 003a0h, 078b9h, 04a02h
    DW 04a4ah, 07d9dh, 0a902h, 02a00h, 07c9dh, 0ca02h, 088cah, 0ec10h
    DW 0246ch, 0a902h, 08d00h, 0022bh, 0a9f0h, 0266ch, 06c02h, 00228h
    DW 018bch, 0d002h, 0bc08h, 00219h, 010f0h, 019deh, 0de02h, 00218h
    DW 008d0h, 019bch, 0d002h, 0a903h, 06000h, 0ffa9h, 00a60h, 02d8dh
    DW 08a02h, 005a2h, 00a8dh, 0cad4h, 0fdd0h, 02daeh, 09d02h, 00217h
    DW 09d98h, 00216h, 06860h, 068a8h, 068aah, 06640h, 07e66h, 00066h
    DW 07c00h, 0ed4ch, 066e8h, 0007ch, 03c00h, 06066h, 06660h, 0003ch
    DW 07800h, 0666ch, 06c66h, 00078h, 07e00h, 07c60h, 06060h, 0007eh
    DW 07e00h, 07c60h, 06060h, 00060h, 03e00h, 06060h, 0666eh, 0003eh
    DW 06600h, 07e66h, 03ca9h, 0028dh, 0a9d3h, 08d3ch, 0d303h, 003a9h
    DW 0328dh, 08502h, 08d41h, 0d20fh, 0ba60h, 0188eh, 0a903h, 08501h
    DW 0ad42h, 00300h, 060c9h, 003d0h, 0804ch, 0a9ebh, 08d00h, 0030fh
    DW 001a9h, 03785h, 00da9h, 03685h, 028a9h, 0048dh, 0a9d2h, 08d00h
    DW 0d206h, 0ad18h, 00300h, 0016dh, 06903h, 08dffh, 0023ah, 002adh
    DW 08d03h, 0023bh, 00aadh, 08d03h, 0023ch, 00badh, 08d03h, 0023dh
    DW 0a918h, 0853ah, 06932h, 08504h, 0a934h, 08502h, 08533h, 0a935h
    DW 08d34h, 0d303h, 08a20h, 0adech, 0023fh, 003d0h, 0d098h, 0c607h
    DW 01036h, 04cb5h, 0ea06h, 003adh, 01003h, 0a90ch, 0850dh, 02036h
    DW 0eb6ah, 08a20h, 0f0ech, 020e8h, 0ec75h, 000a9h, 03f8dh, 02002h
    DW 0ec9bh, 012f0h, 0032ch, 07003h, 0ad07h, 0023fh, 018d0h, 01df0h
    DW 06a20h, 020ebh, 0eae0h, 03fadh, 0f002h, 0ad05h, 00319h, 03085h
    DW 030a5h, 001c9h, 007f0h, 037c6h, 00330h, 0744ch, 020e9h, 0ec5fh
    DW 000a9h, 04285h, 030a4h, 0038ch, 06003h, 000a9h, 03f8dh, 01802h
    DW 03ea9h, 03285h, 00169h, 03485h, 002a9h, 03385h, 03585h, 0ffa9h
    DW 03c85h, 0e020h, 0a0eah, 0a5ffh, 0c930h, 0d001h, 0ad19h, 0023eh
    DW 041c9h, 021f0h, 043c9h, 01df0h, 045c9h, 006d0h, 090a9h, 03085h
    DW 004d0h, 08ba9h, 03085h, 030a5h, 08ac9h, 007f0h, 0ffa9h, 03f8dh
    DW 0d002h, 0a002h, 0a500h, 08d30h, 00319h, 0a960h, 08501h, 02030h
    DW 0ebf2h, 000a0h, 03184h, 03b84h, 03a84h, 032b1h, 00d8dh, 085d2h
    DW 0a531h, 0d011h, 04c03h, 0eda0h, 03aa5h, 0f5f0h, 05f20h, 060ech
    DW 04898h, 032e6h, 002d0h, 033e6h, 032a5h, 034c5h, 033a5h, 035e5h
    DW 01c90h, 03ba5h, 00bd0h, 031a5h, 00d8dh, 0a9d2h, 085ffh, 0d03bh
    DW 0a509h, 00910h, 08508h, 08d10h, 0d20eh, 0a868h, 04068h, 000a0h
    DW 032b1h, 00d8dh, 018d2h, 03165h, 00069h, 03185h, 0ba4ch, 0a5eah
    DW 0f03bh, 0850bh, 0a53ah, 02910h, 085f7h, 08d10h, 0d20eh, 04068h
    DW 000a9h, 00fach, 0d003h, 08502h, 08531h, 08538h, 0a939h, 08501h
    DW 02030h, 0ec1bh, 03ca9h, 0038dh, 0a5d3h, 0d011h, 04c03h, 0eda0h
    DW 017adh, 0f003h, 0a505h, 0f039h, 060f0h, 08aa9h, 03085h, 09860h
    DW 0ad48h, 0d20fh, 00a8dh, 030d2h, 0a004h, 0848ch, 02930h, 0d020h
    DW 0a004h, 0848eh, 0a530h, 0f038h, 0ad13h, 0d20dh, 031c5h, 004f0h
    DW 08fa0h, 03084h, 0ffa9h, 03985h, 0a868h, 04068h, 00dadh, 0a0d2h
    DW 09100h, 01832h, 03165h, 00069h, 03185h, 032e6h, 002d0h, 033e6h
    DW 032a5h, 034c5h, 033a5h, 035e5h, 0de90h, 03ca5h, 006f0h, 000a9h
    DW 03c85h, 0d0f0h, 0ffa9h, 03885h, 0ced0h, 0ad18h, 00304h, 03285h
    DW 0086dh, 08503h, 0ad34h, 00305h, 03385h, 0096dh, 08503h, 06035h
    DW 003adh, 01003h, 0a92eh, 08dcch, 0d204h, 005a9h, 0068dh, 020d2h
    DW 0ebf2h, 00fa0h, 00badh, 03003h, 0a002h, 0a2b4h, 02000h, 0edb9h
    DW 034a9h, 0028dh, 0add3h, 00317h, 0fbd0h, 06a20h, 020ebh, 0ea6bh
    DW 0df4ch, 0a9ebh, 08dffh, 0030fh, 00aa0h, 00badh, 03003h, 0a002h
    DW 0a278h, 02000h, 0edb9h, 034a9h, 0028dh, 0add3h, 00317h, 0fbd0h
    DW 06a20h, 020ebh, 0ec75h, 0b920h, 020edh, 0ed10h, 0e020h, 0adeah
    DW 0030bh, 00530h, 03ca9h, 0028dh, 04cd3h, 0ea0dh, 000a9h, 0178dh
    DW 06003h, 007a9h, 0322dh, 00902h, 0ac20h, 00300h, 060c0h, 00cd0h
    DW 00809h, 007a0h, 0028ch, 0a0d2h, 08c05h, 0d200h, 0328dh, 08d02h
    DW 0d20fh, 0c7a9h, 01025h, 01009h, 0314ch, 0a9ech, 02d07h, 00232h
    DW 01009h, 0328dh, 08d02h, 0d20fh, 00a8dh, 0a9d2h, 025c7h, 00910h
    DW 08520h, 08d10h, 0d20eh, 028a9h, 0088dh, 0a2d2h, 0a906h, 0a4a8h
    DW 0d041h, 0a902h, 09da0h, 0d201h, 0cacah, 0f910h, 0a0a9h, 0058dh
    DW 0acd2h, 00300h, 060c0h, 006f0h, 0018dh, 08dd2h, 0d203h, 0ea60h
    DW 0c7a9h, 01025h, 01085h, 00e8dh, 0a2d2h, 0a906h, 09d00h, 0d201h
    DW 0cacah, 0f910h, 0ad60h, 00306h, 06a6ah, 029a8h, 0aa3fh, 06a98h
    DW 0c029h, 060a8h, 0eb0fh, 0ea90h, 0eacfh, 001a2h, 0ffa0h, 0d088h
    DW 0cafdh, 0f8d0h, 06b20h, 0a0eah, 0a202h, 02000h, 0edb9h, 01a20h
    DW 098eah, 08d60h, 00310h, 0118ch, 02003h, 0ed04h, 0108dh, 0ad03h
    DW 0030ch, 00420h, 08dedh, 0030ch, 010adh, 03803h, 00cedh, 08d03h
    DW 00312h, 011adh, 03803h, 00dedh, 0a803h, 07da9h, 06918h, 08883h
    DW 0fa10h, 06d18h, 00312h, 04aa8h, 04a4ah, 0380ah, 016e9h, 098aah
    DW 00729h, 0a9a8h, 018f5h, 00b69h, 01088h, 0a0fah, 08c00h, 0030eh
    DW 0e938h, 01007h, 0ce03h, 0030eh, 07d18h, 0edd0h, 0ada8h, 0030eh
    DW 0d17dh, 060edh, 07cc9h, 00430h, 0e938h, 0607ch, 06918h, 06007h
    DW 011a5h, 003d0h, 0a04ch, 078edh, 017adh, 0d003h, 0f002h, 0ad25h
    DW 0d20fh, 01029h, 0ead0h, 0168dh, 0ae03h, 0d40bh, 014a4h, 00c8eh
    DW 08c03h, 0030dh, 001a2h, 0158eh, 0a003h, 0a50ah, 0f011h, 0ad61h
    DW 00317h, 004d0h, 04c58h, 0eb0ah, 00fadh, 029d2h, 0cd10h, 00316h
    DW 0e9f0h, 0168dh, 08803h, 0e3d0h, 015ceh, 03003h, 0ad12h, 0d40bh
    DW 014a4h, 0a320h, 08cech, 002eeh, 0ef8dh, 0a002h, 0d009h, 0adcch
    DW 002eeh, 0048dh, 0add2h, 002efh, 0068dh, 0a9d2h, 08d00h, 0d20fh
    DW 032adh, 08d02h, 0d20fh, 055a9h, 03291h, 091c8h, 0a932h, 085aah
    DW 01831h, 032a5h, 00269h, 03285h, 033a5h, 00069h, 03385h, 06058h
    DW 05f20h, 0a9ech, 08d3ch, 0d302h, 0038dh, 0a9d3h, 08580h, 0ae30h
    DW 00318h, 0c69ah, 05811h, 00d4ch, 0a9eah, 08dech, 00226h, 0eba9h
    DW 0278dh, 0a902h, 07801h, 05c20h, 0a9e4h, 08d01h, 00317h, 06058h
    DW 003e8h, 00443h, 0049eh, 004f9h, 00554h, 005afh, 0060ah, 00665h
    DW 006c0h, 0071ah, 00775h, 007d0h, 08524h, 0a0a9h, 0468dh, 06002h
    DW 031a9h, 0008dh, 0ad03h, 00246h, 002aeh, 0e003h, 0f021h, 0a902h
    DW 08d07h, 00306h, 040a2h, 080a0h, 002adh, 0c903h, 0d057h, 0a202h
    DW 0c980h, 0d053h, 0a90ch, 08deah, 00304h, 002a9h, 0058dh, 0a003h
    DW 08e04h, 00303h, 0088ch, 0a903h, 08d00h, 00309h, 05920h, 010e4h
    DW 06001h, 002adh, 0c903h, 0d053h, 0200ah, 0ee6dh, 002a0h, 015b1h
    DW 0468dh, 0ad02h, 00302h, 021c9h, 01fd0h, 06d20h, 0a0eeh, 0c8feh
    DW 0b1c8h, 0c915h, 0d0ffh, 0c8f8h, 015b1h, 0c9c8h, 0d0ffh, 088f2h
    DW 08c88h, 00308h, 000a9h, 0098dh, 0ac03h, 00303h, 0ad60h, 00304h
    DW 01585h, 005adh, 08503h, 06016h, 01ea9h, 01c85h, 0ea60h, 0c002h
    DW 0a903h, 08504h, 0ae1eh, 0ee7dh, 07each, 0a9eeh, 08d53h, 00302h
    DW 00a8dh, 02003h, 0eee6h, 05920h, 030e4h, 02003h, 0ef14h, 02060h
    DW 0ee81h, 000a9h, 01d85h, 08560h, 0201fh, 0ef1ah, 01da6h, 01fa5h
    DW 0c09dh, 0e803h, 01ee4h, 013f0h, 01d86h, 09bc9h, 003f0h, 001a0h
    DW 0a960h, 09d20h, 003c0h, 0e4e8h, 0d01eh, 0a9f8h, 08500h, 0ae1dh
    DW 0ee7fh, 080ach, 020eeh, 0eee6h, 05920h, 060e4h, 01a20h, 0a6efh
    DW 0d01dh, 0a0deh, 06001h, 0048eh, 08c03h, 00305h, 040a9h, 0008dh
    DW 0a903h, 08d01h, 00301h, 080a9h, 002aeh, 0e003h, 0d053h, 0a902h
    DW 08d40h, 00303h, 01ea5h, 0088dh, 0a903h, 08d00h, 00309h, 01ca5h
    DW 0068dh, 06003h, 0ecadh, 08502h, 0601ch, 057a0h, 02ba5h, 04ec9h
    DW 004d0h, 028a2h, 00ed0h, 044c9h, 004d0h, 014a2h, 006d0h, 053c9h
    DW 00bd0h, 01da2h, 01e86h, 0028ch, 08d03h, 0030ah, 0a960h, 0d04eh
    DW 0a9ddh, 08dcch, 002eeh, 005a9h, 0ef8dh, 06002h, 02ba5h, 03e85h
    DW 02aa5h, 00c29h, 004c9h, 005f0h, 008c9h, 039f0h, 0a960h, 08d00h
    DW 00289h, 03f85h, 001a9h, 05820h, 030f0h, 0a924h, 08d34h, 0d302h
    DW 040a0h, 002a2h, 003a9h, 02a8dh, 02002h, 0e45ch, 02aadh, 0d002h
    DW 0a9fbh, 08580h, 08d3dh, 0028ah, 0d34ch, 0a0efh, 0c680h, 0a911h
    DW 08d00h, 00289h, 0a960h, 08d80h, 00289h, 002a9h, 05820h, 030f0h
    DW 0a9eeh, 08dcch, 0d204h, 005a9h, 0068dh, 0a9d2h, 08d60h, 00300h
    DW 06820h, 0a9e4h, 08d34h, 0d302h, 003a9h, 004a2h, 080a0h, 05c20h
    DW 0a9e4h, 08dffh, 0022ah, 011a5h, 0c1f0h, 02aadh, 0d002h, 0a9f7h
    DW 08500h, 0a03dh, 06001h, 03fa5h, 03330h, 03da6h, 08aech, 0f002h
    DW 0bd08h, 00400h, 03de6h, 001a0h, 0a960h, 02052h, 0f095h, 03098h
    DW 0a9f7h, 08500h, 0a23dh, 0ad80h, 003ffh, 0fec9h, 00df0h, 0fac9h
    DW 003d0h, 07faeh, 08e04h, 0028ah, 0d64ch, 0c6efh, 0a03fh, 06088h
    DW 03da6h, 0009dh, 0e604h, 0a03dh, 0e001h, 0f07fh, 06001h, 0fca9h
    DW 0d220h, 0a9f0h, 08500h, 0603dh, 001a0h, 0ad60h, 00289h, 00830h
    DW 001a0h, 03ca9h, 0028dh, 060d3h, 03da6h, 00af0h, 07f8eh, 0a904h
    DW 020fah, 0f0d2h, 0ec30h, 07fa2h, 000a9h, 0009dh, 0ca04h, 0fa10h
    DW 0fea9h, 0d220h, 04cf0h, 0f032h, 04085h, 014a5h, 06918h, 0aa1eh
    DW 0ffa9h, 01f8dh, 0a9d0h, 0a000h, 088f0h, 0fdd0h, 01f8dh, 0a0d0h
    DW 088f0h, 0fdd0h, 014e4h, 0e8d0h, 040c6h, 00bf0h, 0188ah, 00a69h
    DW 0e4aah, 0d014h, 0f0fch, 020d3h, 0f08ch, 06098h, 025adh, 048e4h
    DW 024adh, 048e4h, 08d60h, 00302h, 000a9h, 0098dh, 0a903h, 08d83h
    DW 00308h, 003a9h, 0058dh, 0a903h, 08dfdh, 00304h, 060a9h, 0008dh
    DW 0a903h, 08d00h, 00301h, 023a9h, 0068dh, 0ad03h, 00302h, 040a0h
    DW 052c9h, 002f0h, 080a0h, 0038ch, 0a503h, 08d3eh, 0030bh, 05920h
    DW 060e4h, 0ff8dh, 0a903h, 08d55h, 003fdh, 0fe8dh, 0a903h, 02057h
    DW 0f095h, 05060h, 0e430h, 04043h, 045e4h, 0e400h, 01053h, 04be4h
    DW 0e420h, 0417dh, 04154h, 04952h, 04320h, 04d4fh, 05550h, 04554h
    DW 02052h, 0202dh, 0454dh, 04f4dh, 05020h, 04441h, 0429bh, 04f4fh
    DW 02054h, 05245h, 04f52h, 09b52h, 03a45h, 0789bh, 044adh, 0d002h
    DW 0a904h, 0d0ffh, 07803h, 000a9h, 00885h, 0a2d8h, 09affh, 04420h
    DW 020f2h, 0f277h, 008a5h, 028d0h, 000a9h, 008a0h, 00485h, 00585h
    DW 00491h, 0c0c8h, 0d000h, 0e6f9h, 0a605h, 0e405h, 0d006h, 0adf1h
    DW 0e472h, 00a85h, 073adh, 085e4h, 0a90bh, 08dffh, 00244h, 013d0h
    DW 000a2h, 09d8ah, 00200h, 0009dh, 0ca03h, 0f7d0h, 010a2h, 00095h
    DW 010e8h, 0a9fbh, 08502h, 0a952h, 08527h, 0a253h, 0bd25h, 0e480h
    DW 0009dh, 0ca02h, 0f710h, 08a20h, 058f2h, 00ea2h, 0e3bdh, 09df0h
    DW 0031ah, 010cah, 0a2f7h, 08600h, 08607h, 0ae06h, 002e4h, 090e0h
    DW 00ab0h, 0fcadh, 0d09fh, 0e605h, 02007h, 0f23ch, 0e4aeh, 0e002h
    DW 0b0b0h, 0ae0ah, 0bffch, 005d0h, 006e6h, 03920h, 0a9f2h, 0a203h
    DW 09d00h, 00342h, 018a9h, 0449dh, 0a903h, 09df1h, 00345h, 00ca9h
    DW 04a9dh, 02003h, 0e456h, 00310h, 0254ch, 0e8f1h, 0fdd0h, 010c8h
    DW 020fah, 0f3b2h, 006a5h, 00705h, 012f0h, 006a5h, 003f0h, 0fdadh
    DW 0a6bfh, 0f007h, 00d03h, 09ffdh, 00129h, 003f0h, 0cf20h, 0a9f2h
    DW 08d00h, 00244h, 006a5h, 00af0h, 0fdadh, 029bfh, 0f004h, 06c03h
    DW 0bffah, 007a5h, 00af0h, 0fdadh, 0299fh, 0f004h, 06cdfh, 09ffah
    DW 00a6ch, 0a200h, 0a0f2h, 020f0h, 0f385h, 03020h, 04cf2h, 0f22ah
    DW 005adh, 048e4h, 004adh, 048e4h, 06c60h, 0bffeh, 0fe6ch, 0c99fh
    DW 0d0d0h, 0601ch, 0fceeh, 0adbfh, 0bffch, 008d0h, 0fdadh, 010bfh
    DW 06c03h, 0bffeh, 0fcceh, 0a0bfh, 08400h, 0a905h, 08510h, 0b106h
    DW 04905h, 091ffh, 0d105h, 0d005h, 049dah, 091ffh, 0a505h, 01806h
    DW 01069h, 00685h, 03f4ch, 0a9f2h, 0aa00h, 0009dh, 09dd0h, 0d400h
    DW 0009dh, 0ead2h, 0eaeah, 0d0e8h, 060f1h, 011c6h, 054a9h, 0368dh
    DW 0a902h, 08de7h, 00237h, 006a5h, 0e48dh, 08d02h, 002e6h, 000a9h
    DW 0e58dh, 0a902h, 08d00h, 002e7h, 007a9h, 0e88dh, 02002h, 0e40ch
    DW 01c20h, 020e4h, 0e42ch, 03c20h, 020e4h, 0e44ch, 06e20h, 020e4h
    DW 0e465h, 06b20h, 0ade4h, 0d01fh, 00129h, 002d0h, 04ae6h, 0a560h
    DW 0f008h, 0a50ah, 02909h, 0f001h, 02003h, 0f37eh, 0a960h, 08d01h
    DW 00301h, 053a9h, 0028dh, 02003h, 0e453h, 00110h, 0a960h, 08d00h
    DW 0030bh, 001a9h, 00a8dh, 0a903h, 08d00h, 00304h, 004a9h, 0058dh
    DW 02003h, 0f39dh, 00810h, 08120h, 0a5f3h, 0f04bh, 060e0h, 003a2h
    DW 000bdh, 09d04h, 00240h, 010cah, 0adf7h, 00242h, 00485h, 043adh
    DW 08502h, 0ad05h, 00404h, 00c85h, 005adh, 08504h, 0a00dh, 0b97fh
    DW 00400h, 00491h, 01088h, 018f8h, 004a5h, 08069h, 00485h, 005a5h
    DW 00069h, 00585h, 041ceh, 0f002h, 0ee11h, 0030ah, 09d20h, 010f3h
    DW 020dch, 0f381h, 04ba5h, 0aed0h, 0f2f0h, 04ba5h, 003f0h, 09d20h
    DW 020f3h, 0f36ch, 0a0b0h, 07e20h, 0e6f3h, 06009h, 0ad18h, 00242h
    DW 00669h, 00485h, 043adh, 06902h, 08500h, 06c05h, 00004h, 00c6ch
    DW 0a200h, 0a00dh, 08af1h, 000a2h, 0449dh, 09803h, 0459dh, 0a903h
    DW 09d09h, 00342h, 0ffa9h, 0489dh, 02003h, 0e456h, 0a560h, 0f04bh
    DW 04c03h, 0e47ah, 052a9h, 0028dh, 0a903h, 08d01h, 00301h, 05320h
    DW 060e4h, 008a5h, 00af0h, 009a5h, 00229h, 003f0h, 0e120h, 060f3h
    DW 04aa5h, 01cf0h, 080a9h, 03e85h, 04be6h, 07d20h, 020e4h, 0f301h
    DW 000a9h, 04b85h, 04a85h, 00906h, 00ca5h, 00285h, 00da5h, 00385h
    DW 06c60h, 00002h, 0ffa9h, 0fc8dh, 0ad02h, 002e6h, 0f029h, 06a85h
    DW 040a9h, 0be8dh, 06002h, 02ba5h, 00f29h, 008d0h, 02aa5h, 00f29h
    DW 02a85h, 000a9h, 05785h, 0e0a9h, 0f48dh, 0a902h, 08d02h, 002f3h
    DW 02f8dh, 0a902h, 08501h, 0a94ch, 005c0h, 08510h, 08d10h, 0d20eh
    DW 000a9h, 0938dh, 08502h, 08564h, 08d7bh, 002f0h, 00ea0h, 001a9h
    DW 0a399h, 08802h, 0fa10h, 004a2h, 0c1bdh, 09dfeh, 002c4h, 010cah
    DW 0a4f7h, 0886ah, 0958ch, 0a902h, 08d60h, 00294h, 057a6h, 069bdh
    DW 0d0feh, 0a904h, 08591h, 0854ch, 0a551h, 0856ah, 0bc65h, 0fe45h
    DW 028a9h, 02120h, 088f9h, 0f8d0h, 06fadh, 02902h, 0853fh, 0a867h
    DW 008e0h, 01790h, 06a8ah, 06a6ah, 0c029h, 06705h, 0a9a8h, 02010h
    DW 0f921h, 00be0h, 005d0h, 006a9h, 0c88dh, 08c02h, 0026fh, 064a5h
    DW 05885h, 065a5h, 05985h, 00badh, 0c9d4h, 0d07ah, 020f9h, 0f91fh
    DW 075bdh, 0f0feh, 0a906h, 085ffh, 0c664h, 0a565h, 08564h, 0a568h
    DW 08565h, 02069h, 0f913h, 041a9h, 01720h, 086f9h, 0a966h, 08d18h
    DW 002bfh, 057a5h, 009c9h, 02db0h, 02aa5h, 01029h, 027f0h, 004a9h
    DW 0bf8dh, 0a202h, 0a902h, 02002h, 0f917h, 010cah, 0a4f8h, 0886ah
    DW 02098h, 0f917h, 060a9h, 01720h, 0a9f9h, 02042h, 0f917h, 0a918h
    DW 0650ch, 08566h, 0a466h, 0be66h, 0fe51h, 051a5h, 01720h, 0caf9h
    DW 0f8d0h, 057a5h, 008c9h, 01c90h, 05da2h, 06aa5h, 0e938h, 02010h
    DW 0f917h, 000a9h, 01720h, 0a9f9h, 0204fh, 0f917h, 051a5h, 01720h
    DW 0caf9h, 0f8d0h, 059a5h, 01720h, 0a5f9h, 02058h, 0f917h, 051a5h
    DW 04009h, 01720h, 0a9f9h, 02070h, 0f917h, 070a9h, 01720h, 0a5f9h
    DW 08d64h, 00230h, 065a5h, 0318dh, 0a902h, 02070h, 0f917h, 064a5h
    DW 0e58dh, 0a502h, 08d65h, 002e6h, 068a5h, 06485h, 069a5h, 06585h
    DW 031adh, 02002h, 0f917h, 030adh, 02002h, 0f917h, 04ca5h, 00710h
    DW 02048h, 0f3fch, 0a868h, 0a560h, 0292ah, 0d020h, 0200bh, 0f7b9h
    DW 0908dh, 0a502h, 08d52h, 00291h, 022a9h, 02f0dh, 08d02h, 0022fh
    DW 0214ch, 020f6h, 0fa96h, 0a220h, 020f5h, 0fb32h, 0d420h, 04cf9h
    DW 0f634h, 04720h, 0b1f9h, 02d64h, 002a0h, 06f46h, 003b0h, 0104ah
    DW 08df9h, 002fah, 000c9h, 08d60h, 002fbh, 09620h, 0adfah, 002fbh
    DW 07dc9h, 006d0h, 0b920h, 04cf7h, 0f621h, 0fbadh, 0c902h, 0d09bh
    DW 02006h, 0fa30h, 0214ch, 020f6h, 0f5e0h, 0d820h, 04cf9h, 0f621h
    DW 0ffadh, 0d002h, 0a2fbh, 0b502h, 09554h, 0ca5ah, 0f910h, 0fbadh
    DW 0a802h, 02a2ah, 02a2ah, 00329h, 098aah, 09f29h, 0f61dh, 08dfeh
    DW 002fah, 04720h, 0adf9h, 002fah, 06f46h, 004b0h, 04c0ah, 0f608h
    DW 0a02dh, 08502h, 0ad50h, 002a0h, 0ff49h, 06431h, 05005h, 06491h
    DW 02060h, 0f5a2h, 05d85h, 057a6h, 00ad0h, 0f0aeh, 0d002h, 04905h
    DW 02080h, 0f5ffh, 04ca4h, 001a9h, 04c85h, 0fbadh, 06002h, 0b320h
    DW 020fch, 0fa88h, 06ba5h, 034d0h, 054a5h, 06c85h, 055a5h, 06d85h
    DW 0e220h, 084f6h, 0ad4ch, 002fbh, 09bc9h, 012f0h, 0ad20h, 020f6h
    DW 0fcb3h, 063a5h, 071c9h, 003d0h, 00a20h, 04cf9h, 0f650h, 0e420h
    DW 020fah, 0fc00h, 06ca5h, 05485h, 06da5h, 05585h, 06ba5h, 011f0h
    DW 06bc6h, 00df0h, 04ca5h, 0f830h, 09320h, 08df5h, 002fbh, 0b34ch
    DW 020fch, 0fa30h, 09ba9h, 0fb8dh, 02002h, 0f621h, 04c84h, 0b34ch
    DW 06cfch, 00064h, 0fb8dh, 02002h, 0fcb3h, 08820h, 020fah, 0fae4h
    DW 08d20h, 0f0fch, 00e09h, 002a2h, 0ca20h, 04cf5h, 0fcb3h, 0feadh
    DW 00d02h, 002a2h, 0efd0h, 0a20eh, 0e802h, 0c6bdh, 085feh, 0bd64h
    DW 0fec7h, 06585h, 0a120h, 020f6h, 0f621h, 0b34ch, 0a9fch, 08dffh
    DW 002fch, 02aa5h, 0b04ah, 0a962h, 0a680h, 0f011h, 0ad58h, 002fch
    DW 0ffc9h, 0eef0h, 07c85h, 0ffa2h, 0fc8eh, 02002h, 0fcd8h, 0e0aah
    DW 090c0h, 0a202h, 0bd03h, 0fefeh, 0fb8dh, 0c902h, 0f080h, 0c9ceh
    DW 0d081h, 0ad0bh, 002b6h, 08049h, 0b68dh, 04c02h, 0f6ddh, 082c9h
    DW 007d0h, 000a9h, 0be8dh, 0f002h, 0c9b4h, 0d083h, 0a907h, 08d40h
    DW 002beh, 0a9d0h, 084c9h, 007d0h, 080a9h, 0be8dh, 0d002h, 0c99eh
    DW 0d085h, 0a90ah, 08588h, 0854ch, 0a911h, 0d09bh, 0a526h, 0c97ch
    DW 0b040h, 0ad15h, 002fbh, 061c9h, 00e90h, 07bc9h, 00ab0h, 0beadh
    DW 0f002h, 00505h, 04c7ch, 0f6feh, 08d20h, 0f0fch, 0ad09h, 002fbh
    DW 0b64dh, 08d02h, 002fbh, 0344ch, 0a9f6h, 08d80h, 002a2h, 0c660h
    DW 01054h, 0ae06h, 002bfh, 086cah, 04c54h, 0fc5ch, 054e6h, 054a5h
    DW 0bfcdh, 09002h, 0a2f4h, 0f000h, 0c6eeh, 0a555h, 03055h, 0c504h
    DW 0b052h, 0a504h, 08553h, 04c55h, 0fbddh, 055e6h, 055a5h, 053c5h
    DW 0f590h, 0f3f0h, 052a5h, 0a54ch, 020f7h, 0fcf3h, 000a0h, 09198h
    DW 0c864h, 0fbd0h, 065e6h, 065a6h, 06ae4h, 0f390h, 0ffa9h, 0b299h
    DW 0c802h, 004c0h, 0f890h, 0e420h, 085fch, 08563h, 0a96dh, 08500h
    DW 08554h, 08556h, 0606ch, 063a5h, 052c5h, 021f0h, 055a5h, 052c5h
    DW 003d0h, 07320h, 020fch, 0f799h, 055a5h, 053c5h, 007d0h, 054a5h
    DW 003f0h, 07f20h, 0a9f7h, 08d20h, 002fbh, 0e020h, 04cf5h, 0fbddh
    DW 0aa20h, 0a5f7h, 0c555h, 0d052h, 0200ah, 0fa34h, 02020h, 090fbh
    DW 0b002h, 0a507h, 02063h, 0fb25h, 0e690h, 0dd4ch, 0a5fbh, 04c63h
    DW 0fb06h, 063a5h, 0124ch, 020fbh, 0fc9dh, 0a220h, 085f5h, 0a97dh
    DW 08d00h, 002bbh, 0ff20h, 0a5f5h, 04863h, 0dc20h, 068f9h, 063c5h
    DW 00cb0h, 07da5h, 02048h, 0f5a2h, 07d85h, 04c68h, 0f844h, 0a820h
    DW 0cefch, 002bbh, 00430h, 054c6h, 0f7d0h, 0dd4ch, 020fbh, 0fc9dh
    DW 04720h, 0a5f9h, 08564h, 0a568h, 08565h, 0a569h, 04863h, 0d420h
    DW 068f9h, 063c5h, 010b0h, 054a5h, 0bfcdh, 0b002h, 02009h, 0f5a2h
    DW 000a0h, 06891h, 0daf0h, 000a0h, 09198h, 02068h, 0fc68h, 0a820h
    DW 04cfch, 0fbddh, 02038h, 0fb7bh, 052a5h, 05585h, 04720h, 0a5f9h
    DW 08564h, 01868h, 02869h, 06685h, 065a5h, 06985h, 00069h, 06785h
    DW 054a6h, 017e0h, 008f0h, 04e20h, 0e8fbh, 017e0h, 0f8d0h, 09b20h
    DW 04cfbh, 0fbddh, 0dd20h, 0a4fbh, 08451h, 0a454h, 09854h, 02038h
    DW 0fb23h, 09808h, 06918h, 02878h, 00420h, 0c8fbh, 018c0h, 0edd0h
    DW 0b4adh, 00902h, 08d01h, 002b4h, 052a5h, 05585h, 04720h, 020f9h
    DW 0fbb7h, 02020h, 090fbh, 04cd4h, 0fbddh, 02060h, 0d820h, 088fch
    DW 0fa10h, 0a960h, 0d002h, 0a40ah, 0304ch, 0a02bh, 09100h, 0a964h
    DW 08d01h, 0029eh, 04ca5h, 01e30h, 064a5h, 0ed38h, 0029eh, 06485h
    DW 002b0h, 065c6h, 00fa5h, 065c5h, 00c90h, 006d0h, 00ea5h, 064c5h
    DW 00490h, 093a9h, 04c85h, 0a560h, 04854h, 055a5h, 0a548h, 04856h
    DW 0f320h, 0a5fch, 08554h, 0a966h, 08500h, 0a567h, 00a66h, 06726h
    DW 05185h, 067a4h, 09f8ch, 00a02h, 06726h, 0260ah, 01867h, 05165h
    DW 06685h, 067a5h, 09f6dh, 08502h, 0a667h, 0bc57h, 0fe81h, 03088h
    DW 00607h, 02666h, 04c67h, 0f97eh, 0a5bch, 0a5feh, 0a255h, 08807h
    DW 00a30h, 046cah, 06a56h, 0a16eh, 04c02h, 0f98fh, 018c8h, 06665h
    DW 06685h, 00290h, 067e6h, 06e38h, 002a1h, 0ca18h, 0f910h, 0a1aeh
    DW 0a502h, 01866h, 06465h, 06485h, 05e85h, 067a5h, 06565h, 06585h
    DW 05f85h, 0b1bdh, 08dfeh, 002a0h, 06f85h, 08568h, 06856h, 05585h
    DW 08568h, 06054h, 000a9h, 002f0h, 09ba9h, 07d85h, 063e6h, 055e6h
    DW 002d0h, 056e6h, 055a5h, 057a6h, 08dddh, 0f0feh, 0e00bh, 0d000h
    DW 0c506h, 0f053h, 0b002h, 06001h, 008e0h, 00490h, 056a5h, 0f7f0h
    DW 057a5h, 030d0h, 063a5h, 051c9h, 00a90h, 07da5h, 026f0h, 03020h
    DW 04cfah, 0fa77h, 03420h, 0a5fah, 01854h, 07869h, 02520h, 090fbh
    DW 0a508h, 0f07dh, 01804h, 0a520h, 04cf8h, 0fbddh, 000a9h, 002f0h
    DW 09ba9h, 07d85h, 0e420h, 0a9fch, 08500h, 0e656h, 0a654h, 0a057h
    DW 02418h, 0107bh, 0a005h, 09804h, 003d0h, 099bdh, 0c5feh, 0d054h
    DW 08c26h, 0029dh, 0d08ah, 0a520h, 0f07dh, 0c91ch, 0389bh, 001f0h
    DW 02018h, 0fbach, 0bbeeh, 0c602h, 0ce6ch, 0029dh, 0b2adh, 03802h
    DW 0ef10h, 09dadh, 08502h, 04c54h, 0fbddh, 0b538h, 0e570h, 09574h
    DW 0b570h, 0e571h, 09575h, 06071h, 0bfadh, 0c902h, 0f004h, 0a507h
    DW 0f057h, 02003h, 0f3fch, 027a9h, 053c5h, 002b0h, 05385h, 057a6h
    DW 099bdh, 0c5feh, 09054h, 0f02ah, 0e028h, 0d008h, 0a50ah, 0f056h
    DW 0c913h, 0d001h, 0f01ch, 0a504h, 0d056h, 0bd16h, 0fe8dh, 055c5h
    DW 00f90h, 00df0h, 001a9h, 04c85h, 080a9h, 011a6h, 01185h, 006f0h
    DW 02060h, 0f7d6h, 08da9h, 04c85h, 06868h, 07ba5h, 00310h, 0b920h
    DW 04cfch, 0f634h, 000a0h, 05da5h, 05e91h, 04860h, 00729h, 0bdaah
    DW 0feb9h, 06e85h, 04a68h, 04a4ah, 060aah, 0b42eh, 02e02h, 002b3h
    DW 0b22eh, 06002h, 00c90h, 0eb20h, 0bdfah, 002a3h, 06e05h, 0a39dh
    DW 06002h, 0eb20h, 0a5fah, 0496eh, 03dffh, 002a3h, 0a39dh, 06002h
    DW 054a5h, 06918h, 02078h, 0faebh, 0bd18h, 002a3h, 06e25h, 001f0h
    DW 06038h, 0faadh, 0a402h, 0c057h, 0b003h, 02a0fh, 02a2ah, 0292ah
    DW 0aa03h, 0faadh, 02902h, 01d9fh, 0fefah, 0fb8dh, 06002h, 002a9h
    DW 06585h, 047a9h, 06485h, 027a0h, 066b1h, 05085h, 068b1h, 06691h
    DW 050a5h, 06491h, 01088h, 0a5f1h, 08565h, 0a569h, 08564h, 01868h
    DW 066a5h, 02869h, 06685h, 00290h, 067e6h, 00860h, 017a0h, 02098h
    DW 0fb22h, 09808h, 06918h, 02879h, 00420h, 088fbh, 00430h, 054c4h
    DW 0ecb0h, 054a5h, 06918h, 02878h, 0044ch, 0a5fbh, 08552h, 02055h
    DW 0f947h, 027a0h, 000a9h, 06491h, 01088h, 060fbh, 0fa20h, 0a5fah
    DW 08558h, 0a564h, 08559h, 0a065h, 0b128h, 0a664h, 0ca6ah, 065e4h
    DW 008d0h, 0d7a2h, 064e4h, 002b0h, 000a9h, 000a0h, 06491h, 064e6h
    DW 0e5d0h, 065e6h, 065a5h, 06ac5h, 0ddd0h, 0dd4ch, 0a9fbh, 08500h
    DW 0a563h, 08554h, 0a551h, 02051h, 0fb22h, 00cb0h, 063a5h, 06918h
    DW 08528h, 0c663h, 04c51h, 0fbe5h, 0a518h, 06563h, 08555h, 06063h
    DW 09d20h, 0a5fch, 04863h, 06ca5h, 05485h, 06da5h, 05585h, 001a9h
    DW 06b85h, 017a2h, 07ba5h, 00210h, 003a2h, 054e4h, 00bd0h, 055a5h
    DW 053c5h, 005d0h, 06be6h, 0394ch, 020fch, 0f9d4h, 06be6h, 063a5h
    DW 052c5h, 0ded0h, 054c6h, 09920h, 020f7h, 0f5a2h, 017d0h, 06bc6h
    DW 063a5h, 052c5h, 00ff0h, 09920h, 0a5f7h, 0c555h, 0d053h, 0c602h
    DW 0a554h, 0d06bh, 068e4h, 06385h, 0a820h, 060fch, 0dd20h, 0a5fbh
    DW 08551h, 0a56ch, 08552h, 0606dh, 063a5h, 052c5h, 002d0h, 054c6h
    DW 0dd20h, 0a5fbh, 0c563h, 0f052h, 02013h, 0f947h, 053a5h, 0e538h
    DW 0a852h, 064b1h, 006d0h, 01088h, 04cf9h, 0f8dbh, 0a260h, 0bd2dh
    DW 0fec6h, 0fbcdh, 0f002h, 0ca05h, 0cacah, 0f310h, 0a260h, 0b502h
    DW 09d54h, 002b8h, 010cah, 060f8h, 002a2h, 0b8bdh, 09502h, 0ca54h
    DW 0f810h, 02060h, 0fcb9h, 0344ch, 0adf6h, 002bfh, 018c9h, 017f0h
    DW 00ba2h, 054b5h, 0bd48h, 00290h, 05495h, 09d68h, 00290h, 010cah
    DW 0a5f1h, 0497bh, 085ffh, 0607bh, 07fa2h, 01f8eh, 08ed0h, 0d40ah
    DW 010cah, 060f7h, 000a9h, 07ba6h, 004d0h, 057a6h, 002d0h, 052a5h
    DW 05585h, 0a560h, 08558h, 0a564h, 08559h, 06065h, 000a2h, 022a5h
    DW 011c9h, 008f0h, 012c9h, 003f0h, 084a0h, 0e860h, 0b78eh, 0a502h
    DW 08554h, 0a560h, 08555h, 0a561h, 08556h, 0a962h, 08501h, 08579h
    DW 0387ah, 060a5h, 05ae5h, 07685h, 00db0h, 0ffa9h, 07985h, 076a5h
    DW 0ff49h, 06918h, 08501h, 03876h, 061a5h, 05be5h, 07785h, 062a5h
    DW 05ce5h, 07885h, 016b0h, 0ffa9h, 07a85h, 077a5h, 0ff49h, 07785h
    DW 078a5h, 0ff49h, 07885h, 077e6h, 002d0h, 078e6h, 002a2h, 000a0h
    DW 07384h, 09598h, 0b570h, 0955ah, 0ca54h, 0f610h, 077a5h, 0a8e8h
    DW 078a5h, 07f85h, 07585h, 00bd0h, 077a5h, 076c5h, 005b0h, 076a5h
    DW 002a2h, 098a8h, 07e85h, 07485h, 0a548h, 04a75h, 06a68h, 07095h
    DW 07ea5h, 07f05h, 003d0h, 0424ch, 018feh, 070a5h, 07665h, 07085h
    DW 00290h, 071e6h, 071a5h, 075c5h, 01490h, 006d0h, 070a5h, 074c5h
    DW 00c90h, 0a518h, 06554h, 08579h, 0a254h, 02000h, 0fa7ah, 0a518h
    DW 06572h, 08577h, 0a572h, 06573h, 08578h, 0c573h, 09075h, 0d027h
    DW 0a506h, 0c572h, 09074h, 0241fh, 0107ah, 0c610h, 0a555h, 0c955h
    DW 0d0ffh, 0a50eh, 0f056h, 0c60ah, 01056h, 0e606h, 0d055h, 0e602h
    DW 0a256h, 02002h, 0fa7ah, 09620h, 020fah, 0f5e0h, 0b7adh, 0f002h
    DW 0202fh, 0fc9dh, 0fbadh, 08d02h, 002bch, 054a5h, 02048h, 0f9dch
    DW 08568h, 02054h, 0fa96h, 0a220h, 0d0f5h, 0ad0ch, 002fdh, 0fb8dh
    DW 02002h, 0f5e0h, 00a4ch, 0adfeh, 002bch, 0fb8dh, 02002h, 0fca8h
    DW 0a538h, 0e97eh, 08501h, 0a57eh, 0e97fh, 08500h, 0307fh, 04c03h
    DW 0fd90h, 0344ch, 018f6h, 00a10h, 0100ah, 0341ch, 0c464h, 0c4c4h
    DW 017c4h, 00b17h, 02f17h, 05f2fh, 0615fh, 06161h, 01361h, 00913h
    DW 02713h, 04f27h, 0414fh, 04141h, 00241h, 00706h, 00908h, 00b0ah
    DW 00f0dh, 00f0fh, 0000fh, 00000h, 00000h, 00000h, 00101h, 00101h
    DW 00201h, 00101h, 00000h, 00101h, 00202h, 00202h, 02802h, 01414h
    DW 05028h, 0a050h, 040a0h, 05050h, 01850h, 00c18h, 03018h, 06030h
    DW 0c060h, 0c0c0h, 000c0h, 00000h, 00302h, 00302h, 00302h, 00101h
    DW 00001h, 0f0ffh, 0c00fh, 00c30h, 08003h, 02040h, 00810h, 00204h
    DW 02801h, 094cah, 00046h, 0791bh, 01cf7h, 0f77fh, 08c1dh, 01ef7h
    DW 0f799h, 0aa1fh, 07df7h, 0f7b9h, 0e67eh, 07ff7h, 0f810h, 0309bh
    DW 09cfah, 0f8d4h, 0a49dh, 09ef8h, 0f832h, 02d9fh, 0fdf8h, 0f90ah
    DW 06dfeh, 0fff8h, 0f837h, 00040h, 06020h, 04020h, 06000h, 06a6ch
    DW 0803bh, 06b80h, 02a2bh, 0806fh, 07570h, 0699bh, 03d2dh, 08076h
    DW 08063h, 06280h, 07a78h, 08034h, 03633h, 0351bh, 03132h, 0202ch
    DW 06e2eh, 06d80h, 0812fh, 08072h, 07965h, 0747fh, 07177h, 08039h
    DW 03730h, 0387eh, 03e3ch, 06866h, 08064h, 06782h, 06173h, 04a4ch
    DW 0803ah, 04b80h, 05e5ch, 0804fh, 05550h, 0499bh, 07c5fh, 08056h
    DW 08043h, 04280h, 05a58h, 08024h, 02623h, 0251bh, 02122h, 0205bh
    DW 04e5dh, 04d80h, 0813fh, 08052h, 05945h, 0549fh, 05157h, 08028h
    DW 02729h, 0409ch, 09d7dh, 04846h, 08044h, 04783h, 04153h, 00a0ch
    DW 0807bh, 00b80h, 01f1eh, 0800fh, 01510h, 0099bh, 01d1ch, 08016h
    DW 08003h, 00280h, 01a18h, 08080h, 08085h, 0801bh, 080fdh, 02000h
    DW 00e60h, 00d80h, 08180h, 08012h, 01905h, 0149eh, 01117h, 08080h
    DW 08080h, 080feh, 0ff7dh, 00806h, 08004h, 00784h, 00113h, 009adh
    DW 0cdd2h, 002f2h, 005d0h, 0f1adh, 0d002h, 0ad20h, 0d209h, 09fc9h
    DW 00ad0h, 0ffadh, 04902h, 08dffh, 002ffh, 00fb0h, 0fc8dh, 08d02h
    DW 002f2h, 003a9h, 0f18dh, 0a902h, 08500h, 0a94dh, 08d30h, 0022bh
    DW 04068h, 0ffffh, 0ffffh, 0ffffh, 0e6f3h, 0e791h, 0f125h, 0e6f3h

rgbMapScans LABEL BYTE

    DB    0FFh, 0FFh, 0FFh, 0FFh     ; $00
    DB    01Ch, 05Ch, 09Ch, 0DCh     ; $01 Esc
    DB    01Fh, 05Fh, 09Fh, 0DFh     ; $02 1
    DB    01Eh, 075h, 09Eh, 0DEh     ; $03 2 map to @ not "
    DB    01Ah, 05Ah, 09Ah, 0DAh     ; $04 3
    DB    018h, 058h, 098h, 0D8h     ; $05 4
    DB    01Dh, 05Dh, 09Dh, 0DDh     ; $06 5
    DB    01Bh, 047h, 09Bh, 0DBh     ; $07 6 map to ^ not &
    DB    033h, 05Bh, 0B3h, 0F3h     ; $08 7 map to & not '
    DB    035h, 007h, 0B5h, 0F5h     ; $09 8 map to * not @
    DB    030h, 070h, 0B0h, 0F0h     ; $0A 9
    DB    032h, 072h, 0B2h, 0F2h     ; $0B 0
    DB    00Eh, 04Eh, 08Eh, 0CEh     ; $0C -
    DB    00Fh, 006h, 08Fh, 0CFh     ; $0D =
    DB    034h, 074h, 0B4h, 0F4h     ; $0E Backspace
    DB    02Ch, 06Ch, 0ACh, 0ECh     ; $0F Tab
    DB    02Fh, 06Fh, 0AFh, 0EFh     ; $10 q
    DB    02Eh, 06Eh, 0AEh, 0EEh     ; $11 w
    DB    02Ah, 06Ah, 0AAh, 0EAh     ; $12 e
    DB    028h, 068h, 0A8h, 0E8h     ; $13 r
    DB    02Dh, 06Dh, 0ADh, 0EDh     ; $14 t
    DB    02Bh, 06Bh, 0ABh, 0EBh     ; $15 y
    DB    00Bh, 04Bh, 08Bh, 0CBh     ; $16 u
    DB    00Dh, 04Dh, 08Dh, 0CDh     ; $17 i
    DB    008h, 048h, 088h, 0C8h     ; $18 o
    DB    00Ah, 04Ah, 08Ah, 0CAh     ; $19 p
    DB    060h, 060h, 0A0h, 0E0h     ; $1A [
    DB    062h, 062h, 0A2h, 0E2h     ; $1B ]
    DB    00Ch, 04Ch, 08Ch, 0CCh     ; $1C Return
    DB    0FFh, 0FFh, 0FFh, 0FFh     ; $1D Control
    DB    03Fh, 07Fh, 0BFh, 0FFh     ; $1E a
    DB    03Eh, 07Eh, 0BEh, 0FEh     ; $1F s
    DB    03Ah, 07Ah, 0BAh, 0FAh     ; $20 d
    DB    038h, 078h, 0B8h, 0F8h     ; $21 f
    DB    03Dh, 07Dh, 0BDh, 0FDh     ; $22 g
    DB    039h, 079h, 0B9h, 0F9h     ; $23 h
    DB    001h, 041h, 081h, 0C1h     ; $24 j
    DB    005h, 045h, 085h, 0C5h     ; $25 k
    DB    000h, 040h, 080h, 0C0h     ; $26 l
    DB    002h, 042h, 082h, 0C2h     ; $27 ;
    DB    073h, 05Eh, 0B3h, 0F3h     ; $28 ' map to " not @
    DB    027h, 067h, 0A7h, 0E7h     ; $29 ` = atari key
    DB    0FFh, 0FFh, 0FFh, 0FFh     ; $2A l shift
    DB    046h, 04Fh, 0E6h, 0E6h     ; $2B \
    DB    017h, 057h, 097h, 0D7h     ; $2C z
    DB    016h, 056h, 096h, 0D6h     ; $2D x
    DB    012h, 052h, 092h, 0D2h     ; $2E c
    DB    010h, 050h, 090h, 0D0h     ; $2F v
    DB    015h, 055h, 095h, 0D5h     ; $30 b
    DB    023h, 063h, 0A3h, 0E3h     ; $31 n
    DB    025h, 065h, 0A5h, 0E5h     ; $32 m
    DB    020h, 036h, 0A0h, 0E0h     ; $33 ,
    DB    022h, 037h, 0A2h, 0E2h     ; $34 .
    DB    026h, 066h, 0A6h, 0E6h     ; $35 /
    DB    0FFh, 0FFh, 0FFh, 0FFh     ; $36 r shift
    DB    0FFh, 0FFh, 0FFh, 0FFh     ; $37
    DB    0FFh, 0FFh, 0FFh, 0FFh     ; $38 alt
    DB    021h, 061h, 0A1h, 0E1h     ; $39 space
    DB    03Ch, 07Ch, 0BCh, 0FCh     ; $3A caps

    IF 1

    DB    003h, 043h, 083h, 0C3h     ; $3B F1
    DB    004h, 044h, 084h, 0C4h     ; $3C F2
    DB    013h, 053h, 093h, 0D3h     ; $3D F3
    DB    014h, 054h, 094h, 0D4h     ; $3E F4

    ELSE

    DB    08Eh, 08Eh, 08Eh, 08Eh     ; $3B F1
    DB    08Fh, 08Fh, 08Fh, 08Fh     ; $3C F2
    DB    086h, 086h, 086h, 086h     ; $3D F3
    DB    087h, 087h, 087h, 087h     ; $3E F4

    ENDIF

    DB    0FFh, 0FFh, 0FFh, 0FFh     ; $3F F5
    DB    0FFh, 0FFh, 0FFh, 0FFh     ; $40 F6
    DB    0FFh, 0FFh, 0FFh, 0FFh     ; $41 F7
    DB    0FFh, 0FFh, 0FFh, 0FFh     ; $42 F8
    DB    0FFh, 0FFh, 0FFh, 0FFh     ; $43 F9
    DB    0FFh, 0FFh, 0FFh, 0FFh     ; $44 F10
    DB    0FFh, 0FFh, 0FFh, 0FFh     ; $45 F11
    DB    0FFh, 0FFh, 0FFh, 0FFh     ; $46 F12
    DB    076h, 076h, 0B6h, 0F6h     ; $47 Home
    DB    00Eh, 04Eh, 08Eh, 0CEh     ; $48 up arrow
    DB    0FFh, 0FFh, 0FFh, 0FFh     ; $49
    DB    00Eh, 04Eh, 08Eh, 0CEh     ; $4A pad -
    DB    006h, 046h, 086h, 0C6h     ; $4B l arrow
    DB    0FFh, 0FFh, 0FFh, 0FFh     ; $4C
    DB    007h, 047h, 087h, 0C7h     ; $4D r arrow
    DB    006h, 046h, 086h, 0C6h     ; $4E pad +
    DB    0FFh, 0FFh, 09Ah, 09Ah     ; $4F End
    DB    00Fh, 04Fh, 08Fh, 0CFh     ; $50 d arrow
    DB    0FFh, 0FFh, 0FFh, 0FFh     ; $51
    DB    077h, 077h, 0B7h, 0F7h     ; $52 Insert
    DB    074h, 074h, 0B4h, 0F4h     ; $53 Delete
    DB    003h, 043h, 083h, 0C3h     ; $54 shift F1
    DB    004h, 044h, 084h, 0C4h     ; $55 shift F2
    DB    013h, 053h, 093h, 0D3h     ; $56 shift F3
    DB    014h, 054h, 094h, 0D4h     ; $57 shift F4
    DB    0FFh, 0FFh, 0FFh, 0FFh     ; $58
    DB    0FFh, 0FFh, 0FFh, 0FFh     ; $59
    DB    0FFh, 0FFh, 0FFh, 0FFh     ; $5A
    DB    0FFh, 0FFh, 0FFh, 0FFh     ; $5B
    DB    0FFh, 0FFh, 0FFh, 0FFh     ; $5C
    DB    0FFh, 0FFh, 0FFh, 0FFh     ; $5D
    DB    0FFh, 0FFh, 0FFh, 0FFh     ; $5E
    DB    0FFh, 0FFh, 0FFh, 0FFh     ; $5F
    DB    0FFh, 0FFh, 0FFh, 0FFh     ; $60
    DB    0FFh, 0FFh, 0FFh, 0FFh     ; $61
    DB    0FFh, 0FFh, 0FFh, 0FFh     ; $62
    DB    070h, 070h, 0B0h, 0F0h     ; $63 pad (
    DB    072h, 072h, 0B2h, 0F2h     ; $64 pad )
    DB    026h, 066h, 0A6h, 0E6h     ; $65 pad /
    DB    007h, 047h, 087h, 0C7h     ; $66 pad *
    DB    033h, 073h, 0B3h, 0F3h     ; $67 pad 7
    DB    035h, 075h, 0B5h, 0F5h     ; $68 pad 8
    DB    030h, 070h, 0B0h, 0F0h     ; $69 pad 9
    DB    018h, 058h, 098h, 0D8h     ; $6A pad 4
    DB    01Dh, 05Dh, 09Dh, 0DDh     ; $6B pad 5
    DB    01Bh, 05Bh, 09Bh, 0DBh     ; $6C pad 6
    DB    01Fh, 05Fh, 09Fh, 0DFh     ; $6D pad 1
    DB    01Eh, 05Eh, 09Eh, 0DEh     ; $6E pad 2
    DB    01Ah, 05Ah, 09Ah, 0DAh     ; $6F pad 3
    DB    032h, 072h, 0B2h, 0F2h     ; $70 pad 0
    DB    022h, 062h, 0A2h, 0E2h     ; $71 pad .
    DB    00Ch, 04Ch, 08Ch, 0CCh     ; $72 Enter
    DB    0FFh, 0FFh, 0FFh, 0FFh     ; $73
    DB    0FFh, 0FFh, 0FFh, 0FFh     ; $74
    DB    0FFh, 0FFh, 0FFh, 0FFh     ; $75
    DB    0FFh, 0FFh, 0FFh, 0FFh     ; $76
    DB    0FFh, 0FFh, 0FFh, 0FFh     ; $77
    DB    0FFh, 0FFh, 0FFh, 0FFh     ; $78
    DB    0FFh, 0FFh, 0FFh, 0FFh     ; $79
    DB    0FFh, 0FFh, 0FFh, 0FFh     ; $7A
    DB    0FFh, 0FFh, 0FFh, 0FFh     ; $7B
    DB    0FFh, 0FFh, 0FFh, 0FFh     ; $7C
    DB    0FFh, 0FFh, 0FFh, 0FFh     ; $7D
    DB    0FFh, 0FFh, 0FFh, 0FFh     ; $7E
    DB    0FFh, 0FFh, 0FFh, 0FFh     ; $7F


    END


