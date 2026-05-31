# MiSTer Companion NX

MiSTer Companion NX is a small experimental Nintendo Switch homebrew project inspired by MiSTer Companion.

I created this mainly as a fun learning project to explore Switch homebrew development with devkitPro and libnx. It is not intended to replace or represent the desktop or mobile versions of MiSTer Companion.

## About

This project is focused on interacting with a MiSTer FPGA from a Nintendo Switch.

The goal is simple: learn how Switch homebrew development works while experimenting with a lightweight MiSTer Companion-style interface. The project uses SSH to connect to a MiSTer and provides a small set of Companion-inspired features adapted for the Switch.

## Important Notice

MiSTer Companion NX is separate from the main MiSTer Companion desktop and mobile apps.

It may have fewer features, behave differently, or change direction entirely. Unless this project reaches a point where I am personally happy with the results, it should be seen as a personal experiment rather than an actively supported MiSTer Companion release.

Development may be slow, irregular, or stop at any time.

## Current Features

- **Connection tab**: Connect to a MiSTer manually, manage saved profiles, and scan the local network for MiSTer devices.
- **Device tab**: View basic MiSTer device information and perform simple device actions such as reboot handling.
- **Settings tab**: View and adjust supported MiSTer configuration options.
- **Scripts tab**: Run supported MiSTer scripts such as Update All.
- **Remote tab**: Use the Nintendo Switch controller to control MiSTer navigation, including OSD control and supported in-game input.
- **Wallpapers tab**: Browse and manage supported MiSTer wallpaper options.
- **Extras tab**: Install, update, uninstall, and check supported extras such as Zaparoo Frontend and RetroAchievement Cores.

## Requirements

To build the project, you need a Nintendo Switch homebrew development environment with:

- devkitPro
- devkitA64
- libnx
- switch-libssh2
- switch-mbedtls
- switch-zlib
- switch-libjpeg-turbo

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

## Configuration

MiSTer Companion NX stores its local configuration on the Switch SD card.

Connection profiles can be created from the Connection tab and are saved locally. Profiles include:

- Profile name
- Host / IP address
- Username
- Password

Profiles are intended to make reconnecting to a known MiSTer easier without re-entering connection details every time.

## Disclaimer

MiSTer Companion NX is an unofficial experimental homebrew project.

It is not affiliated with Nintendo.