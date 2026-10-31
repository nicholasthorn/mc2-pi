// tests/unit/test_image_tga.cpp
//
// Image::loadTGA for colour-mapped (paletted, type 1) TGAs, which the engine
// could not load: MechCommander Omnitech ships its sky 12 textures that way
// ("Failed to create texture data/tgl/512/sky12_back_left.tga").

#include "doctest.h"

#include "utils/Image.h"

#include <cstdint>
#include <vector>

namespace
{

// A type 1 TGA: 18-byte header, palette (entryBytes per entry, B G R [A]),
// then one index per pixel, rows bottom-up (attrib 0).
std::vector<uint8_t> makePaletted(int w, int h, const std::vector<uint8_t>& palette, int entries,
								  int entryBits, const std::vector<uint8_t>& indices)
{
	std::vector<uint8_t> f(18, 0);
	f[1] = 1;							// colour map present
	f[2] = 1;							// uncompressed colour-mapped
	f[5] = (uint8_t)entries;			// cmapEntries (low byte; < 256 here)
	f[7] = (uint8_t)entryBits;
	f[12] = (uint8_t)w;
	f[14] = (uint8_t)h;
	f[16] = 8;							// 8-bit indices
	f.insert(f.end(), palette.begin(), palette.end());
	f.insert(f.end(), indices.begin(), indices.end());
	return f;
}

} // namespace

TEST_SUITE("ImageTga")
{

TEST_CASE("paletted TGA expands through its 24-bit palette, bottom-up rows")
{
	// entry 0 red, entry 1 blue (stored B G R)
	const std::vector<uint8_t> palette = { 0, 0, 255,   255, 0, 0 };
	// file rows bottom-up: bottom row [0 1], top row [1 0]
	const std::vector<uint8_t> tga = makePaletted(2, 2, palette, 2, 24, { 0, 1,  1, 0 });

	Image img;
	REQUIRE(img.loadTGA(tga.data(), tga.size()));
	CHECK(img.getWidth() == 2);
	CHECK(img.getHeight() == 2);
	CHECK(img.getFormat() == FORMAT_RGB8);
	const uint8_t* p = img.getPixels();
	REQUIRE((p != nullptr));
	// output is top row first, RGB: top row is [1 0] = blue, red
	CHECK(p[0] == 0);   CHECK(p[1] == 0); CHECK(p[2] == 255);
	CHECK(p[3] == 255); CHECK(p[4] == 0); CHECK(p[5] == 0);
	// bottom row [0 1] = red, blue
	CHECK(p[6] == 255); CHECK(p[8] == 0);
	CHECK(p[9] == 0);   CHECK(p[11] == 255);
}

TEST_CASE("paletted TGA: 32-bit palette keeps alpha; bad index takes entry 0")
{
	const std::vector<uint8_t> palette = { 10, 20, 30, 40,   50, 60, 70, 80 };
	const std::vector<uint8_t> tga = makePaletted(2, 1, palette, 2, 32, { 1, 200 });

	Image img;
	REQUIRE(img.loadTGA(tga.data(), tga.size()));
	CHECK(img.getFormat() == FORMAT_RGBA8);
	const uint8_t* p = img.getPixels();
	REQUIRE((p != nullptr));
	CHECK(p[0] == 70); CHECK(p[1] == 60); CHECK(p[2] == 50); CHECK(p[3] == 80);
	CHECK(p[4] == 30); CHECK(p[5] == 20); CHECK(p[6] == 10); CHECK(p[7] == 40);
}

TEST_CASE("paletted TGA with missing pixel data is refused, not overread")
{
	const std::vector<uint8_t> palette = { 0, 0, 255,   255, 0, 0 };
	std::vector<uint8_t> tga = makePaletted(2, 2, palette, 2, 24, { 0, 1, 1, 0 });
	tga.resize(tga.size() - 2);		// two indices short

	Image img;
	CHECK_FALSE(img.loadTGA(tga.data(), tga.size()));
}

} // TEST_SUITE("ImageTga")
