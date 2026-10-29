// tests/unit/test_audio_pan.cpp
//
// panToStereoLevels(): turns the game's pan (-1 left .. +1 right, from
// SoundSystem::playDigitalSample) into the left/right levels for
// Mix_SetPanning. Centre must stay at full level on both sides, so centred
// sounds are exactly as loud as before panning worked.

#include "doctest.h"

#include "utils/audio_pan.h"

#include <cmath>
#include <limits>

TEST_SUITE("AudioPan")
{

TEST_CASE("centre plays at full level on both sides")
{
	const StereoLevels s = panToStereoLevels(0.0f);
	CHECK(s.left == 255);
	CHECK(s.right == 255);
}

TEST_CASE("hard left and hard right silence the other side")
{
	const StereoLevels l = panToStereoLevels(-1.0f);
	CHECK(l.left == 255);
	CHECK(l.right == 0);

	const StereoLevels r = panToStereoLevels(1.0f);
	CHECK(r.left == 0);
	CHECK(r.right == 255);
}

TEST_CASE("out-of-range and NaN pans are clamped or centred")
{
	const StereoLevels farRight = panToStereoLevels(5.0f);
	CHECK(farRight.left == 0);
	CHECK(farRight.right == 255);

	const StereoLevels farLeft = panToStereoLevels(-5.0f);
	CHECK(farLeft.left == 255);
	CHECK(farLeft.right == 0);

	const StereoLevels nan = panToStereoLevels(std::numeric_limits<float>::quiet_NaN());
	CHECK(nan.left == 255);
	CHECK(nan.right == 255);
}

TEST_CASE("left and right pans mirror each other")
{
	for (float p = 0.0f; p <= 1.0f; p += 0.125f)
	{
		const StereoLevels a = panToStereoLevels(p);
		const StereoLevels b = panToStereoLevels(-p);
		CHECK(a.left == b.right);
		CHECK(a.right == b.left);
	}
}

TEST_CASE("moving right never lowers the right side or raises the left")
{
	StereoLevels prev = panToStereoLevels(-1.0f);
	for (int i = -15; i <= 16; ++i)
	{
		const StereoLevels cur = panToStereoLevels(i / 16.0f);
		CHECK(cur.right >= prev.right);
		CHECK(cur.left <= prev.left);
		prev = cur;
	}
}

TEST_CASE("the near side stays at full level and the far side follows the power curve")
{
	// Half right: the right side is full, the left side is sqrt(2) * cos(3/8 pi).
	const StereoLevels s = panToStereoLevels(0.5f);
	CHECK(s.right == 255);
	const double pi = 3.14159265358979323846;
	const double expected = std::sqrt(2.0) * std::cos(3.0 * pi / 8.0) * 255.0;
	CHECK(s.left == doctest::Approx(expected).epsilon(0.01));
}

}
