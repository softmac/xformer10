cmake_minimum_required(VERSION 3.16)

#
# macOS: pin the minimum OS version before project() so that every vendored,
# statically linked dependency is built against the same target. The .app
# depends only on system frameworks, so this alone decides where it runs.
# Override with -DCMAKE_OSX_DEPLOYMENT_TARGET=... at configure time.
#
if(CMAKE_HOST_APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING
        "Minimum macOS version to target")
endif()

project(xformer10 C)

set(XFORMER_VERSION "10.10.2021")

option(XFORMER_MACOS_BUNDLE
       "macOS: build Xformer10.app instead of a bare executable" ON)

#
# Menu bar / file dialog backend (see src/ui/ui.h):
#   native  Win32 menu + common dialogs, or Cocoa NSMenu + NSOpenPanel
#   sdl     menu bar and file browser drawn inside the SDL window (SDL_ttf)
#
if(WIN32 OR APPLE)
    set(_xf_ui_default "native")
else()
    set(_xf_ui_default "sdl")
endif()
set(XFORMER_UI "${_xf_ui_default}" CACHE STRING
    "Menu bar / file dialog backend: native (Windows, macOS) or sdl")
set_property(CACHE XFORMER_UI PROPERTY STRINGS native sdl)
if(XFORMER_UI STREQUAL "native")
    if(NOT (WIN32 OR APPLE))
        message(FATAL_ERROR "XFORMER_UI=native is only available on Windows and macOS")
    endif()
elseif(NOT XFORMER_UI STREQUAL "sdl")
    message(FATAL_ERROR "XFORMER_UI must be 'native' or 'sdl' (got '${XFORMER_UI}')")
endif()
if(APPLE)
    enable_language(OBJC)   # ui_cocoa.m, platform_macos.m
endif()
set(XFORMER_CODESIGN_IDENTITY "-" CACHE STRING
    "macOS: codesign identity for the .app ('-' = ad-hoc, empty = don't sign)")

# CMake 4.x compatibility for old vendored dependencies such as
# SDL2_ttf's bundled FreeType.
set(CMAKE_POLICY_VERSION_MINIMUM 3.16)

# C11: identical typedef redefinitions (ADDR, PFNREAD, ...) across headers are
# legal in C11, which silences a pile of -Wtypedef-redefinition warnings.
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

include(FetchContent)

#
# Common CFLAGS for Visual Studio builds
#
if(CMAKE_GENERATOR MATCHES "Visual Studio")
	add_compile_options(
		# Inline any suitable
		/Ob2
		# Enable intrinsic functions
		/Oi
		# Multi-process build
		/MP
	)
endif()

if(WIN32 AND MSVC)
	add_link_options(
		/SUBSYSTEM:WINDOWS
	)
endif()

#
# Use LTO for non-Debug builds
#
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
	add_compile_options(
		$<$<NOT:$<CONFIG:Debug>>:-flto=thin>
	)
endif()

#
# Build all vendored libraries statically unless overridden explicitly.
#
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build dependencies as static libraries" FORCE)

#
# SDL2
#
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON  CACHE BOOL "" FORCE)
set(SDL_TEST   OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    SDL2
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG        release-2.32.10
    GIT_SHALLOW    TRUE
)

FetchContent_MakeAvailable(SDL2)

#
# SDL2_ttf -- only the SDL UI backend renders text.
#
# Build/fetch FreeType etc. internally, rather than requiring them from
# the host system.
if(XFORMER_UI STREQUAL "sdl")
    set(SDL2TTF_VENDORED ON  CACHE BOOL "" FORCE)
    set(SDL2TTF_SAMPLES  OFF CACHE BOOL "" FORCE)

    FetchContent_Declare(
        SDL2_ttf
        GIT_REPOSITORY https://github.com/libsdl-org/SDL_ttf.git
        GIT_TAG        release-2.24.0
        GIT_SHALLOW    TRUE
    )

    FetchContent_MakeAvailable(SDL2_ttf)

    #
    # fontconfig is only needed by the Unix implementation of font_sdl.c.
    #
    if(UNIX AND NOT APPLE)
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(FONTCONFIG REQUIRED IMPORTED_TARGET fontconfig)
    endif()
endif()


add_executable(xformer10
    src/atari8.vm/atari800.c
    src/atari8.vm/atarikey.c
    src/atari8.vm/atariosb.c
    src/atari8.vm/atarixl5.c
    src/atari8.vm/atarixlb.c
    src/atari8.vm/atarixlc.c
    src/atari8.vm/atarixlo.c
    src/atari8.vm/x6502.c
    src/atari8.vm/xkey.c
    src/atari8.vm/xlxeram.c
    src/atari8.vm/xmon.c
    src/atari8.vm/xsio.c
    src/atari8.vm/xvideo.c

    src/blocklib/blockapi.c
    src/blocklib/sectorio.c
    src/blocklib/mac_hfs.c

    src/gemul8r.c
    src/ddlib.c
    src/ddlib_sdl.c
    src/sound.c
    src/props.c
    src/printer.c
    src/serial.c
    src/romcard.c
    src/keymap_sdl.c
    src/main_linux.c
    src/font_sdl.c
    src/ui/ui_common.c
)

#
# UI backend sources (src/ui/ui.h).
#
if(XFORMER_UI STREQUAL "sdl")
    target_sources(xformer10 PRIVATE
        src/ui/ui_sdl.c
        src/ui/sdl_filebrowser.c
    )
elseif(WIN32)
    target_sources(xformer10 PRIVATE src/ui/ui_win32.c)
elseif(APPLE)
    target_sources(xformer10 PRIVATE src/ui/ui_cocoa.m)
    set_source_files_properties(src/ui/ui_cocoa.m PROPERTIES
        COMPILE_OPTIONS "-fobjc-arc"
    )
endif()

target_include_directories(xformer10 PRIVATE
    src
    src/atari8.vm
    src/blocklib
    src/res
    src/ui
)

target_compile_definitions(xformer10 PRIVATE
    SDL2_ENABLED
)

#
# Platform-specific definitions/libraries.
#
if(WIN32)
    # shell32/ole32: SHGetKnownFolderPath in font_sdl.c and the folder picker
    # (IFileOpenDialog) in ui_win32.c; comdlg32: GetOpenFileName.
    target_link_libraries(xformer10 PRIVATE
        shell32
        ole32
        comdlg32
    )

    #
    # Windows resources: the application icon (IDI_APP) and the embedded
    # manifest, plus a VERSIONINFO block generated from XFORMER_VERSION. The
    # menu and accelerator tables in gemul8r.rc belong to the retired Win32
    # WinMain and are unused; ui_win32.c builds the menu at runtime.
    #
    enable_language(RC)

    # "10.10.2021" -> "10,10,2021,0" for FILEVERSION/PRODUCTVERSION.
    string(REPLACE "." ";" _xf_ver_parts "${XFORMER_VERSION}")
    list(LENGTH _xf_ver_parts _xf_ver_len)
    while(_xf_ver_len LESS 4)
        list(APPEND _xf_ver_parts 0)
        math(EXPR _xf_ver_len "${_xf_ver_len} + 1")
    endwhile()
    list(JOIN _xf_ver_parts "," XFORMER_VERSION_RC)
    configure_file(src/res/version.rc.in
                   "${CMAKE_CURRENT_BINARY_DIR}/version.rc" @ONLY)

    target_sources(xformer10 PRIVATE
        src/res/gemul8r.rc
        "${CMAKE_CURRENT_BINARY_DIR}/version.rc"
    )

    # gemul8r.rc embeds gemul8r.exe.manifest itself (upstream's .vcxproj
    # sets GenerateManifest=false for the same reason); a linker-generated
    # manifest alongside it would be a duplicate RT_MANIFEST resource.
    if(MSVC)
        target_link_options(xformer10 PRIVATE /MANIFEST:NO)
    endif()
elseif(APPLE)
    target_compile_definitions(xformer10 PRIVATE
        LINUX_PORT
    )

    # Process setup SDL can't do (momentum scrolling, trackpad detection).
    target_sources(xformer10 PRIVATE src/platform_macos.m)
    set_source_files_properties(src/platform_macos.m PROPERTIES
        COMPILE_OPTIONS "-fobjc-arc"
    )
    target_link_libraries(xformer10 PRIVATE "-framework AppKit")

    # CoreText resolves the UI font in font_sdl.c. Everything else the app
    # needs (Cocoa, CoreAudio, Metal, ...) is carried by SDL2::SDL2-static.
    target_link_libraries(xformer10 PRIVATE
        "-framework CoreText"
        "-framework CoreFoundation"
    )
    if(XFORMER_UI STREQUAL "native")
        # UTType for the NSOpenPanel file filters in ui_cocoa.m.
        target_link_libraries(xformer10 PRIVATE
            "-framework UniformTypeIdentifiers"
        )
    endif()

    if(XFORMER_MACOS_BUNDLE)
        set(XFORMER_ICNS "${CMAKE_CURRENT_SOURCE_DIR}/src/res/xformer.icns")
        target_sources(xformer10 PRIVATE "${XFORMER_ICNS}")
        set_source_files_properties("${XFORMER_ICNS}" PROPERTIES
            MACOSX_PACKAGE_LOCATION Resources
        )

        set_target_properties(xformer10 PROPERTIES
            OUTPUT_NAME                        "Xformer10"
            MACOSX_BUNDLE                      TRUE
            MACOSX_BUNDLE_INFO_PLIST           "${CMAKE_CURRENT_SOURCE_DIR}/src/res/Info.plist.in"
            MACOSX_BUNDLE_BUNDLE_NAME          "Xformer 10"
            MACOSX_BUNDLE_GUI_IDENTIFIER       "com.emulators.xformer10"
            MACOSX_BUNDLE_ICON_FILE            "xformer.icns"
            MACOSX_BUNDLE_BUNDLE_VERSION       "${XFORMER_VERSION}"
            MACOSX_BUNDLE_SHORT_VERSION_STRING "${XFORMER_VERSION}"
            MACOSX_BUNDLE_COPYRIGHT            "Copyright (C) 1986-2021 Darek Mihocka and Danny Miller"
        )

        # Seal the bundle so Info.plist and the icon are covered by the
        # signature. Ad-hoc by default; set XFORMER_CODESIGN_IDENTITY to a
        # Developer ID to sign for distribution, or to "" to skip.
        find_program(XFORMER_CODESIGN codesign)
        if(XFORMER_CODESIGN AND NOT XFORMER_CODESIGN_IDENTITY STREQUAL "")
            add_custom_command(TARGET xformer10 POST_BUILD
                COMMAND "${XFORMER_CODESIGN}" --force
                        --sign "${XFORMER_CODESIGN_IDENTITY}"
                        "$<TARGET_BUNDLE_DIR:xformer10>"
                COMMENT "Signing $<TARGET_BUNDLE_DIR:xformer10>"
                VERBATIM
            )
        endif()
    endif()
else()
    target_compile_definitions(xformer10 PRIVATE
        LINUX_PORT
        _GNU_SOURCE
    )

    target_link_libraries(xformer10 PRIVATE
        m
        pthread
    )
endif()

#
# These are GCC/Clang compatibility flags for the old codebase.
#
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(xformer10 PRIVATE
        -Wno-incompatible-pointer-types
        -Wno-unused-value
        -Wno-implicit-function-declaration
        -Wno-duplicate-decl-specifier
        -fno-strict-aliasing
        -fwrapv
        -fcommon
    )
endif()

#
# Portable dependencies.
#
target_link_libraries(xformer10 PRIVATE
    SDL2::SDL2-static
    SDL2::SDL2main
)

if(XFORMER_UI STREQUAL "sdl")
    target_link_libraries(xformer10 PRIVATE
        SDL2_ttf::SDL2_ttf-static
    )
endif()

if(TARGET PkgConfig::FONTCONFIG)
    target_link_libraries(xformer10 PRIVATE
        PkgConfig::FONTCONFIG
    )
endif()
