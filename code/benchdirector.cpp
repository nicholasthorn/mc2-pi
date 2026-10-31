//---------------------------------------------------------------------------
// Bench director: drives the game through a scripted benchmark scenario.
// The scenario logic (segments, camera keys, strike pattern) is in
// benchscenario.cpp; this file only applies it to the game, once per frame.
// Everything here depends on the gameplay frame number, never on time, so
// "-bench" runs stay frame-for-frame identical.
//---------------------------------------------------------------------------

#include "benchdirector.h"
#include "benchscenario.h"

#include "inifile.h"
#include "objmgr.h"
#include "mover.h"
#include "warrior.h"
#include "team.h"
#include "comndr.h"
#include "gamecam.h"
#include "artlry.h"
#include "dartlry.h"
#include "tacordr.h"
#include "terrain.h"

#include <gameos.hpp>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>
#include <vector>

extern bool neverEndingStory;
extern bool ShowMovers;

namespace {

BenchScenario scenario;
bool loaded = false;
bool off = false;					// no scenario, or it failed to load
long frame = 0;						// gameplay frames since the mission started
long segment = -1;
long strikes = 0;
Stuff::Vector3D arena;
float viewX = 0.0f, viewY = 0.0f;	// what the camera keys are relative to
float aimX = 0.0f, aimY = 0.0f;		// framing offset of the aim point from it
std::vector<MoverPtr> fighters;		// both sides, in the arena

// Exists, is on a team, and can still fight.
bool canFight (MoverPtr m) {

	return m && m->getExists() && m->getTeam() && !m->isDisabled() && !m->isDestroyed();
}

Stuff::Vector3D onGround (float x, float y) {

	Stuff::Vector3D p;
	p.x = x;
	p.y = y;
	p.z = 0.0f;
	p.z = land->getTerrainElevation(p);
	return p;
}

bool load (const char* missionName) {

	const char* path = gos_GetBenchScenario();
	if (!path)
		return false;

	FitIniFile fit;
	if (fit.open(path) != 0) {
		printf("[BENCH] ERROR: cannot open scenario %s\n", path);
		return false;
	}
	char err[160] = "";
	const bool ok = scenario.load(fit, err, sizeof(err));
	fit.close();
	if (!ok) {
		printf("[BENCH] ERROR: %s (%s)\n", err, path);
		return false;
	}
	if (scenario.mission[0] && missionName && (S_stricmp(scenario.mission, missionName) != 0))
		printf("[BENCH] warning: scenario is for %s, running in %s\n", scenario.mission, missionName);
	printf("[BENCH] scenario %s: %ld segments, %ld camera keys\n", path, scenario.numSegments, scenario.numKeys);
	return true;
}

// Moves the player's units and the nearest enemies into two facing lines
// across the arena centre.
void gather (void) {

	std::vector<MoverPtr> home, enemies;
	for (long i = 0; i < ObjectManager->getNumMovers(); i++) {
		MoverPtr m = ObjectManager->getMover(i);
		if (!canFight(m))
			continue;
		if (m->getTeam() == Team::home)
			home.push_back(m);
		else if (Team::home && Team::home->isEnemy(m->getTeam()))
			enemies.push_back(m);
	}

	if (scenario.hasArena)
		arena = onGround(scenario.arenaX, scenario.arenaY);
	else {
		float x = 0.0f, y = 0.0f;
		for (size_t i = 0; i < home.size(); i++) {
			x += home[i]->getPosition().x;
			y += home[i]->getPosition().y;
		}
		if (!home.empty()) {
			x /= (float)home.size();
			y /= (float)home.size();
		}
		arena = onGround(x, y);
	}

	// nearest enemies first; stable, so ties keep the mover-list order
	std::stable_sort(enemies.begin(), enemies.end(), [](MoverPtr a, MoverPtr b) {
		return a->distanceFrom(arena) < b->distanceFrom(arena);
	});
	if ((long)enemies.size() > scenario.gather)
		enemies.resize(scenario.gather);

	fighters.clear();
	const std::vector<MoverPtr>* sides[2] = { &home, &enemies };
	for (int side = 0; side < 2; side++) {
		const std::vector<MoverPtr>& units = *sides[side];
		const float x = arena.x + (side ? 0.5f : -0.5f) * scenario.lineGap;
		for (size_t i = 0; i < units.size(); i++) {
			const float y = arena.y + ((float)i - 0.5f * (float)(units.size() - 1)) * scenario.spacing;
			units[i]->setAwake(true);
			Stuff::Vector3D to = onGround(x, y);
			units[i]->setTeleportPosition(to);
			fighters.push_back(units[i]);
		}
	}
	printf("[BENCH] gathered %d player and %d enemy units at %.0f, %.0f\n",
		(int)home.size(), (int)enemies.size(), arena.x, arena.y);
}

// Every fighter attacks the nearest enemy fighter in the arena.
void orderAttacks (void) {

	for (size_t i = 0; i < fighters.size(); i++) {
		MoverPtr m = fighters[i];
		if (!canFight(m) || !m->getPilot())
			continue;
		MoverPtr best = NULL;
		float bestDistance = 0.0f;
		for (size_t j = 0; j < fighters.size(); j++) {
			MoverPtr t = fighters[j];
			if (!canFight(t) || !m->isEnemy(t->getTeam()))
				continue;
			Stuff::Vector3D at = t->getPosition();
			const float d = m->distanceFrom(at);
			if (!best || d < bestDistance) {
				best = t;
				bestDistance = d;
			}
		}
		if (best) {
			const long origin = (m->getTeam() == Team::home) ? ORDER_ORIGIN_PLAYER : ORDER_ORIGIN_COMMANDER;
			m->getPilot()->orderAttackObject(false, origin, best, ATTACK_TO_DESTROY, ATTACKMETHOD_RANGED,
				FIRERANGE_OPTIMAL, -1, -1, TACORDER_PARAM_PURSUE);
		}
	}
}

// FNV-1a over every mover's exact position and facing bits and its status:
// equal across runs while the game logic is, and different from the first
// frame it is not.
unsigned int stateHash (void) {

	unsigned int h = 2166136261u;
	auto mix = [&h](const void* p, size_t n) {
		const unsigned char* b = (const unsigned char*)p;
		for (size_t i = 0; i < n; i++) {
			h ^= b[i];
			h *= 16777619u;
		}
	};
	for (long i = 0; i < ObjectManager->getNumMovers(); i++) {
		MoverPtr m = ObjectManager->getMover(i);
		if (!m)
			continue;
		const Stuff::Vector3D p = m->getPosition();
		const float r = m->getRotation();
		const long status = m->getStatus();
		mix(&p.x, sizeof(p.x));
		mix(&p.y, sizeof(p.y));
		mix(&p.z, sizeof(p.z));
		mix(&r, sizeof(r));
		mix(&status, sizeof(status));
	}
	return h;
}

// MC2_BENCH_DUMP=<frame>[,<count>]: print every mover's exact state for count
// frames (default 300) from that frame, to diff two runs whose state column
// differs around there (determinism hunts; the first differing line names
// the unit and frame).
void dumpState (void) {

	static long dumpFrame = -2, dumpCount = 300;
	if (dumpFrame == -2) {
		const char* s = getenv("MC2_BENCH_DUMP");
		dumpFrame = s ? atol(s) : -1;
		const char* comma = s ? strchr(s, ',') : NULL;
		if (comma)
			dumpCount = atol(comma + 1);
	}
	if (dumpFrame < 0 || frame < dumpFrame || frame >= dumpFrame + dumpCount)
		return;
	for (long i = 0; i < ObjectManager->getNumMovers(); i++) {
		MoverPtr m = ObjectManager->getMover(i);
		if (!m)
			continue;
		const Stuff::Vector3D p = m->getPosition();
		MechWarriorPtr pilot = m->getPilot();
		printf("[STATE] %ld %ld %s team %ld pos %a %a %a rot %a status %d awake %d order %d\n",
			frame, i, m->getName() ? m->getName() : "?", m->getTeamId(), p.x, p.y, p.z,
			m->getRotation(), (int)m->getStatus(), (int)m->getAwake(),
			pilot ? (int)pilot->getCurTacOrder()->code : -1);
	}
}

// Mean position of the units still fighting, and how many there are (x and
// y are left alone when there are none).
long fightCentre (float& x, float& y) {

	float sx = 0.0f, sy = 0.0f;
	long n = 0;
	for (size_t i = 0; i < fighters.size(); i++)
		if (canFight(fighters[i])) {
			sx += fighters[i]->getPosition().x;
			sy += fighters[i]->getPosition().y;
			n++;
		}
	if (n > 0) {
		x = sx / (float)n;
		y = sy / (float)n;
	}
	return n;
}

// Scenario health, at each segment start: how many fighters are left, where
// the fight is and how spread out it is (mean distance from its centre).
void logFight (void) {

	float x = 0.0f, y = 0.0f;
	const long n = fightCentre(x, y);
	if (n == 0) {
		printf("[BENCH] frame %ld: nobody left fighting\n", frame);
		return;
	}
	float spread = 0.0f, farthest = 0.0f;
	for (size_t i = 0; i < fighters.size(); i++)
		if (canFight(fighters[i])) {
			const float dx = fighters[i]->getPosition().x - x;
			const float dy = fighters[i]->getPosition().y - y;
			const float d = sqrtf(dx * dx + dy * dy);
			spread += d;
			farthest = (d > farthest) ? d : farthest;
		}
	printf("[BENCH] frame %ld: %ld of %d fighting, centre %.0f, %.0f, mean spread %.0f, farthest %.0f\n",
		frame, n, (int)fighters.size(), x, y, spread / (float)n, farthest);
	const Stuff::Vector3D at = eye->getPosition();
	const Stuff::Vector3D rot = eye->getRotation();
	const Stuff::Vector3D look = eye->getLookVector();
	printf("[BENCH] frame %ld: camera at %.0f, %.0f, tilt %.0f, yaw %.0f, look %.2f %.2f %.2f\n",
		frame, at.x, at.y, rot.x, rot.y, look.x, look.y, look.z);
}

void strike (void) {

	if (!Commander::home)
		return;
	float dx, dy;
	BenchScenario::strikeOffset(strikes++, scenario.strikeRadius, dx, dy);
	CallArtillery(Commander::home->getId(), ARTILLERY_LARGE, onGround(arena.x + dx, arena.y + dy), 3, false);
}

void moveCamera (void) {

	// nothing else may move the camera: follow targets, ABL goals, movies
	((GameCamera*)eye)->setTarget(NULL);
	eye->setCameraTargetId(-1);
	eye->setGoalPosTime(0.0f);
	eye->setGoalRotTime(0.0f);
	eye->setGoalVelTime(0.0f);
	Camera::inMovieMode = false;

	// Follow: ease towards the centre of the units still fighting (from
	// frame 2, once the teleports have landed), so pursuit never takes the
	// battle out of shot. Positions only, so still frame-for-frame repeatable.
	if (frame < 2 || !scenario.follow) {
		viewX = arena.x;
		viewY = arena.y;
	}
	else {
		float x = 0.0f, y = 0.0f;
		if (fightCentre(x, y) > 0) {
			viewX += (x - viewX) * 0.05f;
			viewY += (y - viewY) * 0.05f;
		}
	}

	// Framing: the camera turns about its aim point, which the engine puts at
	// the centre of the screen, under the HUD. So project the fight's centre
	// with the last camera update (earlier frames only) and move the aim a
	// little each frame until the fight sits at ScreenX, ScreenY, clear of
	// the HUD at any yaw, tilt or altitude.
	const BenchCameraKey key = scenario.cameraAt(frame);
	if (frame >= 2) {
		const float d = 100.0f;
		Stuff::Vector3D p = onGround(viewX + key.dx, viewY + key.dy);
		Stuff::Vector3D px = p, py = p;
		px.x += d;
		py.y += d;
		Stuff::Vector4D s0, sx, sy;
		eye->projectZ(p, s0);
		eye->projectZ(px, sx);
		eye->projectZ(py, sy);
		const float s[2] = { s0.x, s0.y };
		const float t[2] = { scenario.screenX * eye->fgetScreenResX(), scenario.screenY * eye->fgetScreenResY() };
		const float jx[2] = { (sx.x - s0.x) / d, (sx.y - s0.y) / d };
		const float jy[2] = { (sy.x - s0.x) / d, (sy.y - s0.y) / d };
		float dx, dy;
		if (BenchScenario::frameStep(s, t, jx, jy, 0.1f, dx, dy)) {
			aimX += dx;
			aimY += dy;
			// never more than a screen's worth away, whatever the projection did
			const float r = sqrtf(aimX * aimX + aimY * aimY), most = 2.0f * key.altitude;
			if (r > most) {
				aimX *= most / r;
				aimY *= most / r;
			}
		}
	}
	eye->setPosition(onGround(viewX + key.dx + aimX, viewY + key.dy + aimY), false);
	Stuff::Vector3D rot;
	rot.x = key.tilt;
	rot.y = key.yaw;
	rot.z = key.altitude;
	eye->setRotation(rot);
}

}

void BenchDirector::update (const char* missionName) {

	if (off)
		return;
	if (!loaded) {
		if (!load(missionName)) {
			off = true;
			return;
		}
		loaded = true;
	}

	// a battle that never ends and nobody loses
	neverEndingStory = true;
	for (long t = 0; t < MAX_TEAMS; t++)
		Team::noPain[t] = true;
	ShowMovers = true;

	// state as left by the previous frame's updates
	gos_BenchStateHash(stateHash());
	dumpState();

	if (frame == 0)
		gather();
	// teleports apply during the units' own update, so order from frame 2
	if ((frame >= 2) && (scenario.reorderEvery > 0) && ((frame - 2) % scenario.reorderEvery == 0))
		orderAttacks();

	const long seg = scenario.segmentAt(frame);
	if (seg != segment) {
		segment = seg;
		gos_BenchSegment(scenario.segments[seg].name);
		logFight();
	}
	const BenchSegment& s = scenario.segments[seg];
	if ((s.strikeEvery > 0) && ((frame - s.start) % s.strikeEvery == 0))
		strike();
	for (long i = 0; i < scenario.numSegments; i++)
		if (scenario.segments[i].shot == frame)
			gos_BenchScreenshot(scenario.segments[i].name);

	moveCamera();
	frame++;
}
