//---------------------------------------------------------------------------
// waveparse.h - RIFF WAVE header and chunk walk for in-memory sound bites.
//
// Split out of soundsys.cpp so the parse can be unit-tested without the game.
//---------------------------------------------------------------------------

#ifndef WAVEPARSE_H
#define WAVEPARSE_H

#include"platform_windows.h"	// WORD

#ifndef DSTD_H
#include"dstd.h"
#endif

#pragma pack(1)
typedef struct {
	WORD  wFormatTag;
	WORD  nChannels;
	DWORD nSamplesPerSec;
	DWORD nAvgBytesPerSec;
	WORD  nBlockAlign;
} MC2_WAVEFORMATEX;
#pragma pack()

// Finds the "fmt " and "data" chunks of a RIFF WAVE image in memory.
// *lplpWaveHeader and *lplpWaveSamples point into lpChunkOfMemory (no copy);
// the header is packed, so it may sit at any address.
bool wave_ParseWaveMemory(MemoryPtr lpChunkOfMemory, MC2_WAVEFORMATEX** lplpWaveHeader, MemoryPtr* lplpWaveSamples, DWORD *lpcbWaveSize);

#endif
