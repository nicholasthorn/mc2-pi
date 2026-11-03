#include "log_file.h"

namespace log_file {

std::string folder(const char* xdgConfigHome, const char* home)
{
    if (xdgConfigHome && xdgConfigHome[0])
        return std::string(xdgConfigHome) + "/.mechcommander2";
    if (home && home[0])
        return std::string(home) + "/.config/.mechcommander2";
    return std::string();
}

static std::string inFolder(const std::string& folder, const char* name)
{
    return folder.empty() ? std::string(name) : folder + "/" + name;
}

std::string logPath(const std::string& folder)
{
    return inFolder(folder, "mc2.log");
}

std::string previousPath(const std::string& folder)
{
    return inFolder(folder, "mc2.previous.log");
}

namespace {

// Appends to a fixed buffer, keeping room for the terminator.
struct Line {
    char* out;
    size_t size;
    size_t len;

    void text(const char* s)
    {
        for (; *s && len + 1 < size; ++s)
            out[len++] = *s;
        out[len] = '\0';
    }

    void number(uint64_t value, unsigned base)
    {
        char digits[24];
        size_t n = sizeof(digits) - 1;
        digits[n] = '\0';
        do {
            digits[--n] = "0123456789abcdef"[value % base];
            value /= base;
        } while (value != 0);
        text(digits + n);
    }
};

const char* fileName(const char* path)
{
    const char* name = path;
    for (const char* p = path; *p; ++p)
        if (*p == '/' || *p == '\\')
            name = p + 1;
    return name;
}

}

size_t formatCrash(char* out, size_t outSize, const char* what, uint32_t code, uint64_t address)
{
    Line line = { out, outSize, 0 };
    line.text("CRASH: ");
    line.text(what);
    line.text(" ");
    line.number(code, 10);
    line.text(" at 0x");
    line.number(address, 16);
    line.text("\n");
    return line.len;
}

size_t formatFrame(char* out, size_t outSize, int index, const char* module, uint64_t offset)
{
    Line line = { out, outSize, 0 };
    line.text("CRASH:   #");
    line.number(static_cast<uint64_t>(index), 10);
    line.text(" ");
    line.text(module ? fileName(module) : "?");
    line.text("+0x");
    line.number(offset, 16);
    line.text("\n");
    return line.len;
}

}
