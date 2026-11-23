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
screen_kb::Visibility g_visibility;
screen_kb::ButtonTaker g_buttons;
// the field taking the focus, as shares of the screen's height
float g_fieldTop = 0.0f;
float g_fieldBottom = 0.0f;
// keys pressed at the last sample, to go up at this one (0: none)
int g_down[2] = {0, 0};
// a pressed key shows lit for a few samples
const int kFlashSamples = 5;
int g_flashKey = -1;
int g_flashSamples = 0;

bool forced()
{
    static const char* value = getenv("MC2_SCREEN_KEYBOARD");
    return value && value[0] == '1';
}

void pointer(float& x, float& y)
{
    gos_GetMouseInfo(&x, &y, nullptr, nullptr, nullptr, nullptr);
}

void hidden(const char* why)
{
    printf("[KEYBOARD] hidden (%s)\n", why);
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
    if(p.close && g_visibility.shown()) {
        g_visibility.close();
        hidden(p.scancode ? "Enter" : "Close");
    }
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
    g_fieldTop = top;
    g_fieldBottom = bottom;
}

void show(bool on)
{
    // again too: a field clicked while it has the focus brings the
    // keyboard back after Close or Enter, next to wherever it is
    if(on)
        g_keyboard.layout(g_fieldTop, g_fieldBottom);
    if(g_visibility.focus(on) == screen_kb::Visibility::Shown)
        printf("[KEYBOARD] shown\n");
}

void fieldActive()
{
    g_visibility.fieldUpdated();
}

void close()
{
    if(g_visibility.shown()) {
        g_visibility.close();
        hidden("closed by the game");
    }
}

void sample()
{
    switch(g_visibility.sample()) {
    case screen_kb::Visibility::Hidden:
        hidden("focus lost");
        break;
    case screen_kb::Visibility::FieldGone:
        hidden("its field stopped updating");
        break;
    default:
        break;
    }
    for(int& key : g_down) {
        if(key) {
            input::setVirtualKey(key, false);
            key = 0;
        }
    }
    if(g_flashSamples > 0)
        --g_flashSamples;

    const input::MouseInfo* mi = input::getMouseInfo();
    uint32_t pressed = 0, down = 0;
    for(int b = 0; b < input::MouseInfo::NUM_BUTTONS; ++b) {
        const input::KeyState state = mi->button_state_[b];
        if(state == input::KS_PRESSED)
            pressed |= 1u << b;
        if(state == input::KS_PRESSED || state == input::KS_HELD)
            down |= 1u << b;
    }
    float x = 0.0f, y = 0.0f;
    pointer(x, y);
    bool leftPress = false;
    const bool onKeyboard = g_visibility.shown() && g_keyboard.contains(x, y);
    const uint32_t hide = g_buttons.sample(pressed, down, onKeyboard, leftPress);
    if(leftPress)
        press(g_keyboard.hit(x, y));
    // hidden, a held button reads as just pressed at each sample, and as
    // free once it is let go
    for(int b = 0; b < input::MouseInfo::NUM_BUTTONS; ++b)
        if(hide & (1u << b))
            input::hideButton(b);
}

} // namespace gos_screen_keyboard

int __stdcall gos_GetScreenKeyboard(gosScreenKey* panel, gosScreenKey* keys, int maxKeys)
{
    if(!g_visibility.shown())
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
