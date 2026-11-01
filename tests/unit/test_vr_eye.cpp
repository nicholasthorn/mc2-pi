// tests/unit/test_vr_eye.cpp
//
// vr_eye: the head-tracked eyes of the VR plan (Phase 2). OpenXR poses come
// in its own axes (x right, y up, z back) and are turned into the engine
// camera's (x screen left, y screen down, z forward, as Camera::projectZ maps
// them); an eye is the game camera with the head rotation applied level with
// the ground, and its frustum edges come from the eye's field of view. A
// sign error here tilts, mirrors or inverts the world in the headset (the
// first headset run had looking up move the view down), so the directions
// are pinned.

#include "doctest.h"

#include "utils/vr_eye.h"

#include <cmath>

namespace {

const float kPi = 3.14159265f;

bool near3(const float a[3], float x, float y, float z, float eps = 1e-4f)
{
	return std::fabs(a[0] - x) < eps && std::fabs(a[1] - y) < eps && std::fabs(a[2] - z) < eps;
}

// a game camera like the engine's at (10, 20, 30), world y up, looking along
// world +z and pitched down by 'pitch' radians: x is screen left, y screen
// down (towards the ground and back when pitched), z forward
vr_eye::Basis gameCamera(float pitch)
{
	vr_eye::Basis b;
	b.x[0] = 1.0f; b.x[1] = 0.0f; b.x[2] = 0.0f;
	b.y[0] = 0.0f; b.y[1] = -std::cos(pitch); b.y[2] = -std::sin(pitch);
	b.z[0] = 0.0f; b.z[1] = -std::sin(pitch); b.z[2] = std::cos(pitch);
	b.position[0] = 10.0f; b.position[1] = 20.0f; b.position[2] = 30.0f;
	return b;
}

// an OpenXR rotation of 'angle' radians about one of its axes
void xrQuat(int axis, float angle, float q[4])
{
	q[0] = q[1] = q[2] = 0.0f;
	q[axis] = std::sin(angle * 0.5f);
	q[3] = std::cos(angle * 0.5f);
}

const float kWorldUp[3] = {0.0f, 1.0f, 0.0f};
const float kNoMove[3] = {0.0f, 0.0f, 0.0f};
const float kIdentity[4] = {0.0f, 0.0f, 0.0f, 1.0f};

vr_eye::Basis eyeFor(const vr_eye::Basis& game, const float xrPos[3], const float xrQuat[4], float unitsPerMetre)
{
	float pos[3], quat[4];
	vr_eye::fromXr(xrPos, xrQuat, pos, quat);
	return vr_eye::eyeBasis(game, kWorldUp, quat, pos, unitsPerMetre, true);
}

}

TEST_SUITE("VrEye")
{

TEST_CASE("OpenXR axes turn into the engine camera's: right, up and back all flip")
{
	float pos[3], quat[4];
	const float xrForward[3] = {0.0f, 0.0f, -1.0f};
	vr_eye::fromXr(xrForward, kIdentity, pos, quat);
	CHECK(near3(pos, 0.0f, 0.0f, 1.0f));
	CHECK(quat[3] == doctest::Approx(1.0f));

	const float xrRight[3] = {1.0f, 0.0f, 0.0f};     // engine x is screen left
	vr_eye::fromXr(xrRight, kIdentity, pos, quat);
	CHECK(near3(pos, -1.0f, 0.0f, 0.0f));

	const float xrUp[3] = {0.0f, 1.0f, 0.0f};        // engine y is screen down
	vr_eye::fromXr(xrUp, kIdentity, pos, quat);
	CHECK(near3(pos, 0.0f, -1.0f, 0.0f));
}

TEST_CASE("an unmoved head sees exactly the game camera")
{
	const vr_eye::Basis game = gameCamera(0.6f);
	const vr_eye::Basis eye = eyeFor(game, kNoMove, kIdentity, 1.0f);
	CHECK(near3(eye.x, game.x[0], game.x[1], game.x[2]));
	CHECK(near3(eye.y, game.y[0], game.y[1], game.y[2]));
	CHECK(near3(eye.z, game.z[0], game.z[1], game.z[2]));
	CHECK(near3(eye.position, 10.0f, 20.0f, 30.0f));
}

TEST_CASE("turning the head left looks left")
{
	float q[4];
	xrQuat(1, 0.5f * kPi, q);   // OpenXR: +angle about y turns -z towards -x
	const vr_eye::Basis eye = eyeFor(gameCamera(0.0f), kNoMove, q, 1.0f);
	// the game camera's screen left is world +x
	CHECK(near3(eye.z, 1.0f, 0.0f, 0.0f));
	CHECK(near3(eye.y, 0.0f, -1.0f, 0.0f));
}

TEST_CASE("looking up looks up, from the game's pitch")
{
	float q[4];
	xrQuat(0, 0.3f, q);         // OpenXR: +angle about x turns -z towards +y
	const float pitch = 0.6f;
	const vr_eye::Basis eye = eyeFor(gameCamera(pitch), kNoMove, q, 1.0f);
	// pitched down by 0.6 - 0.3 radians now, still facing world +z
	CHECK(eye.z[1] == doctest::Approx(-std::sin(pitch - 0.3f)));
	CHECK(eye.z[2] == doctest::Approx(std::cos(pitch - 0.3f)));
	CHECK(eye.z[0] == doctest::Approx(0.0f).epsilon(1e-4));
}

TEST_CASE("turning the head keeps the horizon level and the game's pitch")
{
	const float pitch = 0.6f;
	float q[4];
	xrQuat(1, 0.5f * kPi, q);
	const vr_eye::Basis eye = eyeFor(gameCamera(pitch), kNoMove, q, 1.0f);
	// still pitched down by the same amount, now towards world +x
	CHECK(eye.z[1] == doctest::Approx(-std::sin(pitch)));
	CHECK(eye.z[0] == doctest::Approx(std::cos(pitch)));
	CHECK(eye.z[2] == doctest::Approx(0.0f).epsilon(1e-4));
	// no roll: the eye's screen-left axis stays horizontal
	CHECK(eye.x[1] == doctest::Approx(0.0f).epsilon(1e-4));
}

TEST_CASE("leaning and rising move the eye level with the ground, in world units")
{
	const float forwardHalfMetre[3] = {0.0f, 0.0f, -0.5f};
	const vr_eye::Basis lean = eyeFor(gameCamera(0.6f), forwardHalfMetre, kIdentity, 4.0f);
	CHECK(near3(lean.position, 10.0f, 20.0f, 32.0f));
	const float upOneMetre[3] = {0.0f, 1.0f, 0.0f};
	const vr_eye::Basis rise = eyeFor(gameCamera(0.6f), upOneMetre, kIdentity, 4.0f);
	CHECK(near3(rise.position, 10.0f, 24.0f, 30.0f));
	const float rightOneMetre[3] = {1.0f, 0.0f, 0.0f};
	const vr_eye::Basis step = eyeFor(gameCamera(0.6f), rightOneMetre, kIdentity, 4.0f);
	CHECK(near3(step.position, 6.0f, 20.0f, 30.0f));
}

TEST_CASE("without the game's pitch the horizon is level and looking down finds the game's view")
{
	const float pitch = 0.6f;
	float pos[3], quat[4];
	vr_eye::fromXr(kNoMove, kIdentity, pos, quat);
	const vr_eye::Basis ahead = vr_eye::eyeBasis(gameCamera(pitch), kWorldUp, quat, pos, 1.0f, false);
	CHECK(near3(ahead.z, 0.0f, 0.0f, 1.0f));
	CHECK(near3(ahead.y, 0.0f, -1.0f, 0.0f));

	float down[4];
	xrQuat(0, -pitch, down);    // the head pitched down by the game's angle
	vr_eye::fromXr(kNoMove, down, pos, quat);
	const vr_eye::Basis game = gameCamera(pitch);
	const vr_eye::Basis looking = vr_eye::eyeBasis(game, kWorldUp, quat, pos, 1.0f, false);
	CHECK(near3(looking.z, game.z[0], game.z[1], game.z[2]));
	CHECK(near3(looking.y, game.y[0], game.y[1], game.y[2]));
}

TEST_CASE("a camera looking straight down still gets a ground frame")
{
	const vr_eye::Basis down = gameCamera(0.5f * kPi);
	const vr_eye::Basis eye = eyeFor(down, kNoMove, kIdentity, 1.0f);
	CHECK(near3(eye.z, down.z[0], down.z[1], down.z[2]));
	CHECK(near3(eye.y, down.y[0], down.y[1], down.y[2]));
}

TEST_CASE("frustum edges in Camera::setOrthogonal's terms")
{
	// OpenXR angles: left negative, right positive, up positive, down negative
	const float fov[4] = {-0.9f, 0.7f, 0.8f, -0.6f};
	float leftClip, rightClip, topClip, bottomClip;
	vr_eye::clipEdges(fov, 2.0f, leftClip, rightClip, topClip, bottomClip);
	// projectZ puts camera x = leftClip at the screen's right edge (x is
	// screen left) and camera y = topClip at the bottom edge (y is screen down)
	CHECK(leftClip == doctest::Approx(-2.0f * std::tan(0.7f)));
	CHECK(rightClip == doctest::Approx(2.0f * std::tan(0.9f)));
	CHECK(topClip == doctest::Approx(2.0f * std::tan(0.6f)));
	CHECK(bottomClip == doctest::Approx(-2.0f * std::tan(0.8f)));

	// a symmetric field of view gives setOrthogonal's own symmetric values
	const float sym[4] = {-0.5f, 0.5f, 0.4f, -0.4f};
	vr_eye::clipEdges(sym, 1.0f, leftClip, rightClip, topClip, bottomClip);
	CHECK(leftClip == doctest::Approx(-rightClip));
	CHECK(topClip == doctest::Approx(-bottomClip));
	CHECK(leftClip < 0.0f);
	CHECK(topClip > 0.0f);
}

}
