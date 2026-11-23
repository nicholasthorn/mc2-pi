// tests/unit/test_ar_view.cpp
//
// ar_view: a phone's view through ARCore turned into what the game draws an
// eye with (AR-TABLETOP.md, phones). ARCore gives a GL projection matrix for
// the screen; the game wants the four angles of the eye's view, as OpenXR
// gives them (left, right, up, down; left and down negative). A tap on the
// screen becomes a ray from the camera through that pixel, the same kind of
// ray the Quest's controllers give. A wrong sign here mirrors the battle or
// sends every order to the other side of the table.

#include "doctest.h"

#include "utils/ar_view.h"

#include <cmath>

namespace {

// a GL projection (column-major) for a frustum at the near plane: left,
// right, bottom, top, as glFrustum takes them
void frustum(float l, float r, float b, float t, float n, float f, float m[16])
{
	for(int i = 0; i < 16; ++i)
		m[i] = 0.0f;
	m[0] = 2.0f * n / (r - l);
	m[5] = 2.0f * n / (t - b);
	m[8] = (r + l) / (r - l);
	m[9] = (t + b) / (t - b);
	m[10] = -(f + n) / (f - n);
	m[11] = -1.0f;
	m[14] = -2.0f * f * n / (f - n);
}

// a quaternion's rotation of v
void rotate(const float q[4], const float v[3], float out[3])
{
	const float t[3] = {2.0f * (q[1] * v[2] - q[2] * v[1]),
	                    2.0f * (q[2] * v[0] - q[0] * v[2]),
	                    2.0f * (q[0] * v[1] - q[1] * v[0])};
	out[0] = v[0] + q[3] * t[0] + (q[1] * t[2] - q[2] * t[1]);
	out[1] = v[1] + q[3] * t[1] + (q[2] * t[0] - q[0] * t[2]);
	out[2] = v[2] + q[3] * t[2] + (q[0] * t[1] - q[1] * t[0]);
}

const float kForward[3] = {0.0f, 0.0f, -1.0f};

}

TEST_SUITE("ArView") {

TEST_CASE("the eye's angles from ARCore's projection, off centre too")
{
	// a phone's camera view is often off centre: tangents 0.6 left, 0.4
	// right, 0.35 up and 0.3 down (a near plane of 0.1)
	float m[16];
	frustum(-0.06f, 0.04f, -0.03f, 0.035f, 0.1f, 100.0f, m);
	float fov[4];
	ar_view::fovFromProjection(m, fov);
	CHECK(fov[0] == doctest::Approx(std::atan(-0.6f)).epsilon(1e-4));
	CHECK(fov[1] == doctest::Approx(std::atan(0.4f)).epsilon(1e-4));
	CHECK(fov[2] == doctest::Approx(std::atan(0.35f)).epsilon(1e-4));
	CHECK(fov[3] == doctest::Approx(std::atan(-0.3f)).epsilon(1e-4));
}

TEST_CASE("a tap at the screen's centre points where the camera looks")
{
	const float fov[4] = {std::atan(-0.5f), std::atan(0.5f), std::atan(0.3f), std::atan(-0.3f)};
	// the camera turned 30 degrees left and looking 40 degrees down
	const float yaw = 0.5236f, pitch = -0.6981f;
	const float qy[4] = {0.0f, std::sin(yaw * 0.5f), 0.0f, std::cos(yaw * 0.5f)};
	const float qx[4] = {std::sin(pitch * 0.5f), 0.0f, 0.0f, std::cos(pitch * 0.5f)};
	// yaw then pitch: q = qy * qx
	const float camera[4] = {
		qy[3] * qx[0] + qy[0] * qx[3] + qy[1] * qx[2] - qy[2] * qx[1],
		qy[3] * qx[1] - qy[0] * qx[2] + qy[1] * qx[3] + qy[2] * qx[0],
		qy[3] * qx[2] + qy[0] * qx[1] - qy[1] * qx[0] + qy[2] * qx[3],
		qy[3] * qx[3] - qy[0] * qx[0] - qy[1] * qx[1] - qy[2] * qx[2]};
	float ray[4];
	ar_view::rayThroughPixel(camera, fov, 0.5f, 0.5f, ray);
	float looks[3], points[3];
	rotate(camera, kForward, looks);
	rotate(ray, kForward, points);
	for(int i = 0; i < 3; ++i)
		CHECK(points[i] == doctest::Approx(looks[i]).epsilon(1e-4));
}

TEST_CASE("a tap at a corner points along that corner of the view")
{
	// the camera unturned: the ray's direction is the corner's tangents
	const float fov[4] = {std::atan(-0.6f), std::atan(0.4f), std::atan(0.35f), std::atan(-0.3f)};
	const float camera[4] = {0.0f, 0.0f, 0.0f, 1.0f};
	float ray[4], d[3];

	// top left of the screen (0, 0): left and up
	ar_view::rayThroughPixel(camera, fov, 0.0f, 0.0f, ray);
	rotate(ray, kForward, d);
	CHECK(d[0] / -d[2] == doctest::Approx(-0.6f).epsilon(1e-4));
	CHECK(d[1] / -d[2] == doctest::Approx(0.35f).epsilon(1e-4));

	// bottom right (1, 1): right and down
	ar_view::rayThroughPixel(camera, fov, 1.0f, 1.0f, ray);
	rotate(ray, kForward, d);
	CHECK(d[0] / -d[2] == doctest::Approx(0.4f).epsilon(1e-4));
	CHECK(d[1] / -d[2] == doctest::Approx(-0.3f).epsilon(1e-4));

	// a unit quaternion
	CHECK(ray[0] * ray[0] + ray[1] * ray[1] + ray[2] * ray[2] + ray[3] * ray[3] == doctest::Approx(1.0f));
}

}
