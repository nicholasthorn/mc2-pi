// tests/unit/test_tga_writer.cpp
//
// writeTGA24(): "-bench" screenshots (gos_BenchScreenshot) go from
// glReadPixels' RGBA, bottom row first, to an uncompressed TGA. Pins the
// header and the RGBA -> BGR byte order, which image viewers and diff tools
// depend on.

#include "doctest.h"

#include "utils/tga_writer.h"

#include <cstdio>
#include <vector>

static std::vector<uint8_t> readAll(const char* path)
{
	std::vector<uint8_t> data;
	FILE* f = fopen(path, "rb");
	if (!f)
		return data;
	int c;
	while ((c = fgetc(f)) != EOF)
		data.push_back((uint8_t)c);
	fclose(f);
	return data;
}

TEST_SUITE("TgaWriter")
{

TEST_CASE("writes the header and BGR pixels in row order")
{
	// 2x2: bottom row red, green; top row blue, white (alpha ignored)
	const uint8_t rgba[] = {
		255, 0, 0, 7,     0, 255, 0, 7,
		0, 0, 255, 7,     255, 255, 255, 7,
	};
	const char* path = "test_tga_writer_scratch.tga";
	REQUIRE(writeTGA24(path, 2, 2, rgba));

	const std::vector<uint8_t> d = readAll(path);
	remove(path);
	REQUIRE(d.size() == 18 + 2 * 2 * 3);
	CHECK(d[2] == 2);       // uncompressed true colour
	CHECK(d[12] == 2);      // width
	CHECK(d[13] == 0);
	CHECK(d[14] == 2);      // height
	CHECK(d[16] == 24);     // bits per pixel
	CHECK(d[17] == 0);      // bottom-left origin

	const uint8_t expected[] = {
		0, 0, 255,      0, 255, 0,
		255, 0, 0,      255, 255, 255,
	};
	for (size_t i = 0; i < sizeof(expected); ++i)
		CHECK(d[18 + i] == expected[i]);
}

TEST_CASE("refuses an empty image and an unwritable path")
{
	const uint8_t px[4] = {};
	CHECK_FALSE(writeTGA24("test_tga_writer_never.tga", 0, 1, px));
	CHECK_FALSE(writeTGA24("no_such_dir/x.tga", 1, 1, px));
}

}
