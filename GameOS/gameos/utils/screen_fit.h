#ifndef SCREEN_FIT_H
#define SCREEN_FIT_H

// Fitting the game's screen to a window of another shape: a phone's wide
// screen (MOBILE.md, aspect ratio). GL- and SDL-free for the unit tests.
namespace screen_fit {

// The resolution a phone or tablet runs the game at: the screen's shape at
// 768 high, the least height the wide HUD layouts need (hud_layout.h), and
// at least 1024 wide, where those layouts start.
void phoneResolution(int screenWidth, int screenHeight, int& width, int& height);

// The shape phoneResolution should take: the display's, landscape, while the
// window spans its shorter side (the window may not have grown over the
// status bar yet when first asked); the window's own in split screen or a
// freeform window. A display size of 0 means unknown.
void screenShape(int windowWidth, int windowHeight, int displayWidth, int displayHeight,
        int& width, int& height);

// The largest rectangle of the game's shape, centred in the drawable, as a
// GL viewport (x, y from the bottom left, width, height). The whole
// drawable when the game's size is not known.
void fitViewport(int drawableWidth, int drawableHeight, int gameWidth, int gameHeight, int vp[4]);

// A drawable pixel coordinate as 0..1 across a viewport span, clamped (a
// finger in a bar is at the nearest edge).
float acrossViewport(float pixel, int start, int size);

}

#endif // SCREEN_FIT_H
