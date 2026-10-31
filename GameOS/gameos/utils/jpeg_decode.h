#ifndef JPEG_DECODE_H
#define JPEG_DECODE_H

#include <cstddef>
#include <cstdint>

// Decodes a JPEG held in memory to 32-bit BGRA (the byte order of the
// engine's TGA loads), top row first, alpha 0xff. Returns a malloc'd buffer
// the caller releases with free() (gos_Free), or NULL if the data is not a
// JPEG or either side exceeds maxDimension. GL-free so it can be unit tested.
uint8_t* decodeJpegBGRA(const uint8_t* data, size_t size,
                        int* width, int* height, int maxDimension);

#endif // JPEG_DECODE_H
