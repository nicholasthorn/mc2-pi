#ifndef MSGLINES_H
#define MSGLINES_H

// Splitting a mission script's text message (MechCommander Omnitech's
// settextmsg and setlargemsg) into the lines the mission's message area
// holds. Game-free so it can be unit tested.

#include <cstddef>
#include <string>
#include <vector>

namespace msglines {

// '*' starts a new line. A line longer than maxLen characters is cut at its
// last space within maxLen (the spaces there dropped), or at maxLen when it
// has none. Empty lines are left out. Null text or maxLen 0 gives no lines.
std::vector<std::string> split(const char* text, size_t maxLen);

} // namespace msglines

#endif // MSGLINES_H
