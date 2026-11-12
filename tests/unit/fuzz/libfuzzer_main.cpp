// tests/unit/fuzz/libfuzzer_main.cpp: the libFuzzer entry point, built once
// per target with MC2_FUZZ_TARGET naming its function (tests/unit/CMakeLists.txt).

#include "fuzz_targets.h"

#ifndef MC2_FUZZ_TARGET
#error "MC2_FUZZ_TARGET names the fuzz target, e.g. fuzzFitFile"
#endif

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
	return MC2_FUZZ_TARGET(data, size);
}
