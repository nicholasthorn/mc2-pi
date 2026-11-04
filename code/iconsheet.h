// code/iconsheet.h
//
// Icon sheets as plain pixels: 32 bits each, top row first. No GL, so it can be
// unit tested.

#ifndef ICONSHEET_H
#define ICONSHEET_H

#include <cstdint>

// The cells of the high resolution sheets.
const int HIGH_MECH_ICON_X = 40;
const int HIGH_MECH_ICON_Y = 48;
const int HIGH_PILOT_ICON_X = 33;
const int HIGH_PILOT_ICON_Y = 46;

// The width a strip of cellX x cellY icons needs to hold every icon of the high
// strip (HIGH_MECH_ICON cells), or 0 if there is nothing to add or the two do
// not have the same number of rows.
int extendedStripWidth(int width, int height, int cellX, int cellY, int highWidth, int highHeight);

// Writes the strip to out, outWidth wide and as high as the strip, with the
// high strip's further icons added at this strip's cell size. By nearest pixel,
// as damage recolours an icon by its exact colours.
void extendStrip(const uint8_t* pixels, int width, int height, int cellX, int cellY,
                 const uint8_t* highPixels, int highWidth, uint8_t* out, int outWidth);

// Copies the icons of the top row of src (srcWidth wide, cellX x cellY cells)
// into the cells of a sheet that are still empty (no colour in any pixel).
// Returns how many it filled.
int fillEmptyCells(uint32_t* sheet, int sheetWidth, int sheetHeight, const uint8_t* src,
                   int srcWidth, int srcHeight, int cellX, int cellY);

#endif // ICONSHEET_H
