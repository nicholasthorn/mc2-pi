#include "touch_bar.h"

namespace touch {

namespace {

// shares of the screen's height
const float kSize = 0.085f;     // a button's height: about 6 mm on a phone
// its width, to its height: the mission draws the label in its own 800x600,
// which a 20:9 screen squeezes to under half the width per pixel
const float kAspect = 1.5f;
const float kGap = 0.012f;
const float kMargin = 0.012f;   // from the right edge
const float kTop = 0.14f;       // below the message lines

bool isHeld(BarCommand c)
{
	return c == BarCommand::Waypoints || c == BarCommand::AddToSelection || c == BarCommand::ForceFire;
}

} // namespace

void Bar::layout(float width, float height)
{
	const float size = kSize * height;
	const float x = width - kMargin * height - kAspect * size;
	float y = kTop * height;
	for(int i = 0; i < kButtons; ++i) {
		rects_[i] = BarRect{x, y, kAspect * size, size};
		y += size + kGap * height;
	}
}

const char* Bar::label(int button)
{
	switch((BarCommand)button) {
	case BarCommand::Waypoints:      return "Wpts";
	case BarCommand::AddToSelection: return "Add";
	case BarCommand::ForceFire:      return "Force";
	case BarCommand::SelectVisible:  return "Vis";
	case BarCommand::NextGroup:      return "Grp";
	case BarCommand::NewGroup:       return "New";
	case BarCommand::Count:          break;
	}
	return "";
}

int Bar::hit(float x, float y) const
{
	for(int i = 0; i < kButtons; ++i)
		if(rects_[i].w > 0.0f && rects_[i].contains(x, y))
			return i;
	return -1;
}

void Bar::press(int button)
{
	if(button < 0 || button >= kButtons)
		return;
	if(isHeld((BarCommand)button)) {
		held_[button] = !held_[button];
		if(button == (int)BarCommand::ForceFire)
			forceEnding_ = false;
	} else {
		tap_ = button;
		// the tap takes Ctrl up, which sends the route: it ends here
		held_[(int)BarCommand::Waypoints] = false;
	}
}

bool Bar::lit(int button) const
{
	if(button < 0 || button >= kButtons)
		return false;
	return held_[button] || tap_ == button;
}

void Bar::ordered()
{
	if(held_[(int)BarCommand::ForceFire])
		forceEnding_ = true;
}

void Bar::clear()
{
	for(bool& h : held_)
		h = false;
	tap_ = -1;
	forceEnding_ = false;
}

BarKeys Bar::take()
{
	BarKeys keys;
	if(tap_ >= 0) {
		keys.tap = true;
		keys.tapCommand = (BarCommand)tap_;
		keys.ctrl = keys.tapCommand == BarCommand::NewGroup;
		tap_ = -1;
		return keys;
	}
	keys.ctrl = held_[(int)BarCommand::Waypoints];
	keys.shift = held_[(int)BarCommand::AddToSelection];
	keys.force = held_[(int)BarCommand::ForceFire];
	// down through the order's own frame, up from the next
	if(forceEnding_) {
		held_[(int)BarCommand::ForceFire] = false;
		forceEnding_ = false;
	}
	return keys;
}

} // namespace touch
