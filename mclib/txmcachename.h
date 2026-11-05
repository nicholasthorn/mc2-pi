// mclib/txmcachename.h
//
// File name of a cached terrain transition (data/textures). The name holds the
// types and the overlay only and is shared by every mission, but a mission with
// a colour map saves a different image under it than one without (a blank alpha
// mask with only cement and overlays drawn in, against the old opaque composite
// of the type textures). So the two never share a name: the colour map
// universe, which is every stock mission, keeps the plain name its shipped
// caches have, and the old composite gets ".old".

#ifndef TXMCACHENAME_H
#define TXMCACHENAME_H

#include <cstddef>
#include <cstdio>
#include <cstring>

// Writes the name into buf (size bytes); returns false if it did not fit.
// usePlain: the plain name (colour map caches); false: the ".old" name.
inline bool txmCacheName(char *buf, size_t size, unsigned typeInfo, unsigned overlayInfo, bool usePlain)
{
	const int n = std::snprintf(buf, size, usePlain ? "%08u.%08u.txm" : "%08u.%08u.old.txm", typeInfo, overlayInfo);
	return n > 0 && (size_t)n < size;
}

// True if a name from a mission's .lst is an ".old" cache.
inline bool txmCacheNameIsOld(const char *name)
{
	static const char suffix[] = ".old.txm";
	const size_t len = std::strlen(name);
	const size_t sl = sizeof(suffix) - 1;
	return len >= sl && std::strcmp(name + len - sl, suffix) == 0;
}

// Which name a quick load reads. A mission without a colour map reads ".old"
// caches, except when its .lst names plain ones: those were saved before the
// split (by a mod, or by an earlier build) and hold the composite for it, while
// a plain file the .lst does not name could be a colour map mask.
inline bool txmCacheUsesPlainName(bool hasColorMap, bool listHasPlainNames)
{
	return hasColorMap || listHasPlainNames;
}

#endif
