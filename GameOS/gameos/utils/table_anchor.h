#ifndef TABLE_ANCHOR_H
#define TABLE_ANCHOR_H

// The tabletop mode's table (planning/future-features/AR-TABLETOP.md): where
// the battle sits in the room, shared by the Quest (OpenXR) and phones
// (ARCore). Poses are in the tracking system's axes, which both share: x
// right, y up, z back, metres. The table is level, so it is a point and a
// heading. GameOS hands the game eyes and pointer rays in the table's frame,
// with the battle's centre at the origin; the game camera then scales them by
// the table's scale. GL-free so it can be unit tested.
namespace table_anchor {

struct Table {
    float position[3];  // the battle's centre on the table's surface
    float yaw;          // about +y, radians; 0: the map's top is ahead (-z)
};

// Real metres to the table's world metres: 1:600 by default, so a mech
// about 10 m tall stands under 2 cm and the terrain window covers a dining
// table. The range the spec names is 1:285 to 1:850; wider is allowed to try.
const float kDefaultScale = 600.0f;
const float kMinScale = 50.0f;
const float kMaxScale = 5000.0f;
// Where a table is put when nothing better is known (the Quest at first, a
// phone before a surface is found): this far ahead of the head and this far
// below it, in metres.
const float kDefaultDistance = 0.6f;
const float kDefaultDrop = 0.5f;

struct Settings {
    float scale;
    float distance;
    float drop;
};

// From MC2_TABLE_SCALE (world metres to a real one) and MC2_TABLE_PLACE
// (distance,drop in metres); null, out of range or not a number: the default.
Settings settingsFrom(const char* scale, const char* place);

// A table ahead of a head at headPosition facing headYaw (about +y), facing
// the same way.
Table inFront(const float headPosition[3], float headYaw, float distance, float drop);

// A tracked pose (position, orientation x, y, z, w) in the table's frame.
void toTable(const Table& table, const float position[3], const float orientation[4],
             float tablePosition[3], float tableOrientation[4]);

}

#endif // TABLE_ANCHOR_H
