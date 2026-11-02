// tests/unit/test_input_latch.cpp
//
// InputLatch: the VR controllers' buttons and keys between input samples.
// In VR the game samples input only on frames it steps (30 Hz against the
// headset's 72); a press shorter than the gap between samples must still
// reach the game once, or the click is lost (it was, before the latch).

#include "doctest.h"

#include "utils/input_latch.h"

TEST_SUITE("InputLatch") {

TEST_CASE("a button held across samples is down at each") {
	InputLatch latch;
	latch.set(1u);
	CHECK(latch.sample() == 1u);
	CHECK(latch.sample() == 1u);
	latch.set(0u);
	CHECK(latch.sample() == 0u);
}

TEST_CASE("a press that starts and ends between samples is down once") {
	InputLatch latch;
	latch.set(1u);
	latch.set(0u);
	CHECK(latch.sample() == 1u);
	CHECK(latch.sample() == 0u);
}

TEST_CASE("buttons are kept apart") {
	InputLatch latch;
	latch.set(1u);
	latch.set(1u | 4u);
	latch.set(4u);
	// the left button was held, the right one pressed: both down once
	CHECK(latch.sample() == (1u | 4u));
	CHECK(latch.sample() == 4u);
}

TEST_CASE("holding does not latch again; a fresh press does") {
	InputLatch latch;
	latch.set(1u);
	CHECK(latch.sample() == 1u);
	latch.set(1u);			// still held: nothing new
	latch.set(0u);
	CHECK(latch.sample() == 0u);
	latch.set(1u);			// pressed again
	latch.set(0u);
	CHECK(latch.sample() == 1u);
}

}
