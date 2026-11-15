#include "touch_bar.h"

namespace touch {

namespace {

// shares of the screen's height
const float kSize = 0.085f;     // a button's side: about 6 mm on a phone
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
	const float x = width - kMargin * height - size;
	float y = kTop * height;
	for(int i = 0; i < kButtons; ++i) {
		rects_[i] = BarRect{x, y, size, size};
		y += size + kGap * height;
	}
}

const char* Bar::label(int button)
{
	switch((BarCommand)button) {
	case BarCommand::Waypoints:      return "Waypts";
	case BarCommand::AddToSelection: return "Add";
	case BarCommand::ForceFire:      return "Force";
	case BarCommand::SelectVisible:  return "Visible";
	case BarCommand::NextGroup:      return "Group";
	case BarCommand::NewGroup:       return "New grp";
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
	if(isHeld((BarCommand)button))
		held_[button] = !held_[button];
	else
		tap_ = button;
}

bool Bar::lit(int button) const
{
	if(button < 0 || button >= kButtons)
		return false;
	return held_[button] || tap_ == button;
}

void Bar::ordered()
{
	held_[(int)BarCommand::ForceFire] = false;
}

void Bar::clear()
{
	for(bool& h : held_)
		h = false;
	tap_ = -1;
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
	return keys;
}

} // namespace touch
