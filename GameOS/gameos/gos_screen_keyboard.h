#ifndef GOS_SCREEN_KEYBOARD_H
#define GOS_SCREEN_KEYBOARD_H

// The keyboard GameOS draws over the interface for a text field where there
// is no real one (utils/screen_keyboard.h): in the VR headset, whose
// immersive session shows no Android keyboard, or anywhere with
// MC2_SCREEN_KEYBOARD=1 (to try it on a desktop). A text field taking the
// focus shows it (gos_TouchTextInput); the pointer's presses on it become
// key presses, and the game never sees those presses as clicks. The game
// draws it (gos_GetScreenKeyboard).

namespace gos_screen_keyboard {

// this keyboard stands in for the touch screen's (Android's) one
bool wanted();
// where the field taking the focus is, as shares of the screen's height
void fieldArea(float top, float bottom);
// a text field took (true) or lost (false) the focus
void show(bool on);
// the field with the focus updated: while the keyboard is up, a field that
// stops updating (its screen went away) takes the keyboard with it
void fieldActive();
// put away, the field keeping the focus (gos_TouchTextInputClose)
void close();
// at each input sample, after the mouse's update and before the keyboard's:
// takes a press on the keyboard from the game and presses its key
void sample();

} // namespace gos_screen_keyboard

#endif // GOS_SCREEN_KEYBOARD_H
