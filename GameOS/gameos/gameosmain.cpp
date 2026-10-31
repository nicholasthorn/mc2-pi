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

#include <signal.h>
#include <vector>

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

float __stdcall gos_GetFixedFrameLength()
{
    return g_bench ? BENCH_FRAME_LENGTH : 0.0f;
}

// MC2_PERF_CTL / MC2_PERF_ACK: paths of the control and ack FIFOs given to
// "perf record --control fd:... --delay=-1" (scripts/bench.sh --perf).
// Sampling is switched on at the first gameplay frame and off when gameplay
// ends, so loading never lands in the profile. Unset: nothing happens.
static FILE* g_perf_ctl = NULL;
static FILE* g_perf_ack = NULL;
static bool g_perf_enabled = false;

static void perf_control(bool enable)
{
    if(!g_perf_ctl || g_perf_enabled == enable)
        return;
    fprintf(g_perf_ctl, "%s\n", enable ? "enable" : "disable");
    fflush(g_perf_ctl);
    // wait for perf to act on it, so the first frames are not missed
    char ack[16];
    if(g_perf_ack && !fgets(ack, sizeof(ack), g_perf_ack))
        printf("[PERF] no ack from perf\n");
    g_perf_enabled = enable;
    printf("[PERF] sampling %s\n", enable ? "on" : "off");
    fflush(stdout);
}

static void perf_open_control()
{
    const char* ctl = getenv("MC2_PERF_CTL");
    if(!ctl || !*ctl)
        return;
    // the launcher holds both FIFOs open read-write, so neither open blocks
    g_perf_ctl = fopen(ctl, "w");
    const char* ack = getenv("MC2_PERF_ACK");
    if(g_perf_ctl && ack && *ack)
        g_perf_ack = fopen(ack, "r");
    if(!g_perf_ctl)
        printf("[PERF] ERROR: cannot open %s\n", ctl);
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

        // -bench: a stray key or the cursor at a screen edge (camera scroll)
        // would change what is drawn; only quit and window events get through
        if(g_bench) {
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
    for(int i=1;i<argc-1;++i) {
        if(0 == strcmp(argv[i], "-autoexit"))
            autoexit_frames = atol(argv[i+1]);
        if(0 == strcmp(argv[i], "-bench"))
            bench_csv = argv[i+1];
    }
    g_bench = bench_csv != NULL;
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
		uint64_t swap_tick = timing::gettickcount();
        graphics::swap_window(win);
		uint64_t swapped_tick = timing::gettickcount();

        g_exit |= gosExitGameOS();

		uint64_t end_tick = timing::gettickcount();
		double dt_sec = timing::ticks2sec(end_tick - start_tick);
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
                s.render_ms = timing::ticks2ms(swap_tick - render_tick);
                s.swap_ms = timing::ticks2ms(swapped_tick - swap_tick);
                s.draws = draws;
                s.tris = tris;
                bench_frames.push_back(s);
            }
        }
        if(autoexit_frames > 0 && frame_count >= autoexit_frames) {
            // scripts/smoke.sh treats a missing [AUTOEXIT] line as a failure
            printf("[AUTOEXIT] %ld gameplay frames in %.1f s (%.1f fps)\n", frame_count,
                    total_sec, total_sec > 0.0 ? frame_count / total_sec : 0.0);
            fflush(stdout);
            g_exit = true;
        }
    }

    // stop sampling before the CSV write and shutdown
    perf_control(false);

    // written after the loop so file IO never lands inside a measured frame
    if(g_bench) {
        printf("%s\n", frame_stats::summaryLine("all", frame_stats::summarize(bench_frames)).c_str());
        FILE* f = fopen(bench_csv, "w");
        if(f) {
            fprintf(f, "%s\n", frame_stats::csvHeader().c_str());
            for(size_t i = 0; i < bench_frames.size(); ++i)
                fprintf(f, "%s\n", frame_stats::csvRow((long)i, bench_frames[i]).c_str());
            fclose(f);
        } else {
            printf("[BENCH] ERROR: cannot write %s\n", bench_csv);
        }
        fflush(stdout);
    }

    Environment.TerminateGameEngine();

    return 0;
}
#endif // DISABLE_GAMEOS_MAIN
