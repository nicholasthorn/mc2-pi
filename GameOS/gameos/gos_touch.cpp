#include "gos_touch.h"

#include <SDL2/SDL.h>
#include <algorithm>
#include <cstdio>
#include <cstdlib>

#include "gameos.hpp"
#include "gos_input.h"
#include "gos_xr.h"
#include "utils/str_bounded.h"
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

// the camera keys held for a scroll direction (-1, 0, 1 each way)
void setScrollKeys(int x, int y)
{
    input::setVirtualKey(SDL_SCANCODE_LEFT, x < 0);
    input::setVirtualKey(SDL_SCANCODE_RIGHT, x > 0);
    input::setVirtualKey(SDL_SCANCODE_UP, y < 0);
    input::setVirtualKey(SDL_SCANCODE_DOWN, y > 0);
}

} // namespace

namespace gos_touch {

void init()
{
    g_enabled = envFlagOr(getenv("MC2_TOUCH"), true);
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

void mouseUsed(const SDL_Event* event)
{
    const Uint32 which = event->type == SDL_MOUSEMOTION ? event->motion.which : event->button.which;
    if(!g_owns || which == SDL_TOUCH_MOUSEID || g_gestures.touching())
        return;
    g_owns = false;
    input::clearVirtualPointer();
    setScrollKeys(0, 0);
}

void cancel()
{
    if(g_enabled)
        g_gestures.cancel();
}

void poll()
{
    // in VR the controllers have the virtual mouse and keys
    if(!g_owns || gos_xr::active())
        return;
    g_gestures.update(now());
    for(const touch::PointerEvent& e : g_gestures.takePointerEvents())
        input::setVirtualPointer(e.x, e.y, e.left ? 1u : 0u);
    const int step = g_gestures.takeWheelStep();
    if(step != 0)
        input::addVirtualWheel((float)step);
    setScrollKeys(g_gestures.scrollX(), g_gestures.scrollY());
}

} // namespace gos_touch

bool __stdcall gos_TouchPointer()
{
    return g_owns;
}
