// code/shopfilter.h
//
// wolfman-MC2X's unit shop (mc2x_m$.fit) has unit-type buttons, ids 61 to 70,
// over the inventory's Group ID column (mc2x_inv.csv, column 2). MC2X lists
// its vehicles, infantry and dropships as mechs, so they are in the shop's
// list already; a button shows one group. Groups, from the data: 2 mechs,
// 3 copters and drones, 4 spheroid dropships (Union, Overlord), 5 aerodyne
// dropships (Leopard, Broadsword), 6 hovercraft, 7 ground vehicles,
// 8 infantry. Stock's and Omnitech's shops have no such buttons, so they
// never filter. Game-free so it can be unit tested.

#ifndef SHOPFILTER_H
#define SHOPFILTER_H

namespace shopfilter
{

const int kAllGroups = -1;	// every unit
const int kNotAFilter = -2;	// the button is not a unit-type filter

// The group a shop button shows, kAllGroups, or kNotAFilter.
inline int groupForButton(unsigned long buttonId)
{
	switch (buttonId)
	{
	case 61: return 2;	// Mech
	case 62: return 1;	// Airplane (no unit in R70 or REAP3 has group 1)
	case 63: return 6;	// Hovercraft
	case 64: return 7;	// Ground vehicle
	case 65: return 8;	// Infantry
	case 66: return 3;	// Copter
	case 67: return 5;	// Dropship, aerodyne
	case 68: return 4;	// Dropship, spheroid
	case 69: return kAllGroups;	// All
	case 70: return kAllGroups;	// "Bingo": undocumented, shows all
	default: return kNotAFilter;
	}
}

// Whether a unit of this group is listed under this filter.
inline bool shows(int filter, int group)
{
	return (filter == kAllGroups) || (filter == group);
}

} // namespace shopfilter

#endif
