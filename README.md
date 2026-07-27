# xformer10

> **Linux / Raspberry Pi port** — This fork adapts Darek Mihocka's xformer10
> for Linux (Raspberry Pi OS aarch64), maintained on the `linux-port` branch.
> Prebuilt `.deb`: see the [GitHub Releases](../../releases) page.
> Read [why this port exists](PORTING.md).
> To build from source, see the
> [Building on Linux / Raspberry Pi](#building-on-linux--raspberry-pi) section below.
> The original Windows README continues below.

Xformer 10, the Atari 800 emulator for Windows 10, 7, and now 11!

It is the 10th generation of the Xformer (pronouned "Transformer") series of
Atari 800 emulators first developed in 1986 for the Atari ST.  The name is a
nod to electrical engineering and the concept of impedance matching; not unlike
how an emulator transforms non-native instruction sets and hardware.

This 10th version of Xformer (first released on 10-10-2018) adds Windows tablet
and touch-screen support, "tiled mode", "roulette mode", and "time travel" mode.
It is also the first natively built Atari 800 emulator for Windows on ARM, and
the only Atari 800 emulator granted permission by Atari Corp. to use Atari ROMs.

Please read XFORMER10-INSTALL.TXT for installation and setup directions,
and the file XFORMER10_README.RTF for a description of what's new.

Xformer is one of several Apple and Atari emulators developed by and available on:

   http://www.emulators.com/

older releases (with source) for Atari ST, MS-DOS, and Windows 95 are at:

   http://www.emulators.com/download.htm

If you have any questions email us at:

   xformer10@gmail.com.

## Building on Linux / Raspberry Pi

This port targets Raspberry Pi OS (aarch64) and was built from the `linux-port` branch.

### Build dependencies

```
sudo apt install cmake build-essential libsdl2-dev libsdl2-ttf-dev fonts-dejavu-core
```

### Build

```
cmake -B build-linux -S . && cmake --build build-linux -j$(nproc)
```

### Run from the build tree

```
./build-linux/xformer10
```

### System install

```
sudo cmake --install build-linux
```

### Produce and install a .deb package

```
(cd build-linux && cpack -G DEB)
sudo apt install ./build-linux/xformer10_*.deb
```

Runtime dependencies pulled in automatically: `libsdl2-2.0-0`, `libsdl2-ttf-2.0-0`, `fonts-dejavu-core`.

### Settings

User settings are stored in `~/.config/xformer`.

