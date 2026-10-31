#include "benchscenario.h"

#include "inifile.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

static bool fail(char* err, size_t errSize, const char* what, const char* block)
{
	snprintf(err, errSize, "bench scenario: %s %s", what, block);
	return false;
}

// FitIniFile's readId* set the value to 0 when the key is missing, so an
// optional key is read into a temporary and kept only when it was found.
static bool readOptional(FitIniFile& fit, const char* id, long& value)
{
	long v = 0;
	if (fit.readIdLong(id, v) != NO_ERR)
		return false;
	value = v;
	return true;
}

static bool readOptional(FitIniFile& fit, const char* id, float& value)
{
	float v = 0.0f;
	if (fit.readIdFloat(id, v) != NO_ERR)
		return false;
	value = v;
	return true;
}

static bool readOptional(FitIniFile& fit, const char* id, bool& value)
{
	bool v = false;
	if (fit.readIdBoolean(id, v) != NO_ERR)
		return false;
	value = v;
	return true;
}

// Segment names end up in log lines and file names: [A-Za-z0-9_-] only.
static bool validSegmentName(const char* name)
{
	if (name[0] == '\0')
		return false;
	for (const char* c = name; *c; c++) {
		const bool ok = (*c >= 'a' && *c <= 'z') || (*c >= 'A' && *c <= 'Z') ||
				(*c >= '0' && *c <= '9') || *c == '_' || *c == '-';
		if (!ok)
			return false;
	}
	return true;
}

bool BenchScenario::load(FitIniFile& fit, char* err, size_t errSize)
{
	if (fit.seekBlock("Bench") != 0)
		return fail(err, errSize, "no", "[Bench] block");

	// optional values keep their defaults when missing
	// readIdString leaves the buffer unterminated when the value does not fit
	const long missionResult = fit.readIdString("Mission", mission, sizeof(mission));
	if (missionResult != NO_ERR && missionResult != (long)VARIABLE_NOT_FOUND)
		return fail(err, errSize, "Mission too long or malformed in", "[Bench]");
	readOptional(fit, "Frames", frames);
	float ax = 0.0f, ay = 0.0f;
	hasArena = readOptional(fit, "ArenaX", ax) && readOptional(fit, "ArenaY", ay);
	if (hasArena) {
		arenaX = ax;
		arenaY = ay;
	}
	readOptional(fit, "Gather", gather);
	readOptional(fit, "LineGap", lineGap);
	readOptional(fit, "Spacing", spacing);
	readOptional(fit, "ReorderEvery", reorderEvery);
	readOptional(fit, "StrikeRadius", strikeRadius);
	readOptional(fit, "Follow", follow);
	readOptional(fit, "ScreenX", screenX);
	readOptional(fit, "ScreenY", screenY);
	if (gather < 0)
		return fail(err, errSize, "Gather is negative in", "[Bench]");
	if (screenX < 0.0f || screenX > 1.0f || screenY < 0.0f || screenY > 1.0f)
		return fail(err, errSize, "ScreenX / ScreenY outside 0..1 in", "[Bench]");
	if (fit.readIdLong("NumSegments", numSegments) != 0 || numSegments < 1 || numSegments > MAX_SEGMENTS)
		return fail(err, errSize, "NumSegments missing or out of range in", "[Bench]");

	numKeys = 0;
	for (long i = 0; i < numSegments; i++) {
		char block[32];
		snprintf(block, sizeof(block), "Segment%ld", i);
		if (fit.seekBlock(block) != 0)
			return fail(err, errSize, "missing block", block);

		BenchSegment& seg = segments[i];
		const long nameResult = fit.readIdString("Name", seg.name, sizeof(seg.name));
		if (nameResult == (long)VARIABLE_NOT_FOUND)
			snprintf(seg.name, sizeof(seg.name), "seg%ld", i);
		else if (nameResult != NO_ERR)
			return fail(err, errSize, "Name too long or malformed in", block);
		if (!validSegmentName(seg.name))
			return fail(err, errSize, "Name is empty or not [A-Za-z0-9_-] in", block);
		if (strcmp(seg.name, "all") == 0)
			return fail(err, errSize, "Name \"all\" is reserved in", block);
		for (long j = 0; j < i; j++)
			if (strcmp(seg.name, segments[j].name) == 0)
				return fail(err, errSize, "Name repeats an earlier segment's in", block);
		if (fit.readIdLong("Start", seg.start) != 0)
			return fail(err, errSize, "no Start in", block);
		if (i == 0 && seg.start != 0)
			return fail(err, errSize, "Start must be 0 in", block);
		if (i > 0 && seg.start <= segments[i - 1].start)
			return fail(err, errSize, "Start not after the previous segment's in", block);
		seg.shot = -1;
		seg.strikeEvery = 0;
		readOptional(fit, "Shot", seg.shot);
		readOptional(fit, "StrikeEvery", seg.strikeEvery);

		long n = 0;
		readOptional(fit, "NumKeys", n);
		for (long k = 0; k < n; k++) {
			if (numKeys == MAX_KEYS)
				return fail(err, errSize, "too many camera keys at", block);
			char id[32];
			snprintf(id, sizeof(id), "Key%ld", k);
			// readIdFloatArray accepts fewer values than asked for and leaves
			// the rest unset, so the element count is checked first
			float v[6] = {};
			if (fit.getIdFloatArrayElements(id) != 6 || fit.readIdFloatArray(id, v, 6) != 0)
				return fail(err, errSize, "camera key missing or not 6 values in", block);
			BenchCameraKey& key = keys[numKeys++];
			key.frame = (long)v[0];
			key.dx = v[1];
			key.dy = v[2];
			key.tilt = v[3];
			key.yaw = v[4];
			key.altitude = v[5];
		}
	}

	// A shot outside its own segment's frames would show another segment's
	// camera. Segments do not overlap, so this also rules out two segments
	// sharing a shot frame.
	for (long i = 0; i < numSegments; i++) {
		const long shot = segments[i].shot;
		const long end = (i + 1 < numSegments) ? segments[i + 1].start : frames;
		if (shot >= 0 && (shot < segments[i].start || shot >= end)) {
			char block[32];
			snprintf(block, sizeof(block), "Segment%ld", i);
			return fail(err, errSize, "Shot outside the segment's frames in", block);
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

bool BenchScenario::frameStep(const float s[2], const float t[2], const float jx[2],
		const float jy[2], float gain, float& dx, float& dy)
{
	// moving the aim by a moves the point on screen by -J a, J = [jx jy]
	dx = dy = 0.0f;
	const float det = jx[0] * jy[1] - jy[0] * jx[1];
	if (std::fabs(det) < 1e-6f)
		return false;
	const float ex = (t[0] - s[0]) * gain;
	const float ey = (t[1] - s[1]) * gain;
	dx = -(jy[1] * ex - jy[0] * ey) / det;
	dy = -(jx[0] * ey - jx[1] * ex) / det;
	return true;
}
