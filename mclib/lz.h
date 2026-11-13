//---------------------------------------------------------------------------
//
// LZ Compress/Decompress Headers
//
//---------------------------------------------------------------------------//
// Copyright (C) Microsoft Corporation. All rights reserved.                 //
//===========================================================================//

#ifndef LZ_H
#define LZ_H

//---------------------------------------------------------------------------
typedef unsigned char* MemoryPtr;
// destLen bounds the output: if the data would decompress past it, nothing
// beyond destLen is written and 0 is returned (as for corrupt data).
size_t LZDecomp (MemoryPtr dest, MemoryPtr src, size_t srcLen, size_t destLen = (size_t)-1);
// level: zlib's, -1 its default (6). The texture cache passes
// LZ_COMPRESS_FAST: it compresses every texture not already stored
// compressed in an FST (Omnitech's terrain: about 180 MB a mission), which at
// the default level was half of a mission load. Data tools keep the default.
const int LZ_COMPRESS_FAST = 1;
size_t LZCompress (MemoryPtr dest,  MemoryPtr src, size_t len, int level = -1);

//---------------------------------------------------------------------------
#endif
