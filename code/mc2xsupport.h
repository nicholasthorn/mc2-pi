// code/mc2xsupport.h
//
// wolfman-MC2X's support units: the HUD's 22 support buttons
// ([Vehicle0] to [Vehicle21] of mc2x_buttonlayout<w>.fit) over two support
// tabs. Blocks 0 to 6 are stock's seven at MC2X's prices, 7 to 20 its own
// units, 21 Cancel. MC2X documents them only in each button's help text
// (name and price) and its inventory (mc2x_inv.csv: each unit's object
// number), its pilot files (pmw<unit>.fit) and the mission files' switches
// (<x>EnabledDefault); this table is read from those. Pure logic: no engine
// types.

#ifndef MC2XSUPPORT_H
#define MC2XSUPPORT_H

namespace mc2xsupport
{

const int kButtonCount = 22;	// [Vehicle0] to [Vehicle21]
const int kFirstUnit = 7;	// the first of MC2X's own units
const int kUnitCount = 14;	// blocks 7 to 20
const int kCancel = 21;

// the mission file's switches for MC2X's own units ("<name>EnabledDefault")
enum Switch
{
	kSpotter = 0,	// JamFly
	kHover,			// infantry carrier
	kRescue,
	kElemental,		// specialist
	kDrone,
	kSupply,
	kMarines,		// mercenaries
	kLander,
	kHumvee,
	kTransport,
	kStinger,		// FlySwatter
	kKamikaze,		// one, and the swarm
	kDeployer,
	kSwitchCount
};

struct Unit
{
	int block;			// [Vehicle<block>]
	long objectNumber;	// its object type (inventory FitID)
	const char* pilot;	// its pilot file, without .fit
	long cost;			// resource points: the price in its help text
	int count;			// units delivered (the kamikaze swarm: 5)
	Switch missionSwitch;
};

// MC2X's own unit on [Vehicle<block>], or nullptr (stock's seven, Cancel,
// out of range).
const Unit* unitForBlock(int block);

// The unit whose object type this is, or nullptr. The kamikaze swarm shares
// the single kamikaze's.
const Unit* unitForObject(long objectNumber);

// The support tab a block is on: 1, or 2 for the second tab MC2X adds
// (blocks 14 to 20). Cancel is on both: 0.
int pageForBlock(int block);

// The price of any block but Cancel, in resource points (stock's seven at
// MC2X's prices); 0 for Cancel and out of range.
long costForBlock(int block);

// The mission file key of a switch, "<name>EnabledDefault".
const char* switchKey(Switch s);

}

#endif
