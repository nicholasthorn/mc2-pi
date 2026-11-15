// code/tonnagetable.h
//
// Per-ton tables (the mech and vehicle turn rates) hold 0 to 109 tons. Mods
// field heavier units: MC2X's Omega is 150 tons, its dropships and Fortress up
// to 600, Omnitech's Broadsword and Union 1000. Pure logic, so the unit tests
// can reach it.

#ifndef TONNAGETABLE_H
#define TONNAGETABLE_H

#include <cstddef>

// table[tonnage], with a unit heavier than the table as its last entry and a
// negative (or NaN) tonnage as its first.
template <typename T, size_t N>
inline T tonnageTableValue(const T (&table)[N], float tonnage)
{
	if (!(tonnage >= 0.0f))
		return table[0];
	if (tonnage >= (float)(N - 1))
		return table[N - 1];
	return table[(size_t)tonnage];
}

#endif
