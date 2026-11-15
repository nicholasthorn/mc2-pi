// code/groupgoals.h
//
// Goal list of a group move order (MoverGroup::calcMoveGoals). Pure logic, so
// the unit tests can reach it.

#ifndef GROUPGOALS_H
#define GROUPGOALS_H

// The search found goals[0..found) for count movers. Every mover past found
// gets fallback (the order's own goal), so no mover is sent to whatever the
// list held before. A negative found counts as none.
template <typename T>
inline void fillMissingGroupGoals(T* goals, long found, long count, const T& fallback)
{
	for (long i = (found > 0) ? found : 0; i < count; i++)
		goals[i] = fallback;
}

#endif
