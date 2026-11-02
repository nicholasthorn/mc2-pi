#include "utils/alpha_mips.h"

bool hasTransparency(const uint8_t* rgba, size_t pixelCount)
{
    for(size_t i = 0; i < pixelCount; ++i)
        if(rgba[i*4 + 3] != 0xff)
            return true;
    return false;
}

// a source texel range per destination texel: even sizes are 2 texels wide, an
// odd size has some of 3, so no texel is left out
static void downsample(const uint8_t* src, int sw, int sh, uint8_t* dst, int dw, int dh)
{
    for(int dy = 0; dy < dh; ++dy) {
        const int y0 = dy * sh / dh;
        const int y1 = (dy + 1) * sh / dh;
        for(int dx = 0; dx < dw; ++dx) {
            const int x0 = dx * sw / dw;
            const int x1 = (dx + 1) * sw / dw;
            uint32_t rgb[3] = {0, 0, 0};
            uint32_t plain[3] = {0, 0, 0};
            uint32_t alpha = 0;
            for(int y = y0; y < y1; ++y) {
                const uint8_t* p = src + ((size_t)y * sw + x0) * 4;
                for(int x = x0; x < x1; ++x, p += 4) {
                    const uint32_t a = p[3];
                    alpha += a;
                    for(int c = 0; c < 3; ++c) {
                        rgb[c] += p[c] * a;
                        plain[c] += p[c];
                    }
                }
            }
            const uint32_t n = (uint32_t)((x1 - x0) * (y1 - y0));
            for(int c = 0; c < 3; ++c)
                dst[c] = (uint8_t)(alpha ? (rgb[c] + alpha/2) / alpha : (plain[c] + n/2) / n);
            dst[3] = (uint8_t)((alpha + n/2) / n);
            dst += 4;
        }
    }
}

void buildAlphaMips(const uint8_t* base, int w, int h, std::vector<uint8_t>& mips)
{
    struct Level { int w, h; size_t offset; };
    std::vector<Level> levels;
    size_t total = 0;
    for(int lw = w, lh = h; lw > 1 || lh > 1; ) {
        lw = nextMipSize(lw);
        lh = nextMipSize(lh);
        levels.push_back({lw, lh, total});
        total += (size_t)lw * lh * 4;
    }
    mips.resize(total);

    const uint8_t* src = base;
    int sw = w, sh = h;
    for(const Level& l : levels) {
        uint8_t* dst = mips.data() + l.offset;
        downsample(src, sw, sh, dst, l.w, l.h);
        src = dst;
        sw = l.w;
        sh = l.h;
    }

    // from the smallest level up, so the colour reaches across empty areas
    for(size_t i = levels.size(); i-- > 1; ) {
        const Level& l = levels[i - 1];
        const Level& parent = levels[i];
        for(int y = 0; y < l.h; ++y) {
            for(int x = 0; x < l.w; ++x) {
                uint8_t* p = mips.data() + l.offset + ((size_t)y * l.w + x) * 4;
                if(p[3] == 0) {
                    const size_t q = parent.offset + ((size_t)(y * parent.h / l.h) * parent.w + x * parent.w / l.w) * 4;
                    p[0] = mips[q];
                    p[1] = mips[q + 1];
                    p[2] = mips[q + 2];
                }
            }
        }
    }
}
