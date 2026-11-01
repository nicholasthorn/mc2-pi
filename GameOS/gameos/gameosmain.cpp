#include "gameos.hpp"
#include "gos_render.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <SDL2/SDL.h>
#include "gos_input.h"

#include "utils/camera.h"
#include "utils/shader_builder.h"
#include "utils/gl_utils.h"
#include "utils/timing.h"
#include "utils/frame_stats.h"
#include "utils/tga_writer.h"

#include <signal.h>
#include <string>
#include <vector>

#ifndef PLATFORM_WINDOWS
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#endif

extern graphics::RenderContextHandle gos_GetRenderContext();
extern void gos_DestroyRenderer();
extern void gos_RendererBeginFrame();
extern void gos_RendererEndFrame();
extern void gos_RendererHandleEvents();
extern void gos_RenderUpdateDebugInput();
extern void gos_RenderEnableDebugDrawCalls();
extern bool gos_RenderGetEnableDebugDrawCalls();
extern void gos_TakeRenderStats(uint32_t* draws, uint32_t* tris);

extern bool gosExitGameOS();

extern bool gos_CreateAudio();
extern void gos_UpdateAudio();
extern void gos_DestroyAudio();

static bool g_exit = false;
static bool g_focus_lost = false;
static bool g_gameplay_started = false;

// -bench <csv>: fixed game step, fixed seed, no player input, per-frame
// timings written to <csv> at exit (scripts/bench.sh)
static const float BENCH_FRAME_LENGTH = 1.0f / 30.0f;
static const unsigned int BENCH_SEED = 1;
static bool g_bench = false;

void gos_SetFixedFrameLength(float seconds);	// gameos.cpp
void gos_SetAutomatedRun(bool automated);	// gameos.cpp
// gameos.cpp: -bench named stages (gos_BenchStageId) and -benchfov
void gos_SetBenchStages(bool on, float fov);
const std::vector<std::string>& gos_GetBenchStageNames();
void gos_TakeBenchStageTimes(double* out);

// -benchscenario <fit>: the scripted scenario code/benchdirector.cpp plays.
// It names segments (timed separately) and asks for screenshots.
static const char* g_bench_csv = NULL;
static const char* g_bench_scenario = NULL;
static std::vector<std::string> g_bench_segments;   // index = FrameSample::segment
static std::string g_bench_shot;                    // screenshot for this frame

// -uismoke <walk>: the front-end screens code/uismokedriver.cpp visits
static const char* g_uismoke_walk = NULL;

const char* __stdcall gos_GetUiSmokeWalk()
{
    return g_uismoke_walk;
}

// main()'s exit status after a clean shutdown (a failed -uismoke walk)
static int g_exit_code = 0;

void __stdcall gos_SetExitCode(int code)
{
    g_exit_code = code;
}

const char* __stdcall gos_GetBenchScenario()
{
    return g_bench ? g_bench_scenario : NULL;
}

void __stdcall gos_BenchSegment(const char* name)
{
    if(g_bench && name)
        g_bench_segments.push_back(name);
}

void __stdcall gos_BenchScreenshot(const char* name)
{
    if(g_bench && name)
        g_bench_shot = name;
}

static uint32_t g_bench_state = 0;      // this frame's gos_BenchStateHash

void __stdcall gos_BenchStateHash(unsigned int hash)
{
    g_bench_state = hash;
}

// <csv without .csv>_<name>.tga, so shots sit next to the run's frame CSV
static std::string bench_output_path(const char* name, const char* ext)
{
    std::string base = g_bench_csv;
    const size_t dot = base.rfind(".csv");
    if(dot != std::string::npos && dot + 4 == base.size())
        base.resize(dot);
    return base + "_" + name + ext;
}

// Reads back the frame just drawn (before swap) and writes it as a TGA.
static void bench_capture(const std::string& name)
{
    // The game's screen is scaled to fill the whole drawable (draw_screen's
    // viewport), which can be bigger than the game resolution, so read all of it.
    const int w = Environment.drawableWidth;
    const int h = Environment.drawableHeight;
    if(w <= 0 || h <= 0)
        return;
    std::vector<uint8_t> px((size_t)w * h * 4);
    GLint pack = 4;
    glGetIntegerv(GL_PACK_ALIGNMENT, &pack);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, px.data());
    glPixelStorei(GL_PACK_ALIGNMENT, pack);
    const std::string path = bench_output_path(name.c_str(), ".tga");
    if(writeTGA24(path.c_str(), w, h, px.data()))
        printf("[BENCH] screenshot %s\n", path.c_str());
    else
        printf("[BENCH] ERROR: cannot write %s\n", path.c_str());
    fflush(stdout);
}

// MC2_PERF_CTL / MC2_PERF_ACK: paths of the control and ack FIFOs given to
// "perf record --control fd:... --delay=-1" (scripts/bench.sh --perf).
// Sampling is switched on at the first gameplay frame and off when gameplay
// ends, so loading never lands in the profile. Unset: nothing happens.
// perf is Linux only; on Windows both functions do nothing.
#ifndef PLATFORM_WINDOWS
static int g_perf_ctl = -1;
static int g_perf_ack = -1;
static bool g_perf_enabled = false;
#endif

// Peak resident memory (Linux VmHWM) in MB, or -1 where unknown: printed at
// autoexit, the number that matters on a 4 GB Pi.
static long peak_resident_mb()
{
#ifdef __linux__
    FILE* f = fopen("/proc/self/status", "r");
    if(!f)
        return -1;
    char line[256];
    long kb = -1;
    while(fgets(line, sizeof(line), f))
        if(sscanf(line, "VmHWM: %ld kB", &kb) == 1)
            break;
    fclose(f);
    return kb < 0 ? -1 : kb / 1024;
#else
    return -1;
#endif
}

static void perf_control(bool enable)
{
#ifndef PLATFORM_WINDOWS
    if(g_perf_ctl < 0 || g_perf_enabled == enable)
        return;
    // drop acks left from an earlier command that timed out
    char buf[64];
    if(g_perf_ack >= 0)
        while(read(g_perf_ack, buf, sizeof(buf)) > 0) {}
    const char* cmd = enable ? "enable\n" : "disable\n";
    const ssize_t len = (ssize_t)strlen(cmd);
    if(write(g_perf_ctl, cmd, (size_t)len) != len) {
        printf("[PERF] ERROR: cannot write to the control FIFO (%s)\n", strerror(errno));
        fflush(stdout);
        return;
    }
    // wait for perf to act on it, so the first frames are not missed; but
    // not forever: a perf that never answers costs 2 s, not the whole run
    if(g_perf_ack >= 0) {
        struct pollfd p;
        p.fd = g_perf_ack;
        p.events = POLLIN;
        p.revents = 0;
        const bool acked = poll(&p, 1, 2000) > 0 && (p.revents & POLLIN) &&
                read(g_perf_ack, buf, sizeof(buf)) > 0;
        if(!acked)
            printf("[PERF] no ack from perf within 2 s\n");
    }
    g_perf_enabled = enable;
    printf("[PERF] sampling %s\n", enable ? "on" : "off");
    fflush(stdout);
#else
    (void)enable;
#endif
}

static void perf_open_control()
{
#ifndef PLATFORM_WINDOWS
    const char* ctl = getenv("MC2_PERF_CTL");
    if(!ctl || !*ctl)
        return;
    // Non-blocking opens: the launcher normally holds both FIFOs open
    // read-write, but without it a plain open would block forever. A control
    // FIFO with no reader fails (ENXIO) and perf control is switched off.
    g_perf_ctl = open(ctl, O_WRONLY | O_NONBLOCK);
    if(g_perf_ctl < 0) {
        printf("[PERF] ERROR: cannot open %s (%s); perf control off\n", ctl, strerror(errno));
        fflush(stdout);
        return;
    }
    const char* ack = getenv("MC2_PERF_ACK");
    if(ack && *ack) {
        g_perf_ack = open(ack, O_RDONLY | O_NONBLOCK);
        if(g_perf_ack < 0)
            printf("[PERF] cannot open %s (%s); not waiting for acks\n", ack, strerror(errno));
    }
#endif
}

void __stdcall gos_MarkGameplayStarted(const char* missionName)
{
    // scripts/smoke.sh requires this line before [AUTOEXIT]
    printf("[MISSION] %s started\n", missionName ? missionName : "");
    fflush(stdout);
    g_gameplay_started = true;
    perf_control(true);
    // same random sequence from the first gameplay frame, whatever loading used
    if(g_bench)
        gos_srand(BENCH_SEED);
}
#if 0
static camera g_camera;
#endif

static void handle_key_down( SDL_Keysym* keysym ) {
    switch( keysym->sym ) {
        case SDLK_ESCAPE:
            if(keysym->mod & KMOD_RALT)
                g_exit = true;
            break;
        case 'd':
            if(keysym->mod & KMOD_RALT)
                gos_RenderEnableDebugDrawCalls();
            break;
    }
}

static void set_mouse_capture(bool enabled)
{
    graphics::RenderWindowHandle win = gos_GetWindow();
    bool fullscreen = graphics::is_window_fullscreen(win);
    SDL_bool capture = (enabled /*&& fullscreen*/) ? SDL_TRUE : SDL_FALSE;
    graphics::grab_window(win, (bool)capture);

    SDL_CaptureMouse(capture);
    SDL_ShowCursor(enabled ? SDL_DISABLE : SDL_ENABLE);
}

static void process_events( void ) {

    input::beginUpdateMouseState();

    SDL_Event event;
    while( SDL_PollEvent( &event ) ) {

        if(g_focus_lost) {
            if(event.type != SDL_WINDOWEVENT || event.window.event != SDL_WINDOWEVENT_FOCUS_GAINED) {
                continue;
            } else {
                g_focus_lost = false;
            }
        }

        // -autoexit / -bench: a stray key or the cursor at a screen edge
        // (camera scroll) would change what is drawn; only quit and window
        // events get through
        if(gos_IsAutomatedRun()) {
            switch( event.type ) {
            case SDL_KEYDOWN: case SDL_KEYUP: case SDL_MOUSEMOTION:
            case SDL_MOUSEBUTTONDOWN: case SDL_MOUSEBUTTONUP: case SDL_MOUSEWHEEL:
                continue;
            }
        }

        switch( event.type ) {
        case SDL_KEYDOWN:
            handle_key_down( &event.key.keysym );
            // fallthrough
        case SDL_KEYUP:
            input::handleKeyEvent(&event);
            break;
        case SDL_QUIT:
            g_exit = true;
            break;
        case SDL_WINDOWEVENT:
            switch(event.window.event) {
                case SDL_WINDOWEVENT_RESIZED:
                case SDL_WINDOWEVENT_SIZE_CHANGED:
                {
                    float w = (float)event.window.data1;(void)w;
                    float h = (float)event.window.data2;(void)h;
                    SPEW(("INPUT", "resize event: w: %f h:%f\n", w, h));
                    break;
                }
                case SDL_WINDOWEVENT_FOCUS_LOST:
                    g_focus_lost = true;
                    set_mouse_capture(false);
                    break;
                case SDL_WINDOWEVENT_FOCUS_GAINED:
                    g_focus_lost = false;
                    set_mouse_capture(true);
                    break;
            }
            break;
        case SDL_MOUSEMOTION:
            input::handleMouseMotion(&event); 
            break;
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:
            //input::handleMouseButton(&event);
            break;
        case SDL_MOUSEWHEEL:
            input::handleMouseWheel(&event);
            break;
        }
    }

    // -autoexit / -bench: the polled SDL state would let the desktop's (or
    // the Pi's) keyboard and mouse in. Keys and buttons stay released and the
    // cursor sits at the centre (at 0, 0 the game would edge-scroll up and
    // left every frame).
    if(gos_IsAutomatedRun()) {
        input::pinMouse(0.5f * (float)Environment.drawableWidth, 0.5f * (float)Environment.drawableHeight);
        return;
    }

    input::updateMouseState();
    input::updateKeyboardState();
}

extern bool g_disable_quads;

static void draw_screen( void )
{
    g_disable_quads = false;
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    glCullFace(GL_FRONT);
    //CHECK_GL_ERROR;
    
	const int viewport_w = Environment.drawableWidth;
	const int viewport_h = Environment.drawableHeight;
    glViewport(0, 0, viewport_w, viewport_h);

    CHECK_GL_ERROR;

    // TODO: reset all states to sane defaults!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    glDepthMask(GL_TRUE);
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    gos_RendererBeginFrame();
    Environment.UpdateRenderers();
    gos_RendererEndFrame();

    glUseProgram(0);
    //CHECK_GL_ERROR;
}

extern float frameRate;


#ifndef DISABLE_GAMEOS_MAIN
int main(int argc, char** argv)
{
    //signal(SIGTRAP, SIG_IGN);

    // gather command line
	size_t cmdline_len = 0;
    for(int i=0;i<argc;++i) {
        cmdline_len += strlen(argv[i]);
        cmdline_len += 1; // ' '
    }
    char* cmdline = new char[cmdline_len + 1];
    size_t offset = 0;
    for(int i=0;i<argc;++i) {
        size_t arglen = strlen(argv[i]);
        memcpy(cmdline + offset, argv[i], arglen);
        cmdline[offset + arglen] = ' ';
        offset += arglen + 1;
    }
    cmdline[cmdline_len] = '\0';

    // -autoexit <frames>: quit cleanly after that many frames of gameplay,
    // counted from gos_MarkGameplayStarted() (smoke runs, scripts/smoke.sh).
    // The game's own command-line parser ignores it.
    long autoexit_frames = 0;
    const char* bench_csv = NULL;
    float bench_fov = 0.0f;
    for(int i=1;i<argc-1;++i) {
        if(0 == strcmp(argv[i], "-autoexit"))
            autoexit_frames = atol(argv[i+1]);
        if(0 == strcmp(argv[i], "-bench"))
            bench_csv = argv[i+1];
        if(0 == strcmp(argv[i], "-benchscenario"))
            g_bench_scenario = argv[i+1];
        if(0 == strcmp(argv[i], "-benchfov"))
            bench_fov = (float)atof(argv[i+1]);
        if(0 == strcmp(argv[i], "-uismoke"))
            g_uismoke_walk = argv[i+1];
    }
    g_bench = bench_csv != NULL;
    gos_SetFixedFrameLength(g_bench ? BENCH_FRAME_LENGTH : 0.0f);
    gos_SetAutomatedRun(g_bench || autoexit_frames > 0 || g_uismoke_walk != NULL);
    gos_SetBenchStages(g_bench, bench_fov);
    g_bench_csv = bench_csv;
    perf_open_control();
    std::vector<frame_stats::FrameSample> bench_frames;
    if(g_bench) {
        bench_frames.reserve(autoexit_frames > 0 ? autoexit_frames : 10000);
        // the game skips its timeGetTime() seed under -bench
        gos_srand(BENCH_SEED);
    }

    // fills in Environment structure
    GetGameOSEnvironment(cmdline);

    delete[] cmdline;
    cmdline = NULL;

    Environment.InitializeGameEngine();

	timing::init();

    graphics::RenderWindowHandle win = gos_GetWindow();
    graphics::RenderContextHandle ctx = gos_GetRenderContext();

    long frame_count = 0;
    double total_sec = 0.0;

    while( !g_exit ) {

		uint64_t start_tick = timing::gettickcount();

        process_events();

		uint64_t logic_tick = timing::gettickcount();
        if(gos_RenderGetEnableDebugDrawCalls()) {
            gos_RenderUpdateDebugInput();
        } else {
            Environment.DoGameLogic();
        }

        win = gos_GetWindow();
        ctx = gos_GetRenderContext();

		gos_RendererHandleEvents();
        gos_UpdateAudio();

        graphics::make_current_context(ctx);
		uint64_t render_tick = timing::gettickcount();
        draw_screen();
		uint64_t capture_ticks = 0;
        if(!g_bench_shot.empty()) {
            // not part of the frame: its time is taken off the timings below.
            // Finish the frame's GPU work first, or glReadPixels waits for it
            // inside the capture span and the shot frame looks too fast.
            glFinish();
            uint64_t capture_tick = timing::gettickcount();
            bench_capture(g_bench_shot);
            g_bench_shot.clear();
            capture_ticks = timing::gettickcount() - capture_tick;
        }
		uint64_t swap_tick = timing::gettickcount();
        graphics::swap_window(win);
		uint64_t swapped_tick = timing::gettickcount();

        g_exit |= gosExitGameOS();

		uint64_t end_tick = timing::gettickcount();
		double dt_sec = timing::ticks2sec(end_tick - start_tick - capture_ticks);
		frameRate = (float)(1.0 / dt_sec);

        uint32_t draws = 0, tris = 0;
        gos_TakeRenderStats(&draws, &tris);

        if(g_gameplay_started) {
            ++frame_count;
            total_sec += dt_sec;
            if(g_bench) {
                frame_stats::FrameSample s;
                s.total_ms = dt_sec * 1000.0;
                s.logic_ms = timing::ticks2ms(render_tick - logic_tick);
                s.render_ms = timing::ticks2ms(swap_tick - render_tick - capture_ticks);
                s.swap_ms = timing::ticks2ms(swapped_tick - swap_tick);
                s.draws = draws;
                s.tris = tris;
                s.segment = g_bench_segments.empty() ? 0 : (uint32_t)(g_bench_segments.size() - 1);
                s.state = g_bench_state;
                g_bench_state = 0;
                gos_TakeBenchStageTimes(s.stage_ms);
                bench_frames.push_back(s);
            }
        }
        double stage_ms[frame_stats::kMaxStages];
        gos_TakeBenchStageTimes(stage_ms);  // drop what no sample took
        if(autoexit_frames > 0 && frame_count >= autoexit_frames) {
            // scripts/smoke.sh treats a missing [AUTOEXIT] line as a failure
            printf("[AUTOEXIT] %ld gameplay frames in %.1f s (%.1f fps)\n", frame_count,
                    total_sec, total_sec > 0.0 ? frame_count / total_sec : 0.0);
            const long peak_mb = peak_resident_mb();
            if(peak_mb >= 0)
                printf("[MEM] peak resident %ld MB\n", peak_mb);
            fflush(stdout);
            g_exit = true;
        }
    }

    // stop sampling before the CSV write and shutdown
    perf_control(false);

    // written after the loop so file IO never lands inside a measured frame
    if(g_bench) {
        // one line per scenario segment, then the whole run
        for(size_t seg = 0; seg < g_bench_segments.size(); ++seg) {
            std::vector<frame_stats::FrameSample> part;
            for(size_t i = 0; i < bench_frames.size(); ++i)
                if(bench_frames[i].segment == seg)
                    part.push_back(bench_frames[i]);
            printf("%s\n", frame_stats::summaryLine(g_bench_segments[seg].c_str(),
                        frame_stats::summarize(part), gos_GetBenchStageNames()).c_str());
        }
        printf("%s\n", frame_stats::summaryLine("all", frame_stats::summarize(bench_frames),
                    gos_GetBenchStageNames()).c_str());
        FILE* f = fopen(bench_csv, "w");
        if(f) {
            fprintf(f, "%s\n", frame_stats::csvHeader(gos_GetBenchStageNames()).c_str());
            for(size_t i = 0; i < bench_frames.size(); ++i)
                fprintf(f, "%s\n", frame_stats::csvRow((long)i, bench_frames[i],
                            gos_GetBenchStageNames().size()).c_str());
            fclose(f);
        } else {
            printf("[BENCH] ERROR: cannot write %s\n", bench_csv);
        }
        fflush(stdout);
    }

    Environment.TerminateGameEngine();

    return g_exit_code;
}
#endif // DISABLE_GAMEOS_MAIN
