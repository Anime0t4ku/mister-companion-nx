# MiSTer Companion NX PoC

MiSTer Companion NX PoC is a small experimental Nintendo Switch homebrew project inspired by MiSTer Companion.

I created this mainly as a fun learning project to explore Switch homebrew development with devkitPro and libnx. It is not intended to replace or represent the desktop or mobile versions of MiSTer Companion.

## About

This project is a proof of concept focused on basic MiSTer interaction from the Nintendo Switch.

At this stage, the goal is simple: learn how Switch homebrew development works while experimenting with a lightweight MiSTer Companion-style interface. The project currently focuses on basic Connection and Device features using SSH.

## Important Notice

MiSTer Companion NX PoC is separate from the main MiSTer Companion desktop and mobile apps.

It may have fewer features, behave differently, or change direction entirely. Unless this project reaches a point where I am personally happy with the results, it should be seen as a personal experiment rather than an actively supported MiSTer Companion release.

Development may be slow, irregular, or stop at any time.

## Current Features

- Basic Nintendo Switch homebrew application
- Simple Connection screen
- SSH-based MiSTer connection handling
- Basic Device screen
- Device actions through SSH

## Requirements

To build the project, you need a Nintendo Switch homebrew development environment with:

- devkitPro
- devkitA64
- libnx
- switch-libssh2
- switch-mbedtls
- switch-zlib

## Building

From the project folder, run:

```sh
make clean
make
```

The build should create:

```text
mister-companion-nx.nro
```

Copy the `.nro` file to your Switch SD card, for example:

```text
/switch/mister-companion-nx/mister-companion-nx.nro
```

Then launch it through the Homebrew Menu.

## Disclaimer

MiSTer Companion NX PoC is an unofficial experimental homebrew project.

It is not affiliated with Nintendo, MiSTer FPGA, or any other official project. Use it at your own risk.