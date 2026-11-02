#ifndef ALPHA_MIPS_H
#define ALPHA_MIPS_H

#include <cstddef>
#include <cstdint>
#include <vector>

// Mipmaps for 8-bit RGBA textures with straight (not premultiplied) alpha.
// glGenerateMipmap averages colour and alpha separately, so the colour of
// transparent texels, which nobody sees at full size, bleeds into every smaller
// level. GL-free so it can be unit tested.

// true if any of the pixelCount RGBA pixels is not fully opaque; stops at the
// first one
bool hasTransparency(const uint8_t* rgba, size_t pixelCount);

// The size of the mip level below one of size dim, as GL has it.
inline int nextMipSize(int dim) { return dim > 1 ? dim / 2 : 1; }

// Levels 1 and down, to 1x1, of the w x h RGBA image base, one after another
// in mips (level 1 first, tightly packed). A texel's colour is its source texels'
// colours weighted by their alpha and its alpha is their average, so the mean
// alpha, which is what alpha to coverage draws, stays the same at every level.
// A texel with no coverage takes its colour from the next smaller level, which
// is what a covered neighbour filters against.
void buildAlphaMips(const uint8_t* base, int w, int h, std::vector<uint8_t>& mips);

#endif // ALPHA_MIPS_H
