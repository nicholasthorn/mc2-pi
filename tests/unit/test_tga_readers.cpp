// tests/unit/test_tga_readers.cpp
//
// The terrain tile readers in mclib/tgainfo.cpp trusted the TGA header: a file
// that claimed more pixels than it held was read past its end, and a run-length
// packet could write past the caller's buffer. A refused file leaves the
// caller's buffer zeroed, since the terrain goes on to use it.
// Overruns show up under ASan (ci.yml runs these tests sanitized).

#include "doctest.h"

#include "file.h"
#include "tgainfo.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <vector>

namespace
{

const uint8_t kTopDown = 0x20;		// image_descriptor: first row is the top one

std::vector<uint8_t> tgaHeader(int type, int w, int h, int depth, uint8_t descriptor = kTopDown)
{
	std::vector<uint8_t> f(18, 0);
	f[2] = (uint8_t)type;
	f[12] = (uint8_t)(w & 0xff);
	f[13] = (uint8_t)((w >> 8) & 0xff);
	f[14] = (uint8_t)(h & 0xff);
	f[15] = (uint8_t)((h >> 8) & 0xff);
	f[16] = (uint8_t)depth;
	f[17] = descriptor;
	return f;
}

std::vector<uint8_t> tgaFile(std::vector<uint8_t> header, const std::vector<uint8_t>& data)
{
	header.insert(header.end(), data.begin(), data.end());
	return header;
}

// Opened by relative name: File::open lowercases the whole path.
void writeScratch(const char* name, const std::vector<uint8_t>& bytes)
{
	FILE* out = fopen(name, "wb");
	REQUIRE(out != nullptr);
	fwrite(bytes.data(), 1, bytes.size(), out);
	fclose(out);
}

// dest starts non-zero, so a refused file is seen to clear it
std::vector<uint8_t> loadTexture(const char* name, const std::vector<uint8_t>& bytes, long w, long h)
{
	writeScratch(name, bytes);
	std::vector<uint8_t> dest((size_t)(w * h * 4), 0xAA);
	{
		File f;
		REQUIRE(f.open(name) == NO_ERR);
		loadTGATexture(&f, dest.data(), w, h);
	}
	remove(name);
	return dest;
}

std::vector<uint8_t> loadMask(const char* name, const std::vector<uint8_t>& bytes, long w, long h)
{
	writeScratch(name, bytes);
	std::vector<uint8_t> dest((size_t)(w * h), 0xAA);
	{
		File f;
		REQUIRE(f.open(name) == NO_ERR);
		loadTGAMask(&f, dest.data(), w, h);
	}
	remove(name);
	return dest;
}

bool allZero(const std::vector<uint8_t>& v)
{
	return std::all_of(v.begin(), v.end(), [](uint8_t b) { return b == 0; });
}

} // namespace

TEST_SUITE("TgaReaders")
{

TEST_CASE("texture: 32-bit pixels are copied as they are")
{
	const std::vector<uint8_t> pixels = { 1,2,3,4,  5,6,7,8 };
	const std::vector<uint8_t> out = loadTexture("tga_t32.tga", tgaFile(tgaHeader(2, 2, 1, 32), pixels), 2, 1);
	CHECK(out == pixels);
}

TEST_CASE("texture: 24-bit pixels gain an opaque alpha")
{
	const std::vector<uint8_t> out = loadTexture("tga_t24.tga", tgaFile(tgaHeader(2, 2, 1, 24), { 1,2,3,  4,5,6 }), 2, 1);
	CHECK(out == std::vector<uint8_t>{ 1,2,3,255,  4,5,6,255 });
}

TEST_CASE("texture: bottom-up rows are flipped")
{
	const std::vector<uint8_t> out = loadTexture("tga_flip.tga",
		tgaFile(tgaHeader(2, 1, 2, 32, 0), { 1,2,3,4,  5,6,7,8 }), 1, 2);
	CHECK(out == std::vector<uint8_t>{ 5,6,7,8,  1,2,3,4 });
}

TEST_CASE("texture: run-length data decodes, runs and raw packets")
{
	// a run of two pixels, then a raw packet of one
	const std::vector<uint8_t> data = { 0x81, 1,2,3,  0x00, 4,5,6 };
	const std::vector<uint8_t> out = loadTexture("tga_rle.tga", tgaFile(tgaHeader(10, 3, 1, 24), data), 3, 1);
	CHECK(out == std::vector<uint8_t>{ 1,2,3,255,  1,2,3,255,  4,5,6,255 });
}

TEST_CASE("texture: a file shorter than its pixels is refused")
{
	const std::vector<uint8_t> out = loadTexture("tga_short.tga", tgaFile(tgaHeader(2, 64, 64, 32), { 1,2,3,4 }), 64, 64);
	CHECK(allZero(out));
}

TEST_CASE("texture: a 24-bit file with a third of its pixels is refused")
{
	const std::vector<uint8_t> out = loadTexture("tga_short24.tga",
		tgaFile(tgaHeader(2, 8, 8, 24), std::vector<uint8_t>(64, 7)), 8, 8);
	CHECK(allZero(out));
}

TEST_CASE("texture: an 8-bit file claiming true colour is refused")
{
	// the copy reads three bytes per pixel whatever the header says
	const std::vector<uint8_t> out = loadTexture("tga_depth8.tga",
		tgaFile(tgaHeader(2, 4, 4, 8), std::vector<uint8_t>(16, 7)), 4, 4);
	CHECK(allZero(out));
}

TEST_CASE("texture: a file shorter than a header is refused")
{
	const std::vector<uint8_t> out = loadTexture("tga_stub.tga", std::vector<uint8_t>(4, 0), 2, 2);
	CHECK(allZero(out));
}

TEST_CASE("texture: run-length data that stops early is refused")
{
	const std::vector<uint8_t> out = loadTexture("tga_rle_end.tga",
		tgaFile(tgaHeader(10, 16, 16, 32), { 0x81, 1,2,3,4 }), 16, 16);
	CHECK(allZero(out));
}

TEST_CASE("texture: a packet stopping inside its own pixel is refused")
{
	const std::vector<uint8_t> out = loadTexture("tga_rle_cut.tga",
		tgaFile(tgaHeader(10, 2, 1, 32), { 0x81, 1,2 }), 2, 1);
	CHECK(allZero(out));
}

TEST_CASE("texture: a run-length packet longer than the image is refused")
{
	// 128 pixels into a 2 pixel image
	const std::vector<uint8_t> out = loadTexture("tga_rle_run.tga",
		tgaFile(tgaHeader(10, 2, 1, 24), { 0xFF, 1,2,3 }), 2, 1);
	CHECK(allZero(out));
}

TEST_CASE("texture: run-length data larger than the caller's buffer is refused")
{
	// the decoder goes by the header, the buffer by the caller
	std::vector<uint8_t> data;
	for (int i = 0; i < 64; i++)
	{
		const std::vector<uint8_t> row = { 0x87, 1,2,3 };
		data.insert(data.end(), row.begin(), row.end());
	}
	const std::vector<uint8_t> out = loadTexture("tga_rle_big.tga", tgaFile(tgaHeader(10, 8, 64, 24), data), 8, 8);
	CHECK(allZero(out));
}

TEST_CASE("mask: 8-bit pixels are copied, a colour map skipped")
{
	std::vector<uint8_t> header = tgaHeader(1, 2, 1, 8);
	header[1] = 1;		// colour map present
	header[5] = 2;		// two entries
	header[7] = 24;		// of three bytes
	const std::vector<uint8_t> out = loadMask("tga_m8.tga", tgaFile(header, { 9,9,9, 9,9,9,  1,2 }), 2, 1);
	CHECK(out == std::vector<uint8_t>{ 1,2 });
}

TEST_CASE("mask: bottom-up rows are flipped")
{
	const std::vector<uint8_t> out = loadMask("tga_mflip.tga", tgaFile(tgaHeader(3, 1, 2, 8, 0), { 1,2 }), 1, 2);
	CHECK(out == std::vector<uint8_t>{ 2,1 });
}

TEST_CASE("mask: a file shorter than its pixels is refused")
{
	const std::vector<uint8_t> out = loadMask("tga_mshort.tga", tgaFile(tgaHeader(3, 64, 64, 8), { 1,2 }), 64, 64);
	CHECK(allZero(out));
}

TEST_CASE("mask: a colour map the file does not hold is refused")
{
	std::vector<uint8_t> header = tgaHeader(1, 2, 2, 8);
	header[1] = 1;
	header[5] = 0xFF;
	header[6] = 0x7F;	// 32767 entries
	header[7] = 32;
	const std::vector<uint8_t> out = loadMask("tga_mcmap.tga", tgaFile(header, { 1,2,3,4 }), 2, 2);
	CHECK(allZero(out));
}

TEST_CASE("mask: a depth that would flip past the buffer is refused")
{
	const std::vector<uint8_t> out = loadMask("tga_mdepth.tga",
		tgaFile(tgaHeader(3, 8, 8, 32, 0), std::vector<uint8_t>(8 * 8 * 4, 7)), 8, 8);
	CHECK(allZero(out));
}

TEST_CASE("tgaHeaderFits: a whole file fits, one byte less does not")
{
	const std::vector<uint8_t> whole = tgaFile(tgaHeader(2, 2, 2, 32), std::vector<uint8_t>(16, 1));
	CHECK(tgaHeaderFits(whole.data(), (long)whole.size()));
	CHECK_FALSE(tgaHeaderFits(whole.data(), (long)whole.size() - 1));
	CHECK(tgaHeaderFits(whole.data(), (long)whole.size() + 100));	// a footer is fine
}

TEST_CASE("tgaHeaderFits: the id and the colour map take room")
{
	std::vector<uint8_t> header = tgaHeader(1, 2, 1, 8);
	header[0] = 3;		// id length
	header[1] = 1;		// colour map present
	header[5] = 2;		// two entries
	header[7] = 24;		// of three bytes
	const std::vector<uint8_t> whole = tgaFile(header, std::vector<uint8_t>(3 + 6 + 2, 1));
	CHECK(tgaHeaderFits(whole.data(), (long)whole.size()));
	CHECK_FALSE(tgaHeaderFits(whole.data(), (long)whole.size() - 1));
}

TEST_CASE("tgaHeaderFits: size and depth a reader cannot use")
{
	const std::vector<uint8_t> room(64, 0);
	auto fits = [&](int w, int h, int depth)
	{
		std::vector<uint8_t> f = tgaHeader(2, w, h, depth);
		f.resize(room.size());
		return tgaHeaderFits(f.data(), (long)f.size());
	};
	CHECK(fits(2, 2, 32));
	CHECK_FALSE(fits(0, 2, 32));
	CHECK_FALSE(fits(2, 0, 32));
	CHECK_FALSE(fits(-1, 2, 32));		// width is signed on disk
	CHECK_FALSE(fits(-1, -1, 32));		// the product must not come out positive
	CHECK_FALSE(fits(2, 2, 0));
	CHECK_FALSE(fits(2, 2, 7));
	CHECK_FALSE(fits(2, 2, 64));
	CHECK_FALSE(fits(32767, 32767, 32));	// must not wrap a 32-bit long (Windows)
}

TEST_CASE("tgaHeaderFits: shorter than a header")
{
	const std::vector<uint8_t> stub(17, 0);
	CHECK_FALSE(tgaHeaderFits(stub.data(), (long)stub.size()));
	CHECK_FALSE(tgaHeaderFits(stub.data(), 0));
}

TEST_CASE("tgaPixelsFit goes by the size it is given, not the header's")
{
	const std::vector<uint8_t> whole = tgaFile(tgaHeader(2, 1, 1, 32), std::vector<uint8_t>(16, 1));
	CHECK(tgaPixelsFit(whole.data(), (long)whole.size(), 2, 2, 32));
	CHECK_FALSE(tgaPixelsFit(whole.data(), (long)whole.size(), 3, 2, 32));
}

TEST_CASE("texture: a type the reader does not copy is refused, not left as it was")
{
	CHECK(allZero(loadTexture("tga_gray.tga", tgaFile(tgaHeader(3, 2, 2, 8), { 1,2,3,4 }), 2, 2)));
	CHECK(allZero(loadTexture("tga_pal.tga", tgaFile(tgaHeader(1, 2, 2, 8), { 1,2,3,4 }), 2, 2)));
}

TEST_CASE("tgaReadSquareHeader: whole square maps pass, the rest come back as NO_IMAGE")
{
	TGAFileHeader info;

	const std::vector<uint8_t> good = tgaFile(tgaHeader(2, 2, 2, 24), std::vector<uint8_t>(12, 1));
	CHECK(tgaReadSquareHeader(info, good.data(), (long)good.size()));
	CHECK(info.image_type == UNC_TRUE);
	CHECK(info.width == 2);

	CHECK_FALSE(tgaReadSquareHeader(info, good.data(), (long)good.size() - 1));
	CHECK(info.image_type == NO_IMAGE);

	const std::vector<uint8_t> wide = tgaFile(tgaHeader(2, 2, 1, 32), std::vector<uint8_t>(64, 1));
	CHECK_FALSE(tgaReadSquareHeader(info, wide.data(), (long)wide.size()));
	CHECK(info.image_type == NO_IMAGE);

	const std::vector<uint8_t> gray = tgaFile(tgaHeader(2, 2, 2, 8), std::vector<uint8_t>(64, 1));
	CHECK_FALSE(tgaReadSquareHeader(info, gray.data(), (long)gray.size()));

	CHECK_FALSE(tgaReadSquareHeader(info, good.data(), 17));
	CHECK(info.image_type == NO_IMAGE);
	CHECK(info.width == 0);
}

TEST_CASE("tgaReadSquareHeader: other image types are not its business")
{
	TGAFileHeader info;
	const std::vector<uint8_t> rle = tgaHeader(10, 512, 512, 32);
	CHECK(tgaReadSquareHeader(info, rle.data(), (long)rle.size()));
	CHECK(info.image_type == RLE_TRUE);
}

TEST_CASE("tgaWidthIsPlausible")
{
	CHECK(tgaWidthIsPlausible(64, 18 + 64 * 64 * 4));		// uncompressed
	CHECK(tgaWidthIsPlausible(64, 18 + 64 * 64 / 128 * 4));	// run-length at its best
	CHECK_FALSE(tgaWidthIsPlausible(32767, 18 + 64 * 64 * 4));
	CHECK_FALSE(tgaWidthIsPlausible(0, 1000));
	CHECK_FALSE(tgaWidthIsPlausible(-64, 1000));
	CHECK_FALSE(tgaWidthIsPlausible(64, 0));
}

} // TEST_SUITE("TgaReaders")
