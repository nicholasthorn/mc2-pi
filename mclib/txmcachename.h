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

// Writes the name into buf (size bytes); returns false if it did not fit.
inline bool txmCacheName(char *buf, size_t size, unsigned typeInfo, unsigned overlayInfo, bool hasColorMap)
{
	const int n = std::snprintf(buf, size, hasColorMap ? "%08u.%08u.txm" : "%08u.%08u.old.txm", typeInfo, overlayInfo);
	return n > 0 && (size_t)n < size;
}

#endif
