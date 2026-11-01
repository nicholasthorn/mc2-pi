#include "gameos.hpp"

#ifdef LINUX_BUILD
#include <cstdarg>
#else
#include <windows.h>
#endif

#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <cstring>
#include <SDL2/SDL.h>
#include "utils/string_utils.h"
#include "utils/str_bounded.h"

#ifdef _DEBUG
static int enter_debugger = 1;
#else
static int enter_debugger = 0;
#endif

static const int MAX_LOG_LINE = 1024;

static void output_debug_text(const char* text)
{
#ifdef LINUX_BUILD
    fprintf(stderr, "%s", text);
#else
	OutputDebugString(text);
#endif

}

// Formats "<header>: <message>\n" into text (MAX_LOG_LINE bytes) and logs it.
static void logmsg(const char* header, const char* fmt, va_list ap, char* text)
{
	assert(fmt);
	if (!strlen(fmt)) return;

    // header plus ": " (the message used to overwrite the separator)
    size_t header_len = strlen(header) + 2;

    snprintf(text, MAX_LOG_LINE, "%s: ", header);
	vsnprintf(text + header_len, MAX_LOG_LINE - header_len - 1, fmt, ap);

	// End each message with a newline (most format strings have none), so
	// the next STOP/PAUSE starts its own line in the log. The vsnprintf
	// limit above leaves room for it.
	size_t len = strlen(text);
	if (text[len - 1] != '\n') {
		text[len] = '\n';
		text[len + 1] = '\0';
	}
    output_debug_text(text);
}

// The message box a player sees before the game ends. Not for automated runs
// (smoke, bench, UI walks): under Xvfb nobody would close it. Fails quietly
// where SDL has no way to show one (e.g. KMSDRM with no window manager).
// Parented to the game window, with the mouse let go, so it is not hidden
// behind a fullscreen window the player cannot click out of.
static void show_error_box(const char* text)
{
    if (gos_IsAutomatedRun())
        return;
    SDL_Window* window = SDL_GL_GetCurrentWindow();
    if (window) {
        SDL_SetRelativeMouseMode(SDL_FALSE);
        SDL_SetWindowGrab(window, SDL_FALSE);
        SDL_ShowCursor(SDL_ENABLE);
    }
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "MechCommander 2", text, window);
}

// _Exit, not exit: static destructors over a broken engine can crash and hide
// the message.
[[noreturn]] static void exit_after_error(const char* text)
{
    fflush(stdout);
    fflush(stderr);
    show_error_box(text);
    _Exit(EXIT_FAILURE);
}

// STOP is documented as "can not be continued", and ends the process (so does
// Assert, which calls it). MC2_STOP_EXITS=0 makes STOP log and carry on
// instead, as the port used to: for bringing up mod data, where a run can
// show several problems at once. Code after a STOP then runs on into whatever
// it was guarding. Fatal and ABL_Fatal end through gos_FatalExit either way.
static void stop_exit_unless_disabled(const char* text)
{
    static const bool exits = envFlagOr(getenv("MC2_STOP_EXITS"), true);
    if (!exits)
        return;
    fprintf(stderr, "STOP: exiting (MC2_STOP_EXITS=0 carries on)\n");
    exit_after_error(text);
}

// those are game specific callbacks (should move them outside to game)
int __cdecl InternalFunctionStop( const char* fmt, ... )
{
	assert(fmt);
	va_list	ap;
	char text[MAX_LOG_LINE] = {0};

    va_start(ap, fmt);
    logmsg("STOP", fmt, ap, text);
    va_end(ap);

    stop_exit_unless_disabled(text);
    return enter_debugger;
}

int __cdecl InternalFunctionStop( const char* Message, const char* value)
{
    // Same "STOP: " prefix and trailing newline as logmsg, so smoke's ^STOP:
    // check also sees STOP(("... %s", str)), which resolves to this overload.
    char text[MAX_LOG_LINE] = {0};
    int len = snprintf(text, sizeof(text), "STOP: ");
    snprintf(text + len, sizeof(text) - len - 1, Message, value);
    len = (int)strlen(text);
    text[len] = '\n';
    output_debug_text(text);
    stop_exit_unless_disabled(text);
    return enter_debugger;
}

void __cdecl gos_FatalExit( const char* message )
{
    char text[MAX_LOG_LINE] = {0};
    if (message) {
        snprintf(text, sizeof(text) - 1, "STOP: %s", message);
        size_t len = strlen(text);
        if (len == 0 || text[len - 1] != '\n')
            text[len] = '\n';
        output_debug_text(text);
    } else {
        snprintf(text, sizeof(text), "A fatal error ended the game; the log has the details.\n");
    }
    fprintf(stderr, "STOP: exiting (fatal error)\n");
    exit_after_error(text);
}

int __cdecl InternalFunctionPause( const char* fmt, ... )
{
	assert(fmt);
	va_list	ap;
	char text[MAX_LOG_LINE] = {0};

    va_start(ap, fmt);
    logmsg("PAUSE", fmt, ap, text);
    va_end(ap);

    return enter_debugger;

}
void __cdecl InternalFunctionSpew( const char* Group, const char* fmt, ... )
{
	assert(fmt);
	va_list	ap;

    char buf[256];
    StringFormat(buf, sizeof(buf)-1, "SPEW: <%s>", Group ? Group : "");
    char text[MAX_LOG_LINE] = {0};

    va_start(ap, fmt);
    logmsg(buf, fmt, ap, text);
    va_end(ap);
}

int __stdcall ErrorHandler( int Flags, const char* Text )
{
    // TODO: additionally check flags to decide if enter debugger or not
    fputs(Text, stderr);
    return enter_debugger;
}
