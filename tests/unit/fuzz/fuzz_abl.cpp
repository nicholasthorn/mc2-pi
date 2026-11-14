// tests/unit/fuzz/fuzz_abl.cpp: the ABL compiler over any bytes.
//
// Compiles the input as a brain script, "fuzz.abl", with ABLi_preProcess:
// the scanner, the parser and the code generator, reading it line by line
// through File::readLineEx as the game does. ABL is set up afresh for each
// input with the game's sizes (code/ablmc2.cpp), and every block it asks for
// goes through callbacks that track it, so a compile cut short by a syntax
// error (ABL_Fatal) is cleaned up before the next. A few functions with the
// kinds of parameters the game's have are registered, so calls parse.

#include "fuzz_targets.h"
#include "fuzz_support.h"

#include "abl.h"
#include "file.h"

#include <cstdlib>
#include <cstring>
#include <list>
#include <set>
#include <vector>

namespace
{

std::vector<char> g_script;
std::set<void*> g_blocks;
// the script files ABL has open (File has no virtual destructor, so not new/delete)
std::list<File> g_files;

void* trackedMalloc(unsigned long size)
{
	void* p = malloc(size ? size : 1);
	if (p)
		g_blocks.insert(p);
	return p;
}

void trackedFree(void* p)
{
	if (!p)
		return;
	g_blocks.erase(p);
	free(p);
}

void freeEverything()
{
	g_files.clear();
	for (void* p : g_blocks)
		free(p);
	g_blocks.clear();
}

long fileCreate(void**, const char*)
{
	// scripts do not write files while compiling
	fuzzReject();
}

long fileOpen(void** file, const char* name)
{
	// the game STOPs on a script it cannot open (ablFileOpenCB)
	if (strcmp(name, "fuzz.abl") != 0)
		fuzzReject();
	g_files.emplace_back();
	File& f = g_files.back();
	if (f.open(g_script.data(), (int)g_script.size()) != NO_ERR)
	{
		g_files.pop_back();
		fuzzReject();
	}
	*file = &f;
	return NO_ERR;
}

long fileClose(void** file)
{
	File* f = static_cast<File*>(*file);
	f->close();
	for (std::list<File>::iterator it = g_files.begin(); it != g_files.end(); ++it)
	{
		if (&*it == f)
		{
			g_files.erase(it);
			break;
		}
	}
	*file = nullptr;
	return 0;
}

bool fileEof(void* file)
{
	return static_cast<File*>(file)->eof();
}

long fileRead(void* file, unsigned char* buffer, long length)
{
	return static_cast<File*>(file)->read(buffer, length);
}

int32_t fileReadInt(void* file)
{
	return static_cast<File*>(file)->readInt();
}

long fileReadLong(void* file)
{
	return static_cast<File*>(file)->readLong();
}

long fileReadString(void* file, unsigned char* buffer)
{
	return static_cast<File*>(file)->readString(buffer);
}

long fileReadLineEx(void* file, unsigned char* buffer, long maxLength)
{
	return static_cast<File*>(file)->readLineEx(buffer, maxLength);
}

long fileWrite(void*, unsigned char*, long length)
{
	return length;
}

long fileWriteByte(void*, unsigned char)
{
	return 1;
}

long fileWriteInt(void*, int32_t)
{
	return 4;
}

long fileWriteLong(void*, long)
{
	return 4;
}

long fileWriteString(void*, const char* buffer)
{
	return (long)strlen(buffer);
}

void debuggerPrint(const char*)
{
}

void fatal(long, const char*)
{
	fuzzReject();
}

// Never run: the input is only compiled.
void execNothing(void)
{
}

void initAbl()
{
	ABLi_init(ABL_RUNTIME_STACK_SIZE, ABL_MAX_CODE_BUFFER_SIZE,
		ABL_MAX_REGISTERED_MODULES, ABL_MAX_STATIC_VARIABLES,
		trackedMalloc, trackedMalloc, trackedMalloc, trackedMalloc,
		trackedFree, trackedFree, trackedFree, trackedFree,
		fileCreate, fileOpen, fileClose, fileEof, fileRead, fileReadInt, fileReadLong,
		fileReadString, fileReadLineEx, fileWrite, fileWriteByte, fileWriteInt,
		fileWriteLong, fileWriteString, debuggerPrint, fatal, true, false, false);
	ABLi_addFunction("fuzzint", false, "i", "i", execNothing);
	ABLi_addFunction("fuzzreal", false, "rr", "r", execNothing);
	ABLi_addFunction("fuzzname", false, "C", "i", execNothing);
	ABLi_addFunction("fuzzorder", true, "ib", "i", execNothing);
	ABLi_addFunction("fuzznone", false, NULL, NULL, execNothing);
}

} // namespace

int fuzzAblScript(const uint8_t* data, size_t size)
{
	if (size == 0 || size > (1 << 18))
		return 0;
	g_script.assign(data, data + size);

	const bool compiled = fuzzGuarded([] {
		initAbl();
		long numErrors = 0;
		long numLines = 0;
		long numFiles = 0;
		if (ABLi_preProcess("fuzz.abl", &numErrors, &numLines, &numFiles, false) >= 0)
			fuzzNoteLoaded();
	});
	// A compile cut short leaves ABL half built: skip its own teardown then,
	// the next input sets it up afresh, and free what it had.
	if (compiled)
		fuzzGuarded([] { ABLi_close(); });
	freeEverything();
	return 0;
}

long ablTestCompile(const char* source)
{
	g_script.assign(source, source + strlen(source));
	long handle = -1;
	if (!fuzzGuarded([&handle] {
			initAbl();
			long numErrors = 0;
			long numLines = 0;
			long numFiles = 0;
			handle = ABLi_preProcess("fuzz.abl", &numErrors, &numLines, &numFiles, false);
		}))
	{
		freeEverything();
		return -1;
	}
	if (handle < 0)
		ablTestClose();
	return handle;
}

void ablTestClose()
{
	fuzzGuarded([] { ABLi_close(); });
	freeEverything();
}
