// EString::Compare (mclib/estring.cpp): the case-sensitive compare read as
// many bytes of this string as the other one has, past the end of a shorter
// one (ASan, the Mechlopedia's pilot list sorting names). Under the
// sanitized unit test run, a regression shows up as a heap overflow.
#include "doctest.h"

#include "estring.h"

TEST_SUITE("EString") {

TEST_CASE("Compare orders a prefix before the longer string, both ways") {
	const EString shortName = "ACE";
	const EString longName = "ACE SPECIALIST";
	CHECK(shortName.Compare(longName.Data(), true) < 0);
	CHECK(longName.Compare(shortName.Data(), true) > 0);
	CHECK(shortName.Compare("ACE", true) == 0);
}

TEST_CASE("Compare of different strings of different lengths") {
	const EString a = "BRAWLER";
	CHECK(a.Compare("ZEALOT SHARPSHOOTER", true) < 0);
	CHECK(a.Compare("ACE", true) > 0);
	CHECK(EString("").Compare("A", true) < 0);
	CHECK(a.Compare("", true) > 0);
}

}
