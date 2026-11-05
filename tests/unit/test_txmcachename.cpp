// tests/unit/test_txmcachename.cpp
//
// mclib/txmcachename.h: a colour map mission and one without must never share
// a transition cache file name, and the colour map name must stay the one the
// shipped caches have.

#include "doctest.h"

#include <cstring>

#include "txmcachename.h"

TEST_SUITE("TxmCacheName")
{

TEST_CASE("colour map: the plain name stock caches have")
{
	char name[64];
	CHECK(txmCacheName(name, sizeof(name), 34669058u, 65535u, true));
	CHECK(std::strcmp(name, "34669058.00065535.txm") == 0);
	CHECK(txmCacheName(name, sizeof(name), 1157759490u, 255u, true));
	CHECK(std::strcmp(name, "1157759490.00000255.txm") == 0);
}

TEST_CASE("no colour map: a different name for the same types and overlay")
{
	char plain[64], old[64];
	CHECK(txmCacheName(plain, sizeof(plain), 235012610u, 65535u, true));
	CHECK(txmCacheName(old, sizeof(old), 235012610u, 65535u, false));
	CHECK(std::strcmp(plain, old) != 0);
	CHECK(std::strcmp(old, "235012610.00065535.old.txm") == 0);
}

TEST_CASE("a buffer too small is reported, not overrun")
{
	char tiny[8];
	CHECK_FALSE(txmCacheName(tiny, sizeof(tiny), 34669058u, 65535u, true));
	CHECK_FALSE(txmCacheName(tiny, 0, 1u, 1u, false));
}

}
