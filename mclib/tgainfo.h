//---------------------------------------------------------------------------
// 
// MechCommander 2 -- Copyright (c) 1998 FASA Interactive
//
// TGA file Specifics
//
//---------------------------------------------------------------------------//
// Copyright (C) Microsoft Corporation. All rights reserved.                 //
//===========================================================================//

#ifndef TGAINFO_H
#define TGAINFO_H

#ifndef DSTD_H
#include"dstd.h"
#endif

#ifndef FILE_H
#include"file.h"
#endif

//---------------------------------------------------------------------------
enum TGAImageType
{
	NO_IMAGE = 0,
	UNC_PAL,
	UNC_TRUE,
	UNC_GRAY,
	RLE_PAL = 9,
	RLE_TRUE,
	RLE_GRAY
};

//---------------------------------------------------------------------------
#pragma pack(1)

struct TGAFileHeader
{
	byte image_id_len;
	byte color_map;
	byte image_type;

	short cm_first_entry;
	short cm_length;
	byte cm_entry_size;

	short x_origin;
	short y_origin;
	short width;
	short height;
	byte pixel_depth;
	byte image_descriptor;
};

#pragma pack()

// true when fileSize bytes hold the header and, past its id and colour map,
// width x height pixels of depth bits (a header can claim anything)
bool tgaPixelsFit (const void *fileImage, long fileSize, long width, long height, long depth);
// the same for the size and depth the header itself claims
bool tgaHeaderFits (const void *fileImage, long fileSize);

// false when the data ends early or does not fit the header's size: dest holds
// the header's width x height x 4 bytes, source sourceSize bytes
bool tgaDecomp (MemoryPtr dest, MemoryPtr source, long sourceSize, TGAFileHeader *tga_header);

void loadTGATexture (FilePtr tgaFile, MemoryPtr ourRAM, long width, long height);
void loadTGAMask (FilePtr tgaFile, MemoryPtr ourRAM, long width, long height);

void flipTopToBottom (MemoryPtr buffer, BYTE depth, long width, long height);
//---------------------------------------------------------------------------
#endif
