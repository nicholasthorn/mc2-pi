#include "gos_screen_keyboard.h"

#include <SDL2/SDL.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

#include "gameos.hpp"
#include "gos_input.h"
#include "gos_xr.h"
#include "utils/screen_keyboard.h"

static_assert(screen_kb::kScanA == SDL_SCANCODE_A, "screen keyboard scancodes are SDL's");
static_assert(screen_kb::kScanA + 25 == SDL_SCANCODE_Z, "screen keyboard scancodes are SDL's");
static_assert(screen_kb::kScan1 == SDL_SCANCODE_1, "screen keyboard scancodes are SDL's");
static_assert(screen_kb::kScan1 + 8 == SDL_SCANCODE_9, "screen keyboard scancodes are SDL's");
static_assert(screen_kb::kScan0 == SDL_SCANCODE_0, "screen keyboard scancodes are SDL's");
static_assert(screen_kb::kScanReturn == SDL_SCANCODE_RETURN, "screen keyboard scancodes are SDL's");
static_assert(screen_kb::kScanBackspace == SDL_SCANCODE_BACKSPACE, "screen keyboard scancodes are SDL's");
static_assert(screen_kb::kScanSpace == SDL_SCANCODE_SPACE, "screen keyboard scancodes are SDL's");
static_assert(screen_kb::kScanMinus == SDL_SCANCODE_MINUS, "screen keyboard scancodes are SDL's");
static_assert(screen_kb::kScanPeriod == SDL_SCANCODE_PERIOD, "screen keyboard scancodes are SDL's");
static_assert(screen_kb::kScanLShift == SDL_SCANCODE_LSHIFT, "screen keyboard scancodes are SDL's");

namespace {

screen_kb::Keyboard g_keyboard;
bool g_shown = false;
// a field let go of the focus: hidden at the next sample unless another
// field takes it first (one frame's fields update in no set order)
bool g_hidePending = false;
// the field taking the focus is in the screen's lower half: the keyboard
// goes along the top
bool g_top = false;
// the focused field's updates since the keyboard showed: once one has come
// (a dialog's field starts updating only after its opening animation), this
// many samples in a row without one hide it
const int kMissedSamples = 4;
bool g_fieldSeen = false;
bool g_fieldActive = false;
int g_fieldMissed = 0;
// keys pressed at the last sample, to go up at this one (0: none)
int g_down[2] = {0, 0};
// a pressed key shows lit for a few samples
const int kFlashSamples = 5;
int g_flashKey = -1;
int g_flashSamples = 0;
// mouse buttons whose press began on the keyboard: hidden from the game
// until they are let go, even if the keyboard went away meanwhile
uint32_t g_taken = 0;

bool forced()
{
    static const char* value = getenv("MC2_SCREEN_KEYBOARD");
    return value && value[0] == '1';
}

void pointer(float& x, float& y)
{
    gos_GetMouseInfo(&x, &y, nullptr, nullptr, nullptr, nullptr);
}

void hide()
{
    if(g_shown)
        printf("[KEYBOARD] hidden\n");
    g_shown = false;
    g_keyboard.reset();
}

void press(int key)
{
    if(key < 0)
        return;
    g_flashKey = key;
    g_flashSamples = kFlashSamples;
    const screen_kb::Press p = g_keyboard.press(key);
    // down for this sample, up at the next (the latch keeps it down until
    // the game has read it once)
    if(p.scancode) {
        input::setVirtualKey(p.scancode, true);
        g_down[0] = p.scancode;
        if(p.shift) {
            input::setVirtualKey(screen_kb::kScanLShift, true);
            g_down[1] = screen_kb::kScanLShift;
        }
    }
    if(p.close)
        hide();
}

void fill(gosScreenKey& out, const screen_kb::Rect& r, const char* label, int state)
{
    out.x = r.x;
    out.y = r.y;
    out.w = r.w;
    out.h = r.h;
    out.label = label;
    out.state = state;
}

} // namespace

namespace gos_screen_keyboard {

bool wanted()
{
    return gos_xr::active() || forced();
}

void fieldArea(float top, float bottom)
{
    g_top = 0.5f * (top + bottom) > 0.5f;
}

void show(bool on)
{
    g_hidePending = !on;
    if(!on)
        return;
    // again too: a field clicked while it has the focus brings the
    // keyboard back after Close or Enter
    g_keyboard.layout(g_top);
    if(!g_shown) {
        printf("[KEYBOARD] shown\n");
        g_fieldSeen = false;
        g_fieldActive = false;
        g_fieldMissed = 0;
    }
    g_shown = true;
}

void fieldActive()
{
    g_fieldActive = true;
}

void sample()
{
    if(g_hidePending) {
        g_hidePending = false;
        hide();
    }
    if(g_shown) {
        if(g_fieldActive) {
            g_fieldSeen = true;
            g_fieldMissed = 0;
        } else if(g_fieldSeen && ++g_fieldMissed >= kMissedSamples) {
            printf("[KEYBOARD] its field stopped updating\n");
            hide();
        }
    }
    g_fieldActive = false;
    for(int& key : g_down) {
        if(key) {
            input::setVirtualKey(key, false);
            key = 0;
        }
    }
    if(g_flashSamples > 0)
        --g_flashSamples;

    const input::MouseInfo* mi = input::getMouseInfo();
    float x = 0.0f, y = 0.0f;
    pointer(x, y);
    for(int b = 0; b < input::MouseInfo::NUM_BUTTONS; ++b) {
        const uint32_t bit = 1u << b;
        const input::KeyState state = mi->button_state_[b];
        if(!(g_taken & bit) && g_shown && state == input::KS_PRESSED && g_keyboard.contains(x, y)) {
            g_taken |= bit;
            if(b == 0)
                press(g_keyboard.hit(x, y));
        }
        if(g_taken & bit) {
            // hidden, the button reads as just pressed each sample it is
            // held, and as free once it is let go
            if(state == input::KS_FREE || state == input::KS_RELEASED)
                g_taken &= ~bit;
            else
                input::hideButton(b);
        }
    }
}

} // namespace gos_screen_keyboard

int __stdcall gos_GetScreenKeyboard(gosScreenKey* panel, gosScreenKey* keys, int maxKeys)
{
    if(!g_shown)
        return 0;
    float x = 0.0f, y = 0.0f;
    pointer(x, y);
    const int under = g_keyboard.hit(x, y);
    if(panel)
        fill(*panel, g_keyboard.panel(), "", 0);
    int count = 0;
    for(int i = 0; keys && i < screen_kb::Keyboard::kKeys && count < maxKeys; ++i, ++count) {
        int state = 0;
        if(g_keyboard.lit(i) || (i == g_flashKey && g_flashSamples > 0))
            state = 2;
        else if(i == under)
            state = 1;
        fill(keys[count], g_keyboard.rect(i), g_keyboard.label(i), state);
    }
    return count;
}
