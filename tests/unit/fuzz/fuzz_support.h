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
void fuzzEndGuard(bool accepted);

// How many inputs have been rejected so far: the replay test checks that its
// seeds, unmutated, are accepted (a seed the engine turns down tests little).
unsigned long fuzzRejections();

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
	fuzzEndGuard(accepted);
	return accepted;
}

// Throws FuzzRejected; for the ABL fatal callback.
[[noreturn]] void fuzzReject();

#endif // MC2_FUZZ_SUPPORT_H
