// tests/unit/test_mc2xhud.cpp
//
// wolfman-MC2X's exchange and spy map buttons (code/mc2xhud.h).

#include "doctest.h"

#include "mc2xhud.h"

TEST_SUITE("MC2XHud")
{

TEST_CASE("C-Bills to resource points: 10000 at a time, when there are 10000")
{
	CHECK(mc2xhud::canUse(mc2xhud::kToResourcePoints, 10000, 0));
	CHECK_FALSE(mc2xhud::canUse(mc2xhud::kToResourcePoints, 9999, 500000));
	CHECK(mc2xhud::exchange(mc2xhud::kToResourcePoints, 25000, 7) == 10000);
	CHECK(mc2xhud::exchange(mc2xhud::kToResourcePoints, 9999, 7) == 0);
}

TEST_CASE("resource points to C-Bills: all of them, when there are any")
{
	CHECK(mc2xhud::canUse(mc2xhud::kToCBills, 0, 1));
	CHECK_FALSE(mc2xhud::canUse(mc2xhud::kToCBills, 5000000, 0));
	CHECK(mc2xhud::exchange(mc2xhud::kToCBills, 0, 85000) == -85000);
	CHECK(mc2xhud::exchange(mc2xhud::kToCBills, 0, 0) == 0);
}

TEST_CASE("spy map: a million C-Bills")
{
	CHECK(mc2xhud::canUse(mc2xhud::kSpyMap, 1000000, 0));
	CHECK_FALSE(mc2xhud::canUse(mc2xhud::kSpyMap, 999999, 2000000));
	CHECK(mc2xhud::exchange(mc2xhud::kSpyMap, 3000000, 0) == 0);
}

TEST_CASE("shows: the buttons for the units selected")
{
	mc2xhud::Selection none;
	CHECK_FALSE(mc2xhud::shows(mc2xhud::kHideId, none));
	CHECK_FALSE(mc2xhud::shows(mc2xhud::kExitUnit, none));

	mc2xhud::Selection mechs;
	mechs.units = 3;
	CHECK(mc2xhud::shows(mc2xhud::kHideId, mechs));
	CHECK_FALSE(mc2xhud::shows(mc2xhud::kUnhideId, mechs));
	CHECK_FALSE(mc2xhud::shows(mc2xhud::kExitUnit, mechs));	// one at a time
	CHECK_FALSE(mc2xhud::shows(mc2xhud::kDropshipFire, mechs));
	CHECK_FALSE(mc2xhud::shows(mc2xhud::kPickUp, mechs));

	mechs.hidden = 3;
	CHECK_FALSE(mc2xhud::shows(mc2xhud::kHideId, mechs));
	CHECK(mc2xhud::shows(mc2xhud::kUnhideId, mechs));
	mechs.hidden = 1;	// some: hide the rest
	CHECK(mc2xhud::shows(mc2xhud::kHideId, mechs));

	mc2xhud::Selection dropships;
	dropships.units = 2;
	dropships.dropships = 2;
	CHECK(mc2xhud::shows(mc2xhud::kDropshipFire, dropships));
	dropships.units = 3;	// and a mech
	CHECK_FALSE(mc2xhud::shows(mc2xhud::kDropshipFire, dropships));

	mc2xhud::Selection transport;
	transport.units = 1;
	transport.transports = 1;
	CHECK(mc2xhud::shows(mc2xhud::kPickUp, transport));
	CHECK_FALSE(mc2xhud::shows(mc2xhud::kDropOff, transport));
	transport.carrying = 1;
	CHECK(mc2xhud::shows(mc2xhud::kDropOff, transport));
	CHECK(mc2xhud::shows(mc2xhud::kExitUnit, transport));

	mc2xhud::Selection supply;
	supply.units = 2;
	supply.supply = 2;
	CHECK(mc2xhud::shows(mc2xhud::kRechargeMode, supply));
	CHECK(mc2xhud::shows(mc2xhud::kReloadMode, supply));
	CHECK_FALSE(mc2xhud::shows(mc2xhud::kRechargeIndicator, supply));

	mc2xhud::Selection repair;
	repair.units = 1;
	repair.repair = 1;
	CHECK(mc2xhud::shows(mc2xhud::kRechargeIndicator, repair));
	CHECK_FALSE(mc2xhud::shows(mc2xhud::kRechargeMode, repair));
}

TEST_CASE("isSelectionButton: the unit buttons, not the exchanges or the spy map")
{
	CHECK(mc2xhud::isSelectionButton(mc2xhud::kPickUp));
	CHECK(mc2xhud::isSelectionButton(mc2xhud::kRechargeIndicator));
	CHECK_FALSE(mc2xhud::isSelectionButton(mc2xhud::kSpyMap));
	CHECK_FALSE(mc2xhud::isSelectionButton(mc2xhud::kToCBills));
	CHECK_FALSE(mc2xhud::isSelectionButton(49));	// the second support tab
}

TEST_CASE("fits: the transport's tons and item limit")
{
	CHECK(mc2xhud::fits(140.f, 0, 0.f, 0, 100.f));
	CHECK_FALSE(mc2xhud::fits(140.f, 0, 50.f, 1, 100.f));	// over the tons
	CHECK(mc2xhud::fits(140.f, 0, 40.f, 1, 100.f));
	CHECK_FALSE(mc2xhud::fits(4.f, 1, 0.f, 1, 1.f));	// the carrier's one item
	CHECK(mc2xhud::fits(4.f, 1, 0.f, 0, 4.f));
	CHECK_FALSE(mc2xhud::fits(0.f, 0, 0.f, 0, 1.f));	// not a transport
}

TEST_CASE("topUp: the supply carrier's points, up to the most")
{
	CHECK(mc2xhud::topUp(0.f, 1000.f, 1.f) == doctest::Approx(mc2xhud::kSupplyPointsPerSecond));
	CHECK(mc2xhud::topUp(995.f, 1000.f, 1.f) == doctest::Approx(1000.f));
	CHECK(mc2xhud::topUp(1000.f, 1000.f, 1.f) == doctest::Approx(1000.f));
	CHECK(mc2xhud::topUp(10.f, 1000.f, 0.f) == doctest::Approx(10.f));
}

TEST_CASE("other buttons do nothing")
{
	CHECK_FALSE(mc2xhud::canUse(21, 9000000, 9000000));	// MC2X's logo button
	CHECK(mc2xhud::exchange(24, 9000000, 9000000) == 0);
}

}
