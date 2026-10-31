#include "benchscenario.h"

#include "inifile.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

static bool fail(char* err, size_t errSize, const char* what, const char* block)
{
	snprintf(err, errSize, "bench scenario: %s %s", what, block);
	return false;
}

bool BenchScenario::load(FitIniFile& fit, char* err, size_t errSize)
{
	if (fit.seekBlock("Bench") != 0)
		return fail(err, errSize, "no", "[Bench] block");

	// optional values keep their defaults when missing
	fit.readIdString("Mission", mission, sizeof(mission));
	fit.readIdLong("Frames", frames);
	hasArena = (fit.readIdFloat("ArenaX", arenaX) == 0) && (fit.readIdFloat("ArenaY", arenaY) == 0);
	fit.readIdLong("Gather", gather);
	fit.readIdFloat("LineGap", lineGap);
	fit.readIdFloat("Spacing", spacing);
	fit.readIdLong("ReorderEvery", reorderEvery);
	fit.readIdFloat("StrikeRadius", strikeRadius);
	fit.readIdBoolean("Follow", follow);
	fit.readIdFloat("Lift", lift);
	if (fit.readIdLong("NumSegments", numSegments) != 0 || numSegments < 1 || numSegments > MAX_SEGMENTS)
		return fail(err, errSize, "NumSegments missing or out of range in", "[Bench]");

	numKeys = 0;
	for (long i = 0; i < numSegments; i++) {
		char block[32];
		snprintf(block, sizeof(block), "Segment%ld", i);
		if (fit.seekBlock(block) != 0)
			return fail(err, errSize, "missing block", block);

		BenchSegment& seg = segments[i];
		fit.readIdString("Name", seg.name, sizeof(seg.name));
		if (fit.readIdLong("Start", seg.start) != 0)
			return fail(err, errSize, "no Start in", block);
		if (i == 0 && seg.start != 0)
			return fail(err, errSize, "Start must be 0 in", block);
		if (i > 0 && seg.start <= segments[i - 1].start)
			return fail(err, errSize, "Start not after the previous segment's in", block);
		fit.readIdLong("Shot", seg.shot);
		fit.readIdLong("StrikeEvery", seg.strikeEvery);

		long n = 0;
		fit.readIdLong("NumKeys", n);
		for (long k = 0; k < n; k++) {
			if (numKeys == MAX_KEYS)
				return fail(err, errSize, "too many camera keys at", block);
			char id[32];
			snprintf(id, sizeof(id), "Key%ld", k);
			float v[6];
			if (fit.readIdFloatArray(id, v, 6) != 0)
				return fail(err, errSize, "bad camera key in", block);
			BenchCameraKey& key = keys[numKeys++];
			key.frame = (long)v[0];
			key.dx = v[1];
			key.dy = v[2];
			key.tilt = v[3];
			key.yaw = v[4];
			key.altitude = v[5];
		}
	}

	std::stable_sort(keys, keys + numKeys,
			[](const BenchCameraKey& a, const BenchCameraKey& b) { return a.frame < b.frame; });
	return true;
}

long BenchScenario::segmentAt(long f) const
{
	long seg = 0;
	for (long i = 1; i < numSegments; i++)
		if (f >= segments[i].start)
			seg = i;
	return seg;
}

static float wrapDegrees(float a)
{
	while (a > 180.0f)
		a -= 360.0f;
	while (a <= -180.0f)
		a += 360.0f;
	return a;
}

BenchCameraKey BenchScenario::cameraAt(long f) const
{
	if (numKeys == 0) {
		BenchCameraKey none;
		none.frame = f;
		return none;
	}
	if (f <= keys[0].frame)
		return keys[0];
	if (f >= keys[numKeys - 1].frame)
		return keys[numKeys - 1];

	long i = 1;
	while (keys[i].frame < f)
		i++;
	const BenchCameraKey& a = keys[i - 1];
	const BenchCameraKey& b = keys[i];
	const float t = (float)(f - a.frame) / (float)(b.frame - a.frame);

	BenchCameraKey c;
	c.frame = f;
	c.dx = a.dx + (b.dx - a.dx) * t;
	c.dy = a.dy + (b.dy - a.dy) * t;
	c.tilt = a.tilt + (b.tilt - a.tilt) * t;
	c.yaw = wrapDegrees(a.yaw + wrapDegrees(b.yaw - a.yaw) * t);
	c.altitude = a.altitude + (b.altitude - a.altitude) * t;
	return c;
}

void BenchScenario::strikeOffset(long i, float radius, float& x, float& y)
{
	// golden-angle spiral, radius cycling through ten rings
	const float goldenAngle = 2.39996323f;
	const float ring = ((float)((i * 7) % 10) + 0.5f) / 10.0f;
	const float r = radius * std::sqrt(ring);
	const float a = goldenAngle * (float)i;
	x = r * std::cos(a);
	y = r * std::sin(a);
}
