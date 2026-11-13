#ifndef FORCEGROUPCYCLE_H
#define FORCEGROUPCYCLE_H

// Choosing a force group (the player's control groups, 1 to 9 and 0) for the
// next / previous group and new group commands. Game-free so it can be unit
// tested; MissionInterfaceManager says which groups have a living unit.
// Groups run in the order of the number keys: 1, 2, ..., 9, 0.

namespace forcegroups {

const int kGroups = 10;

// The group after (direction 1) or before (-1) 'current' in key order that
// has a living unit (live[group]). current -1 (none chosen yet) starts before
// group 1, or after group 0 going back. current itself is chosen again when it
// is the only one; -1 when none has a living unit.
int cycle(const bool live[kGroups], int current, int direction);

// The first group in key order with no living unit, or -1 when all ten have one.
int firstFree(const bool live[kGroups]);

} // namespace forcegroups

#endif // FORCEGROUPCYCLE_H
