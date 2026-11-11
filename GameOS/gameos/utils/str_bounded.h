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

// Copies src into dest, a buffer of destSize bytes, cutting it so it always
// fits and stays terminated: strcpy for text from game data, mods, scripts
// or the network, whose length the 2001 buffers did not plan for. A NULL src
// copies as "". Does nothing when destSize is 0.
void copyBounded(char* dest, size_t destSize, const char* src);

// The same for a char array, sized by the compiler: a pointer does not
// compile, so a swapped-in call cannot take sizeof of a pointer by mistake.
template <size_t N> inline void copyBounded(char (&dest)[N], const char* src)
{
    copyBounded(dest, N, src);
}

template <size_t N> inline void appendBounded(char (&dest)[N], const char* src)
{
    appendBounded(dest, N, src);
}

// The file name part of path, as _splitpath's fname: no directory (either
// slash) and no extension (from the last dot of the name). Cut to fit name,
// a buffer of nameSize bytes; _splitpath itself writes as much as it finds.
void splitPathName(const char* path, char* name, size_t nameSize);

// Formats a log line, "<header>: <message>\n", into out (outSize bytes). A
// long message is cut, but the line always ends in a newline (added when the
// message has none) and a terminator. scripts/smoke.sh fails a run on a line
// starting "STOP: ".
void formatLogLine(char* out, size_t outSize, const char* header, const char* fmt, va_list ap);

// The message part of a line from formatLogLine: line without "<header>:" and
// the spaces after it, or line itself when it does not start with the header.
const char* logLineMessage(const char* line, const char* header);

#endif // STR_BOUNDED_H
