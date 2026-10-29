#ifndef PIXEL_REPACK_H
#define PIXEL_REPACK_H

#include <cstddef>
#include <cstdint>

// Copies pixelCount pixels of 8-bit channel data from src (srcChannels per
// pixel) to dst (dstChannels per pixel). Extra source channels are dropped;
// missing ones are filled with 0xff. GL-free so it can be unit tested.
void repackPixels8(const uint8_t* src, int srcChannels,
                   uint8_t* dst, int dstChannels, size_t pixelCount);

#endif // PIXEL_REPACK_H
