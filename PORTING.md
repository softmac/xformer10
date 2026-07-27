# Why I made this

## The story

This past year I was at VCF (Vintage Computer Festival) in Tukwila and met up
with an old friend — the guy I started computing with in junior high school.
He had an Atari 800, and we would sit for hours playing games on it and typing
in games from Antic magazine (and losing games typed in from Antic during
crashes). Eventually I got an Atari 800XL for Christmas one year, and that was
that.

I'd brought my little Pi 500 to VCF, with the Altirra SDL build I'd found and
compiled, so we could try to play some of our old games. I still have my
original Atari 800 and all the peripherals and disks, but dragging all of that
onto the train would have been a bit much.

About 20 years ago I copied every one of my old Atari floppies to ATR files
using APE and a cable I built to connect my Indus GT drive to my Windows XP
computer. As I dumped them, I tested each one in Xformer to make sure it
worked — and I saved every one of those 378 VMs with its disk image running,
then saved a session of all of them in tiled mode. That session is what I go
back to every time I want to get at any of my old disks: Xformer tiled mode.

Altirra is fabulous, but I'd never used it on Linux, and in the couple of
hours we had for retro gaming I hit problems loading disk images, getting
joysticks working, things like that. I really wanted my Xformer tiled mode.

So, sitting in my hotel room after Saturday at VCF, I decided to try out
Claude Code on the Pi 500. I told it to go get the source from Darek
Mihocka's xformer10 GitHub repo and port it to run on my Pi.

I can't code. Really, really bad at it. But I'm an experienced computer tech
of some 30 years, and I know how to find a way through a project with the
tools at hand. I had just read some articles about Ralph loops — basically,
how someone with the programming skills of Ralph Wiggum could code like a
top-level engineer. After a month of vibe coding my way through three
models/versions of Claude Code — with the new release of Fable 5 tying up all
the loose ends and completing the project when I was stuck on graphical
issues in Sonnet — I now have a fully ported version of Xformer 10 happily
running on my Pi.

It's a dream come true to be able to tell a computer what I want and work
with it like a seasoned programmer who knows what I'm after and makes it
happen.

## How it was done

The port ran as a series of 25 small, gated phases, most of them driven by
Ralph loops — an agent running one verifiable step per iteration against an
immutable spec and plan, committing only when each step's gate passed. The
phase history lives in [CLAUDE.md](CLAUDE.md).

The technical shape of the port:

- **SDL2 replaced the entire Win32 surface** — DirectDraw became SDL textures,
  waveOut became SDL audio, and the Win32 windowing, menus, and file dialogs
  became a native SDL2 menu bar and modal file browser. No GTK, no Wine, no
  translation layers.
- **The portable core was already there.** The modern Windows build compiles
  zero assembly files; the portable C 6502 core is the live one. The work was
  in the platform layer, not the emulation.
- **Everything else came phase by phase**: joystick/gamepad support, the
  numpad and keyboard mapping, drag-and-drop, clipboard paste, time travel,
  settings persistence, an accurate GTIA palette — and yes, tiled mode.
- **Packaging last**: a `.deb` that installs a desktop launcher, so it runs
  from the Pi menu like any other application.

## The hard bug

The port worked early, but DLI-heavy games were wrong in ways that were
maddening to chase: Protector II flashed red where it shouldn't, Pogo Joe's
status row flickered. Five separate frame-timing theories (vsync, DRM vblank,
renderer flags...) were investigated and ruled out — frame delivery was
fine; the emulated pixels themselves were wrong.

The real cause turned out to be a single C portability detail: on aarch64,
plain `char` is **unsigned** by default, where the upstream MSVC code assumes
it is signed. A `char` loop index in `CreateDMATables` that was supposed to
break on `index < 0` instead wrapped from 0 to 255 and filled ANTIC's DMA
timing tables with garbage — which cascaded into corrupted beam positions,
double-fired scan-line bookkeeping, mode lines one scan short, and every
display list interrupt after that point firing on the wrong line. One
compiler flag (`-fsigned-char`) and a one-loop fix for a latent
uninitialized-memory bug (which exists on Windows too, and was reported
upstream as [issue #3](https://github.com/softmac/xformer10/issues/3) with
[PR #4](https://github.com/softmac/xformer10/pull/4)) — and every affected
game rendered perfectly, verified by instrumented before/after measurements.

## Where it stands

Xformer 10 runs natively on Raspberry Pi OS (aarch64), installable from the
`.deb` on the [Releases](../../releases) page. My 378 VMs are back in tiled
mode — this time on a computer that fits in a backpack.

Thanks to Darek Mihocka for xformer10 and for keeping it MIT-licensed, which
is what made this possible.
