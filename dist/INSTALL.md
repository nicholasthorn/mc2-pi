# MechCommander 2: installing a release

This package is the game engine only. MechCommander 2's game data (maps,
art, sounds) is not included: you build it once from
[alariq/mc2srcdata](https://github.com/alariq/mc2srcdata) into this folder.
The data is the same on every platform, so you can build it on one machine
and copy it to another (a Pi, a Windows PC).

`VERSION` says which release this is; `licenses/` lists the licences of the
code it contains.

## 1. Install what the game needs

**Raspberry Pi 4 / 400 (`rpi-arm64`):** Raspberry Pi OS 64-bit (bookworm or
later) with the desktop and the default graphics driver.

```bash
sudo apt install libsdl2-2.0-0 libsdl2-mixer-2.0-0 libsdl2-ttf-2.0-0 libgles2 libegl1 zlib1g
```

**Linux x64 (`linux-x86_64`):** any distribution with glibc 2.35 or later
(Ubuntu 22.04, Debian 12, Fedora 36 and newer) and an OpenGL 4.0 GPU driver.
On Debian or Ubuntu:

```bash
sudo apt install libsdl2-2.0-0 libsdl2-mixer-2.0-0 libsdl2-ttf-2.0-0 libglew2.2 libgl1 zlib1g
```

**Windows 10 / 11 (`windows-x64`):** nothing to install; everything the game
needs is in the folder. An OpenGL 4.0 GPU driver is required. The build is
not code-signed, so Windows SmartScreen may warn the first time: choose
"More info", then "Run anyway".

## 2. Build the game data

You need a Linux machine for this step: the Pi itself, any Linux PC, or WSL on
Windows. The data tools come with the Linux and Pi packages (`tools/`).

```bash
sudo apt install git make
# the data repository; skipping the large movie files is fine, missions run without them
GIT_LFS_SKIP_SMUDGE=1 git clone https://github.com/alariq/mc2srcdata.git
cd mc2-<version>-<platform>
./build_gamedata.sh --src-data ../mc2srcdata --out . --no-movies
```

For the movies, install `git-lfs` and `ffmpeg`, run `git lfs pull` in
`mc2srcdata`, and leave out `--no-movies`. The data build needs no display:
it works over SSH and in WSL.

This fills the folder with `data/`, `assets/`, `*.fst` and `*.cfg`. For
Windows, copy those into the unpacked Windows folder.

## 3. Play

Start the game from its folder:

```bash
cd mc2-<version>-<platform>
./mc2                     # Linux and Pi
```

On Windows, double-click `mc2.exe`.

The first start finds no `options.cfg`, writes one with low settings, and may
end straight after; start the game again. Change the settings in the game's
Options screen.

To jump straight into a mission: `./mc2 -mission mc2_01`.

## When something goes wrong

- A box saying `STOP` with a message means the game hit an error it cannot
  carry on from. The same text is printed to the terminal. Please report it,
  with that text, at https://github.com/nicholasthorn/mc2-pi/issues.
- "Error loading ResourseDLL": start the game from its own folder (the game
  looks for its files in the current directory).
- A black window or an OpenGL error on Linux: check the GPU driver supports
  OpenGL 4.0 (`glxinfo | grep "OpenGL version"`).
