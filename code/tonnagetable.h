// code/tonnagetable.h
//
// Per-ton tables (the mech and vehicle turn rates) hold 0 to 109 tons. Mods
// field heavier units: MC2X's Omega is 150 tons, its dropships and Fortress up
// to 600, Omnitech's Broadsword and Union 1000. Pure logic, so the unit tests
// can reach it.

#ifndef TONNAGETABLE_H
#define TONNAGETABLE_H

#include <cstddef>

// A per-ton table that is never read past its ends: table[tons] is that
// entry, a unit heavier than the table reads the last one and a negative
// tonnage the first. The legacy call sites index it as the plain array it
// replaces, table[long(tonnage)], and stay as upstream wrote them.
template <size_t N>
struct TonnageTable
{
	float values[N];

	float operator[](long tons) const
	{
		if (tons < 0)
			return values[0];
		if ((size_t)tons >= N)
			return values[N - 1];
		return values[tons];
	}
};

#endif
