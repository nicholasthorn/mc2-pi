#include "utils/pe_strings.h"

#include <vector>

namespace {

struct Reader {
    const uint8_t* data;
    size_t size;

    bool has(size_t off, size_t n) const { return off <= size && n <= size - off; }
    uint16_t u16(size_t off) const { return (uint16_t)(data[off] | (data[off + 1] << 8)); }
    uint32_t u32(size_t off) const {
        return (uint32_t)data[off] | ((uint32_t)data[off + 1] << 8) |
               ((uint32_t)data[off + 2] << 16) | ((uint32_t)data[off + 3] << 24);
    }
};

struct Section {
    uint32_t va, vsize, rawPtr, rawSize;
};

// UTF-16 code unit -> Windows-1252 byte
char toCp1252(uint16_t c)
{
    if (c < 0x80 || (c >= 0xa0 && c <= 0xff))
        return (char)c;
    static const uint16_t high[32] = {
        0x20ac, 0, 0x201a, 0x0192, 0x201e, 0x2026, 0x2020, 0x2021,
        0x02c6, 0x2030, 0x0160, 0x2039, 0x0152, 0, 0x017d, 0,
        0, 0x2018, 0x2019, 0x201c, 0x201d, 0x2022, 0x2013, 0x2014,
        0x02dc, 0x2122, 0x0161, 0x203a, 0x0153, 0, 0x017e, 0x0178 };
    for (int i = 0; i < 32; ++i)
        if (high[i] && high[i] == c)
            return (char)(0x80 + i);
    return '?';
}

const uint32_t RT_STRING = 6;
const uint32_t SUBDIR = 0x80000000u;

} // namespace

bool readPeStringTable(const uint8_t* data, size_t size,
                       std::map<uint32_t, std::string>& out)
{
    Reader r = { data, size };
    if (!data || !r.has(0, 0x40) || data[0] != 'M' || data[1] != 'Z')
        return false;
    const size_t pe = r.u32(0x3c);
    if (!r.has(pe, 24) || r.u32(pe) != 0x00004550)   // "PE\0\0"
        return false;
    const uint16_t numSections = r.u16(pe + 6);
    const uint16_t optSize = r.u16(pe + 20);
    const size_t opt = pe + 24;
    if (!r.has(opt, 2))
        return false;
    const uint16_t magic = r.u16(opt);
    size_t dirs;
    if (magic == 0x10b)
        dirs = opt + 96;            // PE32
    else if (magic == 0x20b)
        dirs = opt + 112;           // PE32+
    else
        return false;
    // data directory 2: resources
    if (dirs + 3 * 8 > opt + optSize || !r.has(dirs + 2 * 8, 8))
        return false;
    const uint32_t resRva = r.u32(dirs + 16);
    if (!resRva)
        return false;

    std::vector<Section> sections;
    const size_t secTable = opt + optSize;
    for (uint16_t i = 0; i < numSections; ++i) {
        const size_t s = secTable + (size_t)i * 40;
        if (!r.has(s, 40))
            return false;
        Section sec = { r.u32(s + 12), r.u32(s + 8), r.u32(s + 20), r.u32(s + 16) };
        sections.push_back(sec);
    }
    // RVA -> file offset; (size_t)-1 if unmapped
    auto fileOffset = [&](uint32_t rva) -> size_t {
        for (const Section& s : sections) {
            const uint32_t span = s.vsize > s.rawSize ? s.vsize : s.rawSize;
            if (rva >= s.va && rva - s.va < span && rva - s.va < s.rawSize)
                return (size_t)s.rawPtr + (rva - s.va);
        }
        return (size_t)-1;
    };
    const size_t base = fileOffset(resRva);
    if (base == (size_t)-1 || !r.has(base, 16))
        return false;

    // a resource directory's entries: (id, target), or none if out of range
    auto entries = [&](size_t dir, std::vector<std::pair<uint32_t, uint32_t> >& list) {
        list.clear();
        if (!r.has(dir, 16))
            return;
        const size_t n = (size_t)r.u16(dir + 12) + r.u16(dir + 14);
        for (size_t k = 0; k < n && r.has(dir + 16 + 8 * k, 8); ++k)
            list.push_back(std::make_pair(r.u32(dir + 16 + 8 * k), r.u32(dir + 20 + 8 * k)));
    };

    std::vector<std::pair<uint32_t, uint32_t> > types, blocks, langs;
    entries(base, types);
    for (const auto& type : types) {
        if (type.first != RT_STRING || !(type.second & SUBDIR))
            continue;
        entries(base + (type.second & ~SUBDIR), blocks);
        for (const auto& block : blocks) {
            // block n holds ids (n - 1) * 16 .. (n - 1) * 16 + 15
            if ((block.first & SUBDIR) || block.first == 0 || !(block.second & SUBDIR))
                continue;
            entries(base + (block.second & ~SUBDIR), langs);
            if (langs.empty() || (langs[0].second & SUBDIR))
                continue;
            const size_t entry = base + langs[0].second;
            if (!r.has(entry, 8))
                continue;
            const size_t p0 = fileOffset(r.u32(entry));
            const uint32_t len = r.u32(entry + 4);
            if (p0 == (size_t)-1 || !r.has(p0, len))
                continue;
            size_t p = p0;
            const size_t end = p0 + len;
            for (uint32_t i = 0; i < 16 && p + 2 <= end; ++i) {
                const uint16_t n = r.u16(p);
                p += 2;
                if (n > (end - p) / 2)
                    break;
                if (n) {
                    std::string s(n, '\0');
                    for (uint16_t c = 0; c < n; ++c)
                        s[c] = toCp1252(r.u16(p + 2 * c));
                    out[(block.first - 1) * 16 + i] = s;
                }
                p += 2 * (size_t)n;
            }
        }
    }
    return true;
}
