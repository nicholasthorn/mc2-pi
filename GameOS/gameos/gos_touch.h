#ifndef GOS_TOUCH_H
#define GOS_TOUCH_H

// The touch screen as the mouse and camera keys (MOBILE.md, phase 1): SDL's
// finger events go through touch::Gestures (utils/touch_gestures.h), and what
// it makes goes to the game through GameOS's virtual mouse and keys
// (gos_input.h), as the VR controllers' does. Every platform: phones and
// tablets, and touch screen laptops. MC2_TOUCH=0 turns it off, which leaves
// SDL's own taps-as-clicks.

union SDL_Event;

namespace gos_touch {

// at start, before the first event
void init();
// SDL_FINGERDOWN, SDL_FINGERMOTION, SDL_FINGERUP
void handleEvent(const SDL_Event* event);
// a real mouse moved or its button was pressed or released
// (SDL_MOUSEMOTION, SDL_MOUSEBUTTONDOWN/UP): the cursor is the mouse's again
void mouseUsed(const SDL_Event* event);
// the window lost focus: let go of everything, with no click
void cancel();
// every frame, after the events and before the input is sampled
void poll();

}

#endif // GOS_TOUCH_H
