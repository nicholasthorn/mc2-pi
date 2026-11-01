#include "frame_stats.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace frame_stats {

// percentile() of values already sorted ascending
static double percentileSorted(const std::vector<double>& values, double p)
{
	if (values.empty())
		return 0.0;
	// nearest rank: the smallest value with at least p% of samples at or below it.
	// p * n first: (p / 100) * n rounds 99% of 100 up to rank 100.
	size_t rank = (size_t)std::ceil(p * (double)values.size() / 100.0);
	rank = std::min(std::max(rank, (size_t)1), values.size());
	return values[rank - 1];
}

double percentile(std::vector<double> values, double p)
{
	std::sort(values.begin(), values.end());
	return percentileSorted(values, p);
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
		for (size_t i = 0; i < kMaxStages; ++i)
			s.stage_ms[i] += f.stage_ms[i];
	}

	const double n = (double)frames.size();
	s.avg_ms = total / n;
	s.fps = total > 0.0 ? n * 1000.0 / total : 0.0;
	s.logic_ms /= n;
	s.render_ms /= n;
	s.swap_ms /= n;
	s.draws = draws / n;
	s.tris = tris / n;
	for (size_t i = 0; i < kMaxStages; ++i)
		s.stage_ms[i] /= n;
	std::sort(totals.begin(), totals.end());
	s.p50_ms = percentileSorted(totals, 50.0);
	s.p95_ms = percentileSorted(totals, 95.0);
	s.p99_ms = percentileSorted(totals, 99.0);
	s.max_ms = totals.back();
	return s;
}

std::string csvHeader(const std::vector<std::string>& stages)
{
	std::string header = "frame,total_ms,logic_ms,render_ms,swap_ms,draws,tris,segment,state";
	for (const std::string& name : stages)
		header += "," + name + "_ms";
	return header;
}

std::string csvRow(long frame, const FrameSample& s, const std::vector<std::string>& stages)
{
	char buf[176];
	snprintf(buf, sizeof(buf), "%ld,%.3f,%.3f,%.3f,%.3f,%u,%u,%u,%08x", frame,
			s.total_ms, s.logic_ms, s.render_ms, s.swap_ms,
			(unsigned)s.draws, (unsigned)s.tris, (unsigned)s.segment, (unsigned)s.state);
	std::string row = buf;
	for (size_t i = 0; i < stages.size() && i < kMaxStages; ++i) {
		snprintf(buf, sizeof(buf), ",%.3f", s.stage_ms[i]);
		row += buf;
	}
	return row;
}

std::string summaryLine(const char* segment, const Summary& s,
		const std::vector<std::string>& stages)
{
	char buf[384];
	snprintf(buf, sizeof(buf),
			"[BENCH] segment=%s frames=%zu fps=%.1f avg_ms=%.2f p50_ms=%.2f"
			" p95_ms=%.2f p99_ms=%.2f max_ms=%.2f logic_ms=%.2f"
			" render_ms=%.2f swap_ms=%.2f draws=%.0f tris=%.0f",
			segment, s.frames, s.fps, s.avg_ms, s.p50_ms,
			s.p95_ms, s.p99_ms, s.max_ms, s.logic_ms,
			s.render_ms, s.swap_ms, s.draws, s.tris);
	std::string line = buf;
	for (size_t i = 0; i < stages.size() && i < kMaxStages; ++i) {
		snprintf(buf, sizeof(buf), " %s_ms=%.2f", stages[i].c_str(), s.stage_ms[i]);
		line += buf;
	}
	return line;
}

}
