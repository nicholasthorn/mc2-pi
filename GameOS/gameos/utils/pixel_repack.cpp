#include "utils/pixel_repack.h"

void repackPixels8(const uint8_t* src, int srcChannels,
                   uint8_t* dst, int dstChannels, size_t pixelCount)
{
    for(size_t i = 0; i < pixelCount; ++i)
        for(int c = 0; c < dstChannels; ++c)
            dst[i*dstChannels + c] = c < srcChannels ? src[i*srcChannels + c] : 0xff;
}
