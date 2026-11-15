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
// wave_ParseWaveMemorySized
//   Parses a chunk of memory into the header and samples.
//   This is done by looking for the "fmt " and "data"
//   fields in the memory. Nothing past memSize bytes is read:
//   wolfman-MC2X's sound.pak holds a 1-byte sound bite.
//
//////////////////////////////////////////////////////////////////
bool wave_ParseWaveMemorySized(MemoryPtr lpChunkOfMemory, size_t memSize, MC2_WAVEFORMATEX** lplpWaveHeader, MemoryPtr* lplpWaveSamples,DWORD *lpcbWaveSize)
{
	// Set defaults to NULL or zero
	if (*lplpWaveHeader)
		*lplpWaveHeader = NULL;

	if (*lplpWaveSamples)
		*lplpWaveSamples = NULL;

	if (lpcbWaveSize)
		*lpcbWaveSize = 0;

	const BYTE* mem = lpChunkOfMemory;
	if (memSize < 12)
		return false;

	// Get the type and length of the chunk of memory
	DWORD dwRiff   = readDword(mem);
	DWORD dwLength = readDword(mem + 4);
	DWORD dwType   = readDword(mem + 8);

	// Ensure that this is a RIFF WAVE chunk of memory
	if (dwRiff != 0x46464952)
		return false;	// not even RIFF

	if (dwType != 0x45564157)
		return false;	// not a WAV

	// The RIFF length counts from the "WAVE" tag, at offset 8
	size_t end = 8 + (size_t)dwLength;
	if (end > memSize)
		end = memSize;
	size_t pos = 12;

	// Run through the bytes looking for the tags
	while (pos < end && end - pos >= 8)
	{
		dwType   = readDword(mem + pos);
		dwLength = readDword(mem + pos + 4);
		pos += 8;

		switch (dwType)
		{
		// Found the format part
		case 0x20746d66:

			if (*lplpWaveHeader == NULL)
			{
				if (dwLength < sizeof(MC2_WAVEFORMATEX) || memSize - pos < sizeof(MC2_WAVEFORMATEX))
					return false; // something's wrong! Not a WAV

				// Point the header at this part of the memory chunk
				*lplpWaveHeader = (MC2_WAVEFORMATEX*)(lpChunkOfMemory + pos);

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
					*lplpWaveSamples = lpChunkOfMemory + pos;

				// Set the size of the wave, cut to what the buffer holds
				if (lpcbWaveSize)
					*lpcbWaveSize = ((size_t)dwLength > memSize - pos) ? (DWORD)(memSize - pos) : dwLength;

				// Make sure we have our header pointer set up.
				// If we do, we can exit
				if (lplpWaveHeader)
					return true;
			}
			break;

		} // End case

		// Move to the next chunk, skipping the pad byte of an odd-sized one
		const size_t step = ((size_t)dwLength + 1) & ~(size_t)1;
		if (step > end - pos)
			break;
		pos += step;
	}

	// Failed! If we made it here, we did not get all the pieces
	// of the wave
	return false;
}

//////////////////////////////////////////////////////////////////
//
// wave_ParseWaveMemory
//   The same with no known size: bounded only by the RIFF length.
//
//////////////////////////////////////////////////////////////////
bool wave_ParseWaveMemory(MemoryPtr lpChunkOfMemory, MC2_WAVEFORMATEX** lplpWaveHeader, MemoryPtr* lplpWaveSamples,DWORD *lpcbWaveSize)
{
	return wave_ParseWaveMemorySized(lpChunkOfMemory, (size_t)-1, lplpWaveHeader, lplpWaveSamples, lpcbWaveSize);
}
