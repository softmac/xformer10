PROJ	=XF
DEBUG	=0
CC	=cl
AS	=ml
CFLAGS_G    = /YX /AC /W3 /Ze /Fc /DHDOS16 /G3 /Gy /Gf /DXFORMER
CFLAGS_D	= /Zi /Od /Oa
CFLAGS_R	= /Ox /Oa /O2 /Oz /Gs /DNDEBUG /f-
CFLAGS	=$(CFLAGS_G) $(CFLAGS_R)
AFLAGS_G	= /Cl /W2 /P1 /Fl /DHDOS16
AFLAGS_D	= /Zi /Sa 
AFLAGS_R	= /DNDEBUG 
AFLAGS	=$(AFLAGS_G) $(AFLAGS_R)
LFLAGS_G	= /CP:0xfff /NOI /SE:0x80 /ST:0x800 /MAP /EXEPACK
LFLAGS_D	= /CO /M 
LFLAGS_R	= /EXEPACK /PACKDATA /PACKCODE /FARCALLTRANSLATION
LFLAGS	=$(LFLAGS_G) $(LFLAGS_R)
RUNFLAGS	=
OBJS_EXT = 	
LIBS_EXT = 	
H = common.h vmdos.h atari800.h 6502.h makefile

.asm.obj: ; $(AS) $(AFLAGS) -c $*.asm

all:	$(PROJ).EXE

atari800.obj:	atari800.c $(H)

xatari.obj:	xatari.c $(H)

xsio.obj:	xsio.c $(H)

xsb.obj:	xsb.c $(H)

xvideo.obj:	xvideo.c $(H)

xlxeram.obj:	xlxeram.c $(H)

xkey.obj:	xkey.c $(H)

x6502.obj:	x6502.asm $(INC)

xmon.obj:	xmon.c $(H)

atariosb.obj:	atariosb.asm $(INC)

xeroms.obj:	xeroms.asm $(INC)

xfcable.obj:	xfcable.c $(H)

$(PROJ).EXE: atari800.obj xatari.obj xsio.obj xfcable.obj xsb.obj xlxeram.obj xvideo.obj xkey.obj x6502.obj xmon.obj atariosb.obj xeroms.obj $(OBJS_EXT)
	echo >NUL @<<$(PROJ).crf
xfcable.obj +
xsio.obj +
atari800.obj +
xatari.obj +
xsb.obj +
xlxeram.obj +
xvideo.obj +
xkey.obj +
x6502.obj +
xmon.obj +
atariosb.obj +
xeroms.obj +
$(OBJS_EXT)
$(PROJ).EXE

$(LIBS_EXT);
<<
	link $(LFLAGS) @$(PROJ).crf $(PROJ)

run: $(PROJ).EXE
	$(PROJ) $(RUNFLAGS)

