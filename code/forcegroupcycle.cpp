#include "forcegroupcycle.h"

namespace forcegroups {

namespace {

// key order: group 1 is first, group 0 last
int position(int group) { return (group + kGroups - 1) % kGroups; }
int groupAt(int pos) { return (pos + 1) % kGroups; }

} // namespace

int cycle(const bool live[kGroups], int current, int direction)
{
    const int step = direction < 0 ? kGroups - 1 : 1;
    int pos;
    if (current < 0 || current >= kGroups)
        pos = direction < 0 ? 0 : kGroups - 1;     // so the first step lands on 0 or 1
    else
        pos = position(current);
    for (int i = 0; i < kGroups; ++i) {
        pos = (pos + step) % kGroups;
        if (live[groupAt(pos)])
            return groupAt(pos);
    }
    return -1;
}

int firstFree(const bool live[kGroups])
{
    for (int pos = 0; pos < kGroups; ++pos) {
        if (!live[groupAt(pos)])
            return groupAt(pos);
    }
    return -1;
}

} // namespace forcegroups
