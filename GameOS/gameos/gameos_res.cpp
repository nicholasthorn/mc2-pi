#include "gameos.hpp"
#include "strres.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <SDL2/SDL_loadso.h>
#include <map>
#include <string>
#include <vector>
#include "utils/pe_strings.h"

#ifdef PLATFORM_WINDOWS
static void* DL_Open(const char* name) {
	return SDL_LoadObject(name);
}
static void DL_Close(void* handle) {
	SDL_UnloadObject(handle);
}
static void* DL_LoadFunction(void* handle, const char* name) {
	return SDL_LoadFunction(handle, name);
}
// mimic dlerror
static const char* DL_GetError() {
	const char* err = SDL_GetError();
	SDL_ClearError();
	return strlen(err)==0 ? NULL : err;
}
#else
#include <dlfcn.h>

static void* DL_Open(const char* name) {
#ifdef __ANDROID__
    // the APK's libraries are found by name, not in the data folder ("./")
    if (strncmp(name, "./", 2) == 0)
        name += 2;
#endif
    return dlopen(name, RTLD_LAZY);
}
static void DL_Close(void* handle) {
	dlclose(handle);
}
static void* DL_LoadFunction(void* handle, const char* name) {
	return dlsym(handle, name);
}
static const char* DL_GetError() {
	 const char* err = dlerror();
	 return err;
}
#endif

HSTRRES __stdcall gos_OpenResourceDLL(char const* FileName, const char** strings, int num)
{
    const char *error;
    void *module;
    get_string_by_id_fptr fptr;

	DL_GetError();

    /* Load dynamically loaded library */
    module = DL_Open(FileName);
    gosASSERT(module);
    if (!module) {
        fprintf(stderr, "Couldn't open resourse dll: %s\n", DL_GetError());
        return NULL;
    }

	DL_GetError();    /* Clear any existing error */

    /* Get symbol */
    fptr = (get_string_by_id_fptr)DL_LoadFunction(module, "getStringById");
    if ((error = DL_GetError())) {
        fprintf(stderr, "Couldn't load function getStringById in %s (not exported?): %s\n", FileName, error);
        return NULL;
    }

    init_string_resources_fptr init_fptr;
    init_fptr = (init_string_resources_fptr)DL_LoadFunction(module, "initStringResources");
    if ((error = DL_GetError())) {
        fprintf(stderr, "Couldn't load function initStringResources in %s (not exported?): %s\n", FileName, error);
        return NULL;
    }
    init_fptr();

    gos_StringRes* pstrres = new gos_StringRes();
    pstrres->getStringByIdFptr = fptr;
    pstrres->module = module;

    /* deprecated */
    pstrres->strings = NULL;
    pstrres->num_strings = 0;

    return pstrres;
}
void __stdcall gos_CloseResourceDLL(HSTRRES handle)
{
    gos_StringRes* pstrres = (gos_StringRes*)handle;
    gosASSERT(pstrres && pstrres->module);

    free_string_resources_fptr free_fptr;
    DL_GetError();

    const char *error;
    free_fptr = (free_string_resources_fptr)DL_LoadFunction(pstrres->module, "freeStringResources");
    if ((error = DL_GetError())) {
        fprintf(stderr, "Couldn't find hello: %s\n", error);
    } else {
        free_fptr();
    }

    DL_Close(pstrres->module);
    delete pstrres;
}

// Strings the game's own table lacks, from a Windows resource DLL named
// mc2res.dll in the game folder (MechCommander Omnitech keeps the names and
// descriptions of its new units there). Read on the first miss; the game's
// table always wins, so stock text is never replaced.
static const char* getExtraString(DWORD id)
{
    static std::map<uint32_t, std::string>* extra = NULL;
    if (!extra) {
        extra = new std::map<uint32_t, std::string>();
        FILE* f = fopen("mc2res.dll", "rb");
        if (f) {
            std::vector<uint8_t> dll;
            uint8_t buf[65536];
            size_t n;
            while ((n = fread(buf, 1, sizeof(buf), f)) > 0)
                dll.insert(dll.end(), buf, buf + n);
            fclose(f);
            if (readPeStringTable(dll.data(), dll.size(), *extra))
                fprintf(stderr, "gos_res: %u strings from mc2res.dll for ids the game's table lacks\n",
                        (unsigned)extra->size());
        }
    }
    std::map<uint32_t, std::string>::const_iterator it = extra->find(id);
    return it == extra->end() ? NULL : it->second.c_str();
}

const char* __stdcall gos_GetResourceString(HSTRRES handle, DWORD id)
{
    static const char* dummy_string_res = "missing string res";
    gosASSERT(handle);
    gos_StringRes* pstrres = (gos_StringRes*)(handle);
    const char* str = pstrres->getStringByIdFptr(id);
    if(NULL == str)
        str = getExtraString(id);

    if(NULL == str) {
        fprintf(stderr, "Requested string id: %u not found, return dummy string\n", id);
        return dummy_string_res;
    }
    return str;
}
