// tests/unit/test_screen_keyboard.cpp
//
// screen_kb::Keyboard: the keyboard GameOS draws over the interface for a
// text field in the VR headset. Where its keys are, what a pointer on them
// hits, and the key press each one makes: Shift capitalises one letter,
// Enter and Close put the keyboard away.

#include "doctest.h"

#include "utils/screen_keyboard.h"

#include <cstring>

using screen_kb::Keyboard;
using screen_kb::Press;
using screen_kb::Rect;
using screen_kb::Visibility;
using screen_kb::ButtonTaker;

namespace {

// the key with this label (as it reads now), or -1
int find(const Keyboard& kb, const char* label)
{
	for(int i = 0; i < Keyboard::kKeys; ++i)
		if(std::strcmp(kb.label(i), label) == 0)
			return i;
	return -1;
}

float centreX(const Rect& r) { return r.x + 0.5f * r.w; }
float centreY(const Rect& r) { return r.y + 0.5f * r.h; }

} // namespace

TEST_SUITE("ScreenKeyboard") {

TEST_CASE("the keyboard sits next to its field, on the side with more room") {
	Keyboard kb;
	// a field near the top: the keyboard just below it
	kb.layout(0.20f, 0.25f);
	Rect r = kb.panel();
	CHECK(r.y >= 0.25f);
	CHECK(r.y < 0.30f);
	// near the bottom: just above it
	kb.layout(0.80f, 0.85f);
	r = kb.panel();
	CHECK(r.y + r.h <= 0.80f);
	CHECK(r.y + r.h > 0.75f);
	// at the very top and bottom it stays on the screen
	kb.layout(0.0f, 0.0f);
	CHECK(kb.panel().y >= 0.0f);
	kb.layout(1.0f, 1.0f);
	CHECK(kb.panel().y + kb.panel().h <= 1.0f);
}

TEST_CASE("the keyboard never covers a field of ordinary height") {
	Keyboard kb;
	for(int i = 0; i <= 95; ++i) {
		const float top = 0.01f * (float)i;
		const float bottom = top + 0.05f;
		kb.layout(top, bottom);
		const Rect r = kb.panel();
		CAPTURE(top);
		CHECK(r.y >= 0.0f);
		CHECK(r.y + r.h <= 1.0f);
		const bool overlaps = r.y < bottom && r.y + r.h > top;
		CHECK_FALSE(overlaps);
	}
}

TEST_CASE("every key lies inside the panel and no two keys overlap") {
	Keyboard kb;
	kb.layout(0.2f, 0.25f);
	const Rect panel = kb.panel();
	for(int i = 0; i < Keyboard::kKeys; ++i) {
		const Rect r = kb.rect(i);
		CAPTURE(i);
		CHECK(r.w > 0.0f);
		CHECK(r.h > 0.0f);
		CHECK(r.x >= panel.x);
		CHECK(r.y >= panel.y);
		CHECK(r.x + r.w <= panel.x + panel.w + 1e-5f);
		CHECK(r.y + r.h <= panel.y + panel.h + 1e-5f);
		// a key's own centre hits that key, not another
		CHECK(kb.hit(centreX(r), centreY(r)) == i);
	}
}

TEST_CASE("off the keyboard, and between keys, hits nothing") {
	Keyboard kb;
	kb.layout(0.2f, 0.25f);
	CHECK(kb.hit(0.5f, 0.05f) == -1);
	CHECK_FALSE(kb.contains(0.5f, 0.05f));
	// the panel's own margin, left of the first row
	const Rect panel = kb.panel();
	CHECK(kb.contains(panel.x + 0.001f, panel.y + 0.001f));
	CHECK(kb.hit(panel.x + 0.001f, panel.y + 0.001f) == -1);
}

TEST_CASE("a letter, a digit and the other keys make their own key press") {
	Keyboard kb;
	kb.layout(0.2f, 0.25f);
	Press p = kb.press(find(kb, "a"));
	CHECK(p.scancode == screen_kb::kScanA);
	CHECK_FALSE(p.shift);
	CHECK_FALSE(p.close);
	CHECK(kb.press(find(kb, "z")).scancode == screen_kb::kScanA + 25);
	CHECK(kb.press(find(kb, "1")).scancode == screen_kb::kScan1);
	CHECK(kb.press(find(kb, "9")).scancode == screen_kb::kScan1 + 8);
	CHECK(kb.press(find(kb, "0")).scancode == screen_kb::kScan0);
	CHECK(kb.press(find(kb, ".")).scancode == screen_kb::kScanPeriod);
	CHECK(kb.press(find(kb, "-")).scancode == screen_kb::kScanMinus);
	CHECK(kb.press(find(kb, "Space")).scancode == screen_kb::kScanSpace);
	CHECK(kb.press(find(kb, "Back")).scancode == screen_kb::kScanBackspace);
}

TEST_CASE("every letter of the alphabet is there once") {
	Keyboard kb;
	kb.layout(0.2f, 0.25f);
	for(char c = 'a'; c <= 'z'; ++c) {
		const char label[2] = {c, 0};
		const int key = find(kb, label);
		CAPTURE(c);
		REQUIRE(key >= 0);
		CHECK(kb.press(key).scancode == screen_kb::kScanA + (c - 'a'));
	}
}

TEST_CASE("Shift capitalises the next letter only") {
	Keyboard kb;
	kb.layout(0.2f, 0.25f);
	const int shift = find(kb, "Shift");
	REQUIRE(shift >= 0);
	CHECK_FALSE(kb.lit(shift));
	CHECK(kb.press(shift).scancode == 0);
	CHECK(kb.lit(shift));
	CHECK(find(kb, "Q") >= 0);
	CHECK(find(kb, "q") == -1);

	// a digit keeps Shift on and goes out unshifted
	Press digit = kb.press(find(kb, "5"));
	CHECK_FALSE(digit.shift);
	CHECK(kb.lit(shift));

	Press q = kb.press(find(kb, "Q"));
	CHECK(q.scancode == screen_kb::kScanA + 16);
	CHECK(q.shift);
	CHECK_FALSE(kb.lit(shift));
	CHECK_FALSE(kb.press(find(kb, "q")).shift);

	// pressed twice, it is off again
	kb.press(shift);
	kb.press(shift);
	CHECK_FALSE(kb.lit(shift));
}

TEST_CASE("Enter presses Return and puts the keyboard away; Close only puts it away") {
	Keyboard kb;
	kb.layout(0.2f, 0.25f);
	const int shift = find(kb, "Shift");
	kb.press(shift);
	Press enter = kb.press(find(kb, "Enter"));
	CHECK(enter.scancode == screen_kb::kScanReturn);
	CHECK(enter.close);
	CHECK_FALSE(kb.lit(shift));

	kb.press(shift);
	Press close = kb.press(find(kb, "Close"));
	CHECK(close.scancode == 0);
	CHECK(close.close);
	CHECK_FALSE(kb.lit(shift));
}

TEST_CASE("a press off the keys does nothing") {
	Keyboard kb;
	kb.layout(0.2f, 0.25f);
	Press p = kb.press(-1);
	CHECK(p.scancode == 0);
	CHECK_FALSE(p.close);
	p = kb.press(Keyboard::kKeys);
	CHECK(p.scancode == 0);
	CHECK(std::strcmp(kb.label(-1), "") == 0);
}

TEST_CASE("a field taking the focus shows it; letting go hides it a sample later") {
	Visibility v;
	CHECK_FALSE(v.shown());
	CHECK(v.focus(true) == Visibility::Shown);
	CHECK(v.shown());
	// the same field again (a click on it): nothing new
	CHECK(v.focus(true) == Visibility::None);
	CHECK(v.sample() == Visibility::None);
	CHECK(v.focus(false) == Visibility::None);
	CHECK(v.sample() == Visibility::None);
	CHECK(v.shown());
	CHECK(v.sample() == Visibility::Hidden);
	CHECK_FALSE(v.shown());
	CHECK(v.sample() == Visibility::None);
}

TEST_CASE("focus moving between fields in one frame keeps it up, either way round") {
	Visibility v;
	v.focus(true);
	v.fieldUpdated();
	v.sample();
	// the new field takes the focus first, then the old one lets go; the
	// new one updates with it in the next frame
	v.focus(true);
	v.focus(false);
	CHECK(v.sample() == Visibility::None);
	v.fieldUpdated();
	CHECK(v.sample() == Visibility::None);
	CHECK(v.shown());

	// the old field lets go first, then the new one takes the focus
	v.focus(false);
	v.focus(true);
	v.fieldUpdated();
	CHECK(v.sample() == Visibility::None);
	v.fieldUpdated();
	CHECK(v.sample() == Visibility::None);
	CHECK(v.shown());
}

TEST_CASE("the old field's own update before it let go does not keep it up") {
	Visibility v;
	v.focus(true);
	v.fieldUpdated();
	v.sample();
	// the field updates (with the focus), then lets go in the same frame
	v.fieldUpdated();
	v.focus(false);
	CHECK(v.sample() == Visibility::None);
	CHECK(v.sample() == Visibility::Hidden);
}

TEST_CASE("Close hides it at once; the field clicked again brings it back") {
	Visibility v;
	v.focus(true);
	v.close();
	CHECK_FALSE(v.shown());
	CHECK(v.sample() == Visibility::None);
	CHECK(v.focus(true) == Visibility::Shown);
}

TEST_CASE("a field that stops updating takes the keyboard with it") {
	Visibility v;
	v.focus(true);
	// a dialog's opening animation: no updates yet, it stays
	for(int i = 0; i < 20; ++i)
		CHECK(v.sample() == Visibility::None);
	// the field updates at each step
	for(int i = 0; i < 10; ++i) {
		v.fieldUpdated();
		CHECK(v.sample() == Visibility::None);
	}
	// the dialog closed with Escape: no more updates
	for(int i = 1; i < Visibility::kMissedSamples; ++i)
		CHECK(v.sample() == Visibility::None);
	CHECK(v.sample() == Visibility::FieldGone);
	CHECK_FALSE(v.shown());
}

TEST_CASE("a missed update or two does not hide it") {
	Visibility v;
	v.focus(true);
	v.fieldUpdated();
	v.sample();
	for(int round = 0; round < 5; ++round) {
		for(int i = 1; i < Visibility::kMissedSamples; ++i)
			CHECK(v.sample() == Visibility::None);
		v.fieldUpdated();
		CHECK(v.sample() == Visibility::None);
	}
	CHECK(v.shown());
}

TEST_CASE("a press that starts on the keyboard is the keyboard's until it is let go") {
	ButtonTaker t;
	bool left = false;
	// pressed on the keyboard: a key press, and the game sees nothing
	CHECK(t.sample(1u, 1u, true, left) == 1u);
	CHECK(left);
	// held (hidden, it reads as pressed again): still taken, no second key
	CHECK(t.sample(1u, 1u, true, left) == 1u);
	CHECK_FALSE(left);
	// the pointer leaves the keyboard, or the keyboard goes (Close), while
	// it is held: still hidden, so no click lands on what was underneath
	CHECK(t.sample(1u, 1u, false, left) == 1u);
	CHECK_FALSE(left);
	// let go: the game has nothing to see, and the next press is free
	CHECK(t.sample(0u, 0u, false, left) == 0u);
	CHECK(t.sample(1u, 1u, false, left) == 0u);
	CHECK_FALSE(left);
}

TEST_CASE("a press that starts off the keyboard stays the game's") {
	ButtonTaker t;
	bool left = false;
	// pressed off the keyboard, then dragged onto it
	CHECK(t.sample(1u, 1u, false, left) == 0u);
	CHECK(t.sample(0u, 1u, true, left) == 0u);
	CHECK_FALSE(left);
	// the right button on the keyboard: hidden, but not a key
	CHECK(t.sample(4u, 5u, true, left) == 4u);
	CHECK_FALSE(left);
	CHECK(t.sample(0u, 0u, true, left) == 0u);
}

} // TEST_SUITE
