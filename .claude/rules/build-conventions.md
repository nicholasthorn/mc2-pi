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
- **Windows header macros.** On MSVC, `<windows.h>` (reached through the
  platform headers) defines `small`, `near`, `far`, `min`, `max` and others as
  macros, so a variable called `small` breaks only the Windows build
  (`4b98eee`). Avoid those names, even in tests.
- **`-fpermissive` is gone** (the tree builds without it). Do not bring it back:
  it hid code that Clang and MSVC reject.

## Warnings and static analysis

- **New code builds with `-Wall -Wextra -Werror`** (GCC/Clang) via
  `MC2_STRICT_WARNINGS`, applied per source file with
  `set_source_files_properties(<file> PROPERTIES COMPILE_OPTIONS "${MC2_STRICT_WARNINGS}")`
  in the directory that owns the target. Add every new `.cpp` to that list.
  Legacy files keep the global flags; do not turn the strict set on for them
  wholesale.
- A target that holds new code but includes legacy headers should include those
  headers as `SYSTEM` (see `tests/unit/CMakeLists.txt`), so warnings report the
  new code only.
- **cppcheck** (`scripts/cppcheck_gate.py`) compares against
  `scripts/cppcheck-baseline.txt` and fails on any finding not in it: changed
  `.cpp` files on every push/PR (`ci.yml`), the full tree nightly
  (`static.yml`). Fix new findings; for a genuine false positive use an inline
  `// cppcheck-suppress <id>` with a reason. Never add lines to the baseline by
  hand; regenerate it with the `update_baseline` run of `static.yml` (the
  baseline depends on the cppcheck version, so it comes from CI) after fixing
  findings, so counts only go down.

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
