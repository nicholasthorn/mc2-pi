#ifndef SCREEN_KEYBOARD_H
#define SCREEN_KEYBOARD_H

// The keyboard GameOS draws over the interface for a text field where there
// is no real one: in the VR headset (the Quest's immersive OpenXR session
// shows no Android keyboard), or anywhere with MC2_SCREEN_KEYBOARD=1. A press
// of its pointer on a key becomes that key's press, as from a real keyboard.
// No SDL and no GL, so it can be unit tested; gos_screen_keyboard.cpp takes
// the pointer's presses on it and the game draws it (gos_GetScreenKeyboard).
//
// Letters, digits, space, '-', '.', Backspace and Enter; Shift capitalises
// the next letter (one shot, as on a phone). Enter and Close put it away;
// the field keeps its focus, and a click on it brings the keyboard back.
//
// Positions are shares of the game's screen (0 to 1), y down.

namespace screen_kb {

// SDL scancodes (USB HID usages), which gos_screen_keyboard.cpp checks
// against SDL's own
const int kScanA = 4;           // to Z = 29
const int kScan1 = 30;          // to 9 = 38
const int kScan0 = 39;
const int kScanReturn = 40;
const int kScanBackspace = 42;
const int kScanSpace = 44;
const int kScanMinus = 45;
const int kScanPeriod = 55;
const int kScanLShift = 225;

struct Rect {
	float x, y, w, h;
	bool contains(float px, float py) const
	{
		return px >= x && px < x + w && py >= y && py < y + h;
	}
};

// what a press on a key does
struct Press {
	int scancode = 0;       // a key to press once (0: none)
	bool shift = false;     // with Shift held
	bool close = false;     // put the keyboard away (after the key, if any)
};

class Keyboard {
public:
	static const int kKeys = 43;

	// along the bottom of the screen, or along the top when the field being
	// typed in would be under it
	void layout(bool top);
	Rect panel() const { return panel_; }
	Rect rect(int key) const { return rects_[key]; }
	// as it reads now: letters in capitals while Shift is on
	const char* label(int key) const;
	// Shift is on
	bool lit(int key) const;
	// the key under a point, or -1 (a gap, or off the keyboard)
	int hit(float x, float y) const;
	bool contains(float x, float y) const { return panel_.contains(x, y); }

	Press press(int key);
	// Shift off: the keyboard was put away
	void reset() { shift_ = false; }

private:
	Rect panel_ = {};
	Rect rects_[kKeys] = {};
	bool shift_ = false;
};

} // namespace screen_kb

#endif // SCREEN_KEYBOARD_H
