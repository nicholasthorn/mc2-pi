#ifndef FRAME_STATS_H
#define FRAME_STATS_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

// Per-frame timing for "-bench" (gameosmain.cpp): the samples, the summary
// printed as the [BENCH] line and the CSV that scripts/bench.sh keeps.
// GL- and SDL-free so it can be unit tested.
namespace frame_stats {

struct FrameSample {
	double total_ms = 0.0;   // whole main-loop iteration
	double logic_ms = 0.0;   // Environment.DoGameLogic(), plus the main loop's
	                         // gos_RendererHandleEvents(), gos_UpdateAudio()
	                         // and make_current_context() after it
	double render_ms = 0.0;  // draw_screen(): game render enqueue + GL submit
	double swap_ms = 0.0;    // swap_window(): where a GPU-bound frame waits
	uint32_t draws = 0;      // GL draw calls issued this frame
	uint32_t tris = 0;       // triangles in those draw calls
	uint32_t segment = 0;    // scripted-scenario segment (gos_BenchSegment)
	uint32_t state = 0;      // game-state checksum (gos_BenchStateHash), 0 = none
};

struct Summary {
	size_t frames = 0;
	double fps = 0.0;        // frames / sum of total_ms
	double avg_ms = 0.0;
	double p50_ms = 0.0;
	double p95_ms = 0.0;
	double p99_ms = 0.0;
	double max_ms = 0.0;
	double logic_ms = 0.0;   // averages of the split
	double render_ms = 0.0;
	double swap_ms = 0.0;
	double draws = 0.0;      // average per frame
	double tris = 0.0;
};

// Nearest-rank percentile (p in 0..100) of unsorted values; 0 if empty.
double percentile(std::vector<double> values, double p);

Summary summarize(const std::vector<FrameSample>& frames);

std::string csvHeader();
std::string csvRow(long frame, const FrameSample& s);

// "[BENCH] segment=<name> frames=... fps=... avg_ms=... ..." on one line.
std::string summaryLine(const char* segment, const Summary& s);

}

#endif // FRAME_STATS_H
