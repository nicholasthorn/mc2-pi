// tests/unit/fuzz/fuzz_support.cpp

#include "fuzz_support.h"

#include <cstddef>

#if defined(__has_feature)
#if __has_feature(address_sanitizer)
#define MC2_FUZZ_LSAN 1
#endif
#endif
#if defined(__SANITIZE_ADDRESS__)
#define MC2_FUZZ_LSAN 1
#endif

#if defined(MC2_FUZZ_LSAN) && !defined(_MSC_VER)
#include <sanitizer/lsan_interface.h>
#define MC2_LSAN_DISABLE() __lsan_disable()
#define MC2_LSAN_ENABLE() __lsan_enable()
#else
#define MC2_LSAN_DISABLE() ((void)0)
#define MC2_LSAN_ENABLE() ((void)0)
#endif

// heap_shim.cpp
extern void (*mc2TestStopHook)(const char* message);

namespace
{

unsigned long g_loads = 0;

void throwRejected(const char*)
{
	throw FuzzRejected();
}

} // namespace

void fuzzReject()
{
	throw FuzzRejected();
}

void fuzzBeginGuard()
{
	MC2_LSAN_DISABLE();
	mc2TestStopHook = throwRejected;
}

void fuzzEndGuard()
{
	mc2TestStopHook = nullptr;
	MC2_LSAN_ENABLE();
}

void fuzzNoteLoaded()
{
	g_loads++;
}

unsigned long fuzzLoads()
{
	return g_loads;
}
