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
├── phase1/ … phaseN/  ← per-phase Ralph loop artifacts (one dir per phase, always)
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
| `src/menu_sdl.c` / `menu_sdl.h` | SDL2 dropdown menu bar (File/VM/Window/Disk) |
| `src/sdl_filebrowser.c` / `sdl_filebrowser.h` | Modal SDL2 file browser (replaces zenity) |
| `src/keymap_sdl.c` | SDL scancode → PS/2 scan code mapping |
| `src/stubs_linux.c` | Win32 stubs that are no-ops on Linux |
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

## Known bad logic paths — do not repeat

These were discovered during 5+ failed attempts to fix screen jitter (Phases 14–19). Each one consumed one or more iterations without progress.

**1. SDL_RENDERER_PRESENTVSYNC is a no-op on Pi OS under both Wayland and X11/XWayland.**
The compositor does NOT pass vsync signals to application windows on either backend. `SDL_RenderPresent` returns in 0–2 ms regardless of PRESENTVSYNC flag. Do not rely on it for frame pacing. Use `clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, ...)` for the throttle. See also rule 8 (measured proof) and rule 9 (DRM vblank also doesn't fix jitter).

**2. SDL_Delay is useless for sub-jiffy timing on Pi.**
Pi OS runs a 250 Hz kernel tick (4 ms minimum sleep quantum). `SDL_Delay(1)` sleeps ~4 ms, not 1 ms. A throttle loop that relied on `SDL_Delay(1)` calls ran at ~50 fps instead of 60 fps. Use `clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, ...)` for ~100 µs precision.

**3. Measure actual fps BEFORE investigating root causes.**
All 5 jitter-fix attempts were implemented before the actual frame rate was confirmed. One telemetry line (avg/min/max frame ms) would have exposed the 50 fps problem immediately and saved 3 iterations.

**4. Do not oscillate on renderer flags without a falsifiable hypothesis.**
`SDL_RENDERER_PRESENTVSYNC` ↔ `SDL_RENDERER_ACCELERATED` was toggled 3+ times with no measurement step between changes. Always measure before and after; never change a flag twice.

**5. `vi.qpfCold` and `vi.qpcCold` must be initialized before any throttle code runs.**
`GetCycles()` divides by `vi.qpfCold`. On Linux, `WinMain` is never called, so both values are 0. ARM integer divide-by-zero silently returns 0, making `GetCycles()` always return 0 and causing the `fBrakes` throttle to spin forever. Always initialize both via `QueryPerformanceFrequency` / `QueryPerformanceCounter` before `InitProperties()`.

**6. `fBrakes` must be set to TRUE at Linux startup.**
`fBrakes = TRUE` is only in `WinMain` (gemul8r.c:1615). On Linux, the global is 0 (FALSE = turbo mode) until explicitly set. Add `fBrakes = TRUE;` in `main()` before `InitProperties()`.

**7. Do not chase rendering pipeline or timing if the emulation pixel data itself may be wrong.**
Protector II shows garbled pixels — a data-corruption symptom, not a timing symptom. Timing fixes cannot fix wrong pixel values. When garbled/corrupted pixels are reported, investigate the rendering data path (pvBits content, palette lookup, stride) before frame-rate or VSYNC code.

**8. SDL_RENDERER_PRESENTVSYNC under X11/XWayland is also a no-op on Pi OS.**
Measured with `SDL_GetRendererInfo` + timed `SDL_RenderPresent`: the OpenGL renderer reports `vsync=YES` (flags=0xe) but `SDL_RenderPresent` consistently returns in 0–2 ms. XWayland queues frames without blocking on vblank. Do not rely on PRESENTVSYNC for frame pacing under any compositor on Pi OS.

**9a. Plain `char` is unsigned on aarch64 — the build requires `-fsigned-char`.**
Upstream MSVC code assumes signed `char`. On ARM, `char index; if (index < 0)` never fires and wraps 0→255 instead. This corrupted the entire rgDMAMap cycle table in `InitDMATables` (xvideo.c): the backward free-cycle search walked out of bounds and stored garbage hclock values, which indexed past `rgPIXELMap` into `rgPMGMap`, producing impossible cclock values (4088+) that double-fired `PSLPostpare` → lost scan lines → the Protector II DLI drift chased through Phases 22–23. `add_compile_options(-fsigned-char)` in CMakeLists.txt fixes the whole class. Never remove that flag; suspect char signedness first when ARM-only emulation corruption appears.

**9. DRM vblank wait does not fix scroll jitter on Pi OS.**
`drmWaitVBlank(card1, DRM_VBLANK_RELATIVE, 1)` on `/dev/dri/card1` (vc4-drm HDMI controller) builds and runs correctly but produces no observable improvement in Pogo Joe, Joust, or Protector II. Frame delivery is already phase-correct; the remaining jitter is in the emulation content (wrong pixel data produced by xvideo.c), not in how frames reach the display. Do not add further frame-delivery timing mechanisms — investigate emulation accuracy instead.

---

## Model guardrails — claims, phrasing, verification

Global rules (banned phrases, completion-claim evidence, measure-before/after) live in `~/.claude/CLAUDE.md` and already apply here — see that file rather than restating it. Project-specific note: "the root cause is clear" was used more than once on this project when the cause was in fact unknown — watch for it specifically.

**Self-check:** if you catch yourself (or the user catches you) using a banned pattern, add it to this list in the same session.

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

**Phase folder requirement (mandatory):** Every new phase **must** have a `phaseN/` directory created under `/home/coalis/ClaudeCodeProjects/XFormer/` before any code work begins. That directory must contain all five files (SPEC.md, PLAN.md, PROGRESS.md, PROMPT.md, ralph.sh). Never skip this step, even for small phases. Phases 14–21 had no directory and this made them harder to reproduce and audit — do not repeat that pattern.

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
| 16 | Clipboard paste ASCII/ATASCII via SDL_GetClipboardText | 518e186 |
| 17 | Time-travel rewind/fixpoint via VM menu | 1157c4c |
| 18 | Auto-hide cursor when emulator window is focused | 7344fda |
| 19 | Settings persistence; fix startup crash from premature LoadProperties | d66d8a0 |
| 20 | Accurate GTIA palette; DRM vblank throttle; x11 vsync fix | 84014a1 |
| 21 | File browser mouse-scroll flicker fix (SDL_RenderClear in fb_render) | — |
| 22 | xvideo.c pixel jitter investigation (pvBits diagnostic) | bcab34b |
| 23 | DLI scan-line drift root-caused to unsigned char on aarch64: -fsigned-char + DMA-map tail fill + PSL guards; Protector II/Pogo Joe/Joust verified clean | cca6d92 |
| 24 | Packaging: cmake install target, desktop launcher + icon, CPack .deb, README Linux section; installs from Pi menu as desktop user | 7783e10 |
| 25 | Release & upstream: fork README banner, v10.24-linux release with .deb, upstream issue softmac#3 + PR softmac#4 (CreateDMATables uninit-tail fix) | 6043e58 |

---

## Security / GitHub notes

- `.rom` files in `src/atari8.vm/` (ataribas.rom, atariosb.rom, atarixl.rom) are inherited from the upstream MIT repo — not added by this port.
- No credentials, API keys, or secrets belong in this repo. The `.gitignore` covers build artifacts; no `.env` patterns are needed.
- The `fork` remote uses SSH (`git@github.com`). The `origin` remote is HTTPS read-only to the upstream.
