#ifndef TGA_WRITER_H
#define TGA_WRITER_H

#include <cstdint>

// Writes an uncompressed 24-bit TGA from RGBA rows stored bottom row first
// (as glReadPixels returns them; TGA's default origin is bottom-left too).
// Alpha is dropped: the back buffer's alpha is not meaningful. Returns false
// if the file cannot be written. GL-free so it can be unit tested.
bool writeTGA24(const char* path, int width, int height, const uint8_t* rgba);

#endif // TGA_WRITER_H
