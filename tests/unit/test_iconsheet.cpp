// tests/unit/test_iconsheet.cpp
//
// Icon sheets as plain pixels: a smaller strip gains the high strip's further
// icons, and a sheet's empty cells are filled from a strip.

#include "doctest.h"

#include "iconsheet.h"

#include <cstdint>
#include <cstring>
#include <vector>

namespace {

// One pixel: its cell in r, its column in the cell in g, its row in b, opaque.
std::vector<uint8_t> makeStrip(int cells, int rows, int cellX, int cellY)
{
	const int width = cells * cellX;
	const int height = rows * cellY;
	std::vector<uint8_t> strip((size_t)width * height * 4);
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			uint8_t* p = &strip[((size_t)y * width + x) * 4];
			p[0] = (uint8_t)(x / cellX + 1);
			p[1] = (uint8_t)(x % cellX);
			p[2] = (uint8_t)(y % cellY);
			p[3] = (uint8_t)(y / cellY + 100);
		}
	}
	return strip;
}

const uint8_t* pixelAt(const std::vector<uint8_t>& image, int width, int x, int y)
{
	return &image[((size_t)y * width + x) * 4];
}

} // namespace

TEST_SUITE("IconSheet") {

TEST_CASE("extendedStripWidth: only when the high strip has more icons and the same rows")
{
	// Omnitech's 800 wide mechs: 101 cells of 32x38 against 127 of 40x48, two rows each
	CHECK(extendedStripWidth(3232, 76, 32, 38, 5080, 96) == 127 * 32);
	// its 640 wide vehicles: 25 of 25x30 against 27, one row
	CHECK(extendedStripWidth(625, 30, 25, 30, 1080, 48) == 27 * 25);
	// the same count: nothing to add
	CHECK(extendedStripWidth(864, 38, 32, 38, 1080, 48) == 0);
	// different rows
	CHECK(extendedStripWidth(3232, 38, 32, 38, 5080, 96) == 0);
	CHECK(extendedStripWidth(100, 30, 0, 30, 5080, 96) == 0);
}

TEST_CASE("extendStrip: old cells unchanged, further cells scaled from the same row")
{
	const int cellX = 20;
	const int cellY = 24;
	const std::vector<uint8_t> strip = makeStrip(2, 2, cellX, cellY);
	const std::vector<uint8_t> high = makeStrip(4, 2, HIGH_MECH_ICON_X, HIGH_MECH_ICON_Y);
	const int highWidth = 4 * HIGH_MECH_ICON_X;
	const int outWidth = extendedStripWidth(2 * cellX, 2 * cellY, cellX, cellY, highWidth, 2 * HIGH_MECH_ICON_Y);
	REQUIRE(outWidth == 4 * cellX);

	std::vector<uint8_t> out((size_t)outWidth * 2 * cellY * 4);
	extendStrip(strip.data(), 2 * cellX, 2 * cellY, cellX, cellY, high.data(), highWidth, out.data(), outWidth);

	// the old cells are the strip's
	CHECK(std::memcmp(pixelAt(out, outWidth, 25, 30), pixelAt(strip, 2 * cellX, 25, 30), 4) == 0);

	// cell 2, top row: half way along and down the small cell is half way in the high one
	const uint8_t* p = pixelAt(out, outWidth, 2 * cellX + 10, 12);
	CHECK(p[0] == 3);
	CHECK(p[1] == 20);
	CHECK(p[2] == 24);
	CHECK(p[3] == 100);

	// the last pixel of the last cell, second row, stays inside that cell of the high strip
	p = pixelAt(out, outWidth, outWidth - 1, 2 * cellY - 1);
	CHECK(p[0] == 4);
	CHECK(p[1] == 38);
	CHECK(p[2] == 46);
	CHECK(p[3] == 101);
}

TEST_CASE("fillEmptyCells: only empty cells, from the source's top row")
{
	const int cellX = 4;
	const int cellY = 2;
	// three cells a row, two rows
	const int sheetWidth = 12;
	const int sheetHeight = 4;
	std::vector<uint32_t> sheet((size_t)sheetWidth * sheetHeight, 0);
	sheet[0] = 0xff123456u;			// cell 0 has art; cell 1 is empty
	sheet[2 * cellX] = 0xff000000u;	// cell 2: alpha alone is no art, so empty

	const std::vector<uint8_t> src = makeStrip(4, 2, cellX, cellY);
	CHECK(fillEmptyCells(sheet.data(), sheetWidth, sheetHeight, src.data(), 4 * cellX, 2 * cellY, cellX, cellY) == 3);

	CHECK(sheet[0] == 0xff123456u);
	uint32_t expected;
	std::memcpy(&expected, pixelAt(src, 4 * cellX, 1 * cellX + 1, 1), 4);
	CHECK(sheet[1 * sheetWidth + cellX + 1] == expected);
	// cell 3 is the second row's first
	std::memcpy(&expected, pixelAt(src, 4 * cellX, 3 * cellX, 0), 4);
	CHECK(sheet[2 * sheetWidth] == expected);
	// the source has four cells: the sheet's last two stay empty
	CHECK(sheet[2 * sheetWidth + 2 * cellX] == 0u);

	// filled already, so nothing the second time
	CHECK(fillEmptyCells(sheet.data(), sheetWidth, sheetHeight, src.data(), 4 * cellX, 2 * cellY, cellX, cellY) == 0);
}

TEST_CASE("fillEmptyCells: a source too short does nothing")
{
	std::vector<uint32_t> sheet(16, 0);
	const std::vector<uint8_t> src = makeStrip(2, 1, 2, 2);
	CHECK(fillEmptyCells(sheet.data(), 4, 4, src.data(), 4, 1, 2, 2) == 0);
	CHECK(fillEmptyCells(sheet.data(), 4, 4, src.data(), 4, 2, 0, 2) == 0);
}

} // TEST_SUITE("IconSheet")
