#include "utils/jpeg_decode.h"

#include <climits>

#include "stb/stb_image.h"

uint8_t* decodeJpegBGRA(const uint8_t* data, size_t size,
                        int* width, int* height, int maxDimension)
{
    *width = 0;
    *height = 0;
    if(!data || size == 0 || size > (size_t)INT_MAX)
        return NULL;

    // check the header first so an oversized image is never allocated
    int w = 0, h = 0, channels = 0;
    if(!stbi_info_from_memory(data, (int)size, &w, &h, &channels))
        return NULL;
    if(w <= 0 || h <= 0 || w > maxDimension || h > maxDimension)
        return NULL;

    uint8_t* pixels = stbi_load_from_memory(data, (int)size, &w, &h, &channels, 4);
    if(!pixels)
        return NULL;

    // stb gives RGBA; the engine's colour maps are BGRA
    const size_t count = (size_t)w * (size_t)h;
    for(size_t i = 0; i < count; ++i)
    {
        uint8_t r = pixels[i*4 + 0];
        pixels[i*4 + 0] = pixels[i*4 + 2];
        pixels[i*4 + 2] = r;
    }

    *width = w;
    *height = h;
    return pixels;
}
