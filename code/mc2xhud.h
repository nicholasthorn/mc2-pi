// code/mc2xhud.h
//
// wolfman-MC2X's HUD command buttons beyond formations and support units,
// as their help text in mc2xres.dll describes them (nothing else does):
//   Button22  "Convert 10000 of your current CBills into Resource Points"
//   Button23  "Convert all your current Resource Points into CBills"
//   Button53  "Get a 60 second view on the Secret Spy Map showing everything
//             on the battlefield for 1000000 CBILLS"
// The exchanges are one for one: the help text gives no rate. The buttons
// for the selected units below are this port's reading of their one line
// of help each (planning/future-features/MC2X-COMPAT.md, Progress 23).
// Pure logic.

#ifndef MC2XHUD_H
#define MC2XHUD_H

namespace mc2xhud
{

const int kToResourcePoints = 22;
const int kToCBills = 23;
const int kSpyMap = 53;

// the buttons for the units selected (their help text):
const int kRechargeIndicator = 20;	// "Indicates Recharge Option for selected Repair Unit"
const int kDropshipFire = 44;		// "Order the selected Spheroid Dropship to fire at indicated area"
const int kRechargeMode = 45;		// "Activates Recharge Repair Vehicle Mode for the selected Supply Carrier Unit"
const int kReloadMode = 46;			// "Activates Reload Supply Vehicle Mode for the selected Supply Carrier Unit"
const int kPickUp = 47;				// "Pick Up unit from battlefield to airlift"
const int kDropOff = 48;			// "Drop Off unit from airlift to battlefield"
const int kExitUnit = 50;			// "Exit this unit with your pilot to capture a different unit"
const int kHideId = 51;				// "Hide the real Identification of this unit (Combat IFF => OFF)"
const int kUnhideId = 52;			// "Un-Hide the real Identification of this unit (Combat IFF => ON)"

// What the player's selected units are, for the buttons above.
struct Selection
{
	int units = 0;			// selected units of the player's
	int dropships = 0;		// spheroid dropships among them
	int transports = 0;		// able to carry units
	int carrying = 0;		// transports carrying at least one
	int supply = 0;			// supply carriers
	int repair = 0;			// repair vehicles (not supply carriers)
	int hidden = 0;			// with their ID hidden
};

// The buttons above, which act on the selected units.
inline bool isSelectionButton(int button)
{
	switch (button)
	{
		case kRechargeIndicator:
		case kDropshipFire:
		case kRechargeMode:
		case kReloadMode:
		case kPickUp:
		case kDropOff:
		case kExitUnit:
		case kHideId:
		case kUnhideId:
			return true;
	}
	return false;
}

// Whether a selection shows the button. All of the selection must be of the
// kind; the airlift and Exit work on one unit at a time.
inline bool shows(int button, const Selection& s)
{
	if (s.units <= 0)
		return false;
	switch (button)
	{
		case kDropshipFire:
			return s.dropships == s.units;
		case kRechargeMode:
		case kReloadMode:
			return s.supply == s.units;
		case kRechargeIndicator:
			return s.repair == s.units;
		case kPickUp:
			return (s.units == 1) && (s.transports == 1);
		case kDropOff:
			return (s.units == 1) && (s.carrying == 1);
		case kExitUnit:
			return s.units == 1;
		case kHideId:
			return s.hidden < s.units;
		case kUnhideId:
			return s.hidden == s.units;
	}
	return false;
}

const long kExchangeStep = 10000;
const long kSpyMapCost = 1000000;
const float kSpyMapSeconds = 60.f;

// The airlift: whether a unit of unitTons fits a transport's
// TransportCapacity (tons) beside what it carries, within its
// TransportItemLimit (units; 0 when its profile gives none).
inline bool fits(float capacityTons, long itemLimit, float loadedTons, int loadedItems, float unitTons)
{
	if (capacityTons <= 0.f)
		return false;
	if ((itemLimit > 0) && (loadedItems >= itemLimit))
		return false;
	return loadedTons + unitTons <= capacityTons;
}

// How far (world units) a transport reaches to load a unit, and a supply
// carrier to recharge or reload one; and the points a second it gives.
const float kAirliftReach = 150.f;
const float kSupplyReach = 250.f;
const float kSupplyPointsPerSecond = 10.f;

// A supply carrier's top-up of cur towards max over seconds.
inline float topUp(float cur, float max, float seconds)
{
	if (cur >= max || seconds <= 0.f)
		return cur;
	const float next = cur + kSupplyPointsPerSecond * seconds;
	return (next > max) ? max : next;
}

// Whether a press of the button can do anything with these balances.
inline bool canUse(int button, long cBills, long resourcePoints)
{
	switch (button)
	{
		case kToResourcePoints:
			return cBills >= kExchangeStep;
		case kToCBills:
			return resourcePoints > 0;
		case kSpyMap:
			return cBills >= kSpyMapCost;
	}
	return false;
}

// The exchange a press makes: what moves from C-Bills to resource points
// (negative: the other way). 0 when it cannot be used; the spy map moves none.
inline long exchange(int button, long cBills, long resourcePoints)
{
	if (!canUse(button, cBills, resourcePoints))
		return 0;
	if (button == kToResourcePoints)
		return kExchangeStep;
	if (button == kToCBills)
		return -resourcePoints;
	return 0;
}

}

#endif
