//---------------------------------------------------------------------------
//
// csvfile.cpp - This file contains the class declaration for the CSV Files
//
//				The CSV file is an Excel csv style file.
//
//	MechCommander 2
//
//---------------------------------------------------------------------------//
// Copyright (C) Microsoft Corporation. All rights reserved.                 //
//===========================================================================//

//---------------------------------------------------------------------------
// Include files

#ifndef CSVFILE_H
#include"csvfile.h"
#endif

#ifndef HEAP_H
#include"heap.h"
#endif

#include<stdio.h>
#include<stdlib.h>
#include<math.h>
#include<string.h>
#include<ctype.h>

//#ifndef _MBCS
#include<gameos.hpp>
#include<utils/str_bounded.h>
//#else
//#include<assert.h>
//#define gosASSERT assert
//#define gos_Malloc malloc
//#define gos_Free free
//#endif

//---------------------------------------------------------------------------
// Static Globals

#undef isspace		//Macro Chokes under Intel Compiler!!

//---------------------------------------------------------------------------
// class CSVIniFile
CSVFile::CSVFile (void) : File()
{
	totalRows = totalCols = 0L;
	dataBuffer[0] = '\0';
	bufferedRow = 0;
	rowBuffer[0] = '\0';
}

//---------------------------------------------------------------------------
CSVFile::~CSVFile (void)
{
	close();
}

//---------------------------------------------------------------------------
long CSVFile::countRows (void)
{
	long count = 0;
	long oldPosition = logicalPosition;
	
	seek(0);		//Start at the top.
	char tmp[2048];
	
	// where each line starts, for seekRowCol: row n is the nth line read here
	rowStart.clear();
	bufferedRow = 0;
	rowStart.push_back(logicalPosition);
	readLine((MemoryPtr)tmp,2047);
	while (!eof())
	{
		count++;
		rowStart.push_back(logicalPosition);
		readLine((MemoryPtr)tmp,2047);
	}
	
	//----------------------------------
	// Move back to where we were.
	seek(oldPosition);
	
	return(count);
}

//---------------------------------------------------------------------------
long CSVFile::countCols (void)
{
	long count = 0, maxCols = 0;
	long oldPosition = logicalPosition;
	
	seek(0);		//Start at the top.
	char tmp[2048];
	char *currentChk = tmp;
	
	readLine((MemoryPtr)tmp,2047);
	currentChk = strstr(tmp,",");
	while (currentChk && (*currentChk != '\n') && (*currentChk != '\r'))
	{
		count++;
		currentChk++;
		currentChk = strstr(currentChk,",");
	}
	
	if (count > maxCols)
		maxCols = count;
		
	readLine((MemoryPtr)tmp,2047);
	
	//----------------------------------
	// Move back to where we were.
	seek(oldPosition);
	
	return(maxCols);
}

//---------------------------------------------------------------------------
long CSVFile::getNextWord (char *&line, char *buffer, unsigned long bufLen)
{
	//--------------------------------------------------
	// Check to see if we are at end of line
	if (*line == '\0')
		return(-1);
		
	//--------------------------------------------------
	// Check to see if the rest of the line is comments
	if (*line == '/')
		return(-1);

	if ( *line == ',' ) // empty column, move on
		return -1;
		
	//------------------------------------------
	// Find start of word from current location
	while ((*line != '\0') && ((*line == ' ') || (*line == '\t') || (*line == ',')))
	{
		line++;
	}
	
	//--------------------------------------------------
	// Check to see if we are at end of line
	if (*line == '\0')
		return(-1);
		
	//--------------------------------------------------
	// Check to see if the rest of the line is comments
	if (*line == '/')
		return(-1);
		
	//-------------------------------------------
	// Find length of word from current location
	char *startOfWord = line;
	unsigned long wordLength = 0;
	while ((*line != '\0') && ((*line != ',')))
	{
		line++;
		wordLength++;
	}
	
	if (wordLength > bufLen)
		return(-2);
		
	strncpy(buffer, startOfWord, wordLength);
	buffer[wordLength] = '\0';
	
	return(NO_ERR);
}

//---------------------------------------------------------------------------
long CSVFile::afterOpen (void)
{
	//-------------------------------------------------------
	// Check if we opened this with CREATE and write the
	// FITini Header and position to Write Start.
	if (fileMode == CREATE && parent == NULL)
	{
		STOP(("Cannot write CSV files at present."));
	}
	else
	{
		//------------------------------------------------------
		// Find out how many Rows and cols we have
		totalRows = countRows();
		totalCols = countCols();
	}

	return(NO_ERR);
}

//---------------------------------------------------------------------------
void CSVFile::atClose (void)
{
	//------------------------------------------------------------
	// Check if we are in create mode and if so, write the footer
	if (fileMode == CREATE)
	{
		STOP(("Cannot write CSV files at present."));
	}

	totalRows = totalCols = 0;
	rowStart.clear();
	bufferedRow = 0;
}

//---------------------------------------------------------------------------
float CSVFile::textToFloat (char *num)
{
	float result = atof(num);
	return(result);
}

//---------------------------------------------------------------------------
long CSVFile::textToLong (char *num)
{
	// Every whole-number reader goes through wholeNumberLow32 (str_bounded.h),
	// so a value reads the same on every platform: atol into a long kept
	// 4278190080 whole where long is 64-bit and clamped it on Windows.
	return((long)(int32_t)wholeNumberLow32(num));
}


//---------------------------------------------------------------------------
short CSVFile::textToShort (char *num)
{
	return((short)(int16_t)(uint16_t)wholeNumberLow32(num));
}


//---------------------------------------------------------------------------
char CSVFile::textToChar (char *num)
{
	return((char)(int8_t)(uint8_t)wholeNumberLow32(num));
}

//---------------------------------------------------------------------------
unsigned long CSVFile::textToULong (char *num)
{
	return((unsigned long)wholeNumberLow32(num));
}


//---------------------------------------------------------------------------
unsigned short CSVFile::textToUShort (char *num)
{
	return((unsigned short)wholeNumberLow32(num));
}


//---------------------------------------------------------------------------
unsigned char CSVFile::textToUChar (char *num)
{
	return((unsigned char)wholeNumberLow32(num));
}

//---------------------------------------------------------------------------
bool CSVFile::booleanToLong (char *num)
{
	char testChar = 0;
	while (num[testChar] && isspace(num[testChar]))
		testChar++;

	// 'N' == NO if you can believe that
	if ((toupper(num[testChar]) == 'F') || (toupper(num[testChar]) == '0') || (toupper(num[testChar]) == 'N') )
		return FALSE;
	else
		return(TRUE);
}	

//---------------------------------------------------------------------------
long CSVFile::floatToText (char *result, float num, unsigned long bufLen)
{
	char temp[250];
	sprintf(temp,"%.4f",num);

	unsigned long numLength = strlen(temp);
	if (numLength >= bufLen)
		return(-2);

	strncpy(result,temp,numLength);
	result[numLength] = '\0';
	
	return(NO_ERR);
}			

//---------------------------------------------------------------------------
long CSVFile::longToTextDec (char *result, long num, unsigned long bufLen)
{
	char temp[250];
	sprintf(temp,"%ld",num);

	unsigned long numLength = strlen(temp);
	if (numLength >= bufLen)
		return(-2);

	strncpy(result,temp,numLength);
	result[numLength] = '\0';
	
	return(NO_ERR);
}	

//---------------------------------------------------------------------------
long CSVFile::longToTextHex (char *result, long num, unsigned long bufLen)
{
	char temp[250];
	sprintf(temp,"0x%lx",num);

	unsigned long numLength = strlen(temp);
	if (numLength >= bufLen)
		return(-2);

	strncpy(result,temp,numLength);
	result[numLength] = '\0';
	
	return(NO_ERR);
}	

//---------------------------------------------------------------------------
long CSVFile::shortToTextDec (char *result, short num, unsigned long bufLen)
{
	char temp[250];
	sprintf(temp,"%d",num);

	unsigned long numLength = strlen(temp);
	if (numLength >= bufLen)
		return(-2);

	strncpy(result,temp,numLength);
	result[numLength] = '\0';
	
	return(NO_ERR);
}	

//---------------------------------------------------------------------------
long CSVFile::shortToTextHex (char *result, short num, unsigned long bufLen)
{
	char temp[250];
	sprintf(temp,"0x%x",num);

	unsigned long numLength = strlen(temp);
	if (numLength >= bufLen)
		return(-2);

	strncpy(result,temp,numLength);
	result[numLength] = '\0';
	
	return(NO_ERR);
}	

//---------------------------------------------------------------------------
long CSVFile::byteToTextDec (char *result, byte num, unsigned long bufLen)
{
	char temp[250];
	sprintf(temp,"%d",num);

	unsigned long numLength = strlen(temp);
	if (numLength >= bufLen)
		return(-2);

	strncpy(result,temp,numLength);
	result[numLength] = '\0';
	
	return(NO_ERR);
}	

//---------------------------------------------------------------------------
long CSVFile::byteToTextHex (char *result, byte num, unsigned long bufLen)
{
	char temp[250];
	sprintf(temp,"0x%x",num);

	unsigned long numLength = strlen(temp);
	if (numLength >= bufLen)
		return(-2);

	strncpy(result,temp,numLength);
	result[numLength] = '\0';
	
	return(NO_ERR);
}	

//---------------------------------------------------------------------------
long CSVFile::open (const char* fName, FileMode _mode, long numChild)
{
	long result = File::open(fName,_mode,numChild);
	if (result != NO_ERR)
		return(result);
		
	seek(0);

	result = afterOpen();
	
	return(result);
}

//---------------------------------------------------------------------------
long CSVFile::open (FilePtr _parent, unsigned long fileSize, long numChild)
{
	numChild = -1;		//Force all parented CSVs to load from RAM.
	long result = File::open(_parent,fileSize,numChild);
	if (result != NO_ERR)
		return(result);
		
	result = afterOpen();
	
	return(result);
}

//---------------------------------------------------------------------------
long CSVFile::create (char* fName)
{
	STOP(("CSV file write is not supported %s",fName));
	return(-1);
}

//---------------------------------------------------------------------------
void CSVFile::close (void)
{
	if (isOpen())
	{
		atClose();
		File::close();
	}
}

//---------------------------------------------------------------------------
long CSVFile::seekRowCol (DWORD row, DWORD col)
{
	if ((row > totalRows) || (col > totalCols))
		return -1;
		
	//--------------------------------------------------------------------
	// The row: the one already read, or straight from where countRows saw it
	// start. Rescanning from the top for every cell made Omnitech's variant
	// CSVs (loose files, read a line at a time) most of a mission load.
	char *tmp = rowBuffer;
	if (row == 0)		//rows count from 1: the scan below never found row 0
		return -1;
	if (row != bufferedRow)
	{
		if ((row >= 1) && (row <= rowStart.size()))
		{
			seek(rowStart[row - 1]);
			readLine((MemoryPtr)tmp,2047);
		}
		else
		{
			DWORD rowCount = 0;
			seek(0);		//Start at the top.
			do
			{
				rowCount++;
				readLine((MemoryPtr)tmp,2047);
			} while (rowCount != row);  
		}
		bufferedRow = row;
	}
	else if (row < rowStart.size())
		seek(rowStart[row]);		//just past the row, as reading it leaves the file
	
	char *currentChk = tmp;
	if (col)
	{
		DWORD colCount = 1;
		
		while (currentChk && (colCount != col))
		{
			colCount++;
			currentChk = strstr(currentChk,",");
			if ( currentChk ) // if we increment to one, bad things happen
				currentChk++;

		}
	}
	
	//---------------------------------------------------
	// We are now pointing at the row and col specified.
	if (currentChk)
	{
		char *data = dataBuffer;
		return getNextWord(currentChk,data,2047);
	}
	else
	{
		return -1;
		// CAN'T do this, for some reason excel writes out empty rows.
		//STOP(("Unable to parse CSV %s, ROW %d, COL %D, ERROR: NULL",getFilename(),row,col));
	}
	
	//return(NO_ERR);
}

//---------------------------------------------------------------------------
long CSVFile::readFloat (DWORD row, DWORD col, float &value)
{
	long result = seekRowCol(row,col);
	if (result == NO_ERR)
	{
		value = textToFloat(dataBuffer);
	}
	else
		value = 0.0f;
	
	return(NO_ERR);
}

//---------------------------------------------------------------------------
long CSVFile::readInt (DWORD row, DWORD col, int &value)
{
    long tmp;
    long rv = readLong(row, col, tmp);
    value = tmp;
    return rv;
}

//---------------------------------------------------------------------------
long CSVFile::readLong (DWORD row, DWORD col, long &value)
{
	long result = seekRowCol(row,col);
	if (result == NO_ERR)
	{
		value = textToLong(dataBuffer);
	}
	else
		value = 0.0f;
	
	return(NO_ERR);
}

//---------------------------------------------------------------------------
long CSVFile::readBoolean (DWORD row, DWORD col, bool &value)
{
	long result = seekRowCol(row,col);
	if (result == NO_ERR)
	{
		value = booleanToLong(dataBuffer);
	}
	else
		value = 0;
	
	return(NO_ERR);
}

//---------------------------------------------------------------------------
long CSVFile::readShort (DWORD row, DWORD col, short &value)
{
	long result = seekRowCol(row,col);
	if (result == NO_ERR)
	{
		value = textToShort(dataBuffer);
	}
	else
		value = 0.0f;
	
	return(NO_ERR);
}

//---------------------------------------------------------------------------
long CSVFile::readChar (DWORD row, DWORD col, char &value)
{
	long result = seekRowCol(row,col);
	if (result == NO_ERR)
	{
		value = textToChar(dataBuffer);
	}
	else
		value = 0.0f;
	
	return(NO_ERR);
}

//---------------------------------------------------------------------------
long CSVFile::readULong (DWORD row, DWORD col, unsigned long &value)
{
	long result = seekRowCol(row,col);
	if (result == NO_ERR)
	{
		value = textToULong(dataBuffer);
	}
	else
		value = 0.0f;
	
	return(NO_ERR);
}

//---------------------------------------------------------------------------
long CSVFile::readUShort (DWORD row, DWORD col, unsigned short &value)
{
	long result = seekRowCol(row,col);
	if (result == NO_ERR)
	{
		value = textToUShort(dataBuffer);
	}
	else
		value = 0.0f;
	
	return(NO_ERR);
}

//---------------------------------------------------------------------------
long CSVFile::readUChar (DWORD row, DWORD col, unsigned char &value)
{
	long result = seekRowCol(row,col);
	if (result == NO_ERR)
	{
		value = textToUChar(dataBuffer);
	}
	else
		value = 0.0f;
	
	return(NO_ERR);
}

//---------------------------------------------------------------------------
long CSVFile::copyString (char *dest, char *src, unsigned long bufLen)
{
	unsigned long offset = 0;
	//---------------------------------------
	// Copy each character until close quote
	while (*src != '"' && *src != '\0' && offset < bufLen)
	{
		dest[offset] = *src;
		src++;
		offset++;
	}

	//----------------------------------------------------
	// If this string is longer than buffer, let em know.
	// Cut and terminate it too: callers that ignore the error would read on.
	if (offset == bufLen)
	{
		if (bufLen)
			dest[bufLen - 1] = '\0';
		return(-2);
	}

	//---------------------------------
	// otherwise, NULL term and return	
	dest[offset] = '\0';
	return(NO_ERR);
}

//---------------------------------------------------------------------------
long CSVFile::readString (DWORD row, DWORD col, char *result, unsigned long bufferSize)
{
	long res = seekRowCol(row,col);
	if (res == NO_ERR)
	{
		long errorCode = copyString(result,dataBuffer,bufferSize);
		if (errorCode != NO_ERR)
			return(errorCode);
	}
	else
		return 1;	
	return(0); // gotta return some kind of error!
}

//---------------------------------------------------------------------------
//
// Edit log
//
//---------------------------------------------------------------------------

