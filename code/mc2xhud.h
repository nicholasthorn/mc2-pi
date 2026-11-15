// code/mc2xhud.h
//
// wolfman-MC2X's HUD command buttons beyond formations and support units,
// as their help text in mc2xres.dll describes them (nothing else does):
//   Button22  "Convert 10000 of your current CBills into Resource Points"
//   Button23  "Convert all your current Resource Points into CBills"
//   Button53  "Get a 60 second view on the Secret Spy Map showing everything
//             on the battlefield for 1000000 CBILLS"
// The exchanges are one for one: the help text gives no rate. Pure logic.

#ifndef MC2XHUD_H
#define MC2XHUD_H

namespace mc2xhud
{

const int kToResourcePoints = 22;
const int kToCBills = 23;
const int kSpyMap = 53;

const long kExchangeStep = 10000;
const long kSpyMapCost = 1000000;
const float kSpyMapSeconds = 60.f;

// Whether a press of the button can do anything with these balances.
inline bool canUse(int button, long cBills, long resourcePoints, bool spyMapActive)
{
	switch (button)
	{
		case kToResourcePoints:
			return cBills >= kExchangeStep;
		case kToCBills:
			return resourcePoints > 0;
		case kSpyMap:
			return !spyMapActive && (cBills >= kSpyMapCost);
	}
	return false;
}

// The exchange a press makes: what moves from C-Bills to resource points
// (negative: the other way). 0 when it cannot be used; the spy map moves none.
inline long exchange(int button, long cBills, long resourcePoints)
{
	if (!canUse(button, cBills, resourcePoints, false))
		return 0;
	if (button == kToResourcePoints)
		return kExchangeStep;
	if (button == kToCBills)
		return -resourcePoints;
	return 0;
}

}

#endif
