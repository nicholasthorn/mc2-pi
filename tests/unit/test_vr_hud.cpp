// tests/unit/test_vr_hud.cpp
//
// vr_hud: when the VR interface keeps showing its last image. It redraws the
// frame after a game step, not with it, so that the step has the frame's time.

#include "doctest.h"

#include "utils/vr_hud.h"

namespace {

vr_hud::Frame steady()
{
    vr_hud::Frame f;
    f.missionAskedLastFrame = true;
    f.eyesInUse = true;
    f.everyFrame = false;
    f.hasImage = true;
    f.changed = false;
    f.gameStepped = false;
    f.framesSinceDrawn = 0;
    return f;
}

}  // namespace

TEST_SUITE("VrHud") {

TEST_CASE("rests while nothing has changed") {
    CHECK(vr_hud::rests(steady()));
}

TEST_CASE("draws the frame after a game step, not with it") {
    vr_hud::Frame f = steady();
    f.changed = true;
    f.gameStepped = true;
    CHECK(vr_hud::rests(f));

    f.gameStepped = false;
    f.framesSinceDrawn = 1;
    CHECK_FALSE(vr_hud::rests(f));
}

TEST_CASE("a run of game steps cannot keep it from drawing") {
    vr_hud::Frame f = steady();
    f.changed = true;
    f.gameStepped = true;
    f.framesSinceDrawn = 1;
    CHECK(vr_hud::rests(f));

    f.framesSinceDrawn = 2;
    CHECK_FALSE(vr_hud::rests(f));
}

TEST_CASE("never rests without something to keep showing") {
    vr_hud::Frame f = steady();
    f.hasImage = false;
    CHECK_FALSE(vr_hud::rests(f));
}

TEST_CASE("only a mission's interface in the eyes' view rests") {
    vr_hud::Frame f = steady();
    f.missionAskedLastFrame = false;
    CHECK_FALSE(vr_hud::rests(f));

    f = steady();
    f.eyesInUse = false;
    CHECK_FALSE(vr_hud::rests(f));
}

TEST_CASE("MC2_XR_HUD_EVERY_FRAME turns it off") {
    vr_hud::Frame f = steady();
    f.everyFrame = true;
    CHECK_FALSE(vr_hud::rests(f));
}

}
