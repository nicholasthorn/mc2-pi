#include "android_data.h"

#ifdef __ANDROID__

#include <SDL2/SDL.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <unistd.h>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "utils/data_manifest.h"

namespace {

// in the APK's assets; SDL opens a relative path there when the app's
// internal storage folder does not have it
const char kAssets[] = "mc2data/";
const char kManifest[] = "mc2data/files.txt";
// in the files folder: the stamp of the data installed there
const char kMarker[] = "mc2data.installed";

void fail(const std::string& why)
{
    const std::string msg = "Could not install the game data: " + why;
    printf("[DATA] FATAL: %s\n", msg.c_str());
    fflush(stdout);
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "MechCommander 2", msg.c_str(), NULL);
}

bool readFile(const char* path, std::string& text)
{
    FILE* f = fopen(path, "rb");
    if(!f)
        return false;
    char buf[4096];
    size_t n;
    text.clear();
    while((n = fread(buf, 1, sizeof(buf), f)) > 0)
        text.append(buf, n);
    fclose(f);
    return true;
}

bool fileExists(const char* path)
{
    struct stat st;
    return stat(path, &st) == 0;
}

// the folders above a file
bool makeParents(const std::string& path)
{
    for(size_t slash = path.find('/'); slash != std::string::npos; slash = path.find('/', slash + 1)) {
        const std::string dir = path.substr(0, slash);
        if(mkdir(dir.c_str(), 0755) != 0 && errno != EEXIST)
            return false;
    }
    return true;
}

// written beside the file and renamed over it at the end, so a copy cut off
// half way (the app closed meanwhile) never leaves a short file in its place
bool copyAsset(const data_manifest::Entry& e, std::vector<char>& buf, uint64_t& done, std::string& error)
{
    const std::string asset = std::string(kAssets) + e.path;
    SDL_RWops* in = SDL_RWFromFile(asset.c_str(), "rb");
    if(!in) {
        error = asset + ": " + SDL_GetError();
        return false;
    }
    const std::string part = e.path + ".part";
    FILE* out = makeParents(e.path) ? fopen(part.c_str(), "wb") : NULL;
    if(!out) {
        error = e.path + ": " + strerror(errno);
        SDL_RWclose(in);
        return false;
    }
    uint64_t copied = 0;
    bool ok = true;
    size_t n;
    while((n = SDL_RWread(in, buf.data(), 1, buf.size())) > 0) {
        if(fwrite(buf.data(), 1, n, out) != n) {
            error = e.path + ": " + strerror(errno);
            ok = false;
            break;
        }
        copied += n;
        done += n;
    }
    SDL_RWclose(in);
    if(fclose(out) != 0 && ok) {
        error = e.path + ": " + strerror(errno);
        ok = false;
    }
    if(ok && copied != e.size) {
        error = e.path + ": " + std::to_string(copied) + " bytes, the APK lists " + std::to_string(e.size);
        ok = false;
    }
    if(ok && rename(part.c_str(), e.path.c_str()) != 0) {
        error = e.path + ": " + strerror(errno);
        ok = false;
    }
    if(!ok)
        remove(part.c_str());
    return ok;
}

} // namespace

namespace android_data {

bool install()
{
    size_t len = 0;
    char* text = (char*)SDL_LoadFile(kManifest, &len);
    if(!text) {
        printf("[DATA] no game data in the APK\n");
        return true;
    }
    data_manifest::Manifest manifest;
    std::string error;
    const bool parsed = data_manifest::parse(std::string(text, len), manifest, error);
    SDL_free(text);
    if(!parsed) {
        fail(std::string(kManifest) + ": " + error);
        return false;
    }
    std::string installed;
    if(readFile(kMarker, installed) && installed == manifest.stamp) {
        printf("[DATA] game data %s already installed\n", manifest.stamp.c_str());
        return true;
    }

    const uint64_t mb = 1024 * 1024;
    struct statvfs fs;
    if(statvfs(".", &fs) == 0) {
        const uint64_t avail = (uint64_t)fs.f_bavail * fs.f_frsize;
        if(avail < manifest.totalBytes + 64 * mb) {
            fail("it needs " + std::to_string(manifest.totalBytes / mb + 64) + " MB free, and the device has "
                    + std::to_string(avail / mb) + " MB");
            return false;
        }
    }
    // a changed stamp means new data: the old marker goes first, so a copy
    // cut off half way is redone on the next start
    remove(kMarker);
    printf("[DATA] installing game data %s: %zu files, %llu MB\n", manifest.stamp.c_str(), manifest.files.size(),
            (unsigned long long)(manifest.totalBytes / mb));
    fflush(stdout);

    // no window yet (the game reads its settings before it opens one): a
    // toast every few seconds says how far it has got
    SDL_AndroidShowToast("Installing the game data, the first time only", 1, -1, 0, 0);
    const Uint32 start = SDL_GetTicks();
    Uint32 lastToast = start;
    std::vector<char> buf(1024 * 1024);
    uint64_t done = 0;
    for(const data_manifest::Entry& e : manifest.files) {
        if(!copyAsset(e, buf, done, error)) {
            fail(error);
            return false;
        }
        if(SDL_GetTicks() - lastToast > 4000) {
            const int percent = manifest.totalBytes ? (int)(done * 100 / manifest.totalBytes) : 100;
            const std::string msg = "Installing the game data: " + std::to_string(percent) + "%";
            SDL_AndroidShowToast(msg.c_str(), 0, -1, 0, 0);
            lastToast = SDL_GetTicks();
        }
    }

    FILE* marker = fopen(kMarker, "wb");
    if(!marker || fwrite(manifest.stamp.data(), 1, manifest.stamp.size(), marker) != manifest.stamp.size()) {
        if(marker)
            fclose(marker);
        fail(std::string(kMarker) + ": " + strerror(errno));
        return false;
    }
    fclose(marker);
    printf("[DATA] game data installed in %.1f s\n", (double)(SDL_GetTicks() - start) * 0.001);
    return true;
}

void firstStartSettings()
{
    if(fileExists("options.cfg") || !fileExists("minprefs.cfg"))
        return;
    std::string prefs;
    if(!readFile("minprefs.cfg", prefs))
        return;
    const char* const names[] = { "options.cfg", "orgprefs.cfg" };
    for(const char* name : names) {
        if(fileExists(name))
            continue;
        FILE* f = fopen(name, "wb");
        if(!f)
            continue;
        fwrite(prefs.data(), 1, prefs.size(), f);
        fclose(f);
    }
    printf("[DATA] first start: options.cfg from minprefs.cfg\n");
}

} // namespace android_data

#else

namespace android_data {
bool install() { return true; }
void firstStartSettings() {}
}

#endif
