#include "gos_touch.h"

#include <SDL2/SDL.h>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "gameos.hpp"
#include "gos_input.h"
#include "gos_xr.h"
#include "utils/touch_gestures.h"

namespace {

bool g_enabled = false;
// the touch screen has the virtual pointer and the camera keys: from a
// finger landing until a real mouse moves
bool g_owns = false;
touch::Gestures g_gestures;

double now()
{
    return (double)SDL_GetTicks() * 0.001;
}

// SDL gives finger positions as 0 to 1 across the window; the virtual
// pointer is in drawable pixels
void toDrawable(const SDL_Event* event, float& x, float& y)
{
    x = event->tfinger.x * (float)Environment.drawableWidth;
    y = event->tfinger.y * (float)Environment.drawableHeight;
}

// distances in pixels, from the screen's height: a phone and a desktop
// monitor are both about 1000 pixels high, and a finger's wobble is a
// similar share of either. Tune on devices (MOBILE.md, phase 1).
touch::Settings settingsFor(int height)
{
    touch::Settings s;
    const float h = (float)std::max(height, 480);
    s.slop = 0.02f * h;
    s.panDeadZone = 0.06f * h;
    return s;
}

} // namespace

namespace gos_touch {

void init()
{
    const char* env = getenv("MC2_TOUCH");
    g_enabled = !(env && env[0] == '0');
    if(!g_enabled) {
        printf("[TOUCH] off (MC2_TOUCH=0): SDL turns taps into clicks\n");
        return;
    }
    // the gestures make the mouse: SDL must not make one from the fingers as
    // well (every tap would click twice), nor fingers from the mouse
    SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");
    SDL_SetHint(SDL_HINT_MOUSE_TOUCH_EVENTS, "0");
}

void handleEvent(const SDL_Event* event)
{
    if(!g_enabled || gos_xr::active())
        return;
    // a mouse SDL turned into a finger, should the hint not hold
    if(event->tfinger.touchId == SDL_MOUSE_TOUCHID)
        return;

    float x = 0.0f, y = 0.0f;
    toDrawable(event, x, y);
    const double t = now();
    switch(event->type) {
    case SDL_FINGERDOWN:
        if(!g_gestures.touching())   // the window may have changed size
            g_gestures.setSettings(settingsFor(Environment.drawableHeight));
        g_owns = true;
        g_gestures.down(event->tfinger.fingerId, x, y, t);
        break;
    case SDL_FINGERMOTION:
        g_gestures.move(event->tfinger.fingerId, x, y, t);
        break;
    case SDL_FINGERUP:
        g_gestures.up(event->tfinger.fingerId, x, y, t);
        break;
    }
}

void mouseMoved(const SDL_Event* event)
{
    if(!g_owns || event->motion.which == SDL_TOUCH_MOUSEID || g_gestures.touching())
        return;
    g_owns = false;
    input::clearVirtualPointer();
    input::setVirtualKey(SDL_SCANCODE_LEFT, false);
    input::setVirtualKey(SDL_SCANCODE_RIGHT, false);
    input::setVirtualKey(SDL_SCANCODE_UP, false);
    input::setVirtualKey(SDL_SCANCODE_DOWN, false);
}

void cancel()
{
    if(g_enabled)
        g_gestures.cancel();
}

void poll()
{
    if(!g_owns)
        return;
    g_gestures.update(now());
    for(const touch::PointerEvent& e : g_gestures.takePointerEvents())
        input::setVirtualPointer(e.x, e.y, e.left ? 1u : 0u);
    const int steps = g_gestures.takeWheelSteps();
    if(steps != 0)
        input::addVirtualWheel((float)steps);
    input::setVirtualKey(SDL_SCANCODE_LEFT, g_gestures.scrollX() < 0);
    input::setVirtualKey(SDL_SCANCODE_RIGHT, g_gestures.scrollX() > 0);
    input::setVirtualKey(SDL_SCANCODE_UP, g_gestures.scrollY() < 0);
    input::setVirtualKey(SDL_SCANCODE_DOWN, g_gestures.scrollY() > 0);
}

} // namespace gos_touch
