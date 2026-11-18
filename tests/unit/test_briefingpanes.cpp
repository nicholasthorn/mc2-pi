#include "doctest.h"

#include "briefingpanes.h"

using namespace briefingpanes;

TEST_SUITE("BriefingPanes") {

TEST_CASE("buttons 83 to 86 map to the four panes and back")
{
    CHECK(paneForButton(83) == kBriefing);
    CHECK(paneForButton(84) == kTactical);
    CHECK(paneForButton(85) == kAdditional);
    CHECK(paneForButton(86) == kDescription);
    for (int p = 0; p < kPanes; p++)
        CHECK(paneForButton(buttonForPane(p)) == p);
}

TEST_CASE("other buttons are not pane buttons")
{
    CHECK(paneForButton(82) == -1);   // stop video
    CHECK(paneForButton(87) == -1);
    CHECK(paneForButton(50) == -1);   // next
    CHECK(paneForButton(0) == -1);
}

TEST_CASE("text is only text when it has a visible character")
{
    CHECK_FALSE(hasText(nullptr));
    CHECK_FALSE(hasText(""));
    CHECK_FALSE(hasText(" \r\n\t "));
    CHECK(hasText("x"));
    CHECK(hasText("\r\n>Planet: Ambrosia"));
}

TEST_CASE("a click selects a pane with text and ignores one without")
{
    const bool text[kPanes] = {true, false, true, true};
    CHECK(selectPane(text, kBriefing, 85) == kAdditional);
    CHECK(selectPane(text, kAdditional, 86) == kDescription);
    CHECK(selectPane(text, kDescription, 84) == kDescription);   // no tactical text
    CHECK(selectPane(text, kDescription, 83) == kBriefing);
    CHECK(selectPane(text, kAdditional, 50) == kAdditional);    // not a pane button
}

} // TEST_SUITE
