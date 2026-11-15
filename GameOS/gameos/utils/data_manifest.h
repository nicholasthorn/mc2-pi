#ifndef DATA_MANIFEST_H
#define DATA_MANIFEST_H

#include <cstdint>
#include <string>
#include <vector>

// The list of game data files an Android APK carries in its assets
// (mc2data/files.txt, written by scripts/android/build.sh), which the first
// start copies to the app's files folder. GL- and SDL-free for the unit tests.
//
// Format, one entry a line (LF or CRLF; blank lines are skipped):
//   mc2data <stamp>        the first line; the stamp changes with the data
//   <size> <path>          a file: its size in bytes, then its path relative
//                          to the data folder, '/' separated (spaces allowed)
namespace data_manifest {

struct Entry {
    std::string path;
    uint64_t size;
};

struct Manifest {
    std::string stamp;
    std::vector<Entry> files;
    uint64_t totalBytes = 0;
};

// false, with a reason in error, for a malformed manifest or one that names
// an unsafe path or a player's file
bool parse(const std::string& text, Manifest& out, std::string& error);

// a relative path that stays inside the data folder: '/' separated, no empty,
// "." or ".." part, no backslash
bool safePath(const std::string& path);

// the files in before that after does not have: what an update removes
std::vector<std::string> removed(const Manifest& before, const Manifest& after);

// a file the player's runs make or change (settings, saves, logs, the
// command line, the install marker), which data must never overwrite
bool playerFile(const std::string& path);

}

#endif // DATA_MANIFEST_H
