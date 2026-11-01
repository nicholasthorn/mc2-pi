//---------------------------------------------------------------------------
// waveparse.cpp - RIFF WAVE chunk walk for in-memory sound bites.
//
// RIFF pads each chunk to an even size, not to a multiple of four, so a chunk
// header can start on any even address (an 18-byte "fmt " is enough, and
// MechCommander Omnitech's sounds have such headers). Headers are therefore
// read with memcpy, never through a DWORD pointer.
//---------------------------------------------------------------------------

#include"waveparse.h"

#include<string.h>

static DWORD readDword(const BYTE* p)
{
	DWORD v;
	memcpy(&v, p, sizeof(v));
	return v;
}

//////////////////////////////////////////////////////////////////
//
// wave_ParseWaveMemory
//   Parses a chunk of memory into the header and samples.
//   This is done by looking for the "fmt " and "data"
//   fields in the memory.
//
//////////////////////////////////////////////////////////////////
bool wave_ParseWaveMemory(MemoryPtr lpChunkOfMemory, MC2_WAVEFORMATEX** lplpWaveHeader, MemoryPtr* lplpWaveSamples,DWORD *lpcbWaveSize)
{
	BYTE	*p;
	BYTE	*pEnd;
	DWORD	dwRiff;
	DWORD	dwType;
	DWORD	dwLength;

	// Set defaults to NULL or zero
	if (*lplpWaveHeader)
		*lplpWaveHeader = NULL;

	if (*lplpWaveSamples)
		*lplpWaveSamples = NULL;

	if (lpcbWaveSize)
		*lpcbWaveSize = 0;

	p = lpChunkOfMemory;

	// Get the type and length of the chunk of memory
	dwRiff   = readDword(p);
	dwLength = readDword(p + 4);
	dwType   = readDword(p + 8);
	p += 12;

	// Ensure that this is a RIFF WAVE chunk of memory
	if (dwRiff != 0x46464952)
		return false;	// not even RIFF

	if (dwType != 0x45564157)
		return false;	// not a WAV

	// The RIFF length counts from the "WAVE" tag, which is already behind us
	pEnd = p + dwLength - 4;

	// Run through the bytes looking for the tags
	while (p < pEnd)
	{
		dwType   = readDword(p);
		dwLength = readDword(p + 4);
		p += 8;

		switch (dwType)
		{
		// Found the format part
		case 0x20746d66:

			if (*lplpWaveHeader == NULL)
			{
				if (dwLength < sizeof(MC2_WAVEFORMATEX))
					return false; // something's wrong! Not a WAV

				// Point the header at this part of the memory chunk
				*lplpWaveHeader = (MC2_WAVEFORMATEX*)p;

				// If the samples and their size are already filled out,
				// this chunk of memory has been parsed and we can exit
				if ((!lplpWaveSamples || *lplpWaveSamples) &&
					(!lpcbWaveSize || *lpcbWaveSize))
				{
					return true;
				}
			}
			break;

		// Found the samples
		case 0x61746164:

			if ((lplpWaveSamples && !*lplpWaveSamples) ||
				(lpcbWaveSize && !*lpcbWaveSize))
			{
				// Point the samples pointer to this part of the
				// chunk of memory.
				if (*lplpWaveSamples == NULL)
					*lplpWaveSamples = p;

				// Set the size of the wave
				if (lpcbWaveSize)
					*lpcbWaveSize = dwLength;

				// Make sure we have our header pointer set up.
				// If we do, we can exit
				if (lplpWaveHeader)
					return true;
			}
			break;

		} // End case

		// Move to the next chunk, skipping the pad byte of an odd-sized one
		p += (dwLength + 1) & ~1;
	}

	// Failed! If we made it here, we did not get all the pieces
	// of the wave
	return false;
}
