#include "doctest.h"

#include <initializer_list>

#include "forcegroupcycle.h"

using forcegroups::cycle;
using forcegroups::firstFree;
using forcegroups::kGroups;

namespace {

struct Live {
    bool g[kGroups] = {};
    Live(std::initializer_list<int> groups)
    {
        for (int n : groups)
            g[n] = true;
    }
};

} // namespace

TEST_SUITE("ForceGroupCycle") {

TEST_CASE("forward runs in key order, 1 to 9 then 0, and wraps")
{
    const Live live{0, 1, 3, 9};
    CHECK(cycle(live.g, -1, 1) == 1);
    CHECK(cycle(live.g, 1, 1) == 3);
    CHECK(cycle(live.g, 3, 1) == 9);
    CHECK(cycle(live.g, 9, 1) == 0);
    CHECK(cycle(live.g, 0, 1) == 1);
}

TEST_CASE("backward is the reverse, starting from group 0")
{
    const Live live{0, 1, 3, 9};
    CHECK(cycle(live.g, -1, -1) == 0);
    CHECK(cycle(live.g, 0, -1) == 9);
    CHECK(cycle(live.g, 9, -1) == 3);
    CHECK(cycle(live.g, 1, -1) == 0);
}

TEST_CASE("empty groups are skipped, from an empty current group too")
{
    const Live live{2, 5};
    CHECK(cycle(live.g, -1, 1) == 2);
    CHECK(cycle(live.g, 3, 1) == 5);        // current emptied since it was chosen
    CHECK(cycle(live.g, 7, 1) == 2);
    CHECK(cycle(live.g, 3, -1) == 2);
}

TEST_CASE("the only live group is chosen again; none live gives -1")
{
    const Live one{4};
    CHECK(cycle(one.g, 4, 1) == 4);
    CHECK(cycle(one.g, 4, -1) == 4);
    const Live none{};
    CHECK(cycle(none.g, -1, 1) == -1);
    CHECK(cycle(none.g, 2, -1) == -1);
}

TEST_CASE("an out-of-range current counts as none")
{
    const Live live{1, 2};
    CHECK(cycle(live.g, 10, 1) == 1);
    CHECK(cycle(live.g, -5, -1) == 2);
}

TEST_CASE("first free group in key order")
{
    CHECK(firstFree(Live{}.g) == 1);
    CHECK(firstFree(Live{1, 2}.g) == 3);
    CHECK(firstFree(Live{1, 2, 3, 4, 5, 6, 7, 8, 9}.g) == 0);
    CHECK(firstFree(Live{0, 1, 2, 3, 4, 5, 6, 7, 8, 9}.g) == -1);
}

}
