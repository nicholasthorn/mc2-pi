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

static void logmsg(const char* header, const char* fmt, va_list ap) 
{
	assert(fmt);
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
}

// STOP is documented as "can not be continued", but this port only logs it, so
// code after a STOP (and Fatal, Assert, ABL_Fatal) runs on into what it was
// guarding. MC2_STOP_EXITS=1 ends the process instead; smoke and bench runs
// set it. _Exit, not exit: static destructors over a broken engine can crash
// and hide the message. Exiting by default is a later step (PROGRESS.md).
static void stop_exit_if_requested()
{
    static const bool exits = envFlagEnabled(getenv("MC2_STOP_EXITS"));
    if (!exits)
        return;
    fprintf(stderr, "STOP: exiting (MC2_STOP_EXITS)\n");
    fflush(stdout);
    fflush(stderr);
    _Exit(EXIT_FAILURE);
}

// those are game specific callbacks (should move them outside to game)
int __cdecl InternalFunctionStop( const char* fmt, ... )
{
	assert(fmt);
	va_list	ap;

    va_start(ap, fmt);
    logmsg("STOP", fmt, ap);
    va_end(ap);

    stop_exit_if_requested();
    return enter_debugger;
}

int __cdecl InternalFunctionStop( const char* Message, const char* value)
{
    // Same "STOP: " prefix and trailing newline as logmsg, so smoke's ^STOP:
    // check also sees STOP(("... %s", str)), which resolves to this overload.
    fputs("STOP: ", stderr);
    fprintf(stderr, Message, value);
    fputc('\n', stderr);
    stop_exit_if_requested();
    return enter_debugger;
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
