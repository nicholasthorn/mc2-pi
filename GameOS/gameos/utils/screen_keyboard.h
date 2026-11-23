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
// (A field lets go of the focus on Return, and a dialog then takes Return as
// its OK, so Enter also confirms a dialog.)
//
// Positions are shares of the game's screen (0 to 1), y down.

#include <cstdint>

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

	// next to the field being typed in (top and bottom as shares of the
	// screen's height): below it when it is in the upper half of the screen,
	// above it otherwise, so the rest of a dialog (its buttons) stays clear
	// where it can
	void layout(float fieldTop, float fieldBottom);
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

// Whether the keyboard shows. A field taking the focus shows it. A field
// letting go hides it one sample later, unless a field takes the focus
// meanwhile or a focused field updates after the first sample: one frame's
// fields update in no set order, so the field clicked can take the focus
// before the one it replaces lets go of it. A focused field
// that stops updating without letting go (its dialog closed with Escape)
// hides it too: once the field has updated (a dialog's fields start after
// its opening animation), kMissedSamples samples in a row without an update.
class Visibility {
public:
	static const int kMissedSamples = 4;
	enum Change { None, Shown, Hidden, FieldGone };

	// a field took (true) or lost (false) the focus: Shown when it was hidden
	Change focus(bool on);
	// the focused field updated
	void fieldUpdated() { updated_ = true; }
	// Close or Enter on the keyboard: hidden at once; the field keeps the focus
	void close();
	bool shown() const { return shown_; }
	// at each input sample: Hidden or FieldGone when it goes now. Without
	// gameUpdated (the game's logic did not run since the last sample, so no
	// field could update) nothing changes.
	Change sample(bool gameUpdated = true);

private:
	bool shown_ = false;
	bool pending_ = false;  // the focus was let go
	int pendingSamples_ = 0;
	bool seen_ = false;     // the field has updated since it showed
	bool updated_ = false;  // ... since the last sample
	int missed_ = 0;
};

// The pointer's buttons over the keyboard. A press that starts on the shown
// keyboard is the keyboard's, not the game's, until it is let go, even if
// the keyboard goes away meanwhile (Close, Enter): the game sees the button
// up all along. Buttons as bits (0 left, 1 middle, 2 right).
class ButtonTaker {
public:
	// at each input sample: the buttons that went down at this sample and
	// those down (just pressed or held), and whether the pointer is on the
	// shown keyboard. Returns the buttons to hide from the game; leftPress
	// says this sample's left press is the keyboard's (a key, if one is under
	// the pointer).
	uint32_t sample(uint32_t pressed, uint32_t down, bool onKeyboard, bool& leftPress);

private:
	uint32_t taken_ = 0;
};

} // namespace screen_kb

#endif // SCREEN_KEYBOARD_H
