#include "table_anchor.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace table_anchor {

namespace {

// a float from text, or false when it is not wholly one
bool parseFloat(const char* text, float& value)
{
    if(!text || !*text)
        return false;
    char* end = nullptr;
    const double v = strtod(text, &end);
    if(end == text || (*end != '\0' && *end != ','))
        return false;
    value = (float)v;
    return true;
}

}

Settings settingsFrom(const char* scale, const char* place)
{
    Settings s{kDefaultScale, kDefaultDistance, kDefaultDrop};
    float v = 0.0f;
    if(parseFloat(scale, v) && v >= kMinScale && v <= kMaxScale)
        s.scale = v;
    if(place) {
        if(parseFloat(place, v) && v > 0.0f && v < 5.0f)
            s.distance = v;
        const char* comma = place;
        while(*comma && *comma != ',')
            ++comma;
        if(*comma == ',' && parseFloat(comma + 1, v) && v > -2.0f && v < 3.0f)
            s.drop = v;
    }
    return s;
}

Table inFront(const float headPosition[3], float headYaw, float distance, float drop)
{
    // ahead is -z turned by the yaw about +y: (-sin, 0, -cos)
    Table t;
    t.position[0] = headPosition[0] - distance * std::sin(headYaw);
    t.position[1] = headPosition[1] - drop;
    t.position[2] = headPosition[2] - distance * std::cos(headYaw);
    t.yaw = headYaw;
    return t;
}

void toTable(const Table& table, const float position[3], const float orientation[4],
             float tablePosition[3], float tableOrientation[4])
{
    // the inverse of the table's yaw: a rotation about +y by -yaw
    const float c = std::cos(table.yaw), s = std::sin(table.yaw);
    const float dx = position[0] - table.position[0];
    const float dz = position[2] - table.position[2];
    // R(-yaw) about y: x' = c x - s z, z' = s x + c z
    tablePosition[0] = c * dx - s * dz;
    tablePosition[1] = position[1] - table.position[1];
    tablePosition[2] = s * dx + c * dz;

    // q' = (0, -sin(yaw/2), 0, cos(yaw/2)) * q
    const float hs = -std::sin(table.yaw * 0.5f), hc = std::cos(table.yaw * 0.5f);
    const float x = orientation[0], y = orientation[1], z = orientation[2], w = orientation[3];
    tableOrientation[0] = hc * x + hs * z;
    tableOrientation[1] = hc * y + hs * w;
    tableOrientation[2] = hc * z - hs * x;
    tableOrientation[3] = hc * w - hs * y;
}

}
