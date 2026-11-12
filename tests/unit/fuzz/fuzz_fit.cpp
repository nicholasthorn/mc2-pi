// tests/unit/fuzz/fuzz_fit.cpp: FitIniFile over any bytes.
//
// Opens the input as a FIT file in memory (as ObjectTypeManager hands one
// over), then seeks every block it found and reads the keys the corpus and
// dictionary use with every kind of reader, into buffers both too small and
// roomy, as the game's callers do.

#include "fuzz_targets.h"
#include "fuzz_support.h"

#include "inifile.h"

#include <cstring>
#include <vector>

namespace
{

// Gives the harness the block list FitIniFile built while opening.
class FuzzFit : public FitIniFile
{
public:
	unsigned long numBlocks() const { return fileBlocks ? totalBlocks : 0; }
	const char* blockName(unsigned long i) const { return fileBlocks[i].blockId; }
};

const char* const kKeys[] = {
	"Name", "FileName", "Value", "Values", "NumValues", "Pos", "Ids", "Flag", "Type", "S",
};

void readKey(FitIniFile& fit, const char* key)
{
	char tight[8];
	char roomy[256];
	fit.readIdString(key, tight, sizeof(tight));
	fit.readIdString(key, roomy, sizeof(roomy));
	fit.getIdStringLength(key);

	long l = 0;
	int i = 0;
	DWORD u = 0;
	short s = 0;
	unsigned short us = 0;
	char c = 0;
	unsigned char uc = 0;
	float f = 0.0f;
	double d = 0.0;
	bool b = false;
	fit.readIdLong(key, l);
	fit.readIdInt(key, i);
	fit.readIdULong(key, u);
	fit.readIdShort(key, s);
	fit.readIdUShort(key, us);
	fit.readIdChar(key, c);
	fit.readIdUChar(key, uc);
	fit.readIdFloat(key, f);
	fit.readIdDouble(key, d);
	fit.readIdBoolean(key, b);

	// Arrays: the count the file claims, capped at what the buffer holds,
	// and once more with a buffer of exactly that count.
	const unsigned long kMax = 16;
	float fa[kMax];
	long la[kMax];
	int ia[kMax];
	unsigned char ca[kMax];
	unsigned long n = fit.getIdFloatArrayElements(key);
	fit.readIdFloatArray(key, fa, n < kMax ? n : kMax);
	n = fit.getIdLongArrayElements(key);
	fit.readIdLongArray(key, la, n < kMax ? n : kMax);
	fit.readIdIntArray(key, ia, (unsigned int)(n < kMax ? n : kMax));
	n = fit.getIdUCharArrayElements(key);
	fit.readIdUCharArray(key, ca, n < kMax ? n : kMax);
	if (n > 0 && n <= 4096)
	{
		std::vector<float> exact(n);
		fit.readIdFloatArray(key, exact.data(), n);
	}
}

} // namespace

int fuzzFitFile(const uint8_t* data, size_t size)
{
	if (size == 0 || size > (1 << 20))
		return 0;
	// exactly size bytes and no terminator, so a read past the end is seen
	std::vector<char> image(data, data + size);

	fuzzGuarded([&] {
		File mem;
		if (mem.open(image.data(), (int)image.size()) != NO_ERR)
			return;
		FuzzFit fit;
		if (fit.open(&mem, (unsigned long)image.size()) == NO_ERR)
		{
			fuzzNoteLoaded();
			for (unsigned long b = 0; b < fit.numBlocks(); b++)
			{
				char id[64];
				strncpy(id, fit.blockName(b), sizeof(id) - 1);
				id[sizeof(id) - 1] = '\0';
				if (fit.seekBlock(id) != NO_ERR)
					continue;
				for (const char* key : kKeys)
					readKey(fit, key);
			}
			fit.close();
		}
		mem.close();
	});
	return 0;
}
