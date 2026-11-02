#include "gos_logfile.h"

#include "utils/log_file.h"

#include <SDL2/SDL.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <string>

#ifdef PLATFORM_WINDOWS
#include <direct.h>
#include <fcntl.h>
#include <io.h>
#include <sys/stat.h>
#include <windows.h>
#else
#include <dlfcn.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <unwind.h>
#endif

#if defined(__SANITIZE_ADDRESS__)
#define MC2_LOG_ASAN 1
#elif defined(__has_feature)
#if __has_feature(address_sanitizer)
#define MC2_LOG_ASAN 1
#endif
#endif

void gos_SetLogFlush(void (*flush)());    // gameos_debugging.cpp

#ifndef MC2_VERSION
#define MC2_VERSION "dev"
#endif

// where a crash writes: stderr's descriptor
static int g_error_fd = 2;

#ifdef PLATFORM_WINDOWS
static int fd_dup(int fd) { return _dup(fd); }
static int fd_dup2(int from, int to) { return _dup2(from, to); }
static int fd_read(int fd, char* buf, unsigned n) { return _read(fd, buf, n); }
static int fd_write(int fd, const char* buf, unsigned n) { return _write(fd, buf, n); }
static int fd_close(int fd) { return _close(fd); }
static void make_folder(const std::string& path) { _mkdir(path.c_str()); }
// binary: fd 1 and 2 become this file, and text mode would double the \r of
// the copy that goes on to the console
static int open_for_append(const char* path) { return _open(path, _O_WRONLY | _O_CREAT | _O_APPEND | _O_BINARY, _S_IREAD | _S_IWRITE); }
static int open_for_read(const char* path) { return _open(path, _O_RDONLY | _O_BINARY); }
static int fd_of(FILE* stream) { return _fileno(stream); }
// A process started with no console has no descriptor behind these, and
// _dup2 would not attach one: give them one to replace.
static void give_descriptor(FILE* stream) { if(_fileno(stream) < 0) freopen("NUL", "w", stream); }
#else
static int fd_dup(int fd) { return dup(fd); }
static int fd_dup2(int from, int to) { return dup2(from, to); }
static int fd_read(int fd, char* buf, unsigned n) { return (int)read(fd, buf, n); }
static int fd_write(int fd, const char* buf, unsigned n) { return (int)write(fd, buf, n); }
static int fd_close(int fd) { return close(fd); }
static void make_folder(const std::string& path) { mkdir(path.c_str(), 0755); }
static int open_for_append(const char* path) { return open(path, O_WRONLY | O_CREAT | O_APPEND, 0644); }
static int open_for_read(const char* path) { return open(path, O_RDONLY); }
static int fd_of(FILE* stream) { return fileno(stream); }
static void give_descriptor(FILE*) {}
#endif

// The file is what the game writes to, so everything up to a crash is in it
// with nothing left to flush but stdio. A thread copies it on to the terminal,
// and only that thread writes there: a reader that has stopped (a paused pager,
// a console selection) or gone can then not hold up or kill the game.
static int g_terminal_fd = -1;
static int g_copy_from_fd = -1;
static const int COPY_PERIOD_MS = 100;
static bool g_copying = false;

static int SDLCALL copy_thread(void*)
{
    char buf[4096];
    for(;;) {
        int n;
        while((n = fd_read(g_copy_from_fd, buf, sizeof(buf))) > 0)
            if(fd_write(g_terminal_fd, buf, (unsigned)n) < 0)
                break;
        SDL_Delay(COPY_PERIOD_MS);
    }
}

// for the end of the process: one more pass of the thread for the last lines,
// waited for only this long in case its reader has stopped
static void flush_log()
{
    fflush(stdout);
    fflush(stderr);
    if(g_copying)
        SDL_Delay(COPY_PERIOD_MS * 3 / 2);
}

static std::string log_folder()
{
    const char* xdg = getenv("XDG_CONFIG_HOME");
    const char* home = getenv("HOME");
    const std::string folder = log_file::folder(xdg, home);
    if(!folder.empty()) {
        if(!(xdg && xdg[0]))
            make_folder(std::string(home) + "/.config");
        else
            make_folder(xdg);
        make_folder(folder);
    }
    return folder;
}

static void keep_previous(const std::string& path, const std::string& previous)
{
    remove(previous.c_str());
    rename(path.c_str(), previous.c_str());
}

void gos_LogFileKeepPrevious()
{
    keep_previous(log_file::logPath(""), log_file::previousPath(""));
}

// A copy of fd with a number other than 0, 1 or 2, or -1. With stdout or stderr
// closed, a new descriptor takes its number, and the steps below that replace 1
// and 2 would close it, or leave the log being copied into itself.
static int above_standard(int fd)
{
    while(fd >= 0 && fd <= 2)
        fd = fd_dup(fd);
    return fd;
}

void gos_LogFileStart()
{
    const char* env = getenv("MC2_LOG_FILE");
    if(env && env[0] == '0')
        return;

    const std::string folder = log_folder();
    const std::string path = log_file::logPath(folder);
    keep_previous(path, log_file::previousPath(folder));

    // before the file is opened, which could take a closed stdout's number
    fflush(stdout);
    fflush(stderr);
    give_descriptor(stdout);
    give_descriptor(stderr);
    g_terminal_fd = above_standard(fd_dup(fd_of(stdout)));

    const int file = above_standard(open_for_append(path.c_str()));
    if(file < 0) {
        fprintf(stderr, "[LOG] cannot write %s: output only goes to the terminal\n", path.c_str());
        return;
    }
    fd_dup2(file, fd_of(stdout));
    fd_dup2(file, fd_of(stderr));
    fd_close(file);
    g_error_fd = fd_of(stderr);
    // a line at a time, so a crash loses at most part of one. MSVC has no line
    // buffering: its _IOLBF is full buffering.
#ifdef PLATFORM_WINDOWS
    setvbuf(stdout, nullptr, _IONBF, 0);
#else
    setvbuf(stdout, nullptr, _IOLBF, 0);
#endif

    g_copy_from_fd = open_for_read(path.c_str());
    if(g_terminal_fd >= 0 && g_copy_from_fd >= 0) {
        SDL_Thread* thread = SDL_CreateThread(copy_thread, "mc2log", nullptr);
        g_copying = thread != nullptr;
        if(thread)
            SDL_DetachThread(thread);
        else
            fprintf(stderr, "[LOG] no copy to the terminal: %s\n", SDL_GetError());
    }
    gos_SetLogFlush(flush_log);
    atexit(flush_log);
    printf("[LOG] writing %s\n", path.c_str());
}

void gos_LogFileHeader()
{
    char when[64] = "";
    const time_t now = time(nullptr);
    const struct tm* local = localtime(&now);
    if(local)
        strftime(when, sizeof(when), "%Y-%m-%d %H:%M:%S", local);
    printf("[LOG] MechCommander 2 %s, started %s\n", MC2_VERSION, when);
}

#ifdef PLATFORM_WINDOWS

static const int MAX_CRASH_FRAMES = 32;

static LONG WINAPI on_exception(EXCEPTION_POINTERS* info)
{
    char line[512];
    fflush(stdout);
    fflush(stderr);
    size_t len = log_file::formatCrash(line, sizeof(line), "exception", info->ExceptionRecord->ExceptionCode,
            (uint64_t)(uintptr_t)info->ExceptionRecord->ExceptionAddress);
    fd_write(g_error_fd, line, (unsigned)len);

    void* frames[MAX_CRASH_FRAMES];
    const USHORT count = CaptureStackBackTrace(0, MAX_CRASH_FRAMES, frames, nullptr);
    for(USHORT i = 0; i < count; ++i) {
        HMODULE module = nullptr;
        char path[MAX_PATH] = "";
        if(GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                (LPCSTR)frames[i], &module))
            GetModuleFileNameA(module, path, sizeof(path));
        len = log_file::formatFrame(line, sizeof(line), i, path[0] ? path : nullptr,
                (uint64_t)((uintptr_t)frames[i] - (uintptr_t)module));
        fd_write(g_error_fd, line, (unsigned)len);
    }
    Sleep(200);     // the copy thread's turn, for the terminal
    return EXCEPTION_CONTINUE_SEARCH;
}

void gos_LogFileCatchCrashes()
{
    SetUnhandledExceptionFilter(on_exception);
}

#elif defined(MC2_LOG_ASAN)

// AddressSanitizer's own report is the better one: leave its handlers in place
void gos_LogFileCatchCrashes() {}

#else

static const int MAX_CRASH_FRAMES = 32;
static const int CRASH_SIGNALS[] = { SIGSEGV, SIGBUS, SIGFPE, SIGILL, SIGABRT };
static const int NUM_CRASH_SIGNALS = sizeof(CRASH_SIGNALS) / sizeof(CRASH_SIGNALS[0]);
static struct sigaction g_old_actions[NUM_CRASH_SIGNALS];

static _Unwind_Reason_Code on_frame(struct _Unwind_Context* context, void* arg)
{
    int* index = (int*)arg;
    const uintptr_t ip = _Unwind_GetIP(context);
    if(*index >= MAX_CRASH_FRAMES || !ip)
        return _URC_END_OF_STACK;
    Dl_info info = {};
    const bool found = dladdr((void*)ip, &info) != 0 && info.dli_fname;
    char line[512];
    const size_t len = log_file::formatFrame(line, sizeof(line), *index, found ? info.dli_fname : nullptr,
            found ? ip - (uintptr_t)info.dli_fbase : ip);
    fd_write(g_error_fd, line, (unsigned)len);
    ++*index;
    return _URC_NO_REASON;
}

static void on_signal(int sig, siginfo_t* info, void*)
{
    // not fflush, which can deadlock here: stdout is line buffered, so all that
    // waits is part of a line
    char line[128];
    const size_t len = log_file::formatCrash(line, sizeof(line), "signal", (uint32_t)sig,
            (uint64_t)(uintptr_t)info->si_addr);
    fd_write(g_error_fd, line, (unsigned)len);
    int index = 0;
    _Unwind_Backtrace(on_frame, &index);

    const struct timespec wait = { 0, 200 * 1000 * 1000 };   // the copy thread's turn, for the terminal
    nanosleep(&wait, nullptr);

    // back to what was there (Android's crash reporter, or the default). A
    // fault happens again on return; a signal someone sent does not.
    for(int i = 0; i < NUM_CRASH_SIGNALS; ++i)
        if(CRASH_SIGNALS[i] == sig)
            sigaction(sig, &g_old_actions[i], nullptr);
    if(info->si_code <= 0)
        raise(sig);
}

void gos_LogFileCatchCrashes()
{
    // a stack of its own, or a stack overflow could not run the handler
    static char stack[64 * 1024];
    stack_t alt = {};
    alt.ss_sp = stack;
    alt.ss_size = sizeof(stack);
    sigaltstack(&alt, nullptr);

    // once now: the first call can load libgcc, which is no job for a handler
    int ignored = MAX_CRASH_FRAMES;
    _Unwind_Backtrace(on_frame, &ignored);

    struct sigaction action = {};
    action.sa_sigaction = on_signal;
    action.sa_flags = SA_SIGINFO | SA_ONSTACK;
    sigemptyset(&action.sa_mask);
    for(int i = 0; i < NUM_CRASH_SIGNALS; ++i)
        sigaction(CRASH_SIGNALS[i], &action, &g_old_actions[i]);
}

#endif
