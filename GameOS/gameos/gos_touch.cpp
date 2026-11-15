#include "gos_touch.h"

#include <SDL2/SDL.h>
#include <algorithm>
#include <cstdio>
#include <cstdlib>

#include "gameos.hpp"
#include "gos_input.h"
#include "gos_xr.h"
#include "utils/str_bounded.h"
#include "utils/touch_bar.h"
#include "utils/touch_gestures.h"

namespace {

bool g_enabled = false;
// the touch screen has the virtual pointer and the camera keys: from a
// finger landing until a real mouse moves
bool g_owns = false;
// the cursor goes to the middle of the screen once its size is known, before
// any finger (phones and tablets: init)
bool g_centreCursor = false;
touch::Gestures g_gestures;

// the command bar, while a mission takes orders: frames since it last did
// (gos_VRMissionTakesOrders), and the finger on a button, which the gestures
// never see
const int kOrdersFrames = 3;
int g_ordersAge = kOrdersFrames;
touch::Bar g_bar;
bool g_barFingerDown = false;
SDL_FingerID g_barFinger = 0;
// the left button as the gestures last set it, to see an order end
bool g_pointerLeft = false;
// the touch screen started text input for a text field (gos_TouchTextInput),
// which was off before, so it stops it when the field lets go
bool g_textInput = false;

bool inMission()
{
    return g_ordersAge < kOrdersFrames;
}

bool barShown()
{
    return g_owns && inMission() && !gos_xr::active();
}

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
// A mission adds the long press (the right button: follow a unit with the
// camera, or turn it by dragging); elsewhere a held finger is the left button
// soon, which a menu's held scroll arrow needs.
touch::Settings settingsFor(int height, bool mission)
{
    touch::Settings s;
    s.longPress = mission;
    const float h = (float)std::max(height, 480);
    s.slop = 0.02f * h;
    s.panDeadZone = 0.06f * h;
    return s;
}

// the keys the touch screen holds this frame: the camera's for a scroll
// direction (-1, 0, 1 each way) and the command bar's. A tap's key goes
// down and up at once: the input latch keeps it down for one sample.
void setKeys(int scrollX, int scrollY, const touch::BarKeys& bar)
{
    input::setVirtualKey(SDL_SCANCODE_LEFT, scrollX < 0);
    input::setVirtualKey(SDL_SCANCODE_RIGHT, scrollX > 0);
    input::setVirtualKey(SDL_SCANCODE_UP, scrollY < 0);
    input::setVirtualKey(SDL_SCANCODE_DOWN, scrollY > 0);
    input::setVirtualKey(SDL_SCANCODE_LCTRL, bar.ctrl);    // the game's waypoint key
    input::setVirtualKey(SDL_SCANCODE_LSHIFT, bar.shift);
    input::setVirtualKey(SDL_SCANCODE_F, bar.force);       // force fire
    if(bar.tap) {
        const int key = bar.tapCommand == touch::BarCommand::SelectVisible ? SDL_SCANCODE_E : SDL_SCANCODE_GRAVE;
        input::setVirtualKey(key, true);
        input::setVirtualKey(key, false);
    }
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
#if defined(__ANDROID__) && !defined(MC2_OPENXR)
    // a phone or tablet: the touch screen has the cursor from the start, in
    // the middle of the screen (poll). SDL's mouse sits at 0, 0 until a real
    // one moves, where a mission would draw the cursor through its opening
    // fly-in and scroll the map up and left at the screen's edge.
    g_owns = true;
    g_centreCursor = true;
#endif
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
    // once a run: says the touch screen reaches the game, and where
    static bool logged = false;
    if(!logged && event->type == SDL_FINGERDOWN) {
        logged = true;
        printf("[TOUCH] first finger at %.0f, %.0f of %d x %d\n", x, y,
                Environment.drawableWidth, Environment.drawableHeight);
    }
    // the finger on a bar button: nothing until it lifts
    if(g_barFingerDown && event->tfinger.fingerId == g_barFinger) {
        if(event->type == SDL_FINGERUP)
            g_barFingerDown = false;
        return;
    }
    switch(event->type) {
    case SDL_FINGERDOWN:
        if(barShown() && !g_barFingerDown) {
            g_bar.layout((float)Environment.drawableWidth, (float)Environment.drawableHeight);
            const int button = g_bar.hit(x, y);
            if(button >= 0) {
                g_bar.press(button);
                g_barFingerDown = true;
                g_barFinger = event->tfinger.fingerId;
                return;
            }
        }
        if(!g_gestures.touching())   // the window may have changed size
            g_gestures.setSettings(settingsFor(Environment.drawableHeight, inMission()));
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
    // a motion that does not move is SDL's own (it sends one at 0, 0 when the
    // window gains focus, with no mouse on the device) or a warp's: not a mouse
    if(event->type == SDL_MOUSEMOTION && event->motion.xrel == 0 && event->motion.yrel == 0)
        return;
    g_owns = false;
    g_pointerLeft = false;
    input::clearVirtualPointer();
    g_bar.clear();
    setKeys(0, 0, touch::BarKeys());
}

void cancel()
{
    if(!g_enabled)
        return;
    g_gestures.cancel();
    g_bar.clear();
    g_barFingerDown = false;
}

void missionTakesOrders()
{
    g_ordersAge = 0;
}

void poll()
{
    // every frame, so a mission left with the mouse is not still current when
    // a finger comes back
    if(g_ordersAge < kOrdersFrames)
        ++g_ordersAge;
    // in VR the controllers have the virtual mouse and keys
    if(!g_owns || gos_xr::active())
        return;
    if(!inMission())
        g_bar.clear();
    if(g_centreCursor && Environment.drawableWidth > 0 && Environment.drawableHeight > 0) {
        g_centreCursor = false;
        input::setVirtualPointer(0.5f * (float)Environment.drawableWidth, 0.5f * (float)Environment.drawableHeight, 0u);    }

    g_gestures.update(now());
    for(const touch::PointerEvent& e : g_gestures.takePointerEvents()) {
        // bit 0 left, bit 2 right, as gos_GetMouseInfo reports them
        input::setVirtualPointer(e.x, e.y, (e.left ? 1u : 0u) | (e.right ? 4u : 0u));
        if(g_pointerLeft && !e.left)    // a click or a drag ended: an order
            g_bar.ordered();
        g_pointerLeft = e.left;
    }
    const int step = g_gestures.takeWheelStep();
    if(step != 0)
        input::addVirtualWheel((float)step);
    // the arrow keys only in a mission: elsewhere they move a list's
    // selection or a text box's caret
    if(inMission())
        setKeys(g_gestures.scrollX(), g_gestures.scrollY(), g_bar.take());
    else
        setKeys(0, 0, touch::BarKeys());
}

} // namespace gos_touch

bool __stdcall gos_TouchPointer()
{
    return g_owns;
}

void __stdcall gos_TouchMissionTakesOrders()
{
    gos_touch::missionTakesOrders();
}

void __stdcall gos_TouchTextInput(bool focused)
{
    // started only for the touch screen, and stopped only if it was off
    // before: desktop SDL has text input on from the start (the IME), and
    // stopping it there would leave it off for good
    if(focused && g_owns) {
        if(!g_textInput && !SDL_IsTextInputActive())
            g_textInput = true;
        SDL_StartTextInput();   // again too: it brings back a dismissed keyboard
    } else if(!focused && g_textInput) {
        SDL_StopTextInput();
        g_textInput = false;
    }
}

int __stdcall gos_GetTouchBar(gosTouchBarButton* buttons, int maxButtons)
{
    if(!barShown() || !buttons)
        return 0;
    const float w = (float)Environment.drawableWidth;
    const float h = (float)Environment.drawableHeight;
    if(w <= 0.0f || h <= 0.0f)
        return 0;
    g_bar.layout(w, h);
    int count = 0;
    for(int i = 0; i < touch::Bar::kButtons && count < maxButtons; ++i, ++count) {
        const touch::BarRect r = g_bar.rect(i);
        buttons[count].x = r.x / w;
        buttons[count].y = r.y / h;
        buttons[count].w = r.w / w;
        buttons[count].h = r.h / h;
        buttons[count].label = touch::Bar::label(i);
        buttons[count].lit = g_bar.lit(i);
    }
    return count;
}
