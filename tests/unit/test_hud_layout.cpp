// tests/unit/test_hud_layout.cpp
//
// mclib/hud_layout.h: which layout the mission HUD uses at a screen width and
// where it goes (ControlGui::swapResolutions).

#include "doctest.h"

#include "hud_layout.h"

#include <initializer_list>

TEST_SUITE("HudLayout")
{

TEST_CASE("hudLayoutWidth: a width with its own layout uses it")
{
	for (int width : { 640, 800, 1024, 1280, 1600, 1920 })
		CHECK(hudLayoutWidth(width) == width);
}

TEST_CASE("hudLayoutWidth: other widths take the largest that fits")
{
	CHECK(hudLayoutWidth(1366) == 1280);
	CHECK(hudLayoutWidth(1440) == 1280);
	CHECK(hudLayoutWidth(2560) == 1920);
	CHECK(hudLayoutWidth(3840) == 1920);
	CHECK(hudLayoutWidth(900) == 800);
	CHECK(hudLayoutWidth(320) == 640);
}

TEST_CASE("hudLayoutWidth: stepping down ends at the smallest")
{
	CHECK(hudLayoutWidth(1920 - 1) == 1600);
	CHECK(hudLayoutWidth(800 - 1) == 640);
	CHECK(hudLayoutWidth(640 - 1) == 640);
}

TEST_CASE("hudOffsetX: unchanged where the stock layouts were right")
{
	CHECK(hudOffsetX(1024, 1024, 0) == 0);
	CHECK(hudOffsetX(1280, 1280, 128) == 128);
	CHECK(hudOffsetX(1600, 1600, 288) == 288);
}

TEST_CASE("hudOffsetX: centres where they were not")
{
	CHECK(hudOffsetX(1920, 1920, 288) == 448);
	CHECK(hudOffsetX(2560, 1920, 288) == 768);
	CHECK(hudOffsetX(1366, 1280, 128) == 171);
	// Omnitech at 1920 falls back to its 1600 layout
	CHECK(hudOffsetX(1920, 1600, 288) == 448);
}

TEST_CASE("hudOffsetX: narrow layouts keep their own")
{
	CHECK(hudOffsetX(640, 640, 0) == 0);
	CHECK(hudOffsetX(800, 800, 0) == 0);
	CHECK(hudOffsetX(900, 800, 0) == 0);
}

TEST_CASE("hudRightEdgeShift: follows the screen, not the HUD")
{
	CHECK(hudRightEdgeShift(1920, 1920) == 0);
	CHECK(hudRightEdgeShift(1280, 1280) == 0);
	CHECK(hudRightEdgeShift(2560, 1920) == 640);
	CHECK(hudRightEdgeShift(1920, 1600) == 320);
	CHECK(hudRightEdgeShift(1366, 1280) == 86);
}

TEST_CASE("hudBaseHeight: 1024 and up share one")
{
	CHECK(hudBaseHeight(640) == 480);
	CHECK(hudBaseHeight(800) == 600);
	CHECK(hudBaseHeight(1024) == 768);
	CHECK(hudBaseHeight(1920) == 768);
}

}
