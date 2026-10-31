#ifndef BENCHSCENARIO_H
#define BENCHSCENARIO_H

#include <cstddef>

class FitIniFile;

// One camera keyframe. Positions are world units relative to the arena
// centre (or the fight's centre, with Follow); tilt is degrees above horizontal, yaw degrees (-180..180),
// altitude the camera's distance back from its look-at point.
struct BenchCameraKey {
	long frame = 0;
	float dx = 0.0f;
	float dy = 0.0f;
	float tilt = 30.0f;
	float yaw = 0.0f;
	float altitude = 1200.0f;
};

struct BenchSegment {
	char name[32] = "";
	long start = 0;			// first gameplay frame of the segment
	long shot = -1;			// gameplay frame to screenshot, -1 = none
	long strikeEvery = 0;	// artillery strike every N frames, 0 = none
};

// The scripted benchmark scenario, read from a FIT file (bench/*.fit).
// Everything it answers depends on the gameplay frame number only, so every
// run of a scenario does the same thing on the same frame. GL- and game-free
// so it can be unit tested; code/benchdirector.cpp drives the game with it.
struct BenchScenario {
	static const long MAX_SEGMENTS = 16;
	static const long MAX_KEYS = 64;

	char mission[64] = "";
	long frames = 3600;			// suggested -autoexit
	bool hasArena = false;		// false: centre on the player's units
	float arenaX = 0.0f;
	float arenaY = 0.0f;
	long gather = 16;			// enemy movers moved into the arena
	float lineGap = 600.0f;		// between the two battle lines (world units)
	float spacing = 100.0f;		// between units in a line (world units)
	long reorderEvery = 90;		// re-issue attack orders every N frames
	float strikeRadius = 300.0f;	// artillery lands within this of the centre
	bool follow = true;			// camera keys relative to the fight, not the arena
	float lift = 0.0f;			// aim this far in front of it (towards the viewer),
								// raising it on screen above the HUD

	long numSegments = 0;
	BenchSegment segments[MAX_SEGMENTS];
	long numKeys = 0;
	BenchCameraKey keys[MAX_KEYS];	// all segments' keys, in frame order

	// Reads [Bench] and [Segment0..N-1]. On failure returns false with the
	// reason in err.
	bool load(FitIniFile& fit, char* err, size_t errSize);

	// Index of the segment running at gameplay frame f (0 before the first).
	long segmentAt(long f) const;

	// Camera at gameplay frame f: linear between keys (yaw the short way
	// round), held before the first key and after the last.
	BenchCameraKey cameraAt(long f) const;

	// Offset of the i-th artillery strike from the arena centre: a fixed
	// spiral that spreads strikes over the disc of the given radius.
	static void strikeOffset(long i, float radius, float& x, float& y);
};

#endif // BENCHSCENARIO_H
