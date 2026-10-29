# Build conventions

## CMake is the only build

- Top-level `CMakeLists.txt` adds `mclib/`, `mclib/mlr`, `mclib/gosfx`,
  `mclib/stuff`, `gui/`, `GameOS/gameos`, `GameOS/src`, and lists `code/*.cpp`
  in `SOURCES` by hand. A new game `.cpp` must be added to that list.
- Do not edit `MechCmd2.vcproj` / `mc2res.vcproj`; they are not built.
- Build type: `RelWithDebInfo` unless asked otherwise.

## The four configurations

Every change must build in all four (CI checks them on every push and PR):

| Config | Compiler | Renderer |
|---|---|---|
| Linux x64 | GCC | desktop GL (`MC2_GLES=OFF`) |
| Linux x64 | Clang | desktop GL |
| Linux arm64 | GCC | GLES (`MC2_GLES=ON`) |
| Windows x64 | MSVC (VS 2022) | desktop GL |

When you can only build one locally, say which ones were not built.

## Platform macros

- `LINUX_BUILD` is defined for **every** build, Windows included. Do not use it to
  mean "running on Linux".
- `PLATFORM_WINDOWS` is defined only by MSVC builds. Use it for real
  Windows-only code.
- `MC2_GLES` selects the GLES renderer.
- `USE_ASSEMBLER_CODE=0` always; never add inline x86 assembly. Use portable C++ or
  compiler builtins (see `mclib/heap.cpp`, `__builtin_return_address`).

## Portability traps already hit

- **`char` signedness.** The code assumes signed `char` (x86/MSVC). GCC/Clang
  builds pass `-fsigned-char` because `char` is unsigned on ARM. Do not remove it.
  In new code, use `int8_t` / `uint8_t` when the sign matters.
- **Pointer size.** Legacy code stores pointers in `long` / `DWORD` in places. On
  64-bit Linux `long` is 64-bit but on Windows it is 32-bit. Use `intptr_t` /
  `size_t` in new code and treat any pointer/integer cast as suspect.
- **Type punning.** GCC `-O2` breaks `*(long*)&someDouble` style punning (see the
  `ablxstd.cpp` fix). Use `memcpy`.
- **`-fpermissive`** is on for GCC/Clang and hides non-conforming code. Do not rely
  on it in new code; MSVC and Clang may reject what GCC accepts.

## Dependencies

- Linux: system packages (`libsdl2-dev`, `libsdl2-mixer-dev`, `libsdl2-ttf-dev`,
  `zlib1g-dev`, `libglew-dev` / `libgles-dev`). `cmake/FindSDL2_*.cmake` fall back
  to pkg-config on distros without CMake config files.
- Windows: `3rdparty.zip` (git-lfs). CI caches it by LFS object id to save LFS
  bandwidth: do not make CI fetch LFS objects on every run.
- Adding a dependency means updating: the apt list in `ci.yml`, the Windows
  `3rdparty` story, and all three `BUILD-*.md` files.

## Commits

- One logical change per commit; prefix with the area (`render:`, `gles:`,
  `arm64:`, `build:`, `ci:`, `fix:`, `msvc:`, `clang:`).
- Push to the `pi` remote only.
