#ifndef STR_BOUNDED_H
#define STR_BOUNDED_H

#include <cstddef>

// Small string helpers kept GL- and SDL-free so they can be unit tested.

// True when an environment switch is on: set, non-empty and not one of "0",
// "false", "no" or "off" (any case). NULL means unset.
bool envFlagEnabled(const char* value);

// Appends src to the NUL-terminated string in dest, a buffer of destSize
// bytes, truncating so the result always fits and stays terminated. Does
// nothing when destSize is 0 or dest holds no terminator within destSize.
void appendBounded(char* dest, size_t destSize, const char* src);

#endif // STR_BOUNDED_H
