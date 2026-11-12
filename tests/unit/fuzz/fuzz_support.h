// tests/unit/fuzz/fuzz_support.h: what every fuzz target shares.

#ifndef MC2_FUZZ_SUPPORT_H
#define MC2_FUZZ_SUPPORT_H

// Thrown by the stop hook: the engine turned the input down.
struct FuzzRejected
{
};

// While one is alive, STOP and fatal exits throw FuzzRejected, PAUSEs are not
// printed, and (with leakChecks false) leak checking is off. Its destructor
// puts all of that back, however the scope is left.
class FuzzGuardScope
{
public:
	explicit FuzzGuardScope(bool leakChecks);
	~FuzzGuardScope();

private:
	bool leakChecks_;
	FuzzGuardScope(const FuzzGuardScope&);
	FuzzGuardScope& operator=(const FuzzGuardScope&);
};

// Set by the replay test: an input the reader takes is then run a second
// time with leak checking on, so a leak on a reader's normal path still
// fails the ASan unit-test job. (A rejected input leaves what it allocated,
// so the first run, which cannot know which it will be, has leak checks off.)
// libFuzzer runs leave it off: they run with -detect_leaks=0.
extern bool fuzzRecheckLeaks;
// True in an ASan build (where the recheck is worth running).
bool fuzzLeakChecksAvailable();

// Runs fn under a FuzzGuardScope. Returns false when the input was rejected.
// Other exceptions (bad_alloc, a failed test assertion) pass through.
template <class Fn> bool fuzzGuarded(Fn fn)
{
	{
		FuzzGuardScope scope(false);
		try
		{
			fn();
		}
		catch (const FuzzRejected&)
		{
			return false;
		}
	}
	if (fuzzRecheckLeaks)
	{
		FuzzGuardScope scope(true);
		try
		{
			fn();
		}
		catch (const FuzzRejected&)
		{
			return false;
		}
	}
	return true;
}

// A target calls fuzzNoteLoaded when the reader took the input (the file
// opened and parsed, the script compiled, the image loaded). The replay test
// checks that each good seed is loaded: a seed the reader turns down or
// fails to open tests little.
void fuzzNoteLoaded();
unsigned long fuzzLoads();

// Throws FuzzRejected; for the ABL fatal callback.
[[noreturn]] void fuzzReject();

#endif // MC2_FUZZ_SUPPORT_H
