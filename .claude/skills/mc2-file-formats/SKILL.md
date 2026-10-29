---
name: mc2-file-formats
description: "Work on MC2 file and data handling: File::open and path conversion, FST archives (FastFile), packet/PAK files, FIT/ini (FitIniFile), CSV, LZ compression, TGA, paths.h globals, and the data tools (makefst, pak, aseconv, makersp) that build mc2srcdata. Use when an asset does not load, when changing mclib/file.cpp, ffile.cpp, fastfile.cpp, packet.cpp, inifile.cpp, csvfile.cpp, lz*.cpp, paths.cpp, or data_tools/, or when writing tests for any of them."
---

## Scope

How the engine finds and reads game data, and the tools that produce it. These
are the most unit-testable parts of the engine and the first target for tests
(`planning/QUALITY-ROADMAP.md` Phase 3).

Line numbers drift: grep the symbol before citing a line.

## Key files

- `mclib/file.cpp/.h`: `File` base class. `File::open(const char*, FileMode,
  ...)` lowercases the name (unless `doNotLower`), converts `\` to `/` under
  `LINUX_BUILD` (paths stored inside data files use `\`), then tries the FST
  archives before loose files. Other overloads open a child of a parent file or
  an in-memory buffer.
- `mclib/ffile.cpp`, `mclib/fastfile.cpp`: `FastFile` (FST archive), `FastFileInit`,
  `FastFileFind`, the `fastFiles` registry.
- `mclib/packet.cpp`: `PacketFile` (`.pak` containers of numbered packets).
- `mclib/inifile.cpp`: `FitIniFile` (`.fit` block/variable text format).
- `mclib/csvfile.cpp`: `CSVFile` (Excel-style CSV, e.g. building tables).
- `mclib/lzcomp.cpp`, `mclib/lzdecomp.cpp`: LZ used by FST and packets.
- `mclib/tgainfo.cpp`: TGA headers.
- `mclib/paths.cpp/.h`: path string globals and `PATH_SEPARATOR` (`/` everywhere,
  because `LINUX_BUILD` is always defined).
- `GameOS/src/platform_str.cpp`: `S_strlwr`, `S_stricmp`, `S_strnicmp`.
- `data_tools/`: `makefst`, `pak`, `aseconv`, `makersp` (built with the game).

## Game data

`alariq/mc2srcdata` (git-lfs) is a **sibling checkout, not a submodule**. Its
`build_scripts/` (GNU make) uses the data tools to produce `*.fst`, `data/`,
`assets/`. The result is platform independent: build it on a PC, copy it to the
Pi. Changes to data are commits in that repo, not this one.

## Gotchas

- **Case and slashes.** Data files reference paths in mixed case with `\`. Lookups
  must lowercase and convert separators exactly once, in `File::open`. A second
  conversion elsewhere, or a lookup that skips `File::open`, misses FST entries.
- **`floatToText` format** was `"%f4"` (prints a stray `4`); fixed to `"%.4f"` in
  `csvfile.cpp` and `inifile.cpp`. Watch for the same pattern elsewhere.
- **Short packet reads.** Save files and packets can be truncated; callers in
  `code/` were hardened to tolerate short reads. New readers must check sizes.
- **Endianness and packing.** On-disk structs assume little-endian and MSVC
  packing. ARM64 Linux is little-endian too, but check `#pragma pack` and
  `long` (64-bit on Linux, 32-bit on Windows) in any struct read from disk.
- `STOP` on a parse error logs and continues; return an error explicitly.

## Testing

These files link without GL or SDL once the heap is shimmed (the remaster's
`tests/unit/heap_shim.cpp`). Remaster tests that port almost unchanged:
`test_csvfile`, `test_file_inifile`, `test_fst_roundtrip`, the `paths.cpp` part of
`test_paths_mod`. Fixtures go under the test directory; never read from or write
to a real game install (`.claude/rules/testing-conventions.md`).

## Recipes

**Asset not found:** log the name passed to `File::open` and the name after
lowercasing/conversion; check it against `makefst`'s listing of the archive; check
whether a loose file of the same name shadows it.

**New reader for an on-disk struct:** read field by field or `static_assert` the
struct size; use fixed-width types; test it against a fixture produced by the
real tool.
