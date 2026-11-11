#include "doctest.h"

#include <cstdarg>
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

TEST_CASE("envFlagOr: unset or empty gives the default, otherwise the value") {
    CHECK(envFlagOr(nullptr, true));
    CHECK(envFlagOr("", true));
    CHECK_FALSE(envFlagOr(nullptr, false));
    CHECK_FALSE(envFlagOr("0", true));
    CHECK_FALSE(envFlagOr("off", true));
    CHECK(envFlagOr("1", false));
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

TEST_CASE("copyBounded: copies when it fits") {
    char buf[16] = "old text";
    copyBounded(buf, sizeof(buf), "new");
    CHECK(std::strcmp(buf, "new") == 0);
}

TEST_CASE("copyBounded: a text longer than the buffer is cut, like a mod's long name") {
    char buf[8];
    std::memset(buf, 'x', sizeof(buf));
    copyBounded(buf, sizeof(buf), "Atlas AS7-D Royal");
    CHECK(std::strcmp(buf, "Atlas A") == 0);
}

TEST_CASE("copyBounded: exact fit, empty and NULL source") {
    char buf[4];
    copyBounded(buf, sizeof(buf), "abc");
    CHECK(std::strcmp(buf, "abc") == 0);
    copyBounded(buf, sizeof(buf), "abcd");
    CHECK(std::strcmp(buf, "abc") == 0);
    copyBounded(buf, sizeof(buf), "");
    CHECK(buf[0] == '\0');
    std::strcpy(buf, "abc");
    copyBounded(buf, sizeof(buf), nullptr);
    CHECK(buf[0] == '\0');
}

TEST_CASE("copyBounded: zero size writes nothing, and copying a string onto itself keeps it") {
    char untouched[4] = "abc";
    copyBounded(untouched, 0, "x");
    CHECK(std::strcmp(untouched, "abc") == 0);

    char self[8] = "abcdef";
    copyBounded(self, 4, self);
    CHECK(std::strcmp(self, "abc") == 0);
}

TEST_CASE("copyBounded and appendBounded on an array take its size") {
    char buf[6];
    copyBounded(buf, "MCL_MC_");
    CHECK(std::strcmp(buf, "MCL_M") == 0);
    copyBounded(buf, "ab");
    appendBounded(buf, "cdefgh");
    CHECK(std::strcmp(buf, "abcde") == 0);
}

TEST_CASE("splitPathName: the name without directory or extension") {
    char name[32];
    splitPathName("data\\objects\\atlas.csv", name, sizeof(name));
    CHECK(std::strcmp(name, "atlas") == 0);
    splitPathName("data/missions/mc2_01.fit", name, sizeof(name));
    CHECK(std::strcmp(name, "mc2_01") == 0);
    splitPathName("plain", name, sizeof(name));
    CHECK(std::strcmp(name, "plain") == 0);
    splitPathName("dir.v2/noext", name, sizeof(name));
    CHECK(std::strcmp(name, "noext") == 0);
    splitPathName("a/b.c.tga", name, sizeof(name));
    CHECK(std::strcmp(name, "b.c") == 0);
    splitPathName("", name, sizeof(name));
    CHECK(name[0] == '\0');
    splitPathName(nullptr, name, sizeof(name));
    CHECK(name[0] == '\0');
}

TEST_CASE("splitPathName: a long name is cut to the buffer") {
    char name[8];
    splitPathName("data\\objects\\verylongmechname.csv", name, sizeof(name));
    CHECK(std::strcmp(name, "verylon") == 0);
    char untouched[4] = "abc";
    splitPathName("x/y.z", untouched, 0);
    CHECK(std::strcmp(untouched, "abc") == 0);
}

static void formatLine(char* out, size_t size, const char* header, const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    formatLogLine(out, size, header, fmt, ap);
    va_end(ap);
}

// scripts/smoke.sh fails a run on a line starting "STOP: "; these pin that.
TEST_CASE("formatLogLine: header, colon, message and one newline") {
    char buf[64];
    formatLine(buf, sizeof(buf), "STOP", "Unable to open %s", "x.fit");
    CHECK(std::strcmp(buf, "STOP: Unable to open x.fit\n") == 0);

    formatLine(buf, sizeof(buf), "STOP", "already ends\n");
    CHECK(std::strcmp(buf, "STOP: already ends\n") == 0);

    formatLine(buf, sizeof(buf), "STOP", "%s", " [FATAL 0] msg ");
    CHECK(std::strcmp(buf, "STOP:  [FATAL 0] msg \n") == 0);
}

TEST_CASE("formatLogLine: a long message is cut but keeps the prefix and newline") {
    char buf[16];
    formatLine(buf, sizeof(buf), "STOP", "0123456789abcdef");
    CHECK(std::strcmp(buf, "STOP: 01234567\n") == 0);
}

TEST_CASE("formatLogLine: a header too long for the buffer still ends in a newline") {
    char buf[8];
    formatLine(buf, sizeof(buf), "VERYLONGHEADER", "m");
    CHECK(std::strlen(buf) == sizeof(buf) - 1);
    CHECK(buf[sizeof(buf) - 2] == '\n');
}

TEST_CASE("logLineMessage: drops the header and leading spaces") {
    CHECK(std::strcmp(logLineMessage("STOP: Unable to open x\n", "STOP"), "Unable to open x\n") == 0);
    CHECK(std::strcmp(logLineMessage("STOP:  [FATAL 0] m", "STOP"), "[FATAL 0] m") == 0);
    CHECK(std::strcmp(logLineMessage("A fatal error.", "STOP"), "A fatal error.") == 0);
}

}
