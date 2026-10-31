// tests/unit/test_jpeg_decode.cpp
//
// decodeJpegBGRA(): the stb_image decode behind DecodeJPG, used for
// MechCommander Omnitech's .burnin.jpg terrain colour maps. The fixture is a
// 16x8 JPEG (4:4:4, best quality): left half pure red, right half pure blue.

#include "doctest.h"

#include "utils/jpeg_decode.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

static std::vector<uint8_t> readFixture(const char* name)
{
	std::string path = std::string(MC2_TEST_FIXTURE_DIR) + "/" + name;
	std::vector<uint8_t> data;
	FILE* f = fopen(path.c_str(), "rb");
	if (!f)
		return data;
	int c;
	while ((c = fgetc(f)) != EOF)
		data.push_back((uint8_t)c);
	fclose(f);
	return data;
}

TEST_SUITE("JpegDecode")
{

TEST_CASE("decodes to BGRA, top row first, opaque")
{
	std::vector<uint8_t> jpg = readFixture("jpeg_red_blue.jpg");
	REQUIRE(!jpg.empty());

	int w = -1, h = -1;
	uint8_t* px = decodeJpegBGRA(jpg.data(), jpg.size(), &w, &h, 16384);
	REQUIRE((px != NULL));
	CHECK(w == 16);
	CHECK(h == 8);

	// pixel (2,3): red -> B low, R high
	const uint8_t* red = px + (3 * 16 + 2) * 4;
	CHECK(red[0] < 16);
	CHECK(red[2] > 240);
	CHECK(red[3] == 0xff);

	// pixel (13,3): blue -> B high, R low
	const uint8_t* blue = px + (3 * 16 + 13) * 4;
	CHECK(blue[0] > 240);
	CHECK(blue[2] < 16);
	CHECK(blue[3] == 0xff);

	free(px);
}

TEST_CASE("rejects an image larger than maxDimension without decoding")
{
	std::vector<uint8_t> jpg = readFixture("jpeg_red_blue.jpg");
	REQUIRE(!jpg.empty());

	int w = -1, h = -1;
	CHECK((decodeJpegBGRA(jpg.data(), jpg.size(), &w, &h, 8) == NULL));
	CHECK(w == 0);
	CHECK(h == 0);
}

TEST_CASE("rejects data that is not a JPEG, and truncated JPEGs")
{
	const uint8_t junk[] = { 'F', 'I', 'T', 'i', 'n', 'i', 0, 1, 2, 3 };
	int w = -1, h = -1;
	CHECK((decodeJpegBGRA(junk, sizeof(junk), &w, &h, 16384) == NULL));
	CHECK((decodeJpegBGRA(NULL, 0, &w, &h, 16384) == NULL));

	std::vector<uint8_t> jpg = readFixture("jpeg_red_blue.jpg");
	REQUIRE(jpg.size() > 40);
	uint8_t* px = decodeJpegBGRA(jpg.data(), 40, &w, &h, 16384);
	CHECK((px == NULL));
	free(px);
}

} // TEST_SUITE("JpegDecode")
