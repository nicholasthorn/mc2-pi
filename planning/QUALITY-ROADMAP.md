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
- [ ] Decide where game data comes from in CI. Running missions needs
      `mc2srcdata` (git-lfs) built with `makefst`/`makersp`/`aseconv`.
      GitHub's free LFS bandwidth is small, so build the data once and cache
      it as a CI cache entry keyed on the `mc2srcdata` commit.

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
- [ ] Run `ctest --output-on-failure` after the build (empty at first; Phase 3 fills it).
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

- [ ] `doctest.h`, `test_main.cpp`, `CMakeLists.txt` - but register with
      CTest in the **main** build behind an `MC2_BUILD_TESTS` option
      (remastered's standalone project is why its tests only ran by hand).
- [ ] `heap_shim.cpp` - malloc-backed heap + GameOS-global shim so real
      `mclib` files link without the game. `mclib/heap.h` is identical in
      both repos.
- [ ] `test_csvfile` (`csvfile.cpp` identical in both repos).
- [ ] `test_file_inifile` (`inifile.cpp` 20 lines differ, `packet.cpp` 7).
- [ ] `test_fst_roundtrip` (`ffile.cpp`, `fastfile.cpp`, `lzdecomp.cpp`
      differ slightly; `lzcomp.cpp` identical).
- [ ] `test_paths_mod` - keep the `paths.cpp` part, drop the mod-loading
      part (remastered-only feature).
- [ ] `test_argb_pack`, `test_projection` - self-contained.
- [ ] Optional: `det_rng.h` + `test_det_rng`, `fst_hash` + `test_hashing`.

Platform fixes when porting: replace the hard-coded x64 `zlib.lib` with
`find_package(ZLIB)`; replace `_putenv_s` with `setenv` on Linux; feed
`MC2_TEST_FIXTURE_DIR` via CMake as remastered does.

New tests for the port's own changes:

- [ ] Windows-to-Linux path conversion in `File::open`.
- [ ] GLES 8-bit texture repacking (`ce99084`) - pure function, test-first.
- [ ] `RollDice()` range/distribution (the 0..32767 fix).
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

- [ ] Add an auto-exit: env var or flag to quit after N frames / N seconds
      with exit code 0, nonzero on fatal error. (Remastered's smoke mode is a
      reference, but write a small one - its runner is tied to its own tooling.)
- [ ] `scripts/smoke.sh`: launch a short list of missions under
      `xvfb-run` + llvmpipe; fail on crash, nonzero exit, GL errors or
      `STOP`/assert lines in the log.
- [ ] Nightly CI job on x64 (`MC2_GLES=OFF`) and arm64 (`MC2_GLES=ON`),
      using the cached built data from Phase 0.
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
