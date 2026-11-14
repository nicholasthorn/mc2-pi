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

// Strings the game's own table lacks or only has a placeholder for, from a
// Windows resource DLL in the game folder: mc2res.dll (MechCommander Omnitech
// keeps the names and descriptions of its new units there) or, failing that,
// mc2xres.dll (wolfman-MC2X's name for it). Read on the first lookup. Real
// text in the game's table wins over mc2res.dll, so stock text is never
// replaced; mc2xres.dll is MC2X's whole table, which gives some stock ids
// other text (a mech's name where stock has a weapon's description), so its
// text wins (*replacesTable).
static const char* getExtraString(DWORD id, bool* replacesTable)
{
    static std::map<uint32_t, std::string>* extra = NULL;
    static bool mc2xTable = false;
    if (!extra) {
        extra = new std::map<uint32_t, std::string>();
        static const char* const names[] = { "mc2res.dll", "mc2xres.dll" };
        for (const char* name : names) {
            FILE* f = fopen(name, "rb");
            if (!f)
                continue;
            std::vector<uint8_t> dll;
            uint8_t buf[65536];
            size_t n;
            while ((n = fread(buf, 1, sizeof(buf), f)) > 0)
                dll.insert(dll.end(), buf, buf + n);
            fclose(f);
            if (readPeStringTable(dll.data(), dll.size(), *extra)) {
                mc2xTable = (strcmp(name, "mc2xres.dll") == 0);
                fprintf(stderr, "gos_res: %u strings from %s %s\n", (unsigned)extra->size(), name,
                        mc2xTable ? "over the game's table" : "for ids the game's table lacks");
            }
            break;
        }
    }
    *replacesTable = mc2xTable;
    std::map<uint32_t, std::string>::const_iterator it = extra->find(id);
    return it == extra->end() ? NULL : it->second.c_str();
}

const char* __stdcall gos_GetResourceString(HSTRRES handle, DWORD id)
{
    static const char* dummy_string_res = "missing string res";
    gosASSERT(handle);
    // Id 0 is no text: layouts give it to labels the game fills in at run time
    // (LoadString finds nothing for it). The table only has a placeholder there.
    if(id == 0)
        return "";
    // text the data named a type by, registered at load (addRuntimeString)
    if(id >= kRuntimeStringBase) {
        if(const char* runtime = findRuntimeString(id))
            return runtime;
    }
    gos_StringRes* pstrres = (gos_StringRes*)(handle);
    const char* str = pstrres->getStringByIdFptr(id);
    bool replacesTable = false;
    const char* extra = getExtraString(id, &replacesTable);
    if(extra && (replacesTable || NULL == str || isPlaceholderString(str))
        && (NULL == str || !isPlaceholderString(extra)))
        str = extra;

    if(NULL == str) {
        fprintf(stderr, "Requested string id: %u not found, return dummy string\n", id);
        return dummy_string_res;
    }
    return str;
}
