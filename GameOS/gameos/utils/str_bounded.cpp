#include "str_bounded.h"

#include <cctype>
#include <cstdio>
#include <cstring>

static bool equalsNoCase(const char* a, const char* b)
{
    for (; *a && *b; ++a, ++b)
        if (std::tolower(static_cast<unsigned char>(*a)) != std::tolower(static_cast<unsigned char>(*b)))
            return false;
    return *a == *b;
}

bool envFlagEnabled(const char* value)
{
    if (!value || !value[0])
        return false;
    static const char* const off[] = { "0", "false", "no", "off" };
    for (const char* o : off)
        if (equalsNoCase(value, o))
            return false;
    return true;
}

bool envFlagOr(const char* value, bool unsetDefault)
{
    if (!value || !value[0])
        return unsetDefault;
    return envFlagEnabled(value);
}

void formatLogLine(char* out, size_t outSize, const char* header, const char* fmt, va_list ap)
{
    if (!out || outSize == 0)
        return;
    out[0] = '\0';
    if (outSize < 2)
        return;
    // Format into outSize - 1 bytes, keeping one byte for the newline.
    std::snprintf(out, outSize - 1, "%s: ", header);
    size_t len = std::strlen(out);
    if (len < outSize - 2)
        std::vsnprintf(out + len, outSize - 1 - len, fmt, ap);
    len = std::strlen(out);
    if (len == 0 || out[len - 1] != '\n') {
        out[len] = '\n';
        out[len + 1] = '\0';
    }
}

const char* logLineMessage(const char* line, const char* header)
{
    size_t n = std::strlen(header);
    if (std::strncmp(line, header, n) != 0 || line[n] != ':')
        return line;
    line += n + 1;
    while (*line == ' ')
        ++line;
    return line;
}

void appendBounded(char* dest, size_t destSize, const char* src)
{
    if (!dest || !src || destSize == 0)
        return;
    const void* end = std::memchr(dest, '\0', destSize);
    if (!end)
        return;
    size_t len = static_cast<size_t>(static_cast<const char*>(end) - dest);
    size_t room = destSize - len - 1;
    size_t n = std::strlen(src);
    if (n > room)
        n = room;
    std::memmove(dest + len, src, n);
    dest[len + n] = '\0';
}
