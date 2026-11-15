#ifndef TOUCH_BAR_H
#define TOUCH_BAR_H

// The touch screen's command bar in a mission (MOBILE.md, phase 2): buttons
// for what a mouse player does with the keyboard and the 2001 control panel
// has no button for. No SDL and no GL, so it can be unit tested; gos_touch.cpp
// hit-tests fingers against it and turns what it makes into GameOS's virtual
// keys, and the game draws it (gos_GetTouchBar).
//
// - Waypoints (Ctrl) and Add to selection (Shift) are held from a tap until
//   the next tap on them: a route takes several taps on the map, and so
//   does picking several units. The mission sends a route when Ctrl goes up.
// - Force fire (F) is held from a tap until the next order on the map, and
//   through that order's frame: the game fires on what is clicked while F is
//   down, checking it on the button's release.
// - Select visible (E), Next group (grave) and New group (Ctrl + grave) are
//   taps: the key is pressed once, with only the modifiers it needs, for one
//   frame; the held ones come back the frame after, except Waypoints, which
//   the tap ends (its Ctrl going up has sent the route).
//
// Positions are in drawable pixels.

namespace touch {

enum class BarCommand {
	Waypoints,
	AddToSelection,
	ForceFire,
	SelectVisible,
	NextGroup,
	NewGroup,
	Count
};

struct BarRect {
	float x, y, w, h;
	bool contains(float px, float py) const
	{
		return px >= x && px < x + w && py >= y && py < y + h;
	}
};

// the keys to hold this frame
struct BarKeys {
	bool ctrl = false;
	bool shift = false;
	bool force = false;      // the force fire key
	bool tap = false;        // a tap's key goes down this frame and up after
	BarCommand tapCommand = BarCommand::Count;
};

class Bar {
public:
	static const int kButtons = (int)BarCommand::Count;

	// a column on the right edge, clear of the 2001 interface (control panel
	// and map along the bottom, messages at the top), buttons sized from the
	// screen's height
	void layout(float width, float height);
	BarRect rect(int button) const { return rects_[button]; }
	static const char* label(int button);
	// the button under a point, or -1
	int hit(float x, float y) const;

	void press(int button);
	// lit: a held command is on, or a tap is waiting to go
	bool lit(int button) const;
	// the map took an order (a click or a drag ended there): force fire ends
	void ordered();
	// everything off: the mission stopped taking orders, or a mouse took over
	void clear();
	// the keys for this frame; a waiting tap is handed out once
	BarKeys take();

private:
	BarRect rects_[kButtons] = {};
	bool held_[kButtons] = {};
	int tap_ = -1;  // a tap waiting to go
	bool forceEnding_ = false;  // an order was given: force fire goes up after this frame
};

} // namespace touch

#endif // TOUCH_BAR_H
