// code/formation.h
//
// wolfman-MC2X's move formations: a group ordered to move takes up a shape
// around the point clicked, facing the way it travels. Its HUD has a
// Formation button and a palette of ten (buttons 24 to 34 of
// mc2x_buttonlayout<w>.fit, an indicator of the current one at 35 to 43); its
// string table names them, nothing documents their geometry, so the shapes
// here are this port's own reading of the names. Pure logic: no engine types.

#ifndef FORMATION_H
#define FORMATION_H

#include <cstddef>
#include <vector>

namespace formation
{

enum Kind
{
	kNotAFormation = -1,
	kNone = 0,	// the stock spread (MoverGroup::calcMoveGoals)
	kLine,
	kDoubleLine,
	kColumn,
	kDoubleColumn,
	kWedge,
	kArrow,
	kDispersion,
	kSmallCircle,
	kLargeCircle,
	kCount
};

// MC2X's HUD button ids
const int kFirstPaletteButton = 24;	// 24 to 33: kNone to kLargeCircle
const int kFormationButton = 34;	// opens the palette
const int kFirstIndicatorButton = 35;	// 35 to 43: kLine to kLargeCircle
const int kLastButton = 43;
const int kButtonCount = kLastButton - kFirstPaletteButton + 1;

struct Point
{
	float x;
	float y;
};

// The kind a palette button chooses, or kNotAFormation.
Kind kindForButton(long buttonId);

// The indicator button that shows kind, or -1 (kNone has none).
int indicatorButton(Kind kind);

// count slots of a shape, in units of the spacing between units: x to the
// right of the direction of travel, y forward. Centred on (0, 0), and at least
// one unit apart.
void slots(Kind kind, int count, Point* out);

// The slots placed in the world: centred on goal, facing heading (any
// length; none faces +y), spacing world units per slot.
void place(Kind kind, int count, Point goal, Point heading, float spacing, Point* out);

// Which goal each mover takes (slotOf[mover]), a permutation of 0..count-1:
// the closest mover and goal pair first, so units do not cross over each
// other on the way.
void assign(const Point* movers, const Point* goals, int count, int* slotOf);

// The free cell nearest (row, col) within maxRadius cells, written back, or
// false when there is none. isFree(row, col) says whether a unit may stand
// there.
template <typename FreeFn>
bool nearestFreeCell(int& row, int& col, int maxRadius, FreeFn isFree)
{
	if (isFree(row, col))
		return true;
	int bestRow = 0, bestCol = 0;
	int bestDist = -1;
	for (int dr = -maxRadius; dr <= maxRadius; dr++)
		for (int dc = -maxRadius; dc <= maxRadius; dc++)
		{
			const int dist = dr * dr + dc * dc;
			if ((bestDist >= 0) && (dist >= bestDist))
				continue;
			if (isFree(row + dr, col + dc))
			{
				bestDist = dist;
				bestRow = row + dr;
				bestCol = col + dc;
			}
		}
	if (bestDist < 0)
		return false;
	row = bestRow;
	col = bestCol;
	return true;
}

// The cells a unit can walk to from (row, col), four ways, over cells where
// passable(row, col) holds, without leaving the square of radius cells around
// it. Empty when (row, col) itself is not passable.
class Reach
{
public:
	template <typename PassFn>
	Reach(int row, int col, int squareRadius, PassFn passable)
		: row0(row), col0(col), radius(squareRadius), side(2 * squareRadius + 1),
		  cells((size_t)side * (size_t)side, 0)
	{
		if (!passable(row, col))
			return;
		std::vector<int> open;
		cells[index(row, col)] = 1;
		open.push_back(index(row, col));
		static const int step[4][2] = { { -1, 0 }, { 0, 1 }, { 1, 0 }, { 0, -1 } };
		while (!open.empty())
		{
			const int at = open.back();
			open.pop_back();
			const int r = at / side + row0 - radius;
			const int c = at % side + col0 - radius;
			for (int d = 0; d < 4; d++)
			{
				const int nr = r + step[d][0];
				const int nc = c + step[d][1];
				if (!inside(nr, nc) || cells[index(nr, nc)] || !passable(nr, nc))
					continue;
				cells[index(nr, nc)] = 1;
				open.push_back(index(nr, nc));
			}
		}
	}

	bool contains(int row, int col) const
	{
		return inside(row, col) && cells[index(row, col)];
	}

private:
	bool inside(int row, int col) const
	{
		return (row >= row0 - radius) && (row <= row0 + radius)
			&& (col >= col0 - radius) && (col <= col0 + radius);
	}
	int index(int row, int col) const
	{
		return (row - row0 + radius) * side + (col - col0 + radius);
	}

	int row0, col0, radius, side;
	std::vector<unsigned char> cells;
};

}

#endif
