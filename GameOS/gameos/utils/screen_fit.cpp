#include "screen_fit.h"

namespace screen_fit {

namespace {
const int kHeight = 768;
const int kMinWidth = 1024;
}

void phoneResolution(int screenWidth, int screenHeight, int& width, int& height)
{
    height = kHeight;
    width = kMinWidth;
    if(screenWidth <= 0 || screenHeight <= 0)
        return;
    const long long wide = ((long long)kHeight * screenWidth + screenHeight / 2) / screenHeight;
    if(wide > kMinWidth)
        width = (int)wide;
}

void fitViewport(int drawableWidth, int drawableHeight, int gameWidth, int gameHeight, int vp[4])
{
    vp[0] = 0;
    vp[1] = 0;
    vp[2] = drawableWidth;
    vp[3] = drawableHeight;
    if(gameWidth <= 0 || gameHeight <= 0 || drawableWidth <= 0 || drawableHeight <= 0)
        return;
    // compare the shapes without rounding: dw / dh against gw / gh. Sizes are
    // rounded, so a game resolution made from the screen's shape (rounded to a
    // whole pixel) fills it.
    const long long drawableByGame = (long long)drawableWidth * gameHeight;
    const long long gameByDrawable = (long long)gameWidth * drawableHeight;
    if(drawableByGame > gameByDrawable) {
        // the screen is wider: bars at the sides
        vp[2] = (int)((gameByDrawable + gameHeight / 2) / gameHeight);
        vp[0] = (drawableWidth - vp[2]) / 2;
    } else if(drawableByGame < gameByDrawable) {
        // the screen is taller: bars above and below
        vp[3] = (int)((drawableByGame + gameWidth / 2) / gameWidth);
        vp[1] = (drawableHeight - vp[3]) / 2;
    }
}

float acrossViewport(float pixel, int start, int size)
{
    if(size <= 0)
        return 0.0f;
    const float t = (pixel - (float)start) / (float)size;
    return t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
}

}
