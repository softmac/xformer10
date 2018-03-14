@rem del *.obj
del /f /a x6502.obj x6502.lib xeroms.obj
for %%f in (x6502.c atarikey.c atariosb.c atarixl*.c) do (
    cl -c -MT -Gy -GF -Zi -FAsc -O2 -Ob2 -DHWIN32 %%f
    )
link -lib -out:x6502.lib x6502.obj atarikey.obj atariosb.obj atarixl*.obj

