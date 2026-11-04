#include "iconsheet.h"

#include <cstring>

int extendedStripWidth(int width, int height, int cellX, int cellY, int highWidth, int highHeight)
{
    if (cellX <= 0 || cellY <= 0)
        return 0;
    const int cells = width / cellX;
    const int highCells = highWidth / HIGH_MECH_ICON_X;
    const int outWidth = highCells * cellX;
    // a TGA header holds the width in a signed 16 bits
    if (highCells <= cells || outWidth > 32767
        || height / cellY != highHeight / HIGH_MECH_ICON_Y)
        return 0;
    return outWidth;
}

void extendStrip(const uint8_t* pixels, int width, int height, int cellX, int cellY,
                 const uint8_t* highPixels, int highWidth, uint8_t* out, int outWidth)
{
    const int cells = width / cellX;
    for (int y = 0; y < height; ++y) {
        uint8_t* row = out + (size_t)y * outWidth * 4;
        std::memcpy(row, pixels + (size_t)y * width * 4, (size_t)width * 4);
        std::memset(row + (size_t)width * 4, 0, (size_t)(outWidth - width) * 4);

        const int highRow = (y / cellY) * HIGH_MECH_ICON_Y + (y % cellY) * HIGH_MECH_ICON_Y / cellY;
        for (int x = cells * cellX; x < outWidth; ++x) {
            const int highColumn = (x / cellX) * HIGH_MECH_ICON_X + (x % cellX) * HIGH_MECH_ICON_X / cellX;
            std::memcpy(row + (size_t)x * 4,
                        highPixels + ((size_t)highRow * highWidth + highColumn) * 4, 4);
        }
    }
}

int fillEmptyCells(uint32_t* sheet, int sheetWidth, int sheetHeight, const uint8_t* src,
                   int srcWidth, int srcHeight, int cellX, int cellY)
{
    if (cellX <= 0 || cellY <= 0 || srcHeight < cellY)
        return 0;
    const int perRow = sheetWidth / cellX;
    const int sheetCells = perRow * (sheetHeight / cellY);
    const int srcCells = srcWidth / cellX;
    int filled = 0;
    for (int cell = 0; cell < sheetCells && cell < srcCells; ++cell) {
        uint32_t* dest = sheet + (size_t)(cell / perRow) * cellY * sheetWidth + (cell % perRow) * cellX;
        bool empty = true;
        for (int y = 0; empty && y < cellY; ++y)
            for (int x = 0; empty && x < cellX; ++x)
                empty = (dest[(size_t)y * sheetWidth + x] & 0x00ffffffu) == 0;
        if (!empty)
            continue;
        const uint8_t* from = src + (size_t)cell * cellX * 4;
        for (int y = 0; y < cellY; ++y)
            std::memcpy(dest + (size_t)y * sheetWidth, from + (size_t)y * srcWidth * 4, (size_t)cellX * 4);
        ++filled;
    }
    return filled;
}
