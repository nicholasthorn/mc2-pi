//--------------------------------------------------------------------------
// LZ Decompress Routine
//
//---------------------------------------------------------------------------//
// Copyright (C) Microsoft Corporation. All rights reserved.                 //
//===========================================================================//

//---------------------------------------------------------------------------
// Static Globals

#define HASH_CLEAR		256               //clear hash table command code
#define HASH_EOF        257               //End Of Data command code
#define HASH_FREE       258               //First Hash Table Chain Offset Value

#define BASE_BITS       9
#define MAX_BIT_INDEX   (1 << BASE_BITS)
#define NO_RAM_FOR_LZ_DECOMP	0xCBCB0002

#ifndef NULL
#define NULL			0
#endif

#include <zlib.h>

typedef unsigned char* MemoryPtr;

#ifndef LINUX_BUILD

struct HashStruct
{
	unsigned short Chain;
	unsigned char Suffix;
};

typedef HashStruct *HashStructPtr;
	
HashStructPtr	LZOldChain = NULL;			//Old Chain Value Found
HashStructPtr	LZChain = NULL;				//Current Chain Value Found 
unsigned long	LZMaxIndex = 0;				//Max index value in Hash Table
unsigned long	LZCodeMask = 0;
unsigned long	LZFreeIndex = 0;			//Current Free index into Hash Table
MemoryPtr		LZSrcBufEnd = NULL;			//ptr to 3rd from last byte in src buffer
MemoryPtr		LZOrigDOSBuf = NULL;		//original offset to start of src buffer
char			LZHashBuffer[16384];
char			LZOldSuffix = 0;			//Current Suffix Value found


//-----------------------------

//-------------------------------------------------------------------------------
// LZ DeCompress Routine
// Takes a pointer to dest buffer, a pointer to source buffer and len of source.
// returns length of decompressed image.
long LZDecomp (MemoryPtr dest, MemoryPtr src, unsigned long srcLen)
{
	long result = 0;
	
	__asm
	{
		mov		esi,src
		mov		ebx,srcLen

		mov		edi,dest
		xor		eax,eax

		xor		ecx,ecx							// CH and CL used
		lea		ebx,[ebx+esi-3]

		mov		LZOldChain,eax
		mov		LZSrcBufEnd,ebx

		mov		LZChain,eax
		mov		LZMaxIndex,eax

		mov		LZCodeMask,eax
		mov		LZFreeIndex,eax
	
		mov		LZCodeMask,MAX_BIT_INDEX-1
		mov		LZMaxIndex,MAX_BIT_INDEX		//max index for 9 bits == 512

		mov		LZFreeIndex,HASH_FREE			//set index to 258
		mov		LZOldSuffix,al

		mov		ch,BASE_BITS
		jmp		GetCode

//--------------------------------------------------------------------------
//                                                                          
// ClearHash restarts decompression assuming that it is starting from the   
//           beginning                                                      
//                                                                          
//--------------------------------------------------------------------------

ClearHash:
		mov		ch,BASE_BITS		        	//set up for nine bit codes
		mov		LZCodeMask,MAX_BIT_INDEX-1

		mov		LZMaxIndex,MAX_BIT_INDEX    	//max index for 9 bits == 512
		mov		LZFreeIndex,HASH_FREE       	//set index to 258
		
		cmp		esi,LZSrcBufEnd
	   	ja		error

		mov		eax,[esi+0]
		xor		ebx,ebx

		shr		eax,cl

		add		cl,ch
		mov		edx,LZCodeMask

		mov		bl,cl
		and   	cl,07h

		shr   	ebx,3
		and		eax,edx

		add		esi,ebx
		nop

		mov		LZOldChain,eax      	//previous Chain Offset.
		mov		LZOldSuffix,al

		mov		[edi],al
		inc		edi
//-------------------------------------------------------------------------
// ReadCode gets the next hash code (9 BITS) from LZDOSBuff
//         this WILL ReadFile more data if the buffer is empty
//-------------------------------------------------------------------------
GetCode:
		cmp		esi,LZSrcBufEnd
    	ja		error						// Read Passed End?

		mov		eax,[esi+0]
		xor		ebx,ebx

		shr		eax,cl

		add		cl,ch
		mov		edx,LZCodeMask

		mov		bl,cl
		and   	cl,07h

		shr   	ebx,3
		and		eax,edx

		add		esi,ebx
		nop

		cmp		eax,HASH_EOF
		je		eof

		cmp		eax,HASH_CLEAR         		//are we to clear out hash table?
		je		ClearHash
//---------------------------------------------------------------------------
//                                                                           
// Handle Chain acts on two types of Codes, A previously tabled one and a new
// one. On a previously tabled one, the chain value and suffix for that code 
// are preserved into OldSuffix and OldChain. The block operates on searching
// backward in the chains until a chain offset of 0-255 is found (meaning the
// terminal character has been reached.) Each character in the chain is saved
// on the stack.                                                             
//                                                                           
//---------------------------------------------------------------------------

//HandleChain:
		mov		edx,esp 		
		dec		esp

		mov		LZChain,eax        				//Save new chain as well
		lea		ebx, [LZHashBuffer+eax+eax*2]
		
		cmp		eax,LZFreeIndex					//is code in HASH TABLE already?
		jl		InTable      					//if yes, then process chain
	
		mov		al,LZOldSuffix					//get back the old suffix and plant it
		mov		[esp],al                   		//onto the stack for processing later
		dec		esp
		mov		[ebx+2],al				
		mov		eax,LZOldChain     				//get Old chain for creation of Old Chain
		mov		[ebx],ax
		lea		ebx, [LZHashBuffer+eax+eax*2]

InTable:
		test	ah,ah							//(ax<255) is current chain a character?  
		jz		ChainEnd   						//if yes, then begin Print out

		mov		al,[ebx+2]		 				//push suffix onto stack
		mov		[esp],al                   		//onto the stack for processing later

		dec		esp
		movzx	eax,word ptr [ebx]				//get chain to this code
		lea		ebx, [LZHashBuffer+eax+eax*2]
		jmp   InTable            				//and keep filling up
		
ChainEnd:
		mov		LZOldSuffix,al				//save last character in chain
		mov		[esp],al                   		//onto the stack for processing later

		sub		edx,esp 	
		mov		ebx,LZFreeIndex    				//get new code number index

send_bytes:
		mov		al,[esp]
		inc		esp

		mov		[edi],al
		inc		edi

		dec		edx
		jnz		send_bytes

//---------------------------------------------------------------------------
//                                                                           
// Here we add another chain to the hash table so that we continually use it 
// for more decompressions.                                                  
//                                                                           
//---------------------------------------------------------------------------

		mov		al,LZOldSuffix
		mov		edx,LZOldChain

		mov		byte ptr [LZHashBuffer+ebx+ebx*2+2],al
		mov		word ptr [LZHashBuffer+ebx+ebx*2],dx

		inc		ebx
		mov		eax,LZChain

		mov		LZFreeIndex,ebx
		mov		edx,LZMaxIndex

		mov		LZOldChain,eax
		cmp		ebx,edx

		jl    	GetCode

		cmp		ch,12
		je		GetCode

		inc		ch
		mov		ebx,LZCodeMask

		mov		eax,LZMaxIndex
		add		ebx,ebx

		add		eax,eax
		or		ebx,1

		mov		LZMaxIndex,eax
		mov		LZCodeMask,ebx

		jmp   	GetCode


error:
eof:
		sub		edi,dest
		mov		result,edi
	}
	return(result);
}
#else // LINUX_BUILD

#include"gameos.hpp"

//-------------------------------------------------------------------------------
// Microsoft's original LZ: a C port of the asm LZDecomp above, for data
// written by the asm LZCompress in lzcomp.cpp (retail-era and mod archives,
// e.g. MechCommander Omnitech's tgl.fst). Codes are packed LSB first: 9 bits
// after a CLEAR (256), one bit wider each time the next free table code
// reaches 512, 1024, 2048 (12 bits at most); EOF (257) ends the data and table
// codes start at 258. Unlike the asm, every write is bounded by destLen:
// output past it, input that ends before EOF, or a code that is not in the
// table yet returns 0.
static size_t LZDecompOriginal (MemoryPtr dest, MemoryPtr src, size_t srcLen, size_t destLen)
{
	const unsigned int MAX_CODES = 1 << 12;
	unsigned short chain[MAX_CODES];	// the code each table entry extends
	unsigned char suffix[MAX_CODES];	// and the byte it adds
	unsigned char stack[MAX_CODES];		// one string, last byte first

	size_t srcPos = 0;
	unsigned int bitBuf = 0;
	unsigned int bitCount = 0;
	unsigned int codeBits = BASE_BITS;
	unsigned int maxIndex = MAX_BIT_INDEX;
	unsigned int freeIndex = HASH_FREE;
	bool haveOld = false;			// false straight after a CLEAR
	unsigned int oldCode = 0;
	unsigned char oldFirst = 0;		// first byte of the previous string
	size_t outLen = 0;

	for (;;)
	{
		while (bitCount < codeBits)
		{
			if (srcPos >= srcLen)
				return 0;			// truncated: no EOF
			bitBuf |= (unsigned int)src[srcPos++] << bitCount;
			bitCount += 8;
		}
		const unsigned int code = bitBuf & ((1u << codeBits) - 1);
		bitBuf >>= codeBits;
		bitCount -= codeBits;

		if (code == HASH_EOF)
			return outLen;

		if (code == HASH_CLEAR)
		{
			codeBits = BASE_BITS;
			maxIndex = MAX_BIT_INDEX;
			freeIndex = HASH_FREE;
			haveOld = false;
			continue;
		}

		if (!haveOld)
		{
			// first code after a CLEAR: one byte, no new table entry
			if (code > 255 || outLen >= destLen)
				return 0;
			dest[outLen++] = (unsigned char)code;
			oldCode = code;
			oldFirst = (unsigned char)code;
			haveOld = true;
			continue;
		}

		if (code > freeIndex)
			return 0;

		unsigned int sp = 0;
		unsigned int c = code;
		if (code == freeIndex)
		{
			// not in the table yet: the previous string plus its first byte
			stack[sp++] = oldFirst;
			c = oldCode;
		}
		while (c >= HASH_FREE)
		{
			// each entry extends a lower code, so this ends, but stay in range
			if (sp >= MAX_CODES - 1)
				return 0;
			stack[sp++] = suffix[c];
			c = chain[c];
		}
		const unsigned char first = (unsigned char)c;
		stack[sp++] = first;

		if (sp > destLen - outLen)
			return 0;
		while (sp)
			dest[outLen++] = stack[--sp];

		// new entry: the previous string plus the first byte of this one
		if (freeIndex < MAX_CODES)
		{
			chain[freeIndex] = (unsigned short)oldCode;
			suffix[freeIndex] = first;
			freeIndex++;
		}
		if (freeIndex >= maxIndex && codeBits < 12)
		{
			codeBits++;
			maxIndex <<= 1;
		}

		oldCode = code;
		oldFirst = first;
	}
}

//-------------------------------------------------------------------------------
// zlib, as written by LZCompress under LINUX_BUILD (makefst, the texture and
// packet caches).
static size_t LZDecompZlib (MemoryPtr dest, MemoryPtr src, size_t srcLen, size_t destLen)
{
    const int CHUNK = 16384;
    int ret;
    unsigned have;
    z_stream strm;
    unsigned char in[CHUNK];
    unsigned char out[CHUNK];

    /* allocate inflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    ret = inflateInit(&strm);
    if (ret != Z_OK)
        return 0;

    size_t len_left = srcLen;
    MemoryPtr dataptr = src;
    size_t out_size = 0;
    do {

        strm.avail_in = len_left>CHUNK ? CHUNK : len_left;
        memcpy(in, dataptr, strm.avail_in);
        dataptr += strm.avail_in;
		len_left -= strm.avail_in;

        if (strm.avail_in == 0)
            break;
        strm.next_in = in;

        do {
            strm.avail_out = CHUNK;
            strm.next_out = out;

            ret = inflate(&strm, Z_NO_FLUSH);
            gosASSERT(ret != Z_STREAM_ERROR);  /* state not clobbered */
            switch (ret) {
                case Z_NEED_DICT:
                    ret = Z_DATA_ERROR;     /* and fall through */
                case Z_DATA_ERROR:
                case Z_MEM_ERROR:
                        (void)inflateEnd(&strm);
                    return 0;
            }

            have = CHUNK - strm.avail_out;
            if (have > destLen - out_size) {
                (void)inflateEnd(&strm);
                return 0;
            }
            memcpy(dest, out, have);
            dest += have;
            out_size += have;

        } while (strm.avail_out == 0);
        /* done when inflate() says it's done */
    } while (ret != Z_STREAM_END);

    /* clean up and return */
    (void)inflateEnd(&strm);
    return ret == Z_STREAM_END ? out_size : 0;
}

//-------------------------------------------------------------------------------
// LZ DeCompress Routine
// Takes a pointer to dest buffer, a pointer to source buffer and len of source.
// returns length of decompressed image, or 0 if it is corrupt or would
// exceed destLen.
//
// The data says which format it is. The asm LZCompress always starts with a
// 9-bit CLEAR (256): byte 0 is 0x00 and bit 0 of byte 1 is set. A zlib stream
// starts with its CMF byte, whose low nibble is 8 (deflate), so it never
// starts with 0x00. The FST version cannot tell them apart: this port's
// makefst writes zlib under 0xFADDECAF, the original tools wrote LZ.
size_t LZDecomp (MemoryPtr dest, MemoryPtr src, size_t srcLen, size_t destLen)
{
	if (!dest || !src)
		return 0;
	if (srcLen >= 2 && src[0] == 0 && (src[1] & 1))
		return LZDecompOriginal(dest, src, srcLen, destLen);
	return LZDecompZlib(dest, src, srcLen, destLen);
}
#endif // LINUX_BUILD
