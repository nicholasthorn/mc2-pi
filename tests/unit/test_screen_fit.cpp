#include "doctest.h"

#include "utils/screen_fit.h"

TEST_SUITE("ScreenFit") {

TEST_CASE("a phone runs at its screen's shape, 768 high") {
    int w = 0, h = 0;
    screen_fit::phoneResolution(2424, 1080, w, h);    // Pixel 9, landscape
    CHECK(h == 768);
    CHECK(w == 1724);
    screen_fit::phoneResolution(2560, 1600, w, h);    // a 16:10 tablet
    CHECK(h == 768);
    CHECK(w == 1229);
}

TEST_CASE("never narrower than the wide HUD needs") {
    // the HUD lays out for 1024 x 768 from 1024 wide: a 4:3 or squarer screen
    // still gets 1024 (and is then fitted with bars)
    int w = 0, h = 0;
    screen_fit::phoneResolution(2048, 1536, w, h);
    CHECK(w == 1024);
    CHECK(h == 768);
    screen_fit::phoneResolution(1000, 1000, w, h);
    CHECK(w == 1024);
    screen_fit::phoneResolution(1080, 2424, w, h);    // portrait: as squarer
    CHECK(w == 1024);
}

TEST_CASE("a game the screen's shape fills the screen") {
    int vp[4];
    screen_fit::fitViewport(2424, 1080, 1724, 768, vp);
    CHECK(vp[0] == 0);
    CHECK(vp[1] == 0);
    CHECK(vp[2] == 2424);
    CHECK(vp[3] == 1080);
}

TEST_CASE("a 4:3 game on a wide screen is centred with bars at the sides") {
    int vp[4];
    screen_fit::fitViewport(2424, 1080, 800, 600, vp);
    CHECK(vp[2] == 1440);
    CHECK(vp[3] == 1080);
    CHECK(vp[0] == 492);
    CHECK(vp[1] == 0);
}

TEST_CASE("a wide game on a squarer screen gets bars above and below") {
    int vp[4];
    screen_fit::fitViewport(1600, 1200, 1724, 768, vp);
    CHECK(vp[2] == 1600);
    CHECK(vp[3] == 713);
    CHECK(vp[0] == 0);
    CHECK(vp[1] == 243);
}

TEST_CASE("nothing known yet: the whole drawable") {
    int vp[4];
    screen_fit::fitViewport(2424, 1080, 0, 0, vp);
    CHECK(vp[0] == 0);
    CHECK(vp[2] == 2424);
    CHECK(vp[3] == 1080);
}

TEST_CASE("a drawable pixel to 0..1 across the viewport, clamped") {
    CHECK(screen_fit::acrossViewport(492.0f, 492, 1440) == doctest::Approx(0.0f));
    CHECK(screen_fit::acrossViewport(1212.0f, 492, 1440) == doctest::Approx(0.5f));
    CHECK(screen_fit::acrossViewport(100.0f, 492, 1440) == doctest::Approx(0.0f));    // in a bar
    CHECK(screen_fit::acrossViewport(2400.0f, 492, 1440) == doctest::Approx(1.0f));
    CHECK(screen_fit::acrossViewport(5.0f, 0, 0) == doctest::Approx(0.0f));
}

}
