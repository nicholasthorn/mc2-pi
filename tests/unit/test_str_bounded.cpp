#include "doctest.h"

#include <cstring>
#include <utils/str_bounded.h>

TEST_SUITE("StrBounded") {

TEST_CASE("envFlagEnabled: unset, empty and off-values are off") {
    CHECK_FALSE(envFlagEnabled(nullptr));
    CHECK_FALSE(envFlagEnabled(""));
    CHECK_FALSE(envFlagEnabled("0"));
    CHECK_FALSE(envFlagEnabled("false"));
    CHECK_FALSE(envFlagEnabled("FALSE"));
    CHECK_FALSE(envFlagEnabled("No"));
    CHECK_FALSE(envFlagEnabled("off"));
}

TEST_CASE("envFlagEnabled: anything else is on") {
    CHECK(envFlagEnabled("1"));
    CHECK(envFlagEnabled("yes"));
    CHECK(envFlagEnabled("true"));
    CHECK(envFlagEnabled("offset"));
}

TEST_CASE("appendBounded: appends when it fits") {
    char buf[16] = "abc";
    appendBounded(buf, sizeof(buf), "def");
    CHECK(std::strcmp(buf, "abcdef") == 0);
}

TEST_CASE("appendBounded: truncates to the buffer, like ABL concat into char[40]") {
    // "Distance is: " plus the text of a -5e33 real, as cpl0101a.abl does
    char buf[40] = "Distance is: ";
    const char* huge = "-5239660444437886277848136822030336.00";
    appendBounded(buf, sizeof(buf), huge);
    CHECK(std::strlen(buf) == sizeof(buf) - 1);
    CHECK(std::strncmp(buf, "Distance is: -5239660444", 24) == 0);
}

TEST_CASE("appendBounded: full buffer, zero size and missing terminator change nothing") {
    char full[4] = "abc";
    appendBounded(full, sizeof(full), "x");
    CHECK(std::strcmp(full, "abc") == 0);

    char untouched[4] = "abc";
    appendBounded(untouched, 0, "x");
    CHECK(std::strcmp(untouched, "abc") == 0);

    char unterminated[4] = { 'a', 'b', 'c', 'd' };
    appendBounded(unterminated, sizeof(unterminated), "x");
    CHECK(unterminated[3] == 'd');
}

}
