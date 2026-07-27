cmake_minimum_required(VERSION 3.10)
project(xformer10 VERSION 10.24 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Upstream is MSVC code where plain char is signed; on aarch64 char defaults to
# unsigned, silently breaking every `char x; if (x < 0)` pattern (e.g. the DMA
# map build loop in xvideo.c wrapped 0->255 and filled rgDMAMap with garbage).
add_compile_options(-fsigned-char)

find_package(SDL2 REQUIRED)
find_package(SDL2_ttf REQUIRED)

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
    src/stubs_linux.c
    src/menu_sdl.c
    src/sdl_filebrowser.c
)

target_compile_definitions(xformer10 PRIVATE
    LINUX_PORT
    _GNU_SOURCE
    SDL2_ENABLED
)

target_include_directories(xformer10 PRIVATE
    src
    src/atari8.vm
    src/blocklib
    src/res
)

target_compile_options(xformer10 PRIVATE
    -Wno-incompatible-pointer-types
    -Wno-unused-value
    -Wno-implicit-function-declaration
    -Wno-duplicate-decl-specifier
    -fno-strict-aliasing
    -fwrapv
    -fcommon
)

find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(DRM libdrm)
endif()

if(TARGET SDL2::SDL2)
    target_link_libraries(xformer10 PRIVATE m pthread SDL2::SDL2 SDL2_ttf::SDL2_ttf)
else()
    target_include_directories(xformer10 PRIVATE ${SDL2_INCLUDE_DIRS})
    target_link_libraries(xformer10 PRIVATE m pthread ${SDL2_LIBRARIES} SDL2_ttf::SDL2_ttf)
endif()

if(DRM_FOUND)
    target_include_directories(xformer10 PRIVATE ${DRM_INCLUDE_DIRS})
    target_link_libraries(xformer10 PRIVATE ${DRM_LIBRARIES})
    target_compile_definitions(xformer10 PRIVATE HAVE_LIBDRM=1)
endif()

include(GNUInstallDirs)
install(TARGETS xformer10 RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES packaging/xformer10.desktop
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications)
install(FILES packaging/xformer10.png
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/64x64/apps)

set(CPACK_GENERATOR DEB)
set(CPACK_PACKAGE_NAME xformer10)
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Xformer 10 Atari 800/XL/XE emulator (Linux/Raspberry Pi port)")
set(CPACK_PACKAGE_CONTACT "coalis99@gmail.com")
set(CPACK_DEBIAN_PACKAGE_SECTION games)
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libsdl2-2.0-0, libsdl2-ttf-2.0-0, fonts-dejavu-core")
include(CPack)
