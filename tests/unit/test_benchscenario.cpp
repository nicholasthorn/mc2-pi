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
#include <cstdio>
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
	CHECK(s.screenX == doctest::Approx(0.7f));
	CHECK(s.screenY == doctest::Approx(0.25f));

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

TEST_CASE("framing moves the aim so the fight lands on the screen target")
{
	// screen = s0 - J * aim (moving the aim moves the world the other way);
	// a rotated, mirrored and scaled J, as a camera at some yaw gives
	const float jx[2] = { 0.3f, -0.4f };	// d(screen) / d(world x)
	const float jy[2] = { -0.5f, -0.2f };	// d(screen) / d(world y)
	const float target[2] = { 560.0f, 150.0f };
	float screen[2] = { 400.0f, 300.0f };
	float aimX = 0.0f, aimY = 0.0f;

	for (int i = 0; i < 60; i++) {
		float dx = 0.0f, dy = 0.0f;
		REQUIRE(BenchScenario::frameStep(screen, target, jx, jy, 0.2f, dx, dy));
		aimX += dx;
		aimY += dy;
		screen[0] = 400.0f - (jx[0] * aimX + jy[0] * aimY);
		screen[1] = 300.0f - (jx[1] * aimX + jy[1] * aimY);
	}
	CHECK(screen[0] == doctest::Approx(560.0f).epsilon(0.001));
	CHECK(screen[1] == doctest::Approx(150.0f).epsilon(0.001));

	// one full-gain step lands exactly on a linear target
	float dx = 0.0f, dy = 0.0f;
	const float start[2] = { 400.0f, 300.0f };
	REQUIRE(BenchScenario::frameStep(start, target, jx, jy, 1.0f, dx, dy));
	CHECK(400.0f - (jx[0] * dx + jy[0] * dy) == doctest::Approx(560.0f));
	CHECK(300.0f - (jx[1] * dx + jy[1] * dy) == doctest::Approx(150.0f));
}

TEST_CASE("framing refuses a degenerate projection")
{
	const float jx[2] = { 1.0f, 2.0f };
	const float jy[2] = { 2.0f, 4.0f };	// parallel to jx: no inverse
	const float s[2] = { 0.0f, 0.0f };
	const float t[2] = { 10.0f, 10.0f };
	float dx = 5.0f, dy = 5.0f;
	CHECK_FALSE(BenchScenario::frameStep(s, t, jx, jy, 0.2f, dx, dy));
	CHECK(dx == 0.0f);
	CHECK(dy == 0.0f);
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

TEST_CASE("optional keys keep their defaults when missing")
{
	// FitIniFile's readId* zero the value on a missing key; load() must not
	// let that through
	BenchScenario s;
	char err[128] = "";
	FitIniFile fit;
	REQUIRE(fit.open(MC2_TEST_FIXTURE_DIR "/bench_minimal.fit") == 0);
	REQUIRE_MESSAGE(s.load(fit, err, sizeof(err)), err);
	fit.close();

	CHECK(s.frames == 3600);
	CHECK_FALSE(s.hasArena);
	CHECK(s.gather == 16);
	CHECK(s.lineGap == doctest::Approx(600.0f));
	CHECK(s.spacing == doctest::Approx(100.0f));
	CHECK(s.reorderEvery == 90);
	CHECK(s.strikeRadius == doctest::Approx(300.0f));
	CHECK(s.follow);
	CHECK(s.screenX == doctest::Approx(0.5f));
	CHECK(s.screenY == doctest::Approx(0.5f));
	REQUIRE(s.numSegments == 2);
	CHECK(s.segments[0].shot == -1);
	CHECK(s.segments[0].strikeEvery == 0);
	CHECK(s.segments[1].shot == -1);
	// a missing Name becomes seg<i>
	CHECK(std::strcmp(s.segments[0].name, "seg0") == 0);
	CHECK(std::strcmp(s.segments[1].name, "seg1") == 0);
}

// Writes a two-segment scenario to a scratch file in the working directory
// (the build tree) with the given extra [Bench] lines and per-segment lines,
// loads it and returns load()'s answer.
static bool loadScratch(const char* benchExtra, const char* seg0, const char* seg1,
		char* err, size_t errSize)
{
	const char* path = "bench_scratch.fit";
	FILE* fp = fopen(path, "wb");
	REQUIRE(fp != nullptr);
	fprintf(fp, "FITini\n\n[Bench]\nl NumSegments = 2\n%s\n\n"
			"[Segment0]\nl Start = 0\n%s\n\n"
			"[Segment1]\nl Start = 100\n%s\n\nFITend\n",
			benchExtra, seg0, seg1);
	fclose(fp);

	BenchScenario s;
	FitIniFile fit;
	REQUIRE(fit.open(path) == 0);
	const bool ok = s.load(fit, err, errSize);
	fit.close();
	remove(path);
	return ok;
}

TEST_CASE("bad segment names, negative Gather and repeated shots are refused")
{
	char err[128] = "";
	const char* a = "st Name = \"a\"";
	const char* b = "st Name = \"b-2_X\"";

	// a valid baseline, so the failures below are the lines under test
	CHECK_MESSAGE(loadScratch("", a, b, err, sizeof(err)), err);

	SUBCASE("negative Gather")
	{
		CHECK_FALSE(loadScratch("l Gather = -1", a, b, err, sizeof(err)));
		CHECK(std::strstr(err, "Gather") != nullptr);
	}
	SUBCASE("Gather of 0 is allowed")
	{
		CHECK(loadScratch("l Gather = 0", a, b, err, sizeof(err)));
	}
	SUBCASE("name with a space")
	{
		CHECK_FALSE(loadScratch("", a, "st Name = \"b c\"", err, sizeof(err)));
		CHECK(std::strstr(err, "Segment1") != nullptr);
	}
	SUBCASE("name with a path character")
	{
		CHECK_FALSE(loadScratch("", "st Name = \"../x\"", b, err, sizeof(err)));
		CHECK(std::strstr(err, "Segment0") != nullptr);
	}
	SUBCASE("empty name")
	{
		CHECK_FALSE(loadScratch("", "st Name = \"\"", b, err, sizeof(err)));
	}
	SUBCASE("reserved name all")
	{
		CHECK_FALSE(loadScratch("", a, "st Name = \"all\"", err, sizeof(err)));
		CHECK(std::strstr(err, "all") != nullptr);
	}
	SUBCASE("repeated name")
	{
		CHECK_FALSE(loadScratch("", a, a, err, sizeof(err)));
		CHECK(std::strstr(err, "repeats") != nullptr);
	}
	SUBCASE("two segments with the same Shot frame")
	{
		CHECK_FALSE(loadScratch("", "st Name = \"a\"\nl Shot = 150",
				"st Name = \"b\"\nl Shot = 150", err, sizeof(err)));
		CHECK(std::strstr(err, "Shot") != nullptr);
	}
	SUBCASE("name too long for its buffer is refused, not renamed")
	{
		// 32 characters: no room for the terminator in BenchSegment::name
		CHECK_FALSE(loadScratch("", a, "st Name = \"abcdefghijklmnopqrstuvwxyz012345\"",
				err, sizeof(err)));
		CHECK(std::strstr(err, "Segment1") != nullptr);
	}
	SUBCASE("name of 31 characters fits")
	{
		CHECK(loadScratch("", a, "st Name = \"abcdefghijklmnopqrstuvwxyz01234\"",
				err, sizeof(err)));
	}
	SUBCASE("mission too long for its buffer is refused")
	{
		CHECK_FALSE(loadScratch("st Mission = \"m1234567890123456789012345678901234567890123456789012345678901234\"",
				a, b, err, sizeof(err)));
		CHECK(std::strstr(err, "Mission") != nullptr);
	}
	SUBCASE("two segments without a shot are fine")
	{
		CHECK(loadScratch("", "st Name = \"a\"\nl Shot = -1",
				"st Name = \"b\"\nl Shot = -1", err, sizeof(err)));
	}
}

}
