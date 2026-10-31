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
size_t LZCompress (MemoryPtr dest,  MemoryPtr src, size_t len);

//---------------------------------------------------------------------------
#endif
