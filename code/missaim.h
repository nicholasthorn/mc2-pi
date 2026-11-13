// code/missaim.h
//
// Where a missed shot lands. The 2001 code puts a miss on the ground 25 units
// beside the target's ground position, which suits anything standing on the
// ground. A copter (a mech with MoveType air) keeps its position on the
// ground and only its model flies, so every miss at one landed on the ground
// under it. For a copter the miss passes beside it at the height it was aimed
// and carries on along the line of fire until it meets the ground. No GL and
// no game objects, so it can be unit tested.

#ifndef MISSAIM_H
#define MISSAIM_H

struct MissVec {
	float x;
	float y;
	float z;		// up
};

// The surface a shot meets at (x, y), ground or water, in *height; false off
// the playable map.
typedef bool (*MissGroundFn)(float x, float y, void* context, float* height);

// The surface height: the terrain, or the water level where the cell has water
// above it.
float missSurface(float terrain, bool water, float waterLevel);

// A miss ending more than 10 units above the surface struck nothing there (one
// fired up past a copter): no ground or water effect.
bool missEndsInAir(float z, float surface);

// The four places `radius` from `target`, in the order to try them: left and
// right of the line of fire from `from` (`rightFirst` picks which comes first),
// then beyond the target and in front of it, all at the target's height. With
// no horizontal line of fire (straight down), +x, -x, +y, -y.
void missSides(MissVec from, MissVec target, float radius, bool rightFirst, MissVec out[4]);

// Where the line from `from` through `beside` first meets the surface past
// `beside`, walking `step` units at a time and then halving the last step.
// A line that has not met the surface within `maxPast` beyond `beside` (a shot
// fired upward, at a copter above the shooter) ends there, in the air, and
// the bolt plays no impact effect (weaponbolt.cpp); one that leaves the map
// first ends in the air at the map edge. With `beside` already at or below the
// surface, no direction, or no step, the shot lands at `beside` on the
// surface; with `beside` off the map, it is returned unchanged.
MissVec missFlyPast(MissVec from, MissVec beside, MissGroundFn ground, void* context,
					float maxPast, float step);

#endif // MISSAIM_H
