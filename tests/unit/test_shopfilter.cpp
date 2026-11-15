// tests/unit/test_shopfilter.cpp
//
// wolfman-MC2X's shop buttons over its inventory's unit groups (shopfilter.h).

#include "doctest.h"

#include "shopfilter.h"

TEST_SUITE("ShopFilter")
{

TEST_CASE("groupForButton: MC2X's eight unit types, All and Bingo")
{
	CHECK(shopfilter::groupForButton(61) == 2);	// mechs
	CHECK(shopfilter::groupForButton(62) == 1);	// airplanes
	CHECK(shopfilter::groupForButton(63) == 6);	// hovercraft
	CHECK(shopfilter::groupForButton(64) == 7);	// ground vehicles
	CHECK(shopfilter::groupForButton(65) == 8);	// infantry
	CHECK(shopfilter::groupForButton(66) == 3);	// copters
	CHECK(shopfilter::groupForButton(67) == 5);	// aerodyne dropships
	CHECK(shopfilter::groupForButton(68) == 4);	// spheroid dropships
	CHECK(shopfilter::groupForButton(69) == shopfilter::kAllGroups);
	CHECK(shopfilter::groupForButton(70) == shopfilter::kAllGroups);
}

TEST_CASE("groupForButton: the shop's own buttons are not filters")
{
	// stock's buy, sell, cancel, accept and main menu (50 to 57), and nothing
	const unsigned long ids[] = { 0ul, 50ul, 51ul, 52ul, 53ul, 57ul, 60ul, 71ul };
	for (unsigned long id : ids)
		CHECK(shopfilter::groupForButton(id) == shopfilter::kNotAFilter);
}

TEST_CASE("shows: one group, or all")
{
	CHECK(shopfilter::shows(shopfilter::kAllGroups, 2));
	CHECK(shopfilter::shows(shopfilter::kAllGroups, 0));	// no group in the data
	CHECK(shopfilter::shows(7, 7));
	CHECK_FALSE(shopfilter::shows(7, 2));
	CHECK_FALSE(shopfilter::shows(2, 0));
}

}
