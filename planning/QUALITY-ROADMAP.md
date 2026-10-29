# Quality roadmap: tests, static analysis, CI, performance

Collected to-do list for the `pi-port` branch. Ordered so each phase gives
the next one something to stand on: CI first (so everything after it runs
automatically), then the cheap checks, then tests, then performance.

Starting point (2026-10-29): no test framework, no CI, no static analysis.
The only tests are the original Microsoft `mclib/stuff/*_test.cpp`
`TestClass()` self-checks, which nothing calls. `mc2-opengl-remastered`
has a reusable unit-test harness (see Phase 3).

---

## Phase 0 - Prerequisites

- [x] Own GitHub repo for `pi-port`: `nicholasthorn/mc2-pi` (remote `pi`).
- [x] arm64 runners: CI uses `ubuntu-24.04-arm` (free for public repos only).
- [x] Uncommitted edits committed (`57e2e25`).
- [x] Game data in CI: built by the smoke workflow and cached (Phase 5).

## Phase 1 - CI skeleton (build + test on every push)

Goal: every push and PR builds on all targets in under ~10 minutes.

- [x] `.github/workflows/ci.yml` with a build matrix (`e43cef9`):
  - `ubuntu-24.04` x64, desktop GL (`MC2_GLES=OFF`), GCC
  - `ubuntu-24.04` x64, Clang (catches a different set of warnings/UB)
  - `ubuntu-24.04-arm` arm64, `MC2_GLES=ON` (closest to the Pi)
  - `windows-2022` MSVC, using `3rdparty.zip` (cached by LFS object id)
- [x] Shader validation job (`scripts/validate_shaders.py`, GLSL ES 3.10 + 4.20).
- [x] Ninja + `ccache` on the Linux jobs. (Windows MSVC job has no compiler cache yet.)
- [x] `CMAKE_EXPORT_COMPILE_COMMANDS` on (top-level `CMakeLists.txt` and CI).
- [x] Run `ctest --output-on-failure` after the build, on all four configurations.
- [x] Upload the built binary as an artifact (arm64 GLES package, 14 days).
- [ ] Branch protection on `pi-port`: required CI checks, no direct pushes.
- [x] Dependabot for GitHub Actions versions.
- [ ] Local mirror of the fast loop: a `pre-push` hook (or `scripts/check.sh`)
      that runs build + `ctest`, so CI failures are rare surprises.

## Phase 2 - Compiler warnings and static analysis

Goal: catch the bug classes a 2001 x86 codebase hits on 64-bit ARM / GCC:
undefined behaviour, signedness, alignment/packing, 32-bit pointer assumptions.

- [ ] Raise warnings: `-Wall -Wextra` (GCC/Clang), `/W4` (MSVC). Start with
      new/ported code per target; legacy code stays at current level until
      cleaned. Revisit `-fpermissive` - it hides non-conforming code.
- [ ] Warnings-as-errors (`-Werror` / `/WX`) on code that is already clean,
      so it stays clean.
- [ ] `cppcheck` job (already installed locally via WinLibs). Record a
      baseline file of existing findings; fail CI only on **new** findings.
- [ ] `clang-tidy` job, narrow ruleset to start: `bugprone-*`,
      `clang-analyzer-*`, `cppcoreguidelines-init-variables`,
      `bugprone-narrowing-conversions`. Run on changed files only in PRs
      (`clang-tidy-diff`), full run nightly.
- [ ] Optional: MSVC `/analyze` as an occasional separate build, not per push.
- [ ] Add `.clang-format`, but only enforce on changed lines
      (`git clang-format`). Do not reformat legacy files wholesale: it
      wrecks `git blame` and makes merges from `alariq/mc2` painful.
- [ ] Delete or fix the stale `export_clang_paths.sh` (hard-coded
      `/home/a/Downloads/llvm-build`).

## Phase 3 - Unit tests (TDD where it fits)

Goal: TDD for pure logic, parsers, file formats and anything the port
touches. Not for GL output (that is Phase 5's job) and not as a retrofit
across legacy gameplay code.

Port from `mc2-opengl-remastered/tests/unit/`:

- [x] `doctest.h`, `test_main.cpp`, `CMakeLists.txt`, registered with CTest
      in the **main** build behind `MC2_BUILD_TESTS` (default ON).
- [x] `heap_shim.cpp` (remaster crash-bundle stubs dropped; FST registry
      globals added, since each executable defines them in this repo).
- [x] `test_csvfile`.
- [x] `test_file_inifile` (its `FitIniFile::open(buffer,len)` cases dropped:
      that overload only exists in the remaster).
- [x] `test_fst_roundtrip`.
- [x] `test_paths_mod` -> `test_paths.cpp`: path globals kept, mod loading
      dropped.
- [-] `test_argb_pack`, `test_projection`: not ported. They test the
      remaster's own SSBO packing / projection code, not code in this repo.
- [-] `det_rng`, `fst_hash` tests: not ported (remaster-only code).

Platform fixes made when porting: `ZLIB::ZLIB` instead of the hard-coded x64
`zlib.lib`; the `windows` (GameOS/src) library for the Win32 emulation;
fixtures copied into the build tree and opened by relative path, because
`File::open` lowercases the whole path (an absolute `/mnt/c/Users/...` fails on
Linux).

New tests for the port's own changes:

- [x] Windows-to-Linux path conversion in `File::open` (`test_paths.cpp`).
- [x] GLES 8-bit texture repacking: extracted to GL-free
      `GameOS/gameos/utils/pixel_repack.cpp`, test written first.
- [x] `RollDice()` / `RandomNumber()` over the whole 15-bit range
      (`test_routines.cpp`); a mutation back to `rand()` fails 4 cases.
- [ ] Endianness/struct packing for on-disk formats read on ARM.
- [ ] Wire the legacy `mclib/stuff` `TestClass()` routines into one doctest
      case each - cheap coverage for the math library.

Working practice:

- [ ] Bug fixes in testable areas start with a failing test.
- [ ] New logic-heavy code goes in a GL-free file so it can be tested.
- [ ] Before refactoring legacy code, add characterization tests that pin
      current behaviour, then refactor.

## Phase 4 - Sanitizers

- [ ] CI job: ASan + UBSan (`-fsanitize=address,undefined
      -fno-omit-frame-pointer`) building and running the unit tests on Linux.
      UBSan is especially relevant on ARM (alignment, signed overflow).
- [ ] Nightly: sanitizer build running the smoke missions (Phase 5).

## Phase 5 - Smoke tests (does the game actually run?)

The game already supports `-mission <name>`. Mesa's software renderer
(`LIBGL_ALWAYS_SOFTWARE=1`, llvmpipe) supports GL 4.5 and ES 3.2, so missions
can run on GitHub's GPU-less runners under `Xvfb`.

- [x] Auto-exit: `-autoexit <frames>` (parsed in `gameosmain.cpp`) quits
      cleanly after N **gameplay** frames, counted from
      `gos_MarkGameplayStarted()` (called on a mission's first frame).
      Counting from process start spent nearly all frames on loading.
- [x] `scripts/smoke.sh`: missions under `xvfb-run` + llvmpipe; fails on
      non-zero exit, hang, no `[MISSION]` marker, GL errors, shader errors,
      `STOP`/`FATAL` lines. Both renderers pass locally on five missions.
- [x] Nightly CI job (`.github/workflows/smoke.yml`) on x64 (`MC2_GLES=OFF`)
      and arm64 (`MC2_GLES=ON`), with cached game data.
- [x] Game data in CI: `scripts/build_gamedata.sh --no-movies` from a
      `GIT_LFS_SKIP_SMUDGE=1` shallow clone (only `movies/` is in LFS, so no
      LFS bandwidth is used), cached on the mc2srcdata commit + tool sources.
- [ ] Deterministic runs: seed the RNG in smoke mode so a failure reproduces.
- [ ] Optional later: screenshot at a fixed frame + perceptual diff against
      a baseline. Only after confirming two identical runs produce the same
      image (remastered found frame-to-frame variance blocked this).

## Phase 6 - Performance testing

Two kinds, kept separate because they need different environments.

**Microbenchmarks (CPU, runs in hosted CI):**

- [ ] Add `nanobench` (single header, fits the doctest style) or Google
      Benchmark. Targets: LZ decompress, FST lookup, ini parse, matrix/vector
      math, texture repacking, any hot path you optimise.
- [ ] Hosted runners are noisy (often 10-20% run to run). For regression
      gating, either compare against the base branch in the **same job**, or
      measure instruction counts with `valgrind --tool=cachegrind`, which is
      stable on shared machines.
- [ ] Track results over time with `benchmark-action/github-action-benchmark`
      (stores history on a `gh-pages` branch, comments on regressions).

**Frame-time and load-time benchmarks (needs real hardware):**

- [ ] Env-gated frame-time logger: write per-frame milliseconds to CSV;
      report average, p50, p95, p99 and 1% lows. `gameosmain.cpp` already
      computes `dt_sec` per frame - log it there.
- [ ] A fixed benchmark scenario: one mission, fixed camera path, fixed RNG
      seed, AI behaviour pinned (`-braindead`, `-turrets_off` exist) so runs
      are comparable.
- [ ] Load time: mission start to first rendered frame, measured with
      `hyperfine` over several runs.
- [ ] Profiler: integrate Tracy (remastered uses it behind `TRACY_ENABLE`).
      Coarse zones only (per system per frame), not per-object or per-vertex.
- [ ] Memory: log heap high-water mark per mission; occasional
      `valgrind --tool=massif` run.
- [ ] Do **not** gate on llvmpipe frame times - software rendering numbers
      say nothing about the Pi's GPU.

## Phase 7 - Raspberry Pi in the loop

- [ ] Self-hosted GitHub runner on a Pi 4/400 (labels: `self-hosted`, `pi4`).
- [ ] Nightly (and on demand) job: build, run unit tests, run real-GPU GLES
      smoke, run the frame-time benchmark, upload the CSV.
- [ ] Record `vcgencmd measure_temp` and `vcgencmd get_throttled` before and
      after each benchmark; discard runs that throttled. Use a heatsink/fan.
- [ ] `perf record` / `perf stat` on the Pi for CPU hot spots.
- [ ] Security: self-hosted runners must **never** run workflows from fork
      PRs. Restrict the Pi job to pushes on your own branches.

---

## Suggested order

1. Phase 0 + Phase 1 (CI building on all targets).
2. Phase 3 harness port + the first few tests, running in CI.
3. Phase 2 warnings + cppcheck baseline; Phase 4 sanitizer job.
4. Phase 5 smoke on llvmpipe (needs the auto-exit flag and cached data).
5. Phase 6 frame-time logger + microbenchmarks.
6. Phase 7 Pi runner.
