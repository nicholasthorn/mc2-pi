#include "doctest.h"

#include <string>
#include <vector>

#include "msglines.h"

using msglines::split;

TEST_SUITE("MsgLines") {

TEST_CASE("'*' starts a new line; empty lines are left out") {
    const std::vector<std::string> lines = split("Hold the bridge*Wait for the lance**", 100);
    REQUIRE(lines.size() == 2);
    CHECK(lines[0] == "Hold the bridge");
    CHECK(lines[1] == "Wait for the lance");
}

TEST_CASE("a short message is one line, unchanged") {
    const std::vector<std::string> lines = split("Commander, scouts spotted two companies.", 511);
    REQUIRE(lines.size() == 1);
    CHECK(lines[0] == "Commander, scouts spotted two companies.");
}

TEST_CASE("a long line is cut at its last space within the limit") {
    // limit 10: "aaaa bbbb cccc" -> "aaaa bbbb" (9) then "cccc"
    const std::vector<std::string> lines = split("aaaa bbbb cccc", 10);
    REQUIRE(lines.size() == 2);
    CHECK(lines[0] == "aaaa bbbb");
    CHECK(lines[1] == "cccc");
}

TEST_CASE("a space right at the limit is a cut point, and the spaces there are dropped") {
    const std::vector<std::string> lines = split("aaaa  bbbb", 4);
    REQUIRE(lines.size() == 2);
    CHECK(lines[0] == "aaaa");
    CHECK(lines[1] == "bbbb");
}

TEST_CASE("a word longer than the limit is cut at the limit") {
    const std::vector<std::string> lines = split("abcdefghij kl", 4);
    REQUIRE(lines.size() == 4);
    CHECK(lines[0] == "abcd");
    CHECK(lines[1] == "efgh");
    CHECK(lines[2] == "ij");
    CHECK(lines[3] == "kl");
}

TEST_CASE("no line is longer than the limit, and no text is lost") {
    std::string text;
    for (int i = 0; i < 300; i++)
        text += (i ? " word" : "word") + std::to_string(i);
    const std::vector<std::string> lines = split(text.c_str(), 511);
    CHECK(lines.size() > 1);
    std::string joined;
    for (const std::string& l : lines) {
        CHECK(l.size() <= 511);
        joined += (joined.empty() ? "" : " ") + l;
    }
    CHECK(joined == text);
}

TEST_CASE("null text or a zero limit gives no lines") {
    CHECK(split(nullptr, 10).empty());
    CHECK(split("text", 0).empty());
    CHECK(split("", 10).empty());
}

}
