#ifndef STR_BOUNDED_H
#define STR_BOUNDED_H

#include <cstdarg>
#include <cstddef>

// Small string helpers kept GL- and SDL-free so they can be unit tested.

// True when an environment switch is on: set, non-empty and not one of "0",
// "false", "no" or "off" (any case). NULL means unset.
bool envFlagEnabled(const char* value);

// As envFlagEnabled, but unset (NULL) or empty gives unsetDefault, for a
// switch that is on unless turned off.
bool envFlagOr(const char* value, bool unsetDefault);

// Appends src to the NUL-terminated string in dest, a buffer of destSize
// bytes, truncating so the result always fits and stays terminated. Does
// nothing when destSize is 0 or dest holds no terminator within destSize.
void appendBounded(char* dest, size_t destSize, const char* src);

// Formats a log line, "<header>: <message>\n", into out (outSize bytes). A
// long message is cut, but the line always ends in a newline (added when the
// message has none) and a terminator. scripts/smoke.sh fails a run on a line
// starting "STOP: ".
void formatLogLine(char* out, size_t outSize, const char* header, const char* fmt, va_list ap);

// The message part of a line from formatLogLine: line without "<header>:" and
// the spaces after it, or line itself when it does not start with the header.
const char* logLineMessage(const char* line, const char* header);

#endif // STR_BOUNDED_H
