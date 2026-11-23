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

TEST_CASE("the keyboard sits along the bottom, or the top, inside the screen") {
	Keyboard kb;
	kb.layout(false);
	const Rect bottom = kb.panel();
	CHECK(bottom.y > 0.5f);
	CHECK(bottom.y + bottom.h <= 1.0f);
	kb.layout(true);
	const Rect top = kb.panel();
	CHECK(top.y >= 0.0f);
	CHECK(top.y + top.h < 0.5f);
	CHECK(top.x == doctest::Approx(bottom.x));
}

TEST_CASE("every key lies inside the panel and no two keys overlap") {
	Keyboard kb;
	kb.layout(false);
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
	kb.layout(false);
	CHECK(kb.hit(0.5f, 0.1f) == -1);
	CHECK_FALSE(kb.contains(0.5f, 0.1f));
	// the panel's own margin, left of the first row
	const Rect panel = kb.panel();
	CHECK(kb.contains(panel.x + 0.001f, panel.y + 0.001f));
	CHECK(kb.hit(panel.x + 0.001f, panel.y + 0.001f) == -1);
}

TEST_CASE("a letter, a digit and the other keys make their own key press") {
	Keyboard kb;
	kb.layout(false);
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
	kb.layout(false);
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
	kb.layout(false);
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
	kb.layout(false);
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
	kb.layout(false);
	Press p = kb.press(-1);
	CHECK(p.scancode == 0);
	CHECK_FALSE(p.close);
	p = kb.press(Keyboard::kKeys);
	CHECK(p.scancode == 0);
	CHECK(std::strcmp(kb.label(-1), "") == 0);
}

} // TEST_SUITE
