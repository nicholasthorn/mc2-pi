#pragma once
//---------------------------------------------------------------------------
// render_heuristics.h
//
// Small pure helpers behind render-side performance heuristics: LOD switch
// hysteresis, light reach tests and texture cache timing. No GL or engine
// dependencies, so tests/unit/test_render_heuristics.cpp can test them.
//---------------------------------------------------------------------------

#include <stdint.h>

//---------------------------------------------------------------------------
// Distance past which LOD level is used, given the LOD in use now. Each
// switch rebuilds the shape and looks up its textures again, so keep a 5%
// band around each threshold: an object near it no longer flips back and forth.
inline float LODSwitchDistance (float lodDistance, long level, long currentLOD)
{
	return (level <= currentLOD) ? (lodDistance * 0.95f) : (lodDistance * 1.05f);
}

//---------------------------------------------------------------------------
// Squared distance from point p to the axis-aligned box [boxMin, boxMax],
// 0 when p is inside it.
inline float PointBoxDistanceSquared (const float p[3], const float boxMin[3], const float boxMax[3])
{
	float distanceSquared = 0.0f;
	for (int i = 0; i < 3; i++)
	{
		float d = 0.0f;
		if (p[i] < boxMin[i])
			d = boxMin[i] - p[i];
		else if (p[i] > boxMax[i])
			d = p[i] - boxMax[i];
		distanceSquared += d * d;
	}
	return distanceSquared;
}

//---------------------------------------------------------------------------
// Measures how many turns (frames) pass per second, so code that counts
// time in turns can convert a time into turns at the current frame rate.
// The texture cache's turn counts were tuned for about 30 fps; at 180 fps
// they meant a sixth of the time.
struct TurnRate
{
	float		turnsPerSecond = 30.0f;		// until measured: the rate the old counts assumed
	bool		started = false;
	uint32_t	lastTime = 0;				// ms
	long		lastTurn = 0;

	// Call once per frame. A gap of 500 ms or more (paused: nothing calls
	// this) or turn going backwards (a new mission starts again at 0)
	// restarts the measurement instead of counting as a frame rate.
	void update (uint32_t nowMs, long turn)
	{
		const uint32_t dt = nowMs - lastTime;
		if (!started || (turn < lastTurn) || (dt >= 500))
		{
			started = true;
			lastTime = nowMs;
			lastTurn = turn;
			return;
		}

		// a millisecond timer is too coarse for one frame: measure over 50 ms
		if (dt < 50)
			return;

		const float rate = float(turn - lastTurn) * 1000.0f / float(dt);
		turnsPerSecond += 0.2f * (rate - turnsPerSecond);
		lastTime = nowMs;
		lastTurn = turn;
	}

	// Turns in ms milliseconds at the measured rate, never fewer than minTurns
	long turnsFor (uint32_t ms, long minTurns) const
	{
		const long turns = long(turnsPerSecond * float(ms) / 1000.0f);
		return (turns > minTurns) ? turns : minTurns;
	}
};
