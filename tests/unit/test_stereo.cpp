// tests/unit/test_stereo.cpp
//
// stereo: the desktop side-by-side debug view of the VR plan (Phase 1). Which
// part of the window each eye draws into, and how far each eye's camera moves
// sideways from the game camera (positive = screen right). A sign error here
// swaps the eyes, which only shows as a stereo image that will not fuse, so
// the directions are pinned.

#include "doctest.h"

#include "utils/stereo.h"

TEST_SUITE("Stereo")
{

TEST_CASE("mono draws into the whole window, each eye into one half")
{
	int vp[4];

	stereo::eyeViewport(-1, 800, 600, vp);
	CHECK(vp[0] == 0); CHECK(vp[1] == 0); CHECK(vp[2] == 800); CHECK(vp[3] == 600);

	stereo::eyeViewport(0, 800, 600, vp);
	CHECK(vp[0] == 0); CHECK(vp[1] == 0); CHECK(vp[2] == 400); CHECK(vp[3] == 600);

	stereo::eyeViewport(1, 800, 600, vp);
	CHECK(vp[0] == 400); CHECK(vp[1] == 0); CHECK(vp[2] == 400); CHECK(vp[3] == 600);
}

TEST_CASE("an odd width gives the right eye the extra column, no gap")
{
	int left[4], right[4];
	stereo::eyeViewport(0, 801, 600, left);
	stereo::eyeViewport(1, 801, 600, right);
	CHECK(left[2] == 400);
	CHECK(right[0] == 400);
	CHECK(right[2] == 401);
	CHECK(left[2] + right[2] == 801);
}

TEST_CASE("the left eye moves half the separation left, the right eye right, mono stays")
{
	CHECK(stereo::eyeOffset(0, 20.0f) == doctest::Approx(-10.0f));
	CHECK(stereo::eyeOffset(1, 20.0f) == doctest::Approx(10.0f));
	CHECK(stereo::eyeOffset(-1, 20.0f) == 0.0f);
	CHECK(stereo::eyeOffset(1, 0.0f) == 0.0f);
}

}
