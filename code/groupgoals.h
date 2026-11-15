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

// Areas of the global map a group order has asked about. calcMoveGoals asks
// once per area, not once per cell; past its size an area is asked again.
struct GroupGoalAreas {
	enum { SIZE = 256 };
	long area[SIZE];
	bool reachable[SIZE];
	long count = 0;
};

// May a goal cell in cellArea be given out, for an order clicked in clickArea?
// A cell in no area has no route to it at all (calcGlobalPath gives up on it).
// When the clicked cell has an area, the goal's must be the same or one a
// route reaches from it, so the group is not spread past a wall, a cliff or
// a closed gate. canReach(area) asks the global map.
template <typename CanReach>
inline bool groupGoalAreaUsable(long cellArea, long clickArea, GroupGoalAreas& asked,
	CanReach canReach)
{
	if (cellArea < 0)
		return false;
	if ((clickArea < 0) || (cellArea == clickArea))
		return true;
	for (long i = 0; i < asked.count; i++)
		if (asked.area[i] == cellArea)
			return asked.reachable[i];
	bool reachable = canReach(cellArea);
	if (asked.count < GroupGoalAreas::SIZE) {
		asked.area[asked.count] = cellArea;
		asked.reachable[asked.count] = reachable;
		asked.count++;
	}
	return reachable;
}

#endif
