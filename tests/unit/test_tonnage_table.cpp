// tests/unit/test_tonnage_table.cpp
//
// code/tonnagetable.h: a per-ton table is never read past its end, whatever
// a mod's unit weighs.

#include "doctest.h"

#include "tonnagetable.h"

TEST_SUITE("TonnageTable")
{

static TonnageTable<110> countingTable()
{
	TonnageTable<110> table;
	for (int i = 0; i < 110; i++)
		table.values[i] = (float)i;
	return table;
}

TEST_CASE("stock tonnages read their own entry")
{
	const TonnageTable<110> table = countingTable();
	CHECK(table[0] == 0.0f);
	CHECK(table[35] == 35.0f);
	CHECK(table[long(35.9f)] == 35.0f);	// the call sites truncate, long(tonnage)
	CHECK(table[100] == 100.0f);
	CHECK(table[109] == 109.0f);
}

TEST_CASE("heavier units read the last entry")
{
	const TonnageTable<110> table = countingTable();
	CHECK(table[110] == 109.0f);
	CHECK(table[150] == 109.0f);	// MC2X Omega
	CHECK(table[600] == 109.0f);	// MC2X Fortress
	CHECK(table[1000] == 109.0f);	// Omnitech Broadsword
}

TEST_CASE("negative tonnages read the first entry")
{
	const TonnageTable<3> table = {{7.0f, 8.0f, 9.0f}};
	CHECK(table[-1] == 7.0f);
	CHECK(table[-100000] == 7.0f);
}

}
