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

TEST_CASE("the screen's shape: the display's while the window covers it") {
    int w = 0, h = 0;
    // a Pixel 9 before the status bar has gone: the display, landscape
    screen_fit::screenShape(2188, 1080, 2424, 1080, w, h);
    CHECK(w == 2424);
    CHECK(h == 1080);
    // the display reported portrait: the longer side is the width
    screen_fit::screenShape(2424, 1080, 1080, 2424, w, h);
    CHECK(w == 2424);
    CHECK(h == 1080);
    // no display size: the window
    screen_fit::screenShape(2424, 1080, 0, 0, w, h);
    CHECK(w == 2424);
    CHECK(h == 1080);
}

TEST_CASE("the screen's shape: the window's in split screen") {
    int w = 0, h = 0;
    // a 2560 x 1600 tablet split side by side, and top and bottom
    screen_fit::screenShape(1280, 1600, 2560, 1600, w, h);
    CHECK(w == 1280);
    CHECK(h == 1600);
    screen_fit::screenShape(2560, 800, 2560, 1600, w, h);
    CHECK(w == 2560);
    CHECK(h == 800);
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

// SDL's mouse is in window points, the game's pointer in drawable pixels:
// twice as many on a Retina display (issue #185)
TEST_CASE("window points to drawable pixels and back") {
    // a 1440-point-wide window, 2880 pixels drawn on a Retina Mac
    CHECK(screen_fit::windowToDrawable(720.0f, 1440, 2880) == doctest::Approx(1440.0f));
    CHECK(screen_fit::windowToDrawable(1439.0f, 1440, 2880) == doctest::Approx(2878.0f));
    CHECK(screen_fit::drawableToWindow(1440.0f, 1440, 2880) == doctest::Approx(720.0f));
    // the same size (Linux, Windows, a non-Retina screen): unchanged
    CHECK(screen_fit::windowToDrawable(333.0f, 800, 800) == doctest::Approx(333.0f));
    CHECK(screen_fit::drawableToWindow(333.0f, 800, 800) == doctest::Approx(333.0f));
    // a delta scales the same way
    CHECK(screen_fit::windowToDrawable(-3.0f, 1440, 2880) == doctest::Approx(-6.0f));
}

TEST_CASE("a size not known yet leaves the coordinate as it is") {
    CHECK(screen_fit::windowToDrawable(100.0f, 0, 2880) == doctest::Approx(100.0f));
    CHECK(screen_fit::windowToDrawable(100.0f, 1440, 0) == doctest::Approx(100.0f));
    CHECK(screen_fit::drawableToWindow(100.0f, 1440, 0) == doctest::Approx(100.0f));
    CHECK(screen_fit::drawableToWindow(100.0f, 0, 2880) == doctest::Approx(100.0f));
}

}
