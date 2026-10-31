# Raspberry Pi 4 / 400 (and WSL) build

The `pi-port` branch adds an OpenGL ES 3.1 renderer (`-DMC2_GLES=ON`) so the
game runs on the Raspberry Pi 4/400 GPU (V3D, Mesa), which has no desktop
OpenGL 4.x. The desktop GL renderer is still the default and is unchanged
for other platforms.

## Raspberry Pi OS (64-bit, Debian bookworm)

```
sudo apt install build-essential cmake ninja-build pkg-config git git-lfs \
    libsdl2-dev libsdl2-mixer-dev libsdl2-ttf-dev zlib1g-dev \
    libgles-dev libegl-dev libglew-dev ffmpeg
```

`libglew-dev` is only needed by the data tools' CMake files; the game itself
links `libGLESv2` directly when `MC2_GLES=ON`.

```
mkdir build64 && cd build64
cmake -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DMC2_GLES=ON ..
ninja
```

Game data is built exactly as in `BUILD-LINUX.md` (`mc2srcdata`,
`make all BUILD_PLATFORM=linux`). Building the data is much faster on a PC;
the result is platform independent and can be copied to the Pi.

Game directory layout (e.g. `~/mc2exe`):

```
mc2  libmc2res_64.so  shaders/
data/  assets/  *.fst  *.cfg  testtxm.tga
```

Run with `./mc2`, or `./mc2 -mission mc2_01` to jump straight into a mission.

## Benchmarking and profiling

`scripts/bench.sh` runs missions in the deterministic `-bench` mode (same
frames on every run) and records fps, frame-time percentiles and the CPU
split per run. Run it on the Pi's own display with nothing else busy:

```
scripts/bench.sh --game-dir ~/mc2exe --build-dir build64 --renderers gpu
```

Stock missions open quietly. For a frame with real load (25 units fighting,
artillery, a moving camera), play a scripted scenario; each of its segments
is timed separately and screenshotted (`bench/README.md`):

```
scripts/bench.sh --game-dir ~/mc2exe --build-dir build64 --renderers gpu --scenario bench/mc2_19.fit
```

For CPU profiles, add `--perf` (one extra sampled run per mission, gameplay
frames only). It needs `perf` and, for readable call stacks, a build with
frame pointers in its own directory, so timed numbers still come from a
normal build:

```
sudo apt install linux-perf
sudo sysctl kernel.perf_event_paranoid=2      # if it is 3
cmake -S . -B build-fp -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DMC2_GLES=ON -DMC2_FRAME_POINTERS=ON
cmake --build build-fp
scripts/bench.sh --game-dir ~/mc2exe --build-dir build-fp --renderers gpu --runs 0 --perf
```

Each profile's `*.perf.txt` shows time per library (game, GL driver,
kernel) and the top functions. Mesa's driver functions only have names if
its debug symbols are installed; Pi OS ships its own Mesa build, so matching
`-dbgsym` packages may not exist. The per-library split works either way.
Stacks also stop inside system libraries built without frame pointers;
`--call-graph dwarf` walks through them at the cost of a much larger
`perf.data`.

## Windows / WSL2 (WSLg)

Same steps on Ubuntu 22.04 under WSL2 (`MC2_GLES` ON or OFF).

WSLg's GPU driver (Mesa D3D12) only exposes OpenGL 4.1 and ES 3.0, and the
desktop renderer currently shows a blank white screen on it. Use Mesa's
software rasterizer instead; it supports both GL 4.5 and ES 3.2:

```
LIBGL_ALWAYS_SOFTWARE=1 ./mc2
```

Building the `MC2_GLES=ON` variant and running it this way is the closest
thing to the Pi's renderer available on a PC.

## Shaders

Shader `#version` lines are injected at runtime (`gameos_graphics.cpp`,
`buildShaderHeader`): `#version 420` on desktop GL 4.2+, `4xx` plus
`GL_ARB_shading_language_420pack` on older desktop GL, and `#version 310 es`
with default precisions on GLES. Keep shader code valid for both GLSL 4.20
and GLSL ES 3.10: float literals in float math (`1.0 - x`, not `1 - x`),
`u` suffixes for unsigned constants, and no `const` initialised from
uniforms.
