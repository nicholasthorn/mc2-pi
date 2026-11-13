// code/missaim.cpp

#include "missaim.h"

#include <cmath>

namespace
{

MissVec along(const MissVec& start, float dx, float dy, float dz, float t)
{
	MissVec p;
	p.x = start.x + dx * t;
	p.y = start.y + dy * t;
	p.z = start.z + dz * t;
	return p;
}

MissVec onGround(MissVec p, MissGroundFn ground, void* context)
{
	p.z = ground(p.x, p.y, context);
	return p;
}

} // namespace

MissVec missFlyPast(MissVec from, MissVec beside, MissGroundFn ground, void* context,
					float maxPast, float step)
{
	float dx = beside.x - from.x;
	float dy = beside.y - from.y;
	float dz = beside.z - from.z;
	const float len = std::sqrt(dx * dx + dy * dy + dz * dz);
	if (len < 0.001f || step <= 0.0f || beside.z <= ground(beside.x, beside.y, context))
		return onGround(beside, ground, context);
	dx /= len;
	dy /= len;
	dz /= len;

	float t = 0.0f;
	while (t < maxPast) {
		const float next = t + step < maxPast ? t + step : maxPast;
		const MissVec p = along(beside, dx, dy, dz, next);
		if (p.z <= ground(p.x, p.y, context)) {
			// the ground lies between t (above it) and next (at or below it)
			float lo = t;
			float hi = next;
			while (hi - lo > step / 64.0f) {
				const float mid = (lo + hi) * 0.5f;
				const MissVec m = along(beside, dx, dy, dz, mid);
				if (m.z > ground(m.x, m.y, context))
					lo = mid;
				else
					hi = mid;
			}
			return onGround(along(beside, dx, dy, dz, hi), ground, context);
		}
		t = next;
	}
	return along(beside, dx, dy, dz, maxPast);
}
