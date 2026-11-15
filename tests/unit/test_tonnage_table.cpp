// tests/unit/test_tonnage_table.cpp
//
// code/tonnagetable.h: a per-ton table is never read past its end, whatever
// a mod's unit weighs.

#include "doctest.h"

#include "tonnagetable.h"

#include <cmath>

TEST_SUITE("TonnageTable")
{

TEST_CASE("stock tonnages read their own entry")
{
	float table[110];
	for (int i = 0; i < 110; i++)
		table[i] = (float)i;
	CHECK(tonnageTableValue(table, 0.0f) == 0.0f);
	CHECK(tonnageTableValue(table, 35.0f) == 35.0f);
	CHECK(tonnageTableValue(table, 35.9f) == 35.0f);	// truncated, as long(tonnage)
	CHECK(tonnageTableValue(table, 100.0f) == 100.0f);
	CHECK(tonnageTableValue(table, 109.0f) == 109.0f);
}

TEST_CASE("heavier units read the last entry")
{
	float table[110];
	for (int i = 0; i < 110; i++)
		table[i] = (float)i;
	CHECK(tonnageTableValue(table, 109.5f) == 109.0f);
	CHECK(tonnageTableValue(table, 110.0f) == 109.0f);
	CHECK(tonnageTableValue(table, 150.0f) == 109.0f);	// MC2X Omega
	CHECK(tonnageTableValue(table, 600.0f) == 109.0f);	// MC2X Fortress
	CHECK(tonnageTableValue(table, 1000.0f) == 109.0f);	// Omnitech Broadsword
	CHECK(tonnageTableValue(table, 1.0e30f) == 109.0f);
}

TEST_CASE("bad tonnages read the first entry")
{
	float table[3] = {7.0f, 8.0f, 9.0f};
	CHECK(tonnageTableValue(table, -1.0f) == 7.0f);
	CHECK(tonnageTableValue(table, std::nanf("")) == 7.0f);
}

}
