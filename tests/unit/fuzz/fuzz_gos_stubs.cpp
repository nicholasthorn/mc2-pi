// tests/unit/fuzz/fuzz_gos_stubs.cpp: GameOS functions the engine files
// need that mc2_tests gets from its test files (gos_rand: test_routines.cpp),
// for the libFuzzer executables only, which leave the test files out.

#include <gameos.hpp>

int __stdcall gos_rand()
{
	// fixed, so a crash reproduces from its input alone
	return 12345;
}
