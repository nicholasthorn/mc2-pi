// code/formation.cpp
//
// wolfman-MC2X's move formations (formation.h).

#include "formation.h"

#include <cmath>

namespace formation
{

Kind kindForButton(long buttonId)
{
	const long kind = buttonId - kFirstPaletteButton;
	if ((kind < kNone) || (kind >= kCount))
		return kNotAFormation;
	return (Kind)kind;
}

int indicatorButton(Kind kind)
{
	if ((kind <= kNone) || (kind >= kCount))
		return -1;
	return kFirstIndicatorButton + (kind - kLine);
}

namespace
{

// count slots in rows of perRow, rows one apart going back, each row centred
void ranks(int count, int perRow, float gap, Point* out)
{
	for (int i = 0; i < count; i++)
	{
		const int row = i / perRow;
		const int inRow = ((row + 1) * perRow <= count) ? perRow : count - row * perRow;
		const int col = i % perRow;
		out[i].x = ((float)col - (float)(inRow - 1) * 0.5f) * gap;
		out[i].y = -(float)row * gap;
	}
}

// the same, turned a quarter: files one apart going right
void files(int count, int perFile, Point* out)
{
	ranks(count, perFile, 1.f, out);
	for (int i = 0; i < count; i++)
	{
		const float x = out[i].x;
		out[i].x = -out[i].y;
		out[i].y = x;
	}
}

// a V pointing forward: the leader, then a pair a step further back each time;
// withShaft adds a unit behind the leader at each step (an arrow)
void vee(int count, bool withShaft, Point* out)
{
	const int perStep = withShaft ? 3 : 2;
	out[0].x = 0.f;
	out[0].y = 0.f;
	for (int i = 1; i < count; i++)
	{
		const int step = (i - 1) / perStep + 1;
		const int which = (i - 1) % perStep;
		out[i].x = (which == 0) ? -(float)step : ((which == 1) ? (float)step : 0.f);
		out[i].y = -(float)step;
	}
}

void circle(int count, float radius, Point* out)
{
	const float pi = 3.14159265358979f;
	for (int i = 0; i < count; i++)
	{
		const float a = 2.f * pi * (float)i / (float)count;
		out[i].x = radius * std::sin(a);
		out[i].y = radius * std::cos(a);
	}
}

// the radius at which count units on a circle stand one apart
float tightRadius(int count)
{
	const float pi = 3.14159265358979f;
	const float r = 0.5f / std::sin(pi / (float)count);
	return (r > 1.f) ? r : 1.f;
}

int squareSide(int count)
{
	int side = 1;
	while (side * side < count)
		side++;
	return side;
}

}

void slots(Kind kind, int count, Point* out)
{
	if (count <= 0)
		return;
	if (count == 1)
	{
		out[0].x = out[0].y = 0.f;
		return;
	}

	switch (kind)
	{
		case kLine:
			ranks(count, count, 1.f, out);
			break;
		case kDoubleLine:
			ranks(count, (count + 1) / 2, 1.f, out);
			break;
		case kColumn:
			files(count, count, out);
			break;
		case kDoubleColumn:
			files(count, (count + 1) / 2, out);
			break;
		case kWedge:
			vee(count, false, out);
			break;
		case kArrow:
			vee(count, true, out);
			break;
		case kDispersion:
			ranks(count, squareSide(count), 2.f, out);
			break;
		case kSmallCircle:
			circle(count, tightRadius(count), out);
			break;
		case kLargeCircle:
			circle(count, 2.f * tightRadius(count), out);
			break;
		default:
			// kNone is the stock spread, not a shape: a tight grid
			ranks(count, squareSide(count), 1.f, out);
			break;
	}

	// centred on the point clicked
	Point c = { 0.f, 0.f };
	for (int i = 0; i < count; i++)
	{
		c.x += out[i].x;
		c.y += out[i].y;
	}
	c.x /= (float)count;
	c.y /= (float)count;
	for (int i = 0; i < count; i++)
	{
		out[i].x -= c.x;
		out[i].y -= c.y;
	}
}

void place(Kind kind, int count, Point goal, Point heading, float spacing, Point* out)
{
	float len = std::sqrt(heading.x * heading.x + heading.y * heading.y);
	Point fwd = { 0.f, 1.f };
	if (len > 1e-3f)
	{
		fwd.x = heading.x / len;
		fwd.y = heading.y / len;
	}
	const Point right = { fwd.y, -fwd.x };

	slots(kind, count, out);
	for (int i = 0; i < count; i++)
	{
		const Point s = out[i];
		out[i].x = goal.x + (right.x * s.x + fwd.x * s.y) * spacing;
		out[i].y = goal.y + (right.y * s.x + fwd.y * s.y) * spacing;
	}
}

void assign(const Point* movers, const Point* goals, int count, int* slotOf)
{
	const int kMax = 256;
	bool goalTaken[kMax] = {};
	for (int i = 0; i < count; i++)
		slotOf[i] = -1;
	if (count > kMax)
	{
		// more than a selection can hold: in order
		for (int i = 0; i < count; i++)
			slotOf[i] = i;
		return;
	}

	for (int n = 0; n < count; n++)
	{
		int bestMover = -1, bestGoal = -1;
		float bestDist = 0.f;
		for (int m = 0; m < count; m++)
		{
			if (slotOf[m] >= 0)
				continue;
			for (int g = 0; g < count; g++)
			{
				if (goalTaken[g])
					continue;
				const float dx = movers[m].x - goals[g].x;
				const float dy = movers[m].y - goals[g].y;
				const float dist = dx * dx + dy * dy;
				if ((bestMover < 0) || (dist < bestDist))
				{
					bestMover = m;
					bestGoal = g;
					bestDist = dist;
				}
			}
		}
		if (bestMover < 0)
			break;
		slotOf[bestMover] = bestGoal;
		goalTaken[bestGoal] = true;
	}
}

}
