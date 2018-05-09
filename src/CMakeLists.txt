set (GEMUL8R_SOURCES
	atari16.vm/blitter.c
	atari16.vm/gemdos.c
	atari16.vm/ikbd.c
	atari8.vm/atari800.c
	atari8.vm/atarikey.c
	atari8.vm/atariosb.c
	atari8.vm/atarixl5.c
	atari8.vm/atarixlb.c
	atari8.vm/atarixlc.c
	atari8.vm/atarixlo.c
	atari8.vm/x6502.c
	atari8.vm/xatari.c
	atari8.vm/xfcable.c
	atari8.vm/xkey.c
	atari8.vm/xlxeram.c
	atari8.vm/xmon.c
	atari8.vm/xsb.c
	atari8.vm/xsio.c
	atari8.vm/xvideo.c
	blocklib/blockapi.c
	blocklib/sectorio.c
	blocklib/mac_hfs.c
	ddlib.c
	encrypt.c
	gemul8r.c
	printer.c
	props.c
	res/gemul8r.rc
	romcard.c
	serial.c
	sound.c
	res/gemul8r.exe.manifest
)

# Windows 7 API, needed for WM_GESTURE* events
add_definitions(-D_WIN32_WINNT=0x0601)

add_executable(GEMUL8R WIN32 ${GEMUL8R_SOURCES})
target_link_libraries(GEMUL8R ddraw winmm gdi32 shell32 comdlg32)

# Use LTO options specific to each compiler
if(USE_LTO)
	if(CMAKE_C_COMPILER_ID MATCHES "GNU")
		target_compile_options(GEMUL8R PRIVATE -flto=16 -fno-fat-lto-objects -flto-odr-type-merging)

	# Clang LTO doesn't seem to be available on Windows yet
	#elseif(CMAKE_C_COMPILER_ID MATCHES "Clang")
	#	target_compile_options(GEMUL8R PRIVATE -flto=thin)
	#	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -flto=thin")

	elseif(CMAKE_C_COMPILER_ID MATCHES "MSVC")
		target_compile_options(GEMUL8R PRIVATE /GL)
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /LTCG")

	endif()
endif()

if(CMAKE_C_COMPILER_ID MATCHES "MSVC")
	if(MSVC_C_ARCHITECTURE_ID MATCHES "ARMV7")
		add_definitions(-D_ARM_WINAPI_PARTITION_DESKTOP_SDK_AVAILABLE=1)
	endif()

	if(NOT MSVC_C_ARCHITECTURE_ID MATCHES "ARM")
		# use __fastcall by default
		target_compile_options(GEMUL8R PRIVATE /Gr)

		if (${CMAKE_SIZEOF_VOID_P} MATCHES 4)
			# use SSE2 on 32-bit Windows
			target_compile_options(GEMUL8R PRIVATE /arch:SSE2)
		elseif (${CMAKE_SIZEOF_VOID_P} MATCHES 8)
			# use AVX2 on 64-bit Windows
			target_compile_options(GEMUL8R PRIVATE /arch:AVX2)
		endif()
	endif()

	set(CMAKE_RC_FLAGS "${CMAKE_RC_FLAGS} /nologo")
endif()

if(CMAKE_C_COMPILER_ID MATCHES "GNU" OR
   CMAKE_C_COMPILER_ID MATCHES "Clang")
	target_compile_options(GEMUL8R PRIVATE -march=haswell -ffast-math)

	# Basic correctness (common sense)
	target_compile_options(GEMUL8R PRIVATE -fno-strict-aliasing -fwrapv)

	# workaround for safeseh_xmm problem when using -march=haswell
	target_compile_options(GEMUL8R PRIVATE -fno-asynchronous-unwind-tables)

	# the PFNx typedefs don't have a function signature that matches the actual
	# instances of them, but this is just a language limitation
	target_compile_options(GEMUL8R PRIVATE -Wno-incompatible-pointer-types)
endif()
if(CMAKE_C_COMPILER_ID MATCHES "Clang")
	# Stubbed out emulation functions have "unused values" that prevent MSVC
	# from complaining, but cause Clang to complain.
	target_compile_options(GEMUL8R PRIVATE -Wno-unused-value)
endif()

target_include_directories(GEMUL8R PRIVATE . res)
set_property(TARGET GEMUL8R PROPERTY C_STANDARD 99)
