// tests/unit/test_alpha_mips.cpp
//
// buildAlphaMips(): mip levels for RGBA textures with transparent texels, so
// the colour of those texels does not bleed in (distant chain-link fences in VR).

#include "doctest.h"

#include "utils/alpha_mips.h"

#include <cstdint>
#include <vector>

TEST_SUITE("AlphaMips")
{

TEST_CASE("hasTransparency finds any alpha below 255")
{
	const uint8_t opaque[] = { 1, 2, 3, 255,   4, 5, 6, 255 };
	const uint8_t last[] = { 1, 2, 3, 255,   4, 5, 6, 254 };
	CHECK_FALSE(hasTransparency(opaque, 2));
	CHECK(hasTransparency(last, 2));
	CHECK_FALSE(hasTransparency(opaque, 0));
}

TEST_CASE("transparent texels' colour does not bleed in")
{
	const uint8_t base[] = {
		200, 180, 160, 255,    0, 0, 0, 0,
		0, 0, 0, 0,            200, 180, 160, 255,
	};
	std::vector<uint8_t> mips;
	buildAlphaMips(base, 2, 2, mips);
	CHECK(mips == std::vector<uint8_t>{ 200, 180, 160, 128 });
}

TEST_CASE("colour is weighted by alpha")
{
	const uint8_t base[] = { 100, 0, 0, 255,   200, 0, 0, 85 };
	std::vector<uint8_t> mips;
	buildAlphaMips(base, 2, 1, mips);
	CHECK(mips == std::vector<uint8_t>{ 125, 0, 0, 170 });
}

TEST_CASE("an opaque image gets the plain average")
{
	const uint8_t base[] = {
		10, 20, 30, 255,   30, 40, 50, 255,
		50, 60, 70, 255,   70, 80, 90, 255,
	};
	std::vector<uint8_t> mips;
	buildAlphaMips(base, 2, 2, mips);
	CHECK(mips == std::vector<uint8_t>{ 40, 50, 60, 255 });
}

TEST_CASE("an empty texel takes its colour from the next smaller level")
{
	// 4x1: level 1 is a covered and an empty texel, level 2 averages them
	const uint8_t base[] = {
		200, 100, 50, 255,   200, 100, 50, 255,
		9, 9, 9, 0,          9, 9, 9, 0,
	};
	std::vector<uint8_t> mips;
	buildAlphaMips(base, 4, 1, mips);
	CHECK(mips == std::vector<uint8_t>{
		200, 100, 50, 255,   200, 100, 50, 0,
		200, 100, 50, 128,
	});
}

TEST_CASE("a fully transparent image stays transparent, with its plain average")
{
	const uint8_t base[] = { 10, 10, 10, 0,   30, 30, 30, 0 };
	std::vector<uint8_t> mips;
	buildAlphaMips(base, 2, 1, mips);
	CHECK(mips == std::vector<uint8_t>{ 20, 20, 20, 0 });
}

TEST_CASE("sizes follow GL down to 1x1, odd sizes included")
{
	std::vector<uint8_t> base(5 * 3 * 4, 0x80);
	std::vector<uint8_t> mips;
	buildAlphaMips(base.data(), 5, 3, mips);
	// 2x1, then 1x1
	REQUIRE(mips.size() == (2 + 1) * 4);
	CHECK(mips == std::vector<uint8_t>(12, 0x80));
}

TEST_CASE("an odd size leaves no texel out")
{
	const uint8_t base[] = { 0, 0, 0, 0,   0, 0, 0, 0,   30, 60, 90, 255 };
	std::vector<uint8_t> mips;
	buildAlphaMips(base, 3, 1, mips);
	CHECK(mips == std::vector<uint8_t>{ 30, 60, 90, 85 });
}

TEST_CASE("a 1x1 image has no smaller levels")
{
	const uint8_t base[] = { 1, 2, 3, 4 };
	std::vector<uint8_t> mips(4, 7);
	buildAlphaMips(base, 1, 1, mips);
	CHECK(mips.empty());
}

TEST_CASE("mean alpha stays the same at every level")
{
	// one opaque column in four, the shape of a thin wire
	const int w = 16, h = 16;
	std::vector<uint8_t> base(w * h * 4);
	for(int y = 0; y < h; ++y)
		for(int x = 0; x < w; ++x)
			base[(y * w + x) * 4 + 3] = x % 4 == 0 ? 255 : 0;
	std::vector<uint8_t> mips;
	buildAlphaMips(base.data(), w, h, mips);
	size_t at = 0;
	for(int lw = w / 2, lh = h / 2; lw >= 1; lw /= 2, lh /= 2) {
		uint32_t sum = 0;
		for(int i = 0; i < lw * lh; ++i)
			sum += mips[at + i * 4 + 3];
		CHECK(sum / (double)(lw * lh) == doctest::Approx(255.0 / 4).epsilon(0.02));
		at += (size_t)lw * lh * 4;
	}
}

}
