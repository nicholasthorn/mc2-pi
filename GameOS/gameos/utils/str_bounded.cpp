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

void copyBounded(char* dest, size_t destSize, const char* src)
{
    if (!dest || destSize == 0)
        return;
    size_t n = 0;
    if (src)
    {
        // strnlen is not in C++17's <cstring> everywhere; memchr is.
        const void* end = std::memchr(src, '\0', destSize);
        n = end ? static_cast<size_t>(static_cast<const char*>(end) - src) : destSize - 1;
        std::memmove(dest, src, n);
    }
    dest[n] = '\0';
}

uint32_t wholeNumberLow32(const char* text)
{
    if (!text)
        return 0;
    // Unsigned arithmetic wraps mod 2^64, a multiple of 2^32, so the low 32
    // bits come out right however many digits there are.
    uint64_t value = 0;
    const char* hex = std::strstr(text, "0x");
    if (hex)
    {
        for (const char* p = hex + 2; std::isxdigit(static_cast<unsigned char>(*p)); ++p)
        {
            const int c = std::toupper(static_cast<unsigned char>(*p));
            value = value * 16 + static_cast<uint64_t>(c <= '9' ? c - '0' : c - 'A' + 10);
        }
        return static_cast<uint32_t>(value);
    }
    const char* p = text;
    while (std::isspace(static_cast<unsigned char>(*p)))
        ++p;
    bool negative = false;
    if (*p == '+' || *p == '-')
    {
        negative = (*p == '-');
        ++p;
    }
    for (; *p >= '0' && *p <= '9'; ++p)
        value = value * 10 + static_cast<uint64_t>(*p - '0');
    if (negative)
        value = 0 - value;
    return static_cast<uint32_t>(value);
}

void splitPathName(const char* path, char* name, size_t nameSize)
{
    if (!name || nameSize == 0)
        return;
    if (!path)
        path = "";
    const char* start = path;
    for (const char* p = path; *p; ++p)
    {
        if (*p == '/' || *p == '\\')
            start = p + 1;
    }
    const char* dot = std::strrchr(start, '.');
    size_t n = dot ? static_cast<size_t>(dot - start) : std::strlen(start);
    if (n > nameSize - 1)
        n = nameSize - 1;
    std::memmove(name, start, n);
    name[n] = '\0';
}
