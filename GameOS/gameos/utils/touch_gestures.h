#ifndef TOUCH_GESTURES_H
#define TOUCH_GESTURES_H

// Touch screen gestures turned into the mouse and keys the game already reads
// (planning/future-features/MOBILE.md, phase 1). No SDL and no GL, so it can
// be unit tested with recorded finger tracks; gos_touch.cpp feeds it SDL's
// finger events and applies what it produces.
//
// - One finger is the mouse with its left button. Where it lands the cursor
//   goes at once (the game shows what a click would do), but the button only
//   goes down when the finger has moved past the slop or stayed down for the
//   press delay, so a second finger landing just after the first still makes
//   a two finger gesture rather than a click. A finger lifted before then is a
//   tap: press and release at once. Fingers added while the button is down
//   are ignored until every finger is up.
// - Two fingers move the camera: spreading or pinching them turns the mouse
//   wheel a step at a time (up to zoom in, as the game reads the wheel), and
//   moving them together past the dead zone holds a camera scroll direction
//   for as long as they stay there.
//
// Positions are in drawable pixels, times in seconds, finger ids as SDL gives
// them.

#include <cstdint>
#include <vector>

namespace touch {

struct Settings {
	float slop = 12.0f;             // pixels a finger may wander and still tap
	double pressDelay = 0.08;       // seconds before a still finger presses
	float pinchStep = 1.25f;        // finger distance ratio per wheel step
	float panDeadZone = 40.0f;      // pixels the pair moves before scrolling
};

// the mouse as the touch screen moves it, in order: replay each one
struct PointerEvent {
	float x;
	float y;
	bool left;  // left button down
};

class Gestures {
public:
	explicit Gestures(const Settings& settings = Settings());
	// for the gestures to come; set while no finger is down
	void setSettings(const Settings& settings) { settings_ = settings; }

	void down(int64_t finger, float x, float y, double time);
	void move(int64_t finger, float x, float y, double time);
	void up(int64_t finger, float x, float y, double time);
	// every frame, after the frame's finger events: a finger held still past
	// the press delay presses, and the camera pair's zoom and scroll are
	// worked out here, once both fingers' moves of the frame are in (SDL
	// sends them one finger at a time)
	void update(double time);
	// every finger gone at once (the app lost focus): the button goes up and
	// the camera stops, with no click
	void cancel();

	// the pointer moves since the last call, oldest first
	std::vector<PointerEvent> takePointerEvents();
	// wheel steps since the last call: positive spread the fingers (zoom in)
	int takeWheelSteps();
	// the way the camera should scroll while two fingers are held away from
	// where they landed: -1, 0 or 1. The map follows the fingers, so fingers
	// moved right scroll the camera left (-1), fingers moved down scroll it up
	// (-1).
	int scrollX() const { return scrollX_; }
	int scrollY() const { return scrollY_; }
	// a finger is on the screen
	bool touching() const { return !fingers_.empty(); }

private:
	enum class Mode {
		Idle,       // no finger
		Pending,    // one finger, button not down yet
		Pointer,    // one finger is the mouse, button down
		Camera,     // two fingers move the camera
		Finished    // the gesture is over; waiting for every finger to lift
	};

	struct Finger {
		int64_t id;
		float x, y;
	};

	Finger* find(int64_t finger);
	void remove(int64_t finger);
	void emit(float x, float y, bool left);
	void press();
	void startCamera();
	void updateCamera();
	void stopCamera();

	Settings settings_;
	Mode mode_ = Mode::Idle;
	std::vector<Finger> fingers_;
	std::vector<PointerEvent> events_;
	// the first finger: where and when it landed, and the cursor
	int64_t pointerId_ = 0;
	float startX_ = 0.0f, startY_ = 0.0f;
	double startTime_ = 0.0;
	// the camera pair, fingers_[0] and [1] (later fingers go after them, and
	// either of the pair lifting ends it): where their middle landed, how far
	// apart they landed, and how many wheel steps from there they have made
	float cameraX_ = 0.0f, cameraY_ = 0.0f;
	float baseDistance_ = 1.0f;
	int level_ = 0;
	int wheelSteps_ = 0;
	int scrollX_ = 0, scrollY_ = 0;
};

} // namespace touch

#endif // TOUCH_GESTURES_H
