// tests/unit/fuzz/fuzz_tga.cpp: the TGA readers over any bytes.
//
// GameOS's Image::loadTGA (textures, fonts, the interface) and mclib's
// terrain readers: the colour and height map header check, and the tile and
// mask loaders, which the game calls with the size it expects, not the
// file's.

#include "fuzz_targets.h"
#include "fuzz_support.h"

#include "file.h"
#include "tgainfo.h"
#include "utils/Image.h"

#include <vector>

namespace
{

void loadTerrain(const std::vector<char>& image, long side)
{
	{
		File mem;
		if (mem.open(image.data(), (int)image.size()) != NO_ERR)
			return;
		std::vector<unsigned char> dest((size_t)(side * side * 4));
		loadTGATexture(&mem, dest.data(), side, side);
		mem.close();
	}
	{
		File mem;
		if (mem.open(image.data(), (int)image.size()) != NO_ERR)
			return;
		std::vector<unsigned char> dest((size_t)(side * side));
		loadTGAMask(&mem, dest.data(), side, side);
		mem.close();
	}
}

} // namespace

int fuzzTga(const uint8_t* data, size_t size)
{
	if (size == 0 || size > (1 << 20))
		return 0;
	std::vector<char> image(data, data + size);
	const unsigned char* bytes = reinterpret_cast<const unsigned char*>(image.data());

	fuzzGuarded([&] {
		Image img;
		img.loadTGA(bytes, image.size());
	});

	fuzzGuarded([&] {
		TGAFileHeader info;
		if (tgaReadSquareHeader(info, image.data(), (long)image.size()))
			tgaHeaderFits(image.data(), (long)image.size());
		loadTerrain(image, 4);
		loadTerrain(image, 32);
	});
	return 0;
}
