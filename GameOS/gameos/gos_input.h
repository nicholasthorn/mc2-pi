#ifndef GOS_INPUT_H
#define GOS_INPUT_H

namespace input {

enum KeyState {
    KS_FREE = 0, // in not pressed state
    KS_PRESSED, // just pressed
    KS_HELD,    // in pressed state 
    KS_RELEASED // just released
};

struct MouseInfo {
    MouseInfo();

    static const int NUM_BUTTONS = 5;
    float x_;
    float y_;
    float rel_x_;
    float rel_y_;
    float wheel_vert_;
    float wheel_hor_;
    KeyState button_state_[NUM_BUTTONS];
};

struct KeyboardInfo {
    KeyboardInfo();
    bool key_pressed_;
    bool key_released_;
    SDL_Keysym pressed_keysym_;
    SDL_Keysym released_keysym_;
    uint32_t first_pressed_;
    uint8_t last_state_[512];
};

const MouseInfo* getMouseInfo();
const KeyboardInfo* getKeyboardInfo();

void handleMouseMotion(const SDL_Event* event);
void handleMouseButton(const SDL_Event* event);
void handleMouseWheel(const SDL_Event* event);
void beginUpdateMouseState();
void updateMouseState();
// -bench: cursor held at (x, y), no motion or wheel
void pinMouse(float x, float y);

void handleKeyEvent(const SDL_Event* event);
void updateKeyboardState();

// VR controllers (gos_xr) stand in for the mouse and keyboard: a pointer in
// drawable pixels with the buttons held (bit 0 left, 1 middle, 2 right, as
// gos_GetMouseInfo reports them), and keys held by SDL scancode.
// updateMouseState and updateKeyboardState put them over SDL's own state.
// A press that starts and ends between two samples still counts once.
void setVirtualPointer(float x, float y, uint32_t buttons);
// the pointer is off the interface: the cursor stays, the buttons go up
void clearVirtualPointer();
void setVirtualKey(int scancode, bool down);

void resetKeypress();

}

#endif //GOS_INPUT_H
