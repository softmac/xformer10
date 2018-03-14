#----------------------------------------------------------------------------
#
#    Gemulator 98 makefile for the 680x0 interpreter.
#
#    Builds a version of the 680x0 interpreter and creates a .LIB.
#    Set the first three variables below to determine the build type.
#
#----------------------------------------------------------------------------

.SUFFIXES: .asm .dll .inc .lib .lst .obj

MLFLAGS = -c -coff -Zi -DGEMPRO=$(GEMPRO) -DDEBUG=$(DEBUG) -DJITTER=0 -DBOUNDCHECK=1

OBJS = 68000.obj ops0.obj ops1.obj ops2.obj ops3.obj ops4.obj \
        ops5.obj ops6.obj ops7.obj ops8.obj ops9.obj opsa.obj \
        opsb.obj opsc.obj opsd.obj opse.obj opsf.obj 68kcpu.obj

*.obj: ..\68000.inc

68000.obj: ..\68000.asm ..\68000.inc
    ml $(MLFLAGS) -Fl ..\68000.asm

{..}.asm.obj:
    ml $(MLFLAGS) $<

{..}.c.obj:
    cl -c -Zi -Gz -Gy -O2 $<

#$(LIBFILE): $(OBJS)
#    link -lib -machine:ix86 -out:$(LIBFILE) $(OBJS)

$(DLLFILE): $(OBJS) makefile ..\common.mk
    link -dll -machine:ix86 -out:$(DLLFILE) -implib:$(LIBFILE) -export:cpi68K -optidata -map -debug -debugtype:cv,fixup $(OBJS)

