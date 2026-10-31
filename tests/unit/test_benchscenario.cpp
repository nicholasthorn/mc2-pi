// tests/unit/test_benchscenario.cpp
//
// BenchScenario: the scripted benchmark ("-bench <csv> -benchscenario <fit>")
// read from a FIT file. The director (code/benchdirector.cpp) asks it, per
// gameplay frame, which segment is running and where the camera is, so those
// answers must not depend on anything but the frame number.

#include "doctest.h"

#include "benchscenario.h"
#include "inifile.h"

#include <cmath>
#include <cstring>

static bool loadFixture(BenchScenario& s, char* err, size_t errSize)
{
	FitIniFile fit;
	REQUIRE(fit.open(MC2_TEST_FIXTURE_DIR "/bench_scenario.fit") == 0);
	const bool ok = s.load(fit, err, errSize);
	fit.close();
	return ok;
}

TEST_SUITE("BenchScenario")
{

TEST_CASE("reads the [Bench] block and every segment")
{
	BenchScenario s;
	char err[128] = "";
	REQUIRE_MESSAGE(loadFixture(s, err, sizeof(err)), err);

	CHECK(std::strcmp(s.mission, "mc2_23") == 0);
	CHECK(s.frames == 900);
	CHECK(s.hasArena);
	CHECK(s.arenaX == doctest::Approx(1000.0f));
	CHECK(s.arenaY == doctest::Approx(-2000.0f));
	CHECK(s.gather == 12);
	CHECK(s.lineGap == doctest::Approx(800.0f));
	CHECK(s.spacing == doctest::Approx(120.0f));
	CHECK(s.reorderEvery == 60);
	CHECK(s.strikeRadius == doctest::Approx(300.0f));
	CHECK_FALSE(s.follow);	// the fixture turns following off
	CHECK(s.lift == doctest::Approx(250.0f));

	REQUIRE(s.numSegments == 2);
	CHECK(std::strcmp(s.segments[0].name, "overview") == 0);
	CHECK(s.segments[0].start == 0);
	CHECK(s.segments[0].shot == 100);
	CHECK(s.segments[0].strikeEvery == 0);
	CHECK(std::strcmp(s.segments[1].name, "effects") == 0);
	CHECK(s.segments[1].start == 300);
	CHECK(s.segments[1].shot == -1);
	CHECK(s.segments[1].strikeEvery == 30);

	// keys from every segment, in frame order
	REQUIRE(s.numKeys == 3);
	CHECK(s.keys[0].frame == 0);
	CHECK(s.keys[1].frame == 300);
	CHECK(s.keys[2].frame == 600);
	CHECK(s.keys[2].altitude == doctest::Approx(800.0f));
}

TEST_CASE("segmentAt follows the start frames")
{
	BenchScenario s;
	char err[128] = "";
	REQUIRE(loadFixture(s, err, sizeof(err)));

	CHECK(s.segmentAt(0) == 0);
	CHECK(s.segmentAt(299) == 0);
	CHECK(s.segmentAt(300) == 1);
	CHECK(s.segmentAt(5000) == 1);
}

TEST_CASE("camera is interpolated between keys and held outside them")
{
	BenchScenario s;
	char err[128] = "";
	REQUIRE(loadFixture(s, err, sizeof(err)));

	BenchCameraKey c = s.cameraAt(0);
	CHECK(c.dx == doctest::Approx(0.0f));
	CHECK(c.yaw == doctest::Approx(0.0f));

	c = s.cameraAt(150);	// halfway from key 0 to key 1
	CHECK(c.dx == doctest::Approx(200.0f));
	CHECK(c.yaw == doctest::Approx(45.0f));
	CHECK(c.altitude == doctest::Approx(1600.0f));

	c = s.cameraAt(900);	// after the last key: held
	CHECK(c.dy == doctest::Approx(200.0f));
	CHECK(c.tilt == doctest::Approx(60.0f));
}

TEST_CASE("yaw takes the short way round through 180")
{
	BenchScenario s;
	char err[128] = "";
	REQUIRE(loadFixture(s, err, sizeof(err)));

	// key 1 yaw 90 -> key 2 yaw -170: the short way is +100 degrees via 180
	const BenchCameraKey c = s.cameraAt(450);
	CHECK(c.yaw == doctest::Approx(140.0f));
	const BenchCameraKey late = s.cameraAt(585);
	CHECK(late.yaw == doctest::Approx(-175.0f));	// 90 + 95 = 185 -> -175
}

TEST_CASE("strike offsets are fixed, spread out and inside the radius")
{
	for (long i = 0; i < 50; i++) {
		float x0, y0, x1, y1;
		BenchScenario::strikeOffset(i, 300.0f, x0, y0);
		BenchScenario::strikeOffset(i, 300.0f, x1, y1);
		CHECK(x0 == x1);
		CHECK(y0 == y1);
		CHECK(std::sqrt(x0 * x0 + y0 * y0) <= 300.0f + 0.01f);
	}
	float ax, ay, bx, by;
	BenchScenario::strikeOffset(0, 300.0f, ax, ay);
	BenchScenario::strikeOffset(1, 300.0f, bx, by);
	CHECK(std::fabs(ax - bx) + std::fabs(ay - by) > 50.0f);
}

TEST_CASE("a file without a [Bench] block is refused with a reason")
{
	BenchScenario s;
	char err[128] = "";

	FitIniFile fit;
	REQUIRE(fit.open(MC2_TEST_FIXTURE_DIR "/harness_smoke.fit") == 0);
	CHECK_FALSE(s.load(fit, err, sizeof(err)));
	CHECK(std::strstr(err, "[Bench]") != nullptr);
	fit.close();
}

}
