#ifndef BENCHDIRECTOR_H
#define BENCHDIRECTOR_H

// Plays a scripted benchmark scenario ("-bench <csv> -benchscenario <fit>",
// scripts/bench.sh --scenario): gathers both sides into an arena, keeps
// them fighting and alive, calls in artillery, flies the camera, and tells
// GameOS which segment each frame belongs to and when to take screenshots.
// Called once per frame from Mission::update(), before the camera update.
// Without -benchscenario it only reports the game-state checksum
// (gos_BenchStateHash) on fixed-step frames (-bench, -deterministic).
namespace BenchDirector {
	void update (const char* missionName);
}

#endif // BENCHDIRECTOR_H
