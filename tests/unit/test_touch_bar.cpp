// tests/unit/test_touch_bar.cpp
//
// touch::Bar: the touch screen's command bar in a mission (MOBILE.md,
// phase 2). Where its buttons are, what a finger on them hits, and which
// keys it holds each frame. The mission matches modifiers exactly (Ctrl +
// grave is another command than grave), so a tap must go out with only its
// own modifiers, whatever is held.

#include "doctest.h"

#include "utils/touch_bar.h"

#include <cstring>

using touch::Bar;
using touch::BarCommand;
using touch::BarKeys;
using touch::BarRect;

namespace {

int index(BarCommand c)
{
	return (int)c;
}

// a 20:9 phone and a 4:3 tablet
const float kPhoneW = 2400.0f, kPhoneH = 1080.0f;
const float kTabletW = 2048.0f, kTabletH = 1536.0f;

} // namespace

TEST_SUITE("TouchBar") {

TEST_CASE("the buttons are a column of equal buttons on the right edge")
{
	Bar bar;
	bar.layout(kPhoneW, kPhoneH);
	const BarRect first = bar.rect(0);
	// wider than tall: the mission draws the labels in its own 800x600, which
	// a 20:9 screen squeezes to under half the width per pixel
	CHECK(first.w == doctest::Approx(1.5f * first.h));
	const float gameWidth = first.w / kPhoneW * 800.0f;
	CHECK(gameWidth > 45.0f);                     // room for five characters
	CHECK(first.h > 0.06f * kPhoneH);            // big enough for a finger
	for(int i = 0; i < Bar::kButtons; ++i) {
		const BarRect r = bar.rect(i);
		CHECK(r.x == doctest::Approx(first.x));
		CHECK(r.w == doctest::Approx(first.w));
		CHECK(r.x + r.w <= kPhoneW);
		CHECK(r.x + r.w > 0.97f * kPhoneW);      // at the edge
		if(i > 0)
			CHECK(r.y >= bar.rect(i - 1).y + bar.rect(i - 1).h);   // below, not overlapping
	}
	// clear of the messages at the top and the control panel at the bottom
	CHECK(first.y > 0.1f * kPhoneH);
	const BarRect last = bar.rect(Bar::kButtons - 1);
	CHECK(last.y + last.h < 0.75f * kPhoneH);

	// the same share of the height on a tablet
	Bar tablet;
	tablet.layout(kTabletW, kTabletH);
	CHECK(tablet.rect(0).h / kTabletH == doctest::Approx(first.h / kPhoneH));
}

TEST_CASE("a finger hits the button under it, and nothing beside the bar")
{
	Bar bar;
	bar.layout(kPhoneW, kPhoneH);
	for(int i = 0; i < Bar::kButtons; ++i) {
		const BarRect r = bar.rect(i);
		CHECK(bar.hit(r.x + 0.5f * r.w, r.y + 0.5f * r.h) == i);
	}
	CHECK(bar.hit(0.5f * kPhoneW, 0.5f * kPhoneH) == -1);
	CHECK(bar.hit(kPhoneW - 1.0f, 0.02f * kPhoneH) == -1);   // above it
	Bar unlaid;
	CHECK(unlaid.hit(10.0f, 10.0f) == -1);                   // no layout yet: nothing
}

TEST_CASE("waypoints and add are held from a tap to the next tap on them")
{
	Bar bar;
	BarKeys k = bar.take();
	CHECK_FALSE(k.ctrl);
	CHECK_FALSE(k.shift);

	bar.press(index(BarCommand::Waypoints));
	bar.press(index(BarCommand::AddToSelection));
	CHECK(bar.lit(index(BarCommand::Waypoints)));
	for(int frame = 0; frame < 3; ++frame) {
		bar.ordered();                   // several taps on the map
		k = bar.take();
		CHECK(k.ctrl);
		CHECK(k.shift);
		CHECK_FALSE(k.tap);
	}
	bar.press(index(BarCommand::Waypoints));
	k = bar.take();
	CHECK_FALSE(k.ctrl);
	CHECK(k.shift);
	CHECK_FALSE(bar.lit(index(BarCommand::Waypoints)));
}

TEST_CASE("force fire is held until the next order")
{
	Bar bar;
	bar.press(index(BarCommand::ForceFire));
	CHECK(bar.take().force);
	CHECK(bar.take().force);         // held, frame after frame
	// the order's own frame: still down, as the mission gives the order on
	// the button's release and checks F is held then
	bar.ordered();
	CHECK(bar.take().force);
	CHECK_FALSE(bar.take().force);   // the frame after
	CHECK_FALSE(bar.lit(index(BarCommand::ForceFire)));

	// tapped twice before an order: off again
	bar.press(index(BarCommand::ForceFire));
	bar.press(index(BarCommand::ForceFire));
	CHECK_FALSE(bar.take().force);
}

TEST_CASE("a tap goes out once, with only its own modifiers, then the held ones return")
{
	Bar bar;
	bar.press(index(BarCommand::AddToSelection));  // Shift held
	bar.press(index(BarCommand::ForceFire));
	bar.press(index(BarCommand::SelectVisible));
	CHECK(bar.lit(index(BarCommand::SelectVisible)));

	BarKeys k = bar.take();
	CHECK(k.tap);
	CHECK(k.tapCommand == BarCommand::SelectVisible);
	CHECK_FALSE(k.ctrl);       // E alone, not Shift + E
	CHECK_FALSE(k.shift);
	CHECK_FALSE(k.force);
	CHECK_FALSE(bar.lit(index(BarCommand::SelectVisible)));

	k = bar.take();
	CHECK_FALSE(k.tap);
	CHECK(k.shift);
	CHECK(k.force);
}

TEST_CASE("a tap ends a route: Waypoints goes out rather than coming back")
{
	// Ctrl up sends the route (the mission runs its queued waypoints on
	// Ctrl's release), so a tap that needs Ctrl up ends it for good, rather
	// than sending half of it and starting another with the rest
	Bar bar;
	bar.press(index(BarCommand::Waypoints));
	CHECK(bar.take().ctrl);
	bar.press(index(BarCommand::NextGroup));
	CHECK_FALSE(bar.lit(index(BarCommand::Waypoints)));
	BarKeys k = bar.take();
	CHECK(k.tap);
	CHECK_FALSE(k.ctrl);
	CHECK_FALSE(bar.take().ctrl);   // and stays up
}

TEST_CASE("new group is Ctrl + grave, next group grave alone")
{
	Bar bar;
	bar.press(index(BarCommand::NewGroup));
	BarKeys k = bar.take();
	CHECK(k.tap);
	CHECK(k.tapCommand == BarCommand::NewGroup);
	CHECK(k.ctrl);
	CHECK_FALSE(k.shift);

	bar.press(index(BarCommand::AddToSelection));
	bar.press(index(BarCommand::NextGroup));
	k = bar.take();
	CHECK(k.tapCommand == BarCommand::NextGroup);
	CHECK_FALSE(k.ctrl);
	CHECK_FALSE(k.shift);
}

TEST_CASE("a second tap before the first went out replaces it")
{
	Bar bar;
	bar.press(index(BarCommand::NextGroup));
	bar.press(index(BarCommand::SelectVisible));
	const BarKeys k = bar.take();
	CHECK(k.tapCommand == BarCommand::SelectVisible);
	CHECK_FALSE(bar.take().tap);
}

TEST_CASE("clear lets go of everything")
{
	Bar bar;
	bar.press(index(BarCommand::Waypoints));
	bar.press(index(BarCommand::ForceFire));
	bar.press(index(BarCommand::NextGroup));
	bar.clear();
	const BarKeys k = bar.take();
	CHECK_FALSE(k.ctrl);
	CHECK_FALSE(k.force);
	CHECK_FALSE(k.tap);
	for(int i = 0; i < Bar::kButtons; ++i)
		CHECK_FALSE(bar.lit(i));
}

TEST_CASE("every button has a short label")
{
	for(int i = 0; i < Bar::kButtons; ++i) {
		const char* l = Bar::label(i);
		REQUIRE(l != nullptr);
		CHECK(l[0] != '\0');
		CHECK(std::strlen(l) <= 5);   // fits a button at 800x600 on a 20:9 screen
	}
	CHECK(Bar::label(-1) != nullptr);
	CHECK(Bar::label(Bar::kButtons) != nullptr);
}

}
