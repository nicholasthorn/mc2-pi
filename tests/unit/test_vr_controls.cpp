// tests/unit/test_vr_controls.cpp
//
// vr_controls: the Quest controllers of the VR plan (Phase 3). A pointing
// ray from a controller becomes the mouse where it meets the interface quad,
// the quad can be placed in front of the head, and the sticks become the
// camera keys. All in OpenXR's axes (x right, y up, z back). A sign error
// here puts the cursor on the wrong side of the panel, or scrolls the map
// the wrong way, so the directions are pinned.

#include "doctest.h"

#include "utils/vr_controls.h"

#include <cmath>

namespace {

const float kPi = 3.14159265f;

// a rotation of 'angle' radians about OpenXR axis 0 (x), 1 (y) or 2 (z)
void axisQuat(int axis, float angle, float q[4])
{
	q[0] = q[1] = q[2] = 0.0f;
	q[axis] = std::sin(angle * 0.5f);
	q[3] = std::cos(angle * 0.5f);
}

// q = a then b (b applied after a, both in the parent frame): b * a
void compose(const float b[4], const float a[4], float q[4])
{
	q[0] = b[3] * a[0] + b[0] * a[3] + b[1] * a[2] - b[2] * a[1];
	q[1] = b[3] * a[1] - b[0] * a[2] + b[1] * a[3] + b[2] * a[0];
	q[2] = b[3] * a[2] + b[0] * a[1] - b[1] * a[0] + b[2] * a[3];
	q[3] = b[3] * a[3] - b[0] * a[0] - b[1] * a[1] - b[2] * a[2];
}

vr_controls::Pose pose(float x, float y, float z, const float q[4])
{
	vr_controls::Pose p;
	p.position[0] = x; p.position[1] = y; p.position[2] = z;
	for(int i = 0; i < 4; ++i)
		p.orientation[i] = q[i];
	return p;
}

const float kIdentity[4] = {0.0f, 0.0f, 0.0f, 1.0f};
const float kOrigin[3] = {0.0f, 0.0f, 0.0f};

}

TEST_SUITE("VRControls") {

TEST_CASE("a ray straight ahead hits the middle of a quad ahead") {
	// quad 1.5 m ahead, facing back at the viewer (its +z towards them)
	const vr_controls::Pose quad = pose(0.0f, 0.0f, -1.5f, kIdentity);
	const vr_controls::Pose ray = pose(0.0f, 0.0f, 0.0f, kIdentity);
	float u = -1.0f, v = -1.0f;
	REQUIRE(vr_controls::rayHitsQuad(ray, quad, 2.0f, 1.5f, u, v));
	CHECK(u == doctest::Approx(0.5f));
	CHECK(v == doctest::Approx(0.5f));
}

TEST_CASE("pointing right moves u right, pointing up moves v up (v counts from the top)") {
	const vr_controls::Pose quad = pose(0.0f, 0.0f, -1.5f, kIdentity);
	float q[4];
	// turning about +y is to the left, so -0.3 rad is to the right
	axisQuat(1, -0.3f, q);
	float u = 0.0f, v = 0.0f;
	REQUIRE(vr_controls::rayHitsQuad(pose(0.0f, 0.0f, 0.0f, q), quad, 2.0f, 1.5f, u, v));
	CHECK(u == doctest::Approx(0.5f + 1.5f * std::tan(0.3f) / 2.0f));
	CHECK(v == doctest::Approx(0.5f));

	// turning about +x lifts -z upwards
	axisQuat(0, 0.2f, q);
	REQUIRE(vr_controls::rayHitsQuad(pose(0.0f, 0.0f, 0.0f, q), quad, 2.0f, 1.5f, u, v));
	CHECK(u == doctest::Approx(0.5f));
	CHECK(v == doctest::Approx(0.5f - 1.5f * std::tan(0.2f) / 1.5f));
}

TEST_CASE("the ray's position counts, not only its direction") {
	const vr_controls::Pose quad = pose(0.0f, 0.0f, -1.5f, kIdentity);
	float u = 0.0f, v = 0.0f;
	// a hand 0.5 m to the left and 0.25 m down, pointing straight ahead
	REQUIRE(vr_controls::rayHitsQuad(pose(-0.5f, -0.25f, 0.0f, kIdentity), quad, 2.0f, 1.5f, u, v));
	CHECK(u == doctest::Approx(0.25f));
	CHECK(v == doctest::Approx(0.5f + 0.25f / 1.5f));
}

TEST_CASE("missing the quad, pointing away and hitting it from behind are misses") {
	const vr_controls::Pose quad = pose(0.0f, 0.0f, -1.5f, kIdentity);
	float u = 0.0f, v = 0.0f, q[4];
	// past the right edge
	axisQuat(1, -1.0f, q);
	CHECK_FALSE(vr_controls::rayHitsQuad(pose(0.0f, 0.0f, 0.0f, q), quad, 2.0f, 1.5f, u, v));
	// pointing backwards
	axisQuat(1, kPi, q);
	CHECK_FALSE(vr_controls::rayHitsQuad(pose(0.0f, 0.0f, 0.0f, q), quad, 2.0f, 1.5f, u, v));
	// standing behind the quad, pointing the same way
	CHECK_FALSE(vr_controls::rayHitsQuad(pose(0.0f, 0.0f, -3.0f, kIdentity), quad, 2.0f, 1.5f, u, v));
}

TEST_CASE("the hit distance runs along the ray to the quad") {
	const vr_controls::Pose quad = pose(0.0f, 0.0f, -1.5f, kIdentity);
	float u = 0.0f, v = 0.0f, distance = 0.0f, q[4];
	REQUIRE(vr_controls::rayHitsQuad(pose(0.0f, 0.0f, 0.0f, kIdentity), quad, 2.0f, 1.5f, u, v, &distance));
	CHECK(distance == doctest::Approx(1.5f));
	axisQuat(1, -0.3f, q);
	REQUIRE(vr_controls::rayHitsQuad(pose(0.0f, 0.0f, 0.0f, q), quad, 2.0f, 1.5f, u, v, &distance));
	CHECK(distance == doctest::Approx(1.5f / std::cos(0.3f)));

	// and that far along the ray is on the quad, where u says
	float p[3];
	vr_controls::rayPoint(pose(0.0f, 0.0f, 0.0f, q), distance, p);
	CHECK(p[2] == doctest::Approx(-1.5f));
	CHECK(p[0] == doctest::Approx((u - 0.5f) * 2.0f));
}

TEST_CASE("quadPoint is where a ray hitting u, v meets the quad, on a turned HUD too") {
	const float origin[3] = {0.1f, 1.5f, 0.2f};
	const vr_controls::Pose hud = vr_controls::hudPose(origin, 0.6f, 1.5f, 0.52f);
	float yq[4], pq[4], q[4];
	axisQuat(1, 0.7f, yq);
	axisQuat(0, -0.45f, pq);
	compose(yq, pq, q);
	const vr_controls::Pose ray = pose(origin[0], origin[1], origin[2], q);
	float u = 0.0f, v = 0.0f, distance = 0.0f;
	REQUIRE(vr_controls::rayHitsQuad(ray, hud, 2.0f, 1.5f, u, v, &distance));
	float onQuad[3], onRay[3];
	vr_controls::quadPoint(hud, 2.0f, 1.5f, u, v, onQuad);
	vr_controls::rayPoint(ray, distance, onRay);
	for(int i = 0; i < 3; ++i)
		CHECK(onQuad[i] == doctest::Approx(onRay[i]));
}

namespace {

// the quad's axis 0 (x), 1 (y) or 2 (z) in the parent frame
void quadAxis(const vr_controls::Pose& p, int axis, float out[3])
{
	const float* q = p.orientation;
	// columns of the rotation matrix
	const float m[3][3] = {
		{1 - 2 * (q[1] * q[1] + q[2] * q[2]), 2 * (q[0] * q[1] + q[2] * q[3]), 2 * (q[0] * q[2] - q[1] * q[3])},
		{2 * (q[0] * q[1] - q[2] * q[3]), 1 - 2 * (q[0] * q[0] + q[2] * q[2]), 2 * (q[1] * q[2] + q[0] * q[3])},
		{2 * (q[0] * q[2] + q[1] * q[3]), 2 * (q[1] * q[2] - q[0] * q[3]), 1 - 2 * (q[0] * q[0] + q[1] * q[1])},
	};
	for(int i = 0; i < 3; ++i)
		out[i] = m[axis][i];
}

}

TEST_CASE("the beam runs from the hand to its end and faces the eye") {
	const float from[3] = {0.2f, -0.3f, -0.2f};
	const float to[3] = {0.2f, -0.3f, -2.2f};
	const float eye[3] = {0.0f, 0.0f, 0.0f};
	float length = 0.0f;
	const vr_controls::Pose beam = vr_controls::beamPose(from, to, eye, length);
	CHECK(length == doctest::Approx(2.0f));
	CHECK(beam.position[0] == doctest::Approx(0.2f));
	CHECK(beam.position[1] == doctest::Approx(-0.3f));
	CHECK(beam.position[2] == doctest::Approx(-1.2f));
	float y[3], z[3];
	quadAxis(beam, 1, y);
	quadAxis(beam, 2, z);
	// its length (the quad's height) along the beam
	CHECK(std::fabs(y[2]) == doctest::Approx(1.0f));
	// its face towards the eye, square to the beam: the eye is up and left
	// of the beam, so the face points that way
	CHECK(z[2] == doctest::Approx(0.0f).epsilon(1e-5));
	CHECK(z[0] == doctest::Approx(-0.2f / std::sqrt(0.13f)));
	CHECK(z[1] == doctest::Approx(0.3f / std::sqrt(0.13f)));
}

TEST_CASE("the beam has a pose even when the eye is on its line") {
	const float from[3] = {0.0f, 0.0f, -0.5f};
	const float to[3] = {0.0f, 0.0f, -2.0f};
	const float eye[3] = {0.0f, 0.0f, 0.0f};
	float length = 0.0f;
	const vr_controls::Pose beam = vr_controls::beamPose(from, to, eye, length);
	const float* q = beam.orientation;
	CHECK(q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3] == doctest::Approx(1.0f));
	CHECK(length == doctest::Approx(1.5f));
}

TEST_CASE("the HUD pose at yaw 0 is the fixed one gos_xr used before") {
	const float down = 0.52f, distance = 1.5f;
	const vr_controls::Pose hud = vr_controls::hudPose(kOrigin, 0.0f, distance, down);
	CHECK(hud.position[0] == doctest::Approx(0.0f));
	CHECK(hud.position[1] == doctest::Approx(-distance * std::sin(down)));
	CHECK(hud.position[2] == doctest::Approx(-distance * std::cos(down)));
	CHECK(hud.orientation[0] == doctest::Approx(-std::sin(0.5f * down)));
	CHECK(hud.orientation[1] == doctest::Approx(0.0f));
	CHECK(hud.orientation[2] == doctest::Approx(0.0f));
	CHECK(hud.orientation[3] == doctest::Approx(std::cos(0.5f * down)));
}

TEST_CASE("a ray along the HUD's direction hits its middle, whatever the yaw") {
	const float origin[3] = {0.3f, 1.6f, -0.2f};
	const float yaws[] = {0.0f, 0.8f, -2.0f, 3.0f};
	for(float yaw : yaws) {
		const vr_controls::Pose hud = vr_controls::hudPose(origin, yaw, 1.5f, 0.52f);
		float yq[4], pq[4], q[4];
		axisQuat(1, yaw, yq);
		axisQuat(0, -0.52f, pq);
		compose(yq, pq, q);
		float u = 0.0f, v = 0.0f;
		REQUIRE(vr_controls::rayHitsQuad(pose(origin[0], origin[1], origin[2], q), hud, 2.0f, 1.5f, u, v));
		CHECK(u == doctest::Approx(0.5f));
		CHECK(v == doctest::Approx(0.5f));
	}
}

TEST_CASE("turning left (positive yaw) puts the HUD to the left") {
	const vr_controls::Pose hud = vr_controls::hudPose(kOrigin, 0.5f * kPi, 1.5f, 0.0f);
	CHECK(hud.position[0] == doctest::Approx(-1.5f));
	CHECK(hud.position[2] == doctest::Approx(0.0f).epsilon(1e-5));
}

TEST_CASE("yawOf reads the heading and ignores looking up or down and tilting") {
	float yq[4], pq[4], rq[4], q[4], q2[4];
	axisQuat(1, 0.7f, yq);
	CHECK(vr_controls::yawOf(yq) == doctest::Approx(0.7f));
	axisQuat(0, -0.6f, pq);
	compose(yq, pq, q);
	CHECK(vr_controls::yawOf(q) == doctest::Approx(0.7f));
	axisQuat(2, 0.3f, rq);
	compose(q, rq, q2);
	CHECK(vr_controls::yawOf(q2) == doctest::Approx(0.7f).epsilon(1e-3));
	axisQuat(1, -2.5f, yq);
	CHECK(vr_controls::yawOf(yq) == doctest::Approx(-2.5f));
}

TEST_CASE("click hold: the pointer stays put while the trigger is pulled") {
	vr_controls::ClickHold hold;
	float u = 0.40f, v = 0.50f;
	// trigger at rest: the pointer follows the hand
	vr_controls::holdForClick(hold, 0.0f, u, v);
	CHECK(u == doctest::Approx(0.40f));
	// the pull starts: held from here
	u = 0.41f;
	vr_controls::holdForClick(hold, 0.3f, u, v);
	CHECK(u == doctest::Approx(0.41f));
	// the hand wobbles as the trigger goes down: still held
	u = 0.425f; v = 0.515f;
	vr_controls::holdForClick(hold, 1.0f, u, v);
	CHECK(u == doctest::Approx(0.41f));
	CHECK(v == doctest::Approx(0.50f));
	// let go: follows again
	u = 0.43f;
	vr_controls::holdForClick(hold, 0.05f, u, v);
	CHECK(u == doctest::Approx(0.43f));
}

TEST_CASE("click hold: moving well away is a drag, and then the pointer follows") {
	vr_controls::ClickHold hold;
	float u = 0.40f, v = 0.50f;
	vr_controls::holdForClick(hold, 1.0f, u, v);
	u = 0.50f;
	vr_controls::holdForClick(hold, 1.0f, u, v);
	CHECK(u == doctest::Approx(0.50f));
	// coming back near the start does not hold it again until let go
	u = 0.41f;
	vr_controls::holdForClick(hold, 1.0f, u, v);
	CHECK(u == doctest::Approx(0.41f));
	vr_controls::holdForClick(hold, 0.0f, u, v);
	u = 0.45f;
	vr_controls::holdForClick(hold, 0.5f, u, v);
	u = 0.46f;
	vr_controls::holdForClick(hold, 1.0f, u, v);
	CHECK(u == doctest::Approx(0.45f));
}

TEST_CASE("sticks: small deflections do nothing, full ones give their direction") {
	const float centre[2] = {0.0f, 0.0f};
	const float nudge[2] = {0.2f, -0.2f};
	CHECK(vr_controls::stickActions(nudge, nudge, 0) == 0u);

	const float up[2] = {0.0f, 1.0f};
	const float right[2] = {1.0f, 0.0f};
	const float downLeft[2] = {-0.8f, -0.8f};
	CHECK(vr_controls::stickActions(up, centre, 0) == vr_controls::PAN_UP);
	CHECK(vr_controls::stickActions(right, centre, 0) == vr_controls::PAN_RIGHT);
	CHECK(vr_controls::stickActions(downLeft, centre, 0) == (vr_controls::PAN_DOWN | vr_controls::PAN_LEFT));
	// right stick: x turns, up zooms in
	CHECK(vr_controls::stickActions(centre, right, 0) == vr_controls::ROTATE_RIGHT);
	const float left[2] = {-1.0f, 0.0f};
	CHECK(vr_controls::stickActions(centre, left, 0) == vr_controls::ROTATE_LEFT);
	CHECK(vr_controls::stickActions(centre, up, 0) == vr_controls::ZOOM_IN);
	const float down[2] = {0.0f, -1.0f};
	CHECK(vr_controls::stickActions(centre, down, 0) == vr_controls::ZOOM_OUT);
}

TEST_CASE("sticks: hysteresis keeps a direction on until the stick is mostly back") {
	const float centre[2] = {0.0f, 0.0f};
	const float half[2] = {0.4f, 0.0f};
	// 0.4 does not start a direction
	CHECK(vr_controls::stickActions(half, centre, 0) == 0u);
	// but keeps one going
	CHECK(vr_controls::stickActions(half, centre, vr_controls::PAN_RIGHT) == vr_controls::PAN_RIGHT);
	const float low[2] = {0.2f, 0.0f};
	CHECK(vr_controls::stickActions(low, centre, vr_controls::PAN_RIGHT) == 0u);
}

TEST_CASE("sticks: turning wins over panning and zoom (turning holds shift, which changes those keys)") {
	const float up[2] = {0.0f, 1.0f};
	const float right[2] = {1.0f, 0.0f};
	CHECK(vr_controls::stickActions(up, right, 0) == vr_controls::ROTATE_RIGHT);
	const float upRight[2] = {0.8f, 0.8f};
	CHECK(vr_controls::stickActions(up, upRight, 0) == vr_controls::ROTATE_RIGHT);
}

}
