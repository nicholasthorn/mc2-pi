# Dependencies:

Apt based distros:
```
sudo apt install libsdl2-dev, libsdl2-mixer-dev, libsdl2-ttf-dev, libsdl2-mixer-dev, zlib1g-dev, libglew-dev
```


# Directory Setup

This guide assumes you have the following directory structure:

```
some-empty-directory
|
+---mc2
\---mc2srcdata


```
To properly clone `mc2srcdata` you will need `git-lfs`. Make sure it's installed on your system.

From an empty directory issue the following commands.

```
git clone https://github.com/alariq/mc2.git
git clone https://github.com/alariq/mc2srcdata.git
```


# Compiling

## Executable
```
cd mc2
mkdir build64
cd build64
cmake -DCMAKE_LIBRARY_ARCHITECTURE=x64 ..
make
```

Copy `mc2` executable and the `shaders` folder to your executable folder of preference (say mc2exe):

```
cp -r mc2 ../shaders <mc2exe location>
```

## Resource library
```
cd mc2
cd test_scripts/res_conv
./res_conv.pl
cd ../../res
mkdir build64
cd build64
cmake -DCMAKE_LIBRARY_ARCHITECTURE=x64 ..
make
```
Copy `libmc2res_64.so` to your executable folder of preference (say mc2exe):
```
cp libmc2res_64.so <mc2exe location>
```

## Building data

`mc2srcdata/build_scripts` has its own `README.md`

```
cd mc2srcdata
cd build_scripts
```


### Prerequisites

You need to build data tools from mc2 repository. And copy executeables in `build_scripts` folder.
Those are the necessary tools (they are built along with a game, so no additional actions should be taken):
 - aseconv
 - makefst
 - makersp
 - pak
 - text\_tool

```
cp ../../mc2/build64/out/data_tools/{aseconv,makefst,makersp,pak} .
cp ../../mc2/build64/out/text_tool/text_tool .

```
### Dependencies
 * `ffmpeg` is used to convert movies. You should have it in the PATH environment variable or just puch absolute path in `config.mk`

### Building
`make all BUILD_PLATFORM=linux`

### How to clean data:
`make clean BUILD_PLATFORM=linux`

### Move data to where you've built the game

You'll need to copy the following files to the game directory (mc2exe):

* data
* assets
* \*.cfg
* \*.fst
* testtxm.tga

```
cp -r assets/ data/ *.cfg *.fst testttxm.tga <location of mc2exe>
```

# Running the game

Go to your `mc2exe` folder and execute `./mc2`, your game should hopefully run! Have fun!

# Tested setups (pi-port)

- **Ubuntu 26.04 (WSL2)**: the main development environment. GCC 15.2,
  Clang 21, CMake 4.2, cppcheck 2.19, glslang 16, SDL2 2.32. For the
  checks in `scripts/check.sh` and smoke runs also install `cppcheck`,
  `glslang-tools`, `xvfb` and `mesa-utils`, and build with Ninja:
  `cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo`.
- **Ubuntu 24.04**: the CI runners (`.github/workflows/`).
- **Ubuntu 22.04 (GCC 11)**: still builds. Its SDL2_mixer and SDL2_ttf
  packages have no CMake config files; `cmake/FindSDL2_*.cmake` fall back to
  pkg-config (kept for other distros too).
- Under WSLg, `scripts/smoke.sh` still runs headless (Xvfb); pass
  `--display` to watch a run.
