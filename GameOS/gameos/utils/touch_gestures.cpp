#include "touch_gestures.h"

#include <algorithm>
#include <cmath>

namespace touch {

namespace {

float distance(float x0, float y0, float x1, float y1)
{
	return std::hypot(x1 - x0, y1 - y0);
}

} // namespace

Gestures::Gestures(const Settings& settings) : settings_(settings) {}

Gestures::Finger* Gestures::find(int64_t finger)
{
	for(Finger& f : fingers_)
		if(f.id == finger)
			return &f;
	return nullptr;
}

void Gestures::remove(int64_t finger)
{
	fingers_.erase(std::remove_if(fingers_.begin(), fingers_.end(),
		[finger](const Finger& f) { return f.id == finger; }), fingers_.end());
}

void Gestures::emit(float x, float y, bool left)
{
	events_.push_back(PointerEvent{x, y, left});
}

// the button goes down where the finger landed: a drag starts there
void Gestures::press()
{
	mode_ = Mode::Pointer;
	emit(startX_, startY_, true);
}

void Gestures::down(int64_t finger, float x, float y, double time)
{
	if(find(finger))
		return;
	fingers_.push_back(Finger{finger, x, y});
	switch(mode_) {
	case Mode::Idle:
		mode_ = Mode::Pending;
		pointerId_ = finger;
		startX_ = x;
		startY_ = y;
		startTime_ = time;
		emit(x, y, false);
		break;
	case Mode::Pending:
		startCamera();
		break;
	case Mode::Pointer:
	case Mode::Camera:
	case Mode::Finished:
		break;  // ignored until every finger is up
	}
}

void Gestures::move(int64_t finger, float x, float y, double /*time*/)
{
	Finger* f = find(finger);
	if(!f)
		return;
	f->x = x;
	f->y = y;
	if(finger != pointerId_)
		return;
	if(mode_ == Mode::Pending) {
		if(distance(startX_, startY_, x, y) > settings_.slop) {
			press();
			emit(x, y, true);
		} else {
			emit(x, y, false);
		}
	} else if(mode_ == Mode::Pointer) {
		emit(x, y, true);
	}
	// the camera pair is looked at once a frame, in update
}

void Gestures::up(int64_t finger, float x, float y, double /*time*/)
{
	if(!find(finger))
		return;
	switch(mode_) {
	case Mode::Pending:
		// a tap: where it landed, as a wobble within the slop is not a move
		emit(startX_, startY_, true);
		emit(startX_, startY_, false);
		mode_ = Mode::Finished;
		break;
	case Mode::Pointer:
		if(finger == pointerId_) {
			emit(x, y, false);
			mode_ = Mode::Finished;
		}
		break;
	case Mode::Camera:
		if(finger == fingers_[0].id || finger == fingers_[1].id) {
			stopCamera();
			mode_ = Mode::Finished;
		}
		break;
	case Mode::Idle:
	case Mode::Finished:
		break;
	}
	remove(finger);
	if(fingers_.empty())
		mode_ = Mode::Idle;
}

void Gestures::update(double time)
{
	if(mode_ == Mode::Pending && time - startTime_ >= settings_.pressDelay)
		press();
	else if(mode_ == Mode::Camera)
		updateCamera();
}

void Gestures::cancel()
{
	if(mode_ == Mode::Pointer) {   // the pointer finger is still down
		const Finger* f = find(pointerId_);
		emit(f->x, f->y, false);
	}
	stopCamera();
	fingers_.clear();
	mode_ = Mode::Idle;
}

std::vector<PointerEvent> Gestures::takePointerEvents()
{
	std::vector<PointerEvent> events;
	events.swap(events_);
	return events;
}

int Gestures::takeWheelSteps()
{
	const int steps = wheelSteps_;
	wheelSteps_ = 0;
	return steps;
}

// the first two fingers are the pair
void Gestures::startCamera()
{
	mode_ = Mode::Camera;
	const Finger& a = fingers_[0];
	const Finger& b = fingers_[1];
	cameraX_ = 0.5f * (a.x + b.x);
	cameraY_ = 0.5f * (a.y + b.y);
	baseDistance_ = std::max(distance(a.x, a.y, b.x, b.y), 1.0f);
	level_ = 0;
}

void Gestures::updateCamera()
{
	const Finger* a = &fingers_[0];
	const Finger* b = &fingers_[1];

	// zoom: steps of the spacing as a ratio of where they landed. A step is
	// taken once the spacing reaches the next one, and undone only at the one
	// below, so a pair held near a boundary does not flicker the zoom.
	const float spacing = std::max(distance(a->x, a->y, b->x, b->y), 1.0f);
	const float steps = std::log(spacing / baseDistance_) / std::log(settings_.pinchStep);
	int level = level_;
	if(steps >= (float)(level_ + 1))
		level = (int)std::floor(steps);
	else if(steps <= (float)(level_ - 1))
		level = (int)std::ceil(steps);
	wheelSteps_ += level - level_;
	level_ = level;

	// scroll: the middle held away from where it landed; the map follows the
	// fingers, so the camera goes the other way
	const float dx = 0.5f * (a->x + b->x) - cameraX_;
	const float dy = 0.5f * (a->y + b->y) - cameraY_;
	const float zone = settings_.panDeadZone;
	scrollX_ = dx > zone ? -1 : (dx < -zone ? 1 : 0);
	scrollY_ = dy > zone ? -1 : (dy < -zone ? 1 : 0);
}

void Gestures::stopCamera()
{
	scrollX_ = 0;
	scrollY_ = 0;
}

} // namespace touch
