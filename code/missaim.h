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

// The ground height at (x, y).
typedef float (*MissGroundFn)(float x, float y, void* context);

// Where the line from `from` through `beside` first meets the ground past
// `beside`, walking `step` units at a time and then halving the last step.
// A line that has not met the ground within `maxPast` beyond `beside` (a shot
// fired upward, at a copter above the shooter) ends there, in the air, and
// the bolt plays no impact effect (weaponbolt.cpp). With `beside` already at
// or below the ground, no direction, or no step, the shot lands at `beside`
// on the ground.
MissVec missFlyPast(MissVec from, MissVec beside, MissGroundFn ground, void* context,
					float maxPast, float step);

#endif // MISSAIM_H
