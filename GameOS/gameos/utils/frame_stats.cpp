#include "frame_stats.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace frame_stats {

double percentile(std::vector<double> values, double p)
{
	if (values.empty())
		return 0.0;
	std::sort(values.begin(), values.end());
	// nearest rank: the smallest value with at least p% of samples at or below it.
	// p * n first: (p / 100) * n rounds 99% of 100 up to rank 100.
	size_t rank = (size_t)std::ceil(p * (double)values.size() / 100.0);
	rank = std::min(std::max(rank, (size_t)1), values.size());
	return values[rank - 1];
}

Summary summarize(const std::vector<FrameSample>& frames)
{
	Summary s;
	s.frames = frames.size();
	if (frames.empty())
		return s;

	std::vector<double> totals;
	totals.reserve(frames.size());
	double total = 0.0;
	double draws = 0.0;
	double tris = 0.0;
	for (const FrameSample& f : frames) {
		totals.push_back(f.total_ms);
		total += f.total_ms;
		s.logic_ms += f.logic_ms;
		s.render_ms += f.render_ms;
		s.swap_ms += f.swap_ms;
		draws += f.draws;
		tris += f.tris;
	}

	const double n = (double)frames.size();
	s.avg_ms = total / n;
	s.fps = total > 0.0 ? n * 1000.0 / total : 0.0;
	s.logic_ms /= n;
	s.render_ms /= n;
	s.swap_ms /= n;
	s.draws = draws / n;
	s.tris = tris / n;
	s.p50_ms = percentile(totals, 50.0);
	s.p95_ms = percentile(totals, 95.0);
	s.p99_ms = percentile(totals, 99.0);
	s.max_ms = *std::max_element(totals.begin(), totals.end());
	return s;
}

std::string csvHeader()
{
	return "frame,total_ms,logic_ms,render_ms,swap_ms,draws,tris,segment,state";
}

std::string csvRow(long frame, const FrameSample& s)
{
	char buf[176];
	snprintf(buf, sizeof(buf), "%ld,%.3f,%.3f,%.3f,%.3f,%u,%u,%u,%08x", frame,
			s.total_ms, s.logic_ms, s.render_ms, s.swap_ms,
			(unsigned)s.draws, (unsigned)s.tris, (unsigned)s.segment, (unsigned)s.state);
	return buf;
}

std::string summaryLine(const char* segment, const Summary& s)
{
	char buf[384];
	snprintf(buf, sizeof(buf),
			"[BENCH] segment=%s frames=%zu fps=%.1f avg_ms=%.2f p50_ms=%.2f"
			" p95_ms=%.2f p99_ms=%.2f max_ms=%.2f logic_ms=%.2f"
			" render_ms=%.2f swap_ms=%.2f draws=%.0f tris=%.0f",
			segment, s.frames, s.fps, s.avg_ms, s.p50_ms,
			s.p95_ms, s.p99_ms, s.max_ms, s.logic_ms,
			s.render_ms, s.swap_ms, s.draws, s.tris);
	return buf;
}

}
