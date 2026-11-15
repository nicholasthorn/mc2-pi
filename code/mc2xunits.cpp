// code/mc2xunits.cpp
//
// wolfman-MC2X's unit commands from its HUD (mc2xunits.h).

// the game's 2001 headers as system headers (-isystem): the strict
// warnings are for this file
#include <mclib.h>
#include <mover.h>
#include <gvehicl.h>
#include <objmgr.h>
#include <objtype.h>
#include <team.h>
#include <tacordr.h>
#include <warrior.h>
#include <move.h>
#include "formation.h"
#include "mc2xhud.h"
#include "mc2xsupport.h"
#include "mc2xunits.h"

#include <cmath>
#include <map>
#include <vector>

namespace mc2xunits
{

namespace
{

struct UnitState
{
	bool idHidden = false;
	int supplyMode = kSupplyOff;
	std::vector<unsigned long> carried;	// watch IDs, off the map
	unsigned long pickUpTarget = 0;		// a unit the transport is going for
	Stuff::Vector3D pickUpGoal;			// where it was sent to meet it
};

struct TransportProfile
{
	float capacity = 0.f;	// tons
	long itemLimit = 0;		// units, 0: no limit
};

std::map<unsigned long, UnitState> states;
std::map<long, TransportProfile> profiles;	// by object number

UnitState* find(GameObject* unit)
{
	if (!unit)
		return nullptr;
	std::map<unsigned long, UnitState>::iterator it = states.find(unit->getWatchID());
	return (it == states.end()) ? nullptr : &it->second;
}

UnitState& at(GameObject* unit)
{
	return states[unit->getWatchID()];
}

Mover* moverOf(unsigned long watchID)
{
	GameObjectPtr obj = ObjectManager ? ObjectManager->getByWatchID(watchID) : nullptr;
	return (obj && obj->isMover()) ? (Mover*)obj : nullptr;
}

const TransportProfile& profileOf(Mover* transport)
{
	const long type = transport->getTypeHandle();
	std::map<long, TransportProfile>::iterator it = profiles.find(type);
	if (it != profiles.end())
		return it->second;

	// MC2X keeps these in the object's own packet, under [MechProfile]
	TransportProfile profile;
	FitIniFile file;
	if (ObjectTypeManager::openProfile(type, file) == NO_ERR)
	{
		if (file.seekBlock("MechProfile") == NO_ERR)
		{
			long tons = 0;
			if (file.readIdLong("TransportCapacity", tons) == NO_ERR)
				profile.capacity = (float)tons;
			long items = 0;
			if (file.readIdLong("TransportItemLimit", items) == NO_ERR)
				profile.itemLimit = items;
		}
		file.close();
	}
	return profiles[type] = profile;
}

// a and b no further apart than reach, across the ground
bool within2D(const Stuff::Vector3D& a, const Stuff::Vector3D& b, float reach)
{
	return (a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y) <= reach * reach;
}

bool within2D(GameObject* a, GameObject* b, float reach)
{
	return within2D(a->getPosition(), b->getPosition(), reach);
}

bool alive(Mover* m)
{
	return m && m->getExists() && !m->isDestroyed() && !m->isDisabled();
}

float loadedTons(UnitState& state, int& items)
{
	float tons = 0.f;
	items = 0;
	for (size_t i = 0; i < state.carried.size(); i++)
	{
		if (Mover* m = moverOf(state.carried[i]))
		{
			tons += m->tonnage;
			items++;
		}
	}
	return tons;
}

void load(Mover* transport, Mover* unit)
{
	unit->setSelected(false);
	if (MechWarriorPtr pilot = unit->getPilot())
		pilot->clearCurTacOrder();
	unit->setExists(false);
	at(transport).carried.push_back(unit->getWatchID());
}

// a cell a unit can stand on near spot: the transport may hover over water
// or a cliff it cannot be put down on
void standable(Stuff::Vector3D& spot)
{
	if (!land || !GameMap)
		return;
	int row = 0, col = 0;
	land->worldToCell(spot, row, col);
	auto passable = [](int r, int c) {
		return GameMap->inBounds(r, c) && GameMap->getPassable(r, c);
	};
	if (formation::nearestFreeCell(row, col, 12, passable))
		land->cellToWorld(row, col, spot);
}

// the carried units back on the ground round where the transport is
void putDown(UnitState& state, Stuff::Vector3D where)
{
	for (size_t i = 0; i < state.carried.size(); i++)
	{
		Mover* m = moverOf(state.carried[i]);
		if (!m)
			continue;
		Stuff::Vector3D spot = where;
		const float angle = Stuff::Two_Pi * (float)i / (float)state.carried.size();
		if (state.carried.size() > 1)
		{
			spot.x += 80.f * std::cos(angle);
			spot.y += 80.f * std::sin(angle);
		}
		standable(spot);
		if (land)
			spot.z = land->getTerrainElevation(spot);
		m->setPosition(spot);
		m->setExists(true);
	}
	state.carried.clear();
}

// cargo whose transport is gone (or never named) lands where the transport,
// else the first unit in it, last was
void putDownWhereLost(Mover* transport, UnitState& state)
{
	Stuff::Vector3D where(0.f, 0.f, 0.f);
	if (transport)
		where = transport->getPosition();
	else if (Mover* first = moverOf(state.carried[0]))
		where = first->getPosition();
	putDown(state, where);
}

// nothing set for the unit: its entry can go
bool atDefault(const UnitState& state)
{
	return !state.idHidden && (state.supplyMode == kSupplyOff) && state.carried.empty() && !state.pickUpTarget;
}

void moveTo(Mover* transport, Stuff::Vector3D goal)
{
	TacticalOrder order;
	order.init(ORDER_ORIGIN_PLAYER, TACTICAL_ORDER_MOVETO_POINT, false);
	order.setWayPoint(0, goal);
	order.moveParams.wait = false;
	order.moveParams.wayPath.mode[0] = TRAVEL_MODE_FAST;
	order.pack(NULL, NULL);
	transport->handleTacticalOrder(order);
}

// one unit's pick-up and supply, for seconds
void updateUnit(Mover* unit, UnitState& state, float seconds)
{
	if (state.pickUpTarget)
	{
		Mover* target = moverOf(state.pickUpTarget);
		if (!alive(target) || !canCarry(unit, target))
			state.pickUpTarget = 0;
		else if (within2D(unit, target, mc2xhud::kAirliftReach))
		{
			state.pickUpTarget = 0;
			load(unit, target);
		}
		else
		{
			// the unit moved on: follow it
			const Stuff::Vector3D at = target->getPosition();
			if (!within2D(at, state.pickUpGoal, mc2xhud::kAirliftReach))
			{
				state.pickUpGoal = at;
				moveTo(unit, at);
			}
		}
	}

	if (state.supplyMode == kSupplyOff || !ObjectManager)
		return;
	for (long i = 0; i < ObjectManager->getNumMovers(); i++)
	{
		Mover* other = ObjectManager->getMover(i);
		if (!alive(other) || (other == unit) || (other->getTeam() != unit->getTeam()))
			continue;
		if (!within2D(unit, other, mc2xhud::kSupplyReach))
			continue;
		if (state.supplyMode == kRecharge)
		{
			// a repair vehicle's refit points are its turret armour
			if (!other->isRefit() || mc2xsupport::isSupplyCarrier(other->getTypeHandle()))
				continue;
			ArmorLocation& pool = other->armor[GROUNDVEHICLE_LOCATION_TURRET];
			pool.curArmor = mc2xhud::topUp(pool.curArmor, pool.maxArmor, seconds);
		}
		else
		{
			float used = 0.f;
			other->refit(mc2xhud::kSupplyPointsPerSecond * seconds, used, true);
		}
	}
}

}

void reset()
{
	states.clear();
	profiles.clear();
}

void putDownAll()
{
	for (std::map<unsigned long, UnitState>::iterator it = states.begin(); it != states.end(); ++it)
	{
		if (!it->second.carried.empty())
			putDownWhereLost(moverOf(it->first), it->second);
	}
}

void update(float seconds)
{
	for (std::map<unsigned long, UnitState>::iterator it = states.begin(); it != states.end(); )
	{
		UnitState& state = it->second;
		Mover* unit = moverOf(it->first);

		// a transport lost with its cargo puts it down where it was
		if (!state.carried.empty() && (!unit || unit->isDestroyed() || unit->isDisabled()))
			putDownWhereLost(unit, state);
		if (!alive(unit))
			state.pickUpTarget = 0;
		else
			updateUnit(unit, state, seconds);

		// a unit gone from the game, or with nothing set, needs no entry
		if (!unit || atDefault(state))
			it = states.erase(it);
		else
			++it;
	}
}

bool idHidden(GameObject* unit)
{
	UnitState* state = find(unit);
	return state && state->idHidden;
}

void setIdHidden(Mover* unit, bool hidden)
{
	at(unit).idHidden = hidden;
}

void firedWeapon(unsigned long watchID)
{
	std::map<unsigned long, UnitState>::iterator it = states.find(watchID);
	if (it != states.end())
		it->second.idHidden = false;	// firing gives the unit away
}

float capacity(Mover* transport)
{
	return transport ? profileOf(transport).capacity : 0.f;
}

int carried(Mover* transport)
{
	UnitState* state = find(transport);
	return state ? (int)state->carried.size() : 0;
}

bool canCarry(Mover* transport, Mover* unit)
{
	if (!transport || !unit || (transport == unit) || !alive(unit))
		return false;
	if ((unit->getTeam() != transport->getTeam()) || (capacity(unit) > 0.f))
		return false;	// friends only, and no transport in another
	const TransportProfile& profile = profileOf(transport);
	int items = 0;
	float tons = 0.f;
	if (UnitState* state = find(transport))
		tons = loadedTons(*state, items);
	return mc2xhud::fits(profile.capacity, profile.itemLimit, tons, items, unit->tonnage);
}

bool pickUp(Mover* transport, Mover* unit)
{
	if (!alive(transport) || !canCarry(transport, unit))
		return false;
	if (within2D(transport, unit, mc2xhud::kAirliftReach))
		load(transport, unit);
	else
	{
		UnitState& state = at(transport);
		state.pickUpTarget = unit->getWatchID();
		state.pickUpGoal = unit->getPosition();
		moveTo(transport, state.pickUpGoal);
	}
	return true;
}

void orderGiven(Mover* unit)
{
	if (UnitState* state = find(unit))
		state->pickUpTarget = 0;
}

void dropOff(Mover* transport)
{
	if (UnitState* state = find(transport))
		putDown(*state, transport->getPosition());
}

int supplyMode(Mover* carrier)
{
	UnitState* state = find(carrier);
	return state ? state->supplyMode : kSupplyOff;
}

void setSupplyMode(Mover* carrier, int mode)
{
	at(carrier).supplyMode = mode;
}

bool rechargeInReach(Mover* repairVehicle)
{
	for (std::map<unsigned long, UnitState>::iterator it = states.begin(); it != states.end(); ++it)
	{
		if (it->second.supplyMode != kRecharge)
			continue;
		Mover* carrier = moverOf(it->first);
		if (alive(carrier) && (carrier->getTeam() == repairVehicle->getTeam())
			&& within2D(carrier, repairVehicle, mc2xhud::kSupplyReach))
			return true;
	}
	return false;
}

void exitUnit(Mover* unit)
{
	MechWarriorPtr pilot = unit ? unit->getPilot() : nullptr;
	if (!pilot)
		return;
	// an ejection without the damage: no wound, and a mech keeps its head
	// (Mover's, not BattleMech's, which blows it off)
	const float wounds = pilot->getWounds();
	unit->Mover::handleEjection();
	if (pilot->getStatus() == WARRIOR_STATUS_EJECTED)
		pilot->setWounds(wounds);
	unit->setSelected(false);
}

}
