#include "msglines.h"

#include <cstring>

namespace msglines {

std::vector<std::string> split(const char* text, size_t maxLen)
{
    std::vector<std::string> lines;
    if (!text || maxLen == 0)
        return lines;
    const char* p = text;
    while (*p) {
        const char* star = strchr(p, '*');
        const size_t len = star ? (size_t)(star - p) : strlen(p);
        const std::string piece(p, len);
        size_t start = 0;
        while (start < piece.size()) {
            size_t n = piece.size() - start;
            if (n > maxLen) {
                const size_t cut = piece.rfind(' ', start + maxLen);
                n = (cut == std::string::npos || cut <= start) ? maxLen : cut - start;
            }
            lines.push_back(piece.substr(start, n));
            start += n;
            while (start < piece.size() && piece[start] == ' ')
                start++;
        }
        p += len;
        if (*p == '*')
            p++;
    }
    return lines;
}

} // namespace msglines
