#include "tga_writer.h"

#include <cstdio>
#include <vector>

bool writeTGA24(const char* path, int width, int height, const uint8_t* rgba)
{
	if (width <= 0 || height <= 0 || width > 0xFFFF || height > 0xFFFF)
		return false;

	// 18-byte header: no id, no colour map, type 2 (uncompressed true colour),
	// origin 0,0, size, 24 bpp, descriptor 0 (bottom-left origin, no alpha)
	uint8_t header[18] = {};
	header[2] = 2;
	header[12] = (uint8_t)(width & 0xFF);
	header[13] = (uint8_t)(width >> 8);
	header[14] = (uint8_t)(height & 0xFF);
	header[15] = (uint8_t)(height >> 8);
	header[16] = 24;

	FILE* f = fopen(path, "wb");
	if (!f)
		return false;

	std::vector<uint8_t> row((size_t)width * 3);
	bool ok = fwrite(header, 1, sizeof(header), f) == sizeof(header);
	for (int y = 0; ok && y < height; ++y) {
		const uint8_t* src = rgba + (size_t)y * width * 4;
		for (int x = 0; x < width; ++x) {
			row[x * 3 + 0] = src[x * 4 + 2];	// TGA stores BGR
			row[x * 3 + 1] = src[x * 4 + 1];
			row[x * 3 + 2] = src[x * 4 + 0];
		}
		ok = fwrite(row.data(), 1, row.size(), f) == row.size();
	}
	return (fclose(f) == 0) && ok;
}
