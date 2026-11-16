#include "doctest.h"

#include "utils/data_manifest.h"

TEST_SUITE("DataManifest") {

TEST_CASE("a manifest lists the stamp, the files and their total size") {
    data_manifest::Manifest m;
    std::string error;
    REQUIRE(data_manifest::parse("mc2data 1a2b3c\n120 data/missions/mc2_01.fit\n7 options file.txt\n", m, error));
    CHECK(m.stamp == "1a2b3c");
    REQUIRE(m.files.size() == 2);
    CHECK(m.files[0].path == "data/missions/mc2_01.fit");
    CHECK(m.files[0].size == 120);
    CHECK(m.files[1].path == "options file.txt");    // the rest of the line, spaces too
    CHECK(m.files[1].size == 7);
    CHECK(m.totalBytes == 127);
}

TEST_CASE("CRLF line ends, blank lines and a missing last line end are fine") {
    data_manifest::Manifest m;
    std::string error;
    REQUIRE(data_manifest::parse("mc2data s\r\n\r\n5 a.fst\r\n0 b.txt", m, error));
    REQUIRE(m.files.size() == 2);
    CHECK(m.files[0].path == "a.fst");
    CHECK(m.files[1].path == "b.txt");
    CHECK(m.files[1].size == 0);
}

TEST_CASE("sizes beyond 4 GB are read whole") {
    data_manifest::Manifest m;
    std::string error;
    REQUIRE(data_manifest::parse("mc2data s\n5000000000 big.fst\n", m, error));
    CHECK(m.files[0].size == 5000000000ull);
}

TEST_CASE("a manifest without its header line is refused") {
    data_manifest::Manifest m;
    std::string error;
    CHECK_FALSE(data_manifest::parse("", m, error));
    CHECK_FALSE(error.empty());
    CHECK_FALSE(data_manifest::parse("12 a.fst\n", m, error));
    CHECK_FALSE(data_manifest::parse("mc2data\n12 a.fst\n", m, error));    // no stamp
}

TEST_CASE("a file line needs a size and a path") {
    data_manifest::Manifest m;
    std::string error;
    CHECK_FALSE(data_manifest::parse("mc2data s\nabc a.fst\n", m, error));
    CHECK_FALSE(data_manifest::parse("mc2data s\n12\n", m, error));
    CHECK_FALSE(data_manifest::parse("mc2data s\n12 \n", m, error));
    CHECK_FALSE(data_manifest::parse("mc2data s\n-12 a.fst\n", m, error));
    CHECK_FALSE(data_manifest::parse("mc2data s\n99999999999999999999999 a.fst\n", m, error));
}

TEST_CASE("paths that would leave the data folder are refused") {
    CHECK(data_manifest::safePath("data/a.fst"));
    CHECK(data_manifest::safePath("a..b.txt"));
    CHECK_FALSE(data_manifest::safePath(""));
    CHECK_FALSE(data_manifest::safePath("/etc/passwd"));
    CHECK_FALSE(data_manifest::safePath("../a.fst"));
    CHECK_FALSE(data_manifest::safePath("data/../../a.fst"));
    CHECK_FALSE(data_manifest::safePath("data/./a.fst"));
    CHECK_FALSE(data_manifest::safePath("data//a.fst"));
    CHECK_FALSE(data_manifest::safePath("data/"));
    CHECK_FALSE(data_manifest::safePath("data\\a.fst"));

    data_manifest::Manifest m;
    std::string error;
    CHECK_FALSE(data_manifest::parse("mc2data s\n3 ../a.fst\n", m, error));
}

TEST_CASE("the files the player owns are never in a manifest") {
    // options.cfg is made from minprefs.cfg on the first start and then holds
    // the player's settings; a data update must not put it back
    data_manifest::Manifest m;
    std::string error;
    CHECK_FALSE(data_manifest::parse("mc2data s\n3 options.cfg\n", m, error));
    CHECK_FALSE(data_manifest::parse("mc2data s\n3 savegame/a.sav\n", m, error));
    CHECK_FALSE(data_manifest::parse("mc2data s\n3 .config/mc2/savegame/a.ims\n", m, error));
    CHECK(data_manifest::playerFile("orgprefs.cfg"));
    CHECK(data_manifest::playerFile(".config/mc2/options.cfg"));    // HOME is the files folder
    CHECK(data_manifest::playerFile("mc2data.installed"));
    CHECK(data_manifest::playerFile("mc2args.txt"));
    CHECK(data_manifest::playerFile("mc2.log"));
    CHECK_FALSE(data_manifest::playerFile("minprefs.cfg"));
    CHECK_FALSE(data_manifest::playerFile("data/savegame.txt"));
}

TEST_CASE("an update lists the files the new data no longer has") {
    data_manifest::Manifest before, after;
    std::string error;
    REQUIRE(data_manifest::parse("mc2data a\n1 art.fst\n2 data/old.fit\n3 data/kept.fit\n", before, error));
    REQUIRE(data_manifest::parse("mc2data b\n1 art.fst\n3 data/kept.fit\n4 data/new.fit\n", after, error));
    const std::vector<std::string> gone = data_manifest::removed(before, after);
    REQUIRE(gone.size() == 1);
    CHECK(gone[0] == "data/old.fit");
    CHECK(data_manifest::removed(after, after).empty());
}

}
