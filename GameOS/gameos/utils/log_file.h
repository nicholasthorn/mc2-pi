#ifndef LOG_FILE_H
#define LOG_FILE_H

#include <cstddef>
#include <cstdint>
#include <string>

// The player's log (gos_logfile.cpp): where it goes, and the crash lines. GL-
// and SDL-free for the unit tests.
namespace log_file {

// The game's config folder, as gos_LoadDataFromRegistry builds it, or "" for
// the current folder (the game's) when neither variable is set or non-empty.
std::string folder(const char* xdgConfigHome, const char* home);

// In a folder from folder().
std::string logPath(const std::string& folder);
std::string previousPath(const std::string& folder);

// "CRASH: <what> <code> at 0x<address>\n" and "CRASH:   #<index> <module's
// file name>+0x<offset>\n", cut to fit out and always terminated; they return
// the length. Safe in a signal handler: no printf, no allocation.
size_t formatCrash(char* out, size_t outSize, const char* what, uint32_t code, uint64_t address);
size_t formatFrame(char* out, size_t outSize, int index, const char* module, uint64_t offset);

}

#endif // LOG_FILE_H
