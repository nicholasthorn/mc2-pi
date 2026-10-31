#include "str_bounded.h"

#include <cctype>
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
