// tests/unit/test_touch_gestures.cpp
//
// touch::Gestures: finger tracks in, mouse and camera out (MOBILE.md,
// phase 1). One finger is the left button, after a short delay or some
// movement so a second finger can still join; two fingers zoom with the
// wheel and scroll the camera. A wrong rule here turns every two finger
// pinch into a stray click, so the timing and the directions are pinned.

#include "doctest.h"

#include "utils/touch_gestures.h"

#include <cmath>

namespace {

using touch::Gestures;
using touch::PointerEvent;

// the defaults, written out so the tests read without the header
touch::Settings settings()
{
	touch::Settings s;
	s.slop = 12.0f;
	s.pressDelay = 0.08;
	s.pinchStep = 1.25f;
	s.panDeadZone = 40.0f;
	return s;
}

bool same(const PointerEvent& e, float x, float y, bool left)
{
	return std::fabs(e.x - x) < 0.01f && std::fabs(e.y - y) < 0.01f && e.left == left;
}

} // namespace

TEST_SUITE("TouchGestures") {

TEST_CASE("a finger landing moves the cursor there without pressing")
{
	Gestures g(settings());
	g.down(1, 100.0f, 200.0f, 0.0);
	const auto events = g.takePointerEvents();
	REQUIRE(events.size() == 1);
	CHECK(same(events[0], 100.0f, 200.0f, false));
	CHECK(g.touching());
	CHECK(g.takePointerEvents().empty());   // taken once
}

TEST_CASE("a tap is a press and a release where the finger was")
{
	Gestures g(settings());
	g.down(1, 100.0f, 200.0f, 0.0);
	g.up(1, 103.0f, 198.0f, 0.05);   // within the slop and the delay
	const auto events = g.takePointerEvents();
	REQUIRE(events.size() == 3);
	CHECK(same(events[0], 100.0f, 200.0f, false));
	CHECK(same(events[1], 100.0f, 200.0f, true));
	CHECK(same(events[2], 100.0f, 200.0f, false));
	CHECK_FALSE(g.touching());
}

TEST_CASE("a finger held still presses after the delay, and releases where it lifts")
{
	Gestures g(settings());
	g.down(1, 100.0f, 200.0f, 0.0);
	g.takePointerEvents();

	g.update(0.05);
	CHECK(g.takePointerEvents().empty());   // not yet

	g.update(0.09);
	auto events = g.takePointerEvents();
	REQUIRE(events.size() == 1);
	CHECK(same(events[0], 100.0f, 200.0f, true));

	g.move(1, 300.0f, 250.0f, 0.2);
	g.up(1, 300.0f, 250.0f, 0.3);
	events = g.takePointerEvents();
	REQUIRE(events.size() == 2);
	CHECK(same(events[0], 300.0f, 250.0f, true));    // a drag
	CHECK(same(events[1], 300.0f, 250.0f, false));
}

TEST_CASE("moving past the slop presses where the finger landed, then drags")
{
	Gestures g(settings());
	g.down(1, 100.0f, 200.0f, 0.0);
	g.takePointerEvents();

	g.move(1, 105.0f, 200.0f, 0.01);   // inside the slop: the cursor follows
	auto events = g.takePointerEvents();
	REQUIRE(events.size() == 1);
	CHECK(same(events[0], 105.0f, 200.0f, false));

	g.move(1, 130.0f, 200.0f, 0.02);   // past it
	events = g.takePointerEvents();
	REQUIRE(events.size() == 2);
	CHECK(same(events[0], 100.0f, 200.0f, true));
	CHECK(same(events[1], 130.0f, 200.0f, true));
}

TEST_CASE("a second finger before the press makes a camera gesture, with no click")
{
	Gestures g(settings());
	g.down(1, 100.0f, 200.0f, 0.0);
	g.down(2, 200.0f, 200.0f, 0.03);
	g.update(0.5);
	g.up(1, 100.0f, 200.0f, 0.6);
	g.up(2, 200.0f, 200.0f, 0.6);
	for(const PointerEvent& e : g.takePointerEvents())
		CHECK_FALSE(e.left);
	CHECK(g.takeWheelSteps() == 0);
	CHECK_FALSE(g.touching());
}

TEST_CASE("spreading two fingers zooms in a step per ratio, pinching zooms out")
{
	Gestures g(settings());
	g.down(1, 100.0f, 200.0f, 0.0);
	g.down(2, 200.0f, 200.0f, 0.01);   // 100 apart, middle at 150

	// 200 apart, middle still at 150: 2.0 = 1.25^3.1, three steps
	g.move(1, 50.0f, 200.0f, 0.1);
	g.move(2, 250.0f, 200.0f, 0.1);
	g.update(0.1);
	CHECK(g.takeWheelSteps() == 3);
	CHECK(g.takeWheelSteps() == 0);   // taken once

	// back to 100 apart: three steps out, back where they landed
	g.move(1, 100.0f, 200.0f, 0.2);
	g.move(2, 200.0f, 200.0f, 0.2);
	g.update(0.2);
	CHECK(g.takeWheelSteps() == -3);

	// spreading does not scroll the camera: the middle has not moved
	CHECK(g.scrollX() == 0);
	CHECK(g.scrollY() == 0);
}

TEST_CASE("moving two fingers together scrolls the camera against them")
{
	Gestures g(settings());
	g.down(1, 100.0f, 200.0f, 0.0);
	g.down(2, 200.0f, 200.0f, 0.01);

	g.move(1, 130.0f, 200.0f, 0.1);
	g.move(2, 230.0f, 200.0f, 0.1);   // middle 30 right: inside the dead zone
	g.update(0.1);
	CHECK(g.scrollX() == 0);

	// one finger moved and the other not yet, within a frame: nothing until
	// the frame's update, which sees both
	g.move(1, 160.0f, 260.0f, 0.2);
	CHECK(g.scrollX() == 0);
	g.move(2, 260.0f, 260.0f, 0.2);   // middle 60 right, 60 down
	g.update(0.2);
	CHECK(g.scrollX() == -1);         // the map follows the fingers
	CHECK(g.scrollY() == -1);
	CHECK(g.takeWheelSteps() == 0);   // still 100 apart

	g.move(1, 40.0f, 140.0f, 0.3);
	g.move(2, 140.0f, 140.0f, 0.3);   // middle 60 left, 60 up
	g.update(0.3);
	CHECK(g.takeWheelSteps() == 0);   // the pair never changed its spacing in a frame
	CHECK(g.scrollX() == 1);
	CHECK(g.scrollY() == 1);

	g.up(1, 40.0f, 140.0f, 0.4);      // one finger off ends the gesture
	CHECK(g.scrollX() == 0);
	CHECK(g.scrollY() == 0);
	CHECK(g.touching());
}

TEST_CASE("after a camera gesture the last finger does nothing until a new touch")
{
	Gestures g(settings());
	g.down(1, 100.0f, 200.0f, 0.0);
	g.down(2, 200.0f, 200.0f, 0.01);
	g.up(2, 200.0f, 200.0f, 0.2);
	g.takePointerEvents();

	g.move(1, 400.0f, 200.0f, 0.3);
	g.update(1.0);
	g.up(1, 400.0f, 200.0f, 1.1);
	for(const PointerEvent& e : g.takePointerEvents())
		CHECK_FALSE(e.left);
	CHECK_FALSE(g.touching());

	// a fresh finger taps again
	g.down(3, 50.0f, 60.0f, 2.0);
	g.up(3, 50.0f, 60.0f, 2.02);
	const auto events = g.takePointerEvents();
	REQUIRE(events.size() == 3);
	CHECK(same(events[1], 50.0f, 60.0f, true));
}

TEST_CASE("a finger added while the button is down is ignored")
{
	Gestures g(settings());
	g.down(1, 100.0f, 200.0f, 0.0);
	g.update(0.1);                     // pressed
	g.takePointerEvents();

	g.down(2, 300.0f, 300.0f, 0.2);
	g.move(2, 350.0f, 300.0f, 0.25);
	g.up(2, 350.0f, 300.0f, 0.3);
	CHECK(g.takePointerEvents().empty());
	CHECK(g.takeWheelSteps() == 0);

	g.move(1, 120.0f, 200.0f, 0.4);
	g.up(1, 120.0f, 200.0f, 0.5);
	const auto events = g.takePointerEvents();
	REQUIRE(events.size() == 2);
	CHECK(same(events[0], 120.0f, 200.0f, true));
	CHECK(same(events[1], 120.0f, 200.0f, false));
}

TEST_CASE("moves and lifts of unknown fingers are ignored")
{
	Gestures g(settings());
	g.move(7, 10.0f, 10.0f, 0.0);
	g.up(7, 10.0f, 10.0f, 0.0);
	CHECK(g.takePointerEvents().empty());
	CHECK_FALSE(g.touching());

	g.down(1, 100.0f, 200.0f, 0.1);
	g.down(1, 100.0f, 200.0f, 0.1);    // a repeated down is not a second finger
	g.up(1, 100.0f, 200.0f, 0.12);
	const auto events = g.takePointerEvents();
	REQUIRE(events.size() == 3);
	CHECK(same(events[1], 100.0f, 200.0f, true));
}

TEST_CASE("a third finger does not change a camera gesture")
{
	Gestures g(settings());
	g.down(1, 100.0f, 200.0f, 0.0);
	g.down(2, 200.0f, 200.0f, 0.01);
	g.down(3, 500.0f, 500.0f, 0.02);
	g.move(3, 900.0f, 900.0f, 0.1);
	g.update(0.1);
	CHECK(g.takeWheelSteps() == 0);
	CHECK(g.scrollX() == 0);
	g.up(3, 900.0f, 900.0f, 0.2);
	CHECK(g.touching());
}

TEST_CASE("cancel lifts the button and stops the camera without a click")
{
	Gestures g(settings());
	g.down(1, 100.0f, 200.0f, 0.0);
	g.update(0.1);
	g.takePointerEvents();
	g.cancel();
	auto events = g.takePointerEvents();
	REQUIRE(events.size() == 1);
	CHECK(same(events[0], 100.0f, 200.0f, false));
	CHECK_FALSE(g.touching());

	// a pending finger cancelled does not click
	g.down(2, 10.0f, 20.0f, 1.0);
	g.cancel();
	for(const PointerEvent& e : g.takePointerEvents())
		CHECK_FALSE(e.left);

	// nor does a camera pair
	g.down(3, 100.0f, 200.0f, 2.0);
	g.down(4, 200.0f, 200.0f, 2.01);
	g.move(3, 200.0f, 300.0f, 2.1);
	g.move(4, 300.0f, 300.0f, 2.1);
	g.update(2.1);
	CHECK(g.scrollX() == -1);
	g.cancel();
	CHECK(g.scrollX() == 0);
	CHECK(g.scrollY() == 0);
	CHECK_FALSE(g.touching());
}

}
