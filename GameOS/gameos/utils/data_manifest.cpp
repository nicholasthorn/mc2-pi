#include "data_manifest.h"

#include <cstring>
#include <set>

namespace data_manifest {

namespace {

const char kHeader[] = "mc2data ";

// the lines of text without their line ends; blank ones dropped
std::vector<std::string> lines(const std::string& text)
{
    std::vector<std::string> out;
    size_t start = 0;
    while(start <= text.size()) {
        size_t end = text.find('\n', start);
        if(end == std::string::npos)
            end = text.size();
        std::string line = text.substr(start, end - start);
        if(!line.empty() && line[line.size() - 1] == '\r')
            line.erase(line.size() - 1);
        if(!line.empty())
            out.push_back(line);
        start = end + 1;
    }
    return out;
}

// digits only, and no more than a uint64_t holds
bool parseSize(const std::string& digits, uint64_t& size)
{
    if(digits.empty())
        return false;
    size = 0;
    for(char c : digits) {
        if(c < '0' || c > '9')
            return false;
        const uint64_t digit = (uint64_t)(c - '0');
        if(size > (UINT64_MAX - digit) / 10)
            return false;
        size = size * 10 + digit;
    }
    return true;
}

} // namespace

bool safePath(const std::string& path)
{
    if(path.empty() || path.find('\\') != std::string::npos)
        return false;
    size_t start = 0;
    while(true) {
        const size_t end = path.find('/', start);
        const std::string part = path.substr(start, end == std::string::npos ? std::string::npos : end - start);
        if(part.empty() || part == "." || part == "..")
            return false;
        if(end == std::string::npos)
            return true;
        start = end + 1;
    }
}

bool playerFile(const std::string& path)
{
    static const char* const kFiles[] = {
        "options.cfg", "orgprefs.cfg", "mc2args.txt", "mc2.log", "mc2.previous.log", "mc2data.installed",
    };
    for(const char* name : kFiles)
        if(path == name)
            return true;
    // HOME is the files folder: the user data directory (settings, saves) is
    // a hidden folder in it; savegame/ is the old place for saves
    return !path.empty() && (path[0] == '.' || path.compare(0, 9, "savegame/") == 0);
}

std::vector<std::string> removed(const Manifest& before, const Manifest& after)
{
    std::set<std::string> kept;
    for(const Entry& e : after.files)
        kept.insert(e.path);
    std::vector<std::string> out;
    for(const Entry& e : before.files)
        if(!kept.count(e.path))
            out.push_back(e.path);
    return out;
}

bool parse(const std::string& text, Manifest& out, std::string& error)
{
    out = Manifest();
    const std::vector<std::string> all = lines(text);
    if(all.empty() || all[0].compare(0, strlen(kHeader), kHeader) != 0 || all[0].size() == strlen(kHeader)) {
        error = "no \"mc2data <stamp>\" first line";
        return false;
    }
    out.stamp = all[0].substr(strlen(kHeader));
    for(size_t i = 1; i < all.size(); ++i) {
        const std::string& line = all[i];
        const size_t space = line.find(' ');
        Entry e;
        if(space == std::string::npos || !parseSize(line.substr(0, space), e.size)) {
            error = "line " + std::to_string(i + 1) + ": not \"<size> <path>\"";
            return false;
        }
        e.path = line.substr(space + 1);
        if(!safePath(e.path) || playerFile(e.path)) {
            error = "line " + std::to_string(i + 1) + ": path not allowed: " + e.path;
            return false;
        }
        out.totalBytes += e.size;
        out.files.push_back(e);
    }
    return true;
}

} // namespace data_manifest
