# XFormer — Claude Code Project Guide

Porting xformer10 (Atari 800 emulator, MIT-licensed) from Windows to Linux/Raspberry Pi OS (aarch64).
Upstream: https://github.com/softmac/xformer10 — your fork: git@github.com:coalis99/xformer10.git

---

## Directory layout

```
/home/coalis/ClaudeCodeProjects/XFormer/
├── xformer10/          ← git repo (origin = upstream, fork = your fork)
│   ├── src/            ← all port work happens here
│   └── build-linux/    ← cmake out-of-tree build (gitignored)
├── phase1/ … phase13/  ← per-phase Ralph loop artifacts (phases 14–15 have no dir)
└── CLAUDE.md           ← this file
```

---

## Branch rules — read before touching git

| Branch | Rule |
|--------|------|
| `master` | Upstream mirror — **never modify, never push to origin** |
| `linux-port` | Active port branch — all work goes here |
| `fork/linux-port` | Your GitHub fork — push here when phases complete |

Always verify you are on `linux-port` before editing:
```bash
git -C xformer10 rev-parse --abbrev-ref HEAD
```

---

## Build

```bash
cd /home/coalis/ClaudeCodeProjects/XFormer/xformer10
cmake -B build-linux -S . && cmake --build build-linux -j$(nproc)
# binary: build-linux/xformer10
```

Dependencies: `libsdl2-dev`, `libsdl2-ttf-dev`

### macOS

```bash
cmake -B build-macos -S . -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build-macos
# bundle: build-macos/Xformer10.app   (run from a terminal: build-macos/Xformer10.app/Contents/MacOS/Xformer10)
```

- No Homebrew runtime dependencies: SDL2, SDL2_ttf and FreeType are fetched and linked statically; the app links only system frameworks. Verify with `otool -L build-macos/Xformer10.app/Contents/MacOS/Xformer10`.
- Minimum macOS defaults to 11.0 (`CMAKE_OSX_DEPLOYMENT_TARGET`). Universal binary: `-DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"`.
- `-DXFORMER_MACOS_BUNDLE=OFF` builds a bare `xformer10` executable instead. The bundle is ad-hoc signed; set `XFORMER_CODESIGN_IDENTITY` to sign for distribution.
- Menus and file dialogs are native (Cocoa) by default; `-DXFORMER_UI=sdl` uses the in-window SDL menu bar and file browser instead (same default as Linux; Windows likewise defaults to native Win32). Only the SDL backend needs SDL_ttf and a UI font (CoreText in `src/font_sdl.c`).
- Config lives in `~/Library/Application Support/xformer`.
- HiDPI: the window is created with `SDL_WINDOW_ALLOW_HIGHDPI`; all layout stays in window units (points) and `ddlib_sdl.c` sets the render scale to the backing ratio each frame, so an integer zoom lands on device pixels (crisp on Retina). The SDL UI backend rasterizes its text at that ratio too.
- Icon: `src/res/xformer.icns` is generated from `gemul8r.ico` (nearest-neighbor upscale); `src/res/Info.plist.in` is the bundle plist template.

### Windows

```powershell
cmake -B build -S . -G "Visual Studio 17 2022" -A x64 -T ClangCL
cmake --build build --config RelWithDebInfo --target xformer10
# binary: build/RelWithDebInfo/xformer10.exe
```

- Resources: `src/res/gemul8r.rc` (icon `IDI_APP`, embedded `gemul8r.exe.manifest`) plus a `VERSIONINFO` block configured from `XFORMER_VERSION` via `src/res/version.rc.in`. The `.rc`'s menu and accelerator tables belong to the retired Win32 `WinMain` and are unused; `ui_win32.c` builds the menu at runtime.
- The linker runs with `/MANIFEST:NO` because the `.rc` embeds the manifest itself (same as upstream's `atari.vcxproj`). Add manifest entries to `gemul8r.exe.manifest`, not to CMake.
- SDL uses the exe's first icon group for the window class, so the title-bar/taskbar icon needs no code.

---

## Canonical source list

The live C source list is the `add_executable(xformer10 ...)` block in **`CMakeLists.txt`** (root level). That is the single source of truth for what compiles.

**`src/CMakeLists.txt` is stale** — it references files that don't exist (`atari16.vm/blitter.c`, `xatari.c`, `xfcable.c`, `xsb.c`, `encrypt.c`). Do not use it as a manifest.

**Dead code — do not port or budget time for:**
- `src/atari8.vm/atariosb.asm`, `x6502.asm`, `xeroms.asm` — the 3 `.asm` files are unreferenced by the modern build; the portable `x6502.c` is the live 6502 core.

---

## Key files added or substantially modified by the port

**New files (did not exist in upstream):**

| File | Purpose |
|------|---------|
| `src/main_linux.c` | Linux entry point, SDL2 event loop, 70 Hz throttle |
| `src/ddlib_sdl.c` / `ddlib_sdl.h` | DirectDraw → SDL_Texture shim; `GetSDLRenderer()` / `GetSDLWindow()` accessors |
| `src/ui/ui.h` | Platform UI layer: menu bar + file dialogs interface |
| `src/ui/ui_common.c` | Shared menu table, enabled/checked state, command dispatch, accelerators |
| `src/ui/ui_sdl.c` | SDL backend: dropdown menu bar drawn with SDL_ttf (Linux default) |
| `src/ui/sdl_filebrowser.c` / `.h` | Modal SDL2 file browser used by the SDL backend |
| `src/ui/ui_win32.c` | Native Win32 backend: HMENU on the SDL window, GetOpenFileName, IFileOpenDialog |
| `src/ui/ui_cocoa.m` | Native macOS backend: NSMenu in the system menu bar, NSOpenPanel/NSSavePanel |
| `src/platform_macos.m` / `.h` | macOS process setup outside SDL: momentum scrolling on, trackpad pixel deltas for the tile view |
| `src/keymap_sdl.c` | SDL scancode → PS/2 scan code mapping |
| `src/atari8.vm/atari800bus.h` | `PeekBAtari`/`PokeBAtari` bus dispatchers, moved out of `atari800.c` so non-MSVC builds inline them into `x6502.c` (MSVC relies on an `__assume` hint that the port's no-op `Assert` loses) |
| `src/compat_win.h` | Win32 type/macro compatibility layer |
| `src/compat_winfile.h` | Win32 file API compatibility |

**Substantially modified from upstream:**

| File | What changed |
|------|-------------|
| `src/ddlib.c` | DirectDraw surface management gutted; now delegates all rendering to ddlib_sdl.c |
| `src/sound.c` | waveOut backend replaced with SDL audio (PulseAudio via SDL) |
| `src/gemul8r.c` | Linux menu integration, `LinuxPickFile()`, SDL_DROPFILE drag-and-drop |

---

## Linux/Pi runtime rules

**Never use `getuid()` or `id -u` to derive socket paths.** The terminal runs as effective UID 0 while the graphical session is UID 1000. UID-based paths produce `/run/user/0/...` which doesn't exist.

- Use `XDG_RUNTIME_DIR` when available, with a glob fallback: `/run/user/*/pulse/native`
- For PulseAudio diagnosis: `PULSE_LOG=99 ./build-linux/xformer10` — libpulse prints the exact socket path it tries.

---

## Ralph loop workflow

Each phase has its own directory (`phaseN/`) containing five files:

| File | Role |
|------|------|
| `SPEC.md` | Immutable goal, DoD, constraints, known facts |
| `PLAN.md` | Immutable ordered gated steps |
| `PROGRESS.md` | Mutable agent log; first line is `STATUS: IN_PROGRESS / COMPLETE / BLOCKED` |
| `PROMPT.md` | Per-iteration Claude instructions |
| `ralph.sh` | Driver script |

**Running a loop:**
```bash
cd /home/coalis/ClaudeCodeProjects/XFormer/phaseN
./ralph.sh                          # defaults: Sonnet 4.6, MAX_ITER=25, COOLDOWN=5s
MAX_ITER=10 MODEL=claude-opus-4-8 ./ralph.sh
```

`ralph.sh` refuses to run unless `xformer10` is on `linux-port`. Stops on `STATUS: COMPLETE`, `STATUS: BLOCKED`, or 2 consecutive Claude failures. Each iteration logs to `phaseN/logs/iter-NN-*.log`.

**Draft review rule:** always show the user SPEC.md / PLAN.md / PROMPT.md drafts for review before writing them. Do not chain step writes without explicit user opt-in.

---

## Phase completion history

| Phase | Description | Commit |
|-------|-------------|--------|
| 1 | Source audit + portable core compiles on Pi aarch64; stub binary links; 84 `// PHASE3:` markers placed | — |
| 3→7 | DirectDraw→SDL_Texture (ddlib_sdl.c); waveOut→SDL audio (sound.c); PulseAudio socket fix | — |
| 8 | SDL2 menu bar (menu_sdl.c) — File/VM/Window/Disk with live checkmarks | — |
| 9 | Native SDL2 file browser (sdl_filebrowser.c) replaces zenity | 5994562 |
| 10 | File menu + Disk menu fully wired; save-as mode; ext auto-append | — |
| 11 | Window menu: fullscreen, stretch, turbo, Alt+Enter/F12/Alt+S | — |
| 12 | SDL joystick/gamepad input | — |
| 13 | Tiled-window VM overview mode | — |
| 14 | Sprite rendering fix (70 Hz throttle); vRefresh from SDL; gamma-2.2 LUT | d13bc72 |
| 15 | Numpad keys (VK_NUMPAD*); F5/tile shortcut fix; SDL_DROPFILE drag-and-drop | e0ba901 |

---

## Security / GitHub notes

- `.rom` files in `src/atari8.vm/` (ataribas.rom, atariosb.rom, atarixl.rom) are inherited from the upstream MIT repo — not added by this port.
- No credentials, API keys, or secrets belong in this repo. The `.gitignore` covers build artifacts; no `.env` patterns are needed.
- The `fork` remote uses SSH (`git@github.com`). The `origin` remote is HTTPS read-only to the upstream.
