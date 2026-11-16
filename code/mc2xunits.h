// code/mc2xunits.h
//
// wolfman-MC2X's unit commands from its HUD (mc2xhud.h): a hidden ID, the
// airlift, the supply carriers' modes and leaving a unit. The state lives
// here, by watch ID, not in Mover: it is runtime only, so a saved game does
// not keep it (a unit carried is put down before saving, see putDownAll).

#ifndef MC2XUNITS_H
#define MC2XUNITS_H

class Mover;
class GameObject;

namespace mc2xunits
{

enum SupplyMode
{
	kSupplyOff = 0,
	kRecharge,	// recharge repair vehicles' refit points
	kReload		// reload units' ammunition
};

// A new mission: forget every unit's state.
void reset();

// Before a save: every carried unit back on the ground (the airlift is not
// saved).
void putDownAll();

// Each game frame: transports reaching a unit to pick up, carried units of a
// transport that is lost (put down where it was), supply carriers' modes.
void update(float seconds);

// The hidden ID: enemies cannot tell what the unit is, so their sensors and
// eyes do not count it (SensorSystem::calcContactStatus), until it fires.
bool idHidden(GameObject* unit);
void setIdHidden(Mover* unit, bool hidden);
void firedWeapon(unsigned long watchID);

// The airlift: a transport's capacity (its profile's TransportCapacity, in
// tons) and what it carries.
float capacity(Mover* transport);
int carried(Mover* transport);
bool canCarry(Mover* transport, Mover* unit);
// loads the unit when the transport is in reach, else sends it there first;
// false when the unit cannot be carried
bool pickUp(Mover* transport, Mover* unit);
// the player gave the unit another order: a pick-up under way is off
void orderGiven(Mover* unit);
void dropOff(Mover* transport);

// A supply carrier's mode, and whether one recharging is in reach of a repair
// vehicle.
int supplyMode(Mover* carrier);
void setSupplyMode(Mover* carrier, int mode);
bool rechargeInReach(Mover* repairVehicle);

// The pilot leaves the unit, which stands empty (as after an ejection, but
// whole and with the pilot unhurt); the salvage craft can put a pilot into
// another one.
void exitUnit(Mover* unit);

}

#endif
