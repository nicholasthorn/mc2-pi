# Fuzz tests

Fuzz targets for the readers that take what a mod can change, after Omnitech's
long objective text overran a 2001 buffer on the Quest
(`planning/STRING-OVERFLOW-TRIAGE.md`):

| Target | Reads | Function |
|---|---|---|
| `fit` | FIT files: every block, the usual keys, every reader, small and roomy buffers | `fuzzFitFile` (`fuzz_fit.cpp`) |
| `csv` | CSV files: every cell (up to 64 x 32), every reader | `fuzzCsvFile` (`fuzz_csv.cpp`) |
| `tga` | `Image::loadTGA`, and the terrain readers in `mclib/tgainfo.cpp` at 4x4 and 32x32 | `fuzzTga` (`fuzz_tga.cpp`) |
| `abl` | the ABL compiler (`ABLi_preProcess`) on a brain script, read through `File::readLineEx` | `fuzzAblScript` (`fuzz_abl.cpp`) |

A `STOP`, `Fatal` or ABL syntax error is how the game turns bad data down, so
the targets count it as a rejected input (the test shim's stop hook throws).
Anything else must not crash or touch memory outside its buffers.

## In every unit-test run

`test_fuzz_replay.cpp` runs each target on its seeds in `corpus/<target>/`
(each must load cleanly, except those named `bad-*`) and on 150 fixed mutations of each. The mutations
lean on long runs of one character, long lines, missing quotes and header
fields at their extremes. CI runs it under ASan + UBSan with the other unit
tests. `MC2_FUZZ_REPLAY_MUTANTS=5000` runs more, and
`MC2_FUZZ_REPLAY_LAST=last.bin` writes each input to `last.bin` before
running it, so after a crash it holds the input that crashed (feed it to the
matching `mc2_fuzz_*`, or turn it into a `bad-*` seed).

## With libFuzzer (Clang, Linux)

```bash
cmake -S . -B build-fuzz -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ \
  -DMC2_FUZZ=ON -DMC2_SANITIZE=address,undefined -DMC2_NET=OFF
cmake --build build-fuzz --target mc2_fuzz_fit mc2_fuzz_csv mc2_fuzz_tga mc2_fuzz_abl
```

Run one from `tests/unit/fuzz`, with a scratch corpus so the seeds stay as
they are:

```bash
mkdir -p /tmp/fit && ../../../build-fuzz/out/tests/unit/mc2_fuzz_fit /tmp/fit corpus/fit \
  -dict=fit.dict -max_len=8192 -max_total_time=600 -print_final_stats=1
```

(The executables land in the build's `out/tests/unit/`.) Set
`UBSAN_OPTIONS=halt_on_error=1` so UBSan findings stop the run too.

A crash leaves `crash-<hash>` in the current directory. Reproduce it with
`mc2_fuzz_fit crash-<hash>`. Then fix it, and add the input (or a small
version of it, `-minimize_crash=1`) to `corpus/<target>/` as `bad-<what it broke>`
(a broken file the engine may turn down). The replay test keeps it fixed from then on.
