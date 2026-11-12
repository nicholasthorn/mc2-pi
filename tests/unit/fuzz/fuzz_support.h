// tests/unit/fuzz/fuzz_support.h: what every fuzz target shares.

#ifndef MC2_FUZZ_SUPPORT_H
#define MC2_FUZZ_SUPPORT_H

// Thrown by the stop hook: the engine turned the input down.
struct FuzzRejected
{
};

// Runs fn with STOP and fatal exits turned into FuzzRejected. Returns false
// when the input was rejected. Leak checking is off while fn runs: a rejected
// input leaves whatever it had allocated, and leaks are not what this hunts.
template <class Fn> bool fuzzGuarded(Fn fn);

void fuzzBeginGuard();
void fuzzEndGuard();

// A target calls fuzzNoteLoaded when the reader took the input (the file
// opened and parsed, the script compiled, the image loaded). The replay test
// checks that each good seed is loaded: a seed the reader turns down or
// fails to open tests little.
void fuzzNoteLoaded();
unsigned long fuzzLoads();

template <class Fn> bool fuzzGuarded(Fn fn)
{
	bool accepted = true;
	fuzzBeginGuard();
	try
	{
		fn();
	}
	catch (const FuzzRejected&)
	{
		accepted = false;
	}
	fuzzEndGuard();
	return accepted;
}

// Throws FuzzRejected; for the ABL fatal callback.
[[noreturn]] void fuzzReject();

#endif // MC2_FUZZ_SUPPORT_H
