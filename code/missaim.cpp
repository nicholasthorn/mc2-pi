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

} // namespace

float missSurface(float terrain, bool water, float waterLevel)
{
	return (water && waterLevel > terrain) ? waterLevel : terrain;
}

bool missEndsInAir(float z, float surface)
{
	return z > surface + 10.0f;
}

void missSides(MissVec from, MissVec target, float radius, bool rightFirst, MissVec out[4])
{
	float fx = target.x - from.x;
	float fy = target.y - from.y;
	const float len = std::sqrt(fx * fx + fy * fy);
	float sx[4] = {1.0f, -1.0f, 0.0f, 0.0f};
	float sy[4] = {0.0f, 0.0f, 1.0f, -1.0f};
	if (len >= 0.001f) {
		fx /= len;
		fy /= len;
		// right of the line of fire (z up) is (fy, -fx), left (-fy, fx)
		const float side = rightFirst ? 1.0f : -1.0f;
		sx[0] = fy * side;
		sy[0] = -fx * side;
		sx[1] = -sx[0];
		sy[1] = -sy[0];
		sx[2] = fx;
		sy[2] = fy;
		sx[3] = -fx;
		sy[3] = -fy;
	}
	for (int i = 0; i < 4; i++) {
		out[i].x = target.x + sx[i] * radius;
		out[i].y = target.y + sy[i] * radius;
		out[i].z = target.z;
	}
}

MissVec missFlyPast(MissVec from, MissVec beside, MissGroundFn ground, void* context,
					float maxPast, float step)
{
	float surface = 0.0f;
	if (!ground(beside.x, beside.y, context, &surface))
		return beside;
	float dx = beside.x - from.x;
	float dy = beside.y - from.y;
	float dz = beside.z - from.z;
	const float len = std::sqrt(dx * dx + dy * dy + dz * dz);
	if (len < 0.001f || step <= 0.0f || beside.z <= surface) {
		beside.z = surface;
		return beside;
	}
	dx /= len;
	dy /= len;
	dz /= len;

	float t = 0.0f;
	while (t < maxPast) {
		const float next = t + step < maxPast ? t + step : maxPast;
		const MissVec p = along(beside, dx, dy, dz, next);
		float h = 0.0f;
		const bool onMap = ground(p.x, p.y, context, &h);
		if (!onMap || p.z <= h) {
			// between t (on the map, above the surface) and next: the map edge
			// or the surface; a point off the map counts as past it
			float lo = t;
			float hi = next;
			while (hi - lo > step / 64.0f) {
				const float mid = (lo + hi) * 0.5f;
				const MissVec m = along(beside, dx, dy, dz, mid);
				float mh = 0.0f;
				const bool midOnMap = ground(m.x, m.y, context, &mh);
				const bool past = onMap ? (midOnMap && m.z <= mh) : !midOnMap;
				if (past)
					hi = mid;
				else
					lo = mid;
			}
			if (!onMap)
				return along(beside, dx, dy, dz, lo);		// in the air at the edge
			MissVec hit = along(beside, dx, dy, dz, hi);
			ground(hit.x, hit.y, context, &hit.z);
			return hit;
		}
		t = next;
	}
	return along(beside, dx, dy, dz, maxPast);
}
