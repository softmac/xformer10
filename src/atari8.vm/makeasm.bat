del *.obj
for %%f in (*.asm) do ml -c -Zi -coff -Fl -DHWIN32 %%f
@rem link -lib -out:x6502.lib x6502.obj atariosb.obj xeroms.obj
link -lib -out:x6502.lib x6502.obj atariosb.obj xeroms.obj

