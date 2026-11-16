// mclib/hud_layout.h
//
// Which buttonlayout<width>.fit the in-mission HUD uses at a screen width, and
// where it goes. The HUD's art is a fixed number of pixels: a layout moves it,
// nothing scales it. From 1024 up, every layout places it for a 1024-wide
// centre column, so it can be centred on any screen. The narrower layouts are
// made for exactly their own width.

#pragma once

// The largest layout no wider than maxWidth, or the smallest there is. To step
// down when a layout file is missing (Omnitech has no 1920), ask again with
// one less than the width that failed; the same width back means no more.
inline int hudLayoutWidth(int maxWidth)
{
	static const int widths[] = { 1920, 1600, 1280, 1024, 800 };
	for (int width : widths)
		if (width <= maxWidth)
			return width;
	return 640;
}

// Where the HUD's layout coordinates start across the screen. fileOffset is the
// layout's own xOffset: stock buttonlayout1920.fit has 1600's value in it, so it
// can't be trusted from 1024 up.
inline long hudOffsetX(int screenWidth, int layoutWidth, long fileOffset)
{
	return layoutWidth >= 1024 ? (screenWidth - 1024) / 2 : fileOffset;
}

// Things anchored to the screen's right edge (the pilot video window) are at
// fixed places for the layout's own width: how far to move them on this screen.
inline long hudRightEdgeShift(int screenWidth, int layoutWidth)
{
	return screenWidth - layoutWidth;
}

// The HUD's coordinates end this far down, so the screen's height less this is
// where they start: it sits on the bottom edge at any height.
inline long hudBaseHeight(int layoutWidth)
{
	return layoutWidth == 640 ? 480 : layoutWidth == 800 ? 600 : 768;
}

// wolfman-MC2X's mc2x_buttonlayout<w>.fit has 22 support-unit buttons in its
// own order (Vehicle0 to Vehicle21) where stock has 8. Which of MC2X's is
// stock's support button vehicleIndex (airstrike, guard tower / artillery,
// sensor probe, repair vehicle, scout, minelayer, recovery, cancel), or -1.
inline int mc2xVehicleButton(int vehicleIndex)
{
	static const int buttons[8] = { 0, 6, 1, 2, 3, 4, 5, 21 };
	return (vehicleIndex >= 0) && (vehicleIndex < 8) ? buttons[vehicleIndex] : -1;
}
