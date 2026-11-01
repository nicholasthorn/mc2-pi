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

// Formats "<header>: <message>\n" and logs it; also copies it into out
// (MAX_LOG_LINE bytes) when given one, for STOP's message box.
static void logmsg(const char* header, const char* fmt, va_list ap, char* out = NULL)
{
	assert(fmt);
	if (out) out[0] = '\0';
	if (!strlen(fmt)) return;

	char				text[MAX_LOG_LINE] = {0};

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
    if (out) memcpy(out, text, MAX_LOG_LINE);
}

static void logmsgf(char* out, const char* header, const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    logmsg(header, fmt, ap, out);
    va_end(ap);
}

// The message box a player sees before the game ends. Only once the game has
// started SDL video (SDL_VideoInit, which SDL_WasInit does not report): the
// command-line tools (makefst, pak, ...) link this file too and must not block
// a build script on a box. Not for automated runs (smoke, bench, UI walks):
// under Xvfb nobody would close it. Fails quietly where SDL has no way to
// show one (e.g. KMSDRM with no window manager). Parented to the game window,
// with the mouse let go as set_mouse_capture() does (gameosmain.cpp), so a
// fullscreen window cannot hide it. Shows the message without the log's
// "STOP: " prefix.
static void show_error_box(const char* text)
{
    if (gos_IsAutomatedRun() || !SDL_GetCurrentVideoDriver())
        return;
    if (strncmp(text, "STOP: ", 6) == 0)
        text += 6;
    while (*text == ' ')
        text++;
    SDL_Window* window = SDL_GL_GetCurrentWindow();
    if (window) {
        SDL_SetWindowGrab(window, SDL_FALSE);
        SDL_CaptureMouse(SDL_FALSE);
        SDL_ShowCursor(SDL_ENABLE);
    }
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "MechCommander 2", text, window);
}

// _Exit, not exit: static destructors over a broken engine can crash and hide
// the message. SDL_Quit first, after the message is out, so a fullscreen or
// KMSDRM (Pi console) display mode is put back; it also tears down video
// started with SDL_VideoInit, and does nothing when SDL never started.
[[noreturn]] static void exit_after_error(const char* text, const char* reason)
{
    fprintf(stderr, "STOP: exiting (%s)\n", reason);
    fflush(stdout);
    fflush(stderr);
    show_error_box(text);
    SDL_Quit();
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
    if (exits)
        exit_after_error(text, "MC2_STOP_EXITS=0 carries on");
}

// those are game specific callbacks (should move them outside to game)
int __cdecl InternalFunctionStop( const char* fmt, ... )
{
	assert(fmt);
	va_list	ap;
	char text[MAX_LOG_LINE];

    va_start(ap, fmt);
    logmsg("STOP", fmt, ap, text);
    va_end(ap);

    stop_exit_unless_disabled(text);
    return enter_debugger;
}

int __cdecl InternalFunctionStop( const char* Message, const char* value)
{
    // STOP(("... %s", str)) resolves to this overload; same "STOP: " line as
    // the variadic one, so smoke's ^STOP: check sees it.
    char text[MAX_LOG_LINE];
    logmsgf(text, "STOP", Message, value);
    stop_exit_unless_disabled(text);
    return enter_debugger;
}

void __cdecl gos_FatalExit( const char* message )
{
    char text[MAX_LOG_LINE];
    if (message)
        logmsgf(text, "STOP", "%s", message);
    else
        snprintf(text, sizeof(text), "A fatal error ended the game.");
    // As the STOP macro does: a debug build stops in the debugger here, with
    // the stack of the Fatal() still there.
    if (enter_debugger)
        ENTER_DEBUGGER
    exit_after_error(text, "fatal error");
}

int __cdecl InternalFunctionPause( const char* fmt, ... )
{
	assert(fmt);
	va_list	ap;

    va_start(ap, fmt);
    logmsg("PAUSE", fmt, ap);
    va_end(ap);

    return enter_debugger;

}
void __cdecl InternalFunctionSpew( const char* Group, const char* fmt, ... )
{
	assert(fmt);
	va_list	ap;

    char buf[256];
    StringFormat(buf, sizeof(buf)-1, "SPEW: <%s>", Group ? Group : "");

    va_start(ap, fmt);
    logmsg(buf, fmt, ap);
    va_end(ap);
}

int __stdcall ErrorHandler( int Flags, const char* Text )
{
    // TODO: additionally check flags to decide if enter debugger or not
    fputs(Text, stderr);
    return enter_debugger;
}
