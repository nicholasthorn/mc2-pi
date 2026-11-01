// csvfield.h: one comma-separated field from a logistics CSV row
// (LogisticsComponent::extractString). Game-free so it can be unit tested.
#ifndef CSVFIELD_H
#define CSVFIELD_H

#include <cstring>

// Copies the field at pLine into pBuffer (cut to bufferLength - 1 characters,
// always terminated) and moves pLine past the ',' or '\n' that ends it. A field
// ended by the string's NUL leaves pLine on the NUL, so further reads of a short
// row return empty fields instead of reading past it. Returns the field's
// length, or 0 with pLine unchanged when no separator is found in 512 bytes.
inline int csvNextField(char*& pLine, char* pBuffer, int bufferLength)
{
	if (bufferLength <= 0)
		return 0;
	*pBuffer = '\0';
	int i = 0;
	for (; i < 512; ++i)
	{
		if ((pLine[i] == '\n') || (pLine[i] == ',') || (pLine[i] == '\0'))
			break;
	}
	if (i == 512)
		return 0;

	const int copy = (i < bufferLength) ? i : (bufferLength - 1);
	memcpy(pBuffer, pLine, copy);
	pBuffer[copy] = '\0';
	pLine += (pLine[i] == '\0') ? i : (i + 1);
	return i;
}

#endif
