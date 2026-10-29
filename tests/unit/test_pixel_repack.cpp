// tests/unit/test_pixel_repack.cpp
//
// repackPixels8(): converts 8-bit-per-channel pixel data between channel
// counts. The GLES renderer needs it because ES, unlike desktop GL, refuses
// to upload e.g. RGBA8 data into an RGB8 or R8 texture (updateTexture()).

#include "doctest.h"

#include "utils/pixel_repack.h"

#include <cstdint>
#include <vector>

TEST_SUITE("PixelRepack")
{

TEST_CASE("RGBA -> RGB drops alpha and keeps channel order")
{
	const uint8_t src[] = { 1, 2, 3, 4,   5, 6, 7, 8 };
	std::vector<uint8_t> dst(2 * 3, 0xAA);
	repackPixels8(src, 4, dst.data(), 3, 2);
	CHECK(dst == std::vector<uint8_t>{ 1, 2, 3,   5, 6, 7 });
}

TEST_CASE("RGBA -> R keeps the first channel")
{
	const uint8_t src[] = { 10, 20, 30, 40,   50, 60, 70, 80,   90, 100, 110, 120 };
	std::vector<uint8_t> dst(3, 0);
	repackPixels8(src, 4, dst.data(), 1, 3);
	CHECK(dst == std::vector<uint8_t>{ 10, 50, 90 });
}

TEST_CASE("fewer source channels: missing ones are filled with 0xff (opaque)")
{
	const uint8_t src[] = { 1, 2, 3,   4, 5, 6 };
	std::vector<uint8_t> dst(2 * 4, 0);
	repackPixels8(src, 3, dst.data(), 4, 2);
	CHECK(dst == std::vector<uint8_t>{ 1, 2, 3, 0xff,   4, 5, 6, 0xff });
}

TEST_CASE("same channel count is a plain copy")
{
	const uint8_t src[] = { 9, 8,   7, 6 };
	std::vector<uint8_t> dst(4, 0);
	repackPixels8(src, 2, dst.data(), 2, 2);
	CHECK(dst == std::vector<uint8_t>{ 9, 8, 7, 6 });
}

TEST_CASE("zero pixels writes nothing")
{
	const uint8_t src[] = { 1, 2, 3, 4 };
	uint8_t dst[4] = { 7, 7, 7, 7 };
	repackPixels8(src, 4, dst, 3, 0);
	CHECK(dst[0] == 7);
	CHECK(dst[3] == 7);
}

} // TEST_SUITE("PixelRepack")
