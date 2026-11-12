// tests/unit/fuzz/fuzz_targets.h
//
// Fuzz targets for the readers that take what a mod can change: FIT files,
// CSV files, TGA images and ABL scripts. Each takes any bytes. A STOP, a
// Fatal or an ABL syntax error is how the game turns bad data down, so it is
// a rejected input here; anything else must neither crash nor touch memory
// outside its buffers (run them under ASan to see that).
//
// They are built two ways: as libFuzzer executables (-DMC2_FUZZ=ON, Clang;
// tests/unit/fuzz/README.md) and replayed by mc2_tests over the seed corpus
// in tests/unit/fuzz/corpus plus fixed mutations of it (test_fuzz_replay.cpp).

#ifndef MC2_FUZZ_TARGETS_H
#define MC2_FUZZ_TARGETS_H

#include <cstddef>
#include <cstdint>

// Each returns 0, as libFuzzer wants.
int fuzzFitFile(const uint8_t* data, size_t size);
int fuzzCsvFile(const uint8_t* data, size_t size);
int fuzzTga(const uint8_t* data, size_t size);
int fuzzAblScript(const uint8_t* data, size_t size);

#endif // MC2_FUZZ_TARGETS_H
