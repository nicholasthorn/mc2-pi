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

TEST_CASE("the view direction is minus z: the engine camera looks that way")
{
	// a pointer ray taken along z went up when the hand pointed down (Quest,
	// VR plan Phase 3); minus z is where the eye, or a pointer, points
	const vr_eye::Basis eye = vr_eye::eyeBasis(gameCamera(0.6f), kWorldUp, kIdentity, kNoMove, 5.0f, false);
	float direction[3];
	vr_eye::viewDirection(eye, direction);
	CHECK(near3(direction, -eye.z[0], -eye.z[1], -eye.z[2]));
	// level with the ground for a level head (the horizon stays level)
	CHECK(direction[1] == doctest::Approx(0.0f).epsilon(1e-5));
}

}

TEST_CASE("following the head: the game camera looks where the eyes look, from where it was")
{
	// a head turned and tilted: yaw left, then up
	float q[4];
	CHECK(vr_eye::parseHeadPose("70,-25", q));

	const vr_eye::Basis game = gameCamera(0.6f);
	for(int keepPitch = 0; keepPitch < 2; ++keepPitch) {
		const vr_eye::Basis follow = vr_eye::followHead(game, kWorldUp, q, keepPitch != 0);
		float pos[3], quat[4];
		vr_eye::fromXr(kNoMove, q, pos, quat);
		const vr_eye::Basis eye = vr_eye::eyeBasis(game, kWorldUp, quat, pos, 1.0f, keepPitch != 0);
		CHECK(near3(follow.x, eye.x[0], eye.x[1], eye.x[2]));
		CHECK(near3(follow.y, eye.y[0], eye.y[1], eye.y[2]));
		CHECK(near3(follow.z, eye.z[0], eye.z[1], eye.z[2]));
		// the head's own movement stays out of it: the game camera stays put
		CHECK(near3(follow.position, 10.0f, 20.0f, 30.0f));
	}
}

TEST_CASE("following the head: a head at rest with the game's pitch kept is the game camera")
{
	const vr_eye::Basis game = gameCamera(0.6f);
	const vr_eye::Basis follow = vr_eye::followHead(game, kWorldUp, kIdentity, true);
	CHECK(near3(follow.x, game.x[0], game.x[1], game.x[2]));
	CHECK(near3(follow.y, game.y[0], game.y[1], game.y[2]));
	CHECK(near3(follow.z, game.z[0], game.z[1], game.z[2]));
}

TEST_CASE("a camera's axes in the game's world, as Camera::cameraFrame holds them")
{
	// Camera::calculateProjectionConstants at yaw 0 and pitch p (world z up,
	// y ahead): i = (1, 0, 0), j = (0, cos p, -sin p), k = (0, sin p, cos p).
	// The same camera as cameraOrigin's rows, read from a run of the game (the
	// names x screen left, y screen down hold after the engine's -x, z, y):
	const float p = 0.6f, c = std::cos(p), s = std::sin(p);
	vr_eye::Basis camera;
	camera.x[0] = -1.0f; camera.x[1] = 0.0f; camera.x[2] = 0.0f;
	camera.y[0] = 0.0f; camera.y[1] = c; camera.y[2] = s;
	camera.z[0] = 0.0f; camera.z[1] = s; camera.z[2] = -c;
	camera.position[0] = camera.position[1] = camera.position[2] = 0.0f;

	float i[3], j[3], k[3];
	vr_eye::worldFrame(camera, i, j, k);
	CHECK(near3(i, 1.0f, 0.0f, 0.0f));
	CHECK(near3(j, 0.0f, c, -s));
	CHECK(near3(k, 0.0f, s, c));
}

TEST_CASE("where a point on the game's screen meets the ground, for the minimap")
{
	const float deg45 = 0.25f * kPi;
	const float c = std::cos(deg45);
	// looking along world +y, pitched down 45 degrees, 100 above the ground
	const float i[3] = {1.0f, 0.0f, 0.0f}, j[3] = {0.0f, c, -c}, k[3] = {0.0f, c, c};
	const float origin[3] = {5.0f, 7.0f, 100.0f};
	float point[3];

	// the middle of the screen: 100 ahead
	vr_eye::screenToGround(i, j, k, origin, 1.0f, 0.5f, 0.0f, 0.0f, 0.0f, 1000.0f, point);
	CHECK(near3(point, 5.0f, 107.0f, 0.0f));

	// the right edge is to the right, the bottom edge nearer
	vr_eye::screenToGround(i, j, k, origin, 1.0f, 0.5f, 1.0f, 0.0f, 0.0f, 1000.0f, point);
	CHECK(point[0] > 5.0f);
	vr_eye::screenToGround(i, j, k, origin, 1.0f, 0.5f, 0.0f, 1.0f, 0.0f, 1000.0f, point);
	CHECK(point[1] < 107.0f);
	CHECK(point[1] > 7.0f);

	// above the horizon: where it heads, as far as the limit, never behind
	const float ahead[3] = {0.0f, 1.0f, 0.0f}, top[3] = {0.0f, 0.0f, 1.0f};
	vr_eye::screenToGround(i, ahead, top, origin, 1.0f, 0.5f, 0.0f, -1.0f, 0.0f, 300.0f, point);
	CHECK(near3(point, 5.0f, 307.0f, 0.0f));
	// just below the horizon: the same limit, not a point miles away
	vr_eye::screenToGround(i, ahead, top, origin, 1.0f, 0.5f, 0.0f, 0.01f, 0.0f, 300.0f, point);
	CHECK(near3(point, 5.0f, 307.0f, 0.0f));
	// straight up: the point under the camera, not NaN
	const float up[3] = {0.0f, 0.0f, 1.0f}, back[3] = {0.0f, -1.0f, 0.0f};
	vr_eye::screenToGround(i, up, back, origin, 1.0f, 0.5f, 0.0f, 0.0f, 0.0f, 300.0f, point);
	CHECK(near3(point, 5.0f, 7.0f, 0.0f));
}

TEST_CASE("a head pose given as text: yaw left then pitch up, in degrees")
{
	float q[4];
	REQUIRE(vr_eye::parseHeadPose("0,0", q));
	CHECK(q[3] == doctest::Approx(1.0f));

	// yaw 90 looks along the game camera's screen left (see "turning the head left")
	REQUIRE(vr_eye::parseHeadPose("90,0", q));
	const vr_eye::Basis left = eyeFor(gameCamera(0.0f), kNoMove, q, 1.0f);
	CHECK(near3(left.z, 1.0f, 0.0f, 0.0f));

	// pitch is about the turned head's own axis: still level, then up
	REQUIRE(vr_eye::parseHeadPose("90,30", q));
	const float deg30 = 30.0f * kPi / 180.0f;
	const vr_eye::Basis up = eyeFor(gameCamera(0.0f), kNoMove, q, 1.0f);
	CHECK(near3(up.z, std::cos(deg30), std::sin(deg30), 0.0f));

	CHECK_FALSE(vr_eye::parseHeadPose("", q));
	CHECK_FALSE(vr_eye::parseHeadPose("90", q));
	CHECK_FALSE(vr_eye::parseHeadPose("left,up", q));
}

TEST_CASE("the game camera's field of view covers both eyes, at the window's aspect")
{
	const float deg = kPi / 180.0f;
	// a wide window, so the eyes' width decides: the widest half, doubled
	const float left[4] = {-50.0f * deg, 40.0f * deg, 30.0f * deg, -30.0f * deg};
	const float right[4] = {-40.0f * deg, 50.0f * deg, 30.0f * deg, -30.0f * deg};
	CHECK(vr_eye::gameFieldOfView(left, right, 0.75f) == doctest::Approx(100.0f).epsilon(0.001));

	// the camera is symmetric: one eye reaching further out sets both sides
	const float lopsided[4] = {-55.0f * deg, 40.0f * deg, 30.0f * deg, -30.0f * deg};
	CHECK(vr_eye::gameFieldOfView(lopsided, right, 0.75f) == doctest::Approx(110.0f).epsilon(0.001));

	// camera_fov is horizontal: a tall view needs more of it at 4:3, by
	// tan(h / 2) = tan(v / 2) / (height / width)
	const float tall[4] = {-30.0f * deg, 30.0f * deg, 45.0f * deg, -40.0f * deg};
	const float expected = 2.0f * std::atan(1.0f / 0.75f) / deg;
	CHECK(vr_eye::gameFieldOfView(tall, tall, 0.75f) == doctest::Approx(expected).epsilon(0.001));
}

TEST_CASE("the game camera's field of view stays in the range the view cone can use")
{
	const float deg = kPi / 180.0f;
	const float huge[4] = {-85.0f * deg, 85.0f * deg, 85.0f * deg, -85.0f * deg};
	CHECK(vr_eye::gameFieldOfView(huge, huge, 0.75f) == doctest::Approx(120.0f));

	// nothing usable: the game's own minimum, not 0 or NaN
	const float none[4] = {0.0f, 0.0f, 0.0f, 0.0f};
	CHECK(vr_eye::gameFieldOfView(none, none, 0.75f) == doctest::Approx(20.0f));
	const float notNumber = std::nanf("");
	const float bad[4] = {notNumber, notNumber, notNumber, notNumber};
	CHECK(vr_eye::gameFieldOfView(bad, bad, 0.75f) == doctest::Approx(20.0f));

	// no window yet: the eyes' width alone
	const float wide[4] = {-50.0f * deg, 50.0f * deg, 45.0f * deg, -45.0f * deg};
	CHECK(vr_eye::gameFieldOfView(wide, wide, 0.0f) == doctest::Approx(100.0f).epsilon(0.001));
}

TEST_CASE("overlay text in an eye: the same size whatever the game's resolution")
{
	float x = 0.0f, y = 0.0f;
	// 800 x 600 over a Quest 2 eye: a window unit is 1.8 by 2.64 eye pixels
	vr_eye::overlayTextScale(800, 600, 1440, 1584, x, y);
	CHECK(x * 1440.0f / 800.0f == doctest::Approx(vr_eye::kOverlayTextEyePixels));
	CHECK(y * 1584.0f / 600.0f == doctest::Approx(vr_eye::kOverlayTextEyePixels));
	// 3664 x 1920: 0.39 by 0.825, the size the text was judged right at
	vr_eye::overlayTextScale(3664, 1920, 1440, 1584, x, y);
	CHECK(x == doctest::Approx(2.099f).epsilon(0.001));
	CHECK(y == doctest::Approx(1.0f));
	// a size not known yet
	vr_eye::overlayTextScale(0, 1920, 1440, 1584, x, y);
	CHECK(x == 1.0f);
	CHECK(y == 1.0f);
	vr_eye::overlayTextScale(3664, 1920, 1440, 0, x, y);
	CHECK(x == 1.0f);
	CHECK(y == 1.0f);
}

TEST_CASE("overlay markers in an eye: narrower than text by a fixed share")
{
	float x = 0.0f, y = 0.0f;
	vr_eye::overlayTextScale(800, 600, 1440, 1584, x, y);
	// a game pixel of a bar across a Quest 2 eye at 800 x 600
	CHECK(x * vr_eye::kOverlayMarkerShareX * 1440.0f / 800.0f == doctest::Approx(0.393f));
}

TEST_CASE("a field of view given as text: in the range, or 0 to turn it off")
{
	CHECK(vr_eye::parseGameFov("20") == doctest::Approx(20.0f));
	CHECK(vr_eye::parseGameFov("90.5") == doctest::Approx(90.5f));
	CHECK(vr_eye::parseGameFov("120") == doctest::Approx(120.0f));
	CHECK(vr_eye::parseGameFov("0") == 0.0f);

	// a typo must not read as 0, which silently keeps the game's 40 degrees
	CHECK(vr_eye::parseGameFov("wide") == -1.0f);
	CHECK(vr_eye::parseGameFov("") == -1.0f);
	CHECK(vr_eye::parseGameFov("90deg") == -1.0f);
	CHECK(vr_eye::parseGameFov("19.9") == -1.0f);
	CHECK(vr_eye::parseGameFov("500") == -1.0f);
	CHECK(vr_eye::parseGameFov("-60") == -1.0f);
}

TEST_CASE("the interface's size in a headset: 1280x960 unless another layout was chosen")
{
	int w = 800, h = 600;
	CHECK(vr_eye::fitInterfaceResolution(w, h));	// also a file with no size
	CHECK(w == 1280);
	CHECK(h == 960);

	w = 3664, h = 1920;	// the headset's window, as the options screen saves it
	CHECK(vr_eye::fitInterfaceResolution(w, h));
	CHECK(w == 1280);
	CHECK(h == 960);

	// the same shape as 800x600, so the game camera stays as it was judged
	CHECK(vr_eye::kInterfaceWidth * 600 == vr_eye::kInterfaceHeight * 800);

	const int kept[][2] = {{1024, 768}, {1280, 960}, {1280, 1024}, {1600, 1200}, {1920, 1080}, {800, 500}};
	for(const auto& size : kept) {
		w = size[0], h = size[1];
		CHECK_FALSE(vr_eye::fitInterfaceResolution(w, h));
		CHECK(w == size[0]);
		CHECK(h == size[1]);
	}
}
