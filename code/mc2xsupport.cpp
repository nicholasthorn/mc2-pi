// code/mc2xsupport.cpp
//
// wolfman-MC2X's support units (mc2xsupport.h).

#include "mc2xsupport.h"

namespace mc2xsupport
{

namespace
{

// Object numbers are the FitID column of MC2X's inventory (mc2x_inv.csv).
// Two are this port's reading: the rescue vehicle is the inventory's
// Ambulance (the only rescue-like vehicle, and it repairs); the FlySwatter
// button's help says it brings a RavenDX, but its name, its pilot (Swatter)
// and the inventory's Swatter row name the FlySwatter vehicle, which is used.
const Unit kUnits[kUnitCount] = {
	{ 7, 1464, "pmwjamfly", 250000, 1, kSpotter },
	{ 8, 238, "pmwcarrier", 80000, 1, kHover },
	{ 9, 219, "pmwrescue", 20000, 1, kRescue },
	{ 10, 1342, "pmwspecialist", 5000, 1, kElemental },
	{ 11, 1366, "pmwdrone", 15000, 1, kDrone },
	{ 12, 1259, "pmwsupply", 20000, 1, kSupply },
	{ 13, 1444, "pmwmercenary", 20000, 1, kMarines },
	{ 14, 1470, "pmwlander", 250000, 1, kLander },
	{ 15, 1336, "pmwhumvee", 25000, 1, kHumvee },
	{ 16, 1465, "pmwtransport", 220000, 1, kTransport },
	{ 17, 1480, "pmwswatter", 655000, 1, kStinger },
	{ 18, 1478, "pmwkamikaze", 25000, 1, kKamikaze },
	{ 19, 1478, "pmwkamikaze", 125000, 5, kKamikaze },
	{ 20, 1473, "pmwdeployer", 1020000, 1, kDeployer },
};

// stock's seven in MC2X's order: airstrike, sensor probe, repair vehicle,
// scout copter, minelayer, salvage craft, mobile artillery
const long kStockCosts[kFirstUnit] = { 20000, 3000, 21000, 20000, 22000, 25000, 20000 };

const char* const kSwitchKeys[kSwitchCount] = {
	"SpotterEnabledDefault",
	"HoverEnabledDefault",
	"RescueEnabledDefault",
	"ElementalEnabledDefault",
	"DroneEnabledDefault",
	"SupplyEnabledDefault",
	"MarinesEnabledDefault",
	"LanderEnabledDefault",
	"HumveeEnabledDefault",
	"TransportEnabledDefault",
	"StingerEnabledDefault",
	"KamikazeEnabledDefault",
	"DeployerEnabledDefault",
};

}

const Unit* unitForBlock(int block)
{
	if ((block < kFirstUnit) || (block >= kFirstUnit + kUnitCount))
		return nullptr;
	return &kUnits[block - kFirstUnit];
}

const Unit* unitForObject(long objectNumber)
{
	for (int i = 0; i < kUnitCount; i++)
		if (kUnits[i].objectNumber == objectNumber)
			return &kUnits[i];
	return nullptr;
}

int pageForBlock(int block)
{
	if (block == kCancel)
		return 0;
	return (block >= 14) && (block <= 20) ? 2 : 1;
}

long costForBlock(int block)
{
	if ((block >= 0) && (block < kFirstUnit))
		return kStockCosts[block];
	const Unit* unit = unitForBlock(block);
	return unit ? unit->cost : 0;
}

const char* switchKey(Switch s)
{
	if ((s < 0) || (s >= kSwitchCount))
		return nullptr;
	return kSwitchKeys[s];
}

}
