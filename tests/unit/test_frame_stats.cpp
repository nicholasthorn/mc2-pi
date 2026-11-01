// tests/unit/test_frame_stats.cpp
//
// frame_stats: per-frame samples recorded by "-bench" (gameosmain.cpp), the
// percentile summary printed as the [BENCH] line, and the CSV rows that
// scripts/bench.sh keeps. The summary is what runs are compared on, so the
// percentile rule and the averages are pinned here.

#include "doctest.h"

#include "utils/frame_stats.h"

#include <string>
#include <vector>

using frame_stats::FrameSample;

static FrameSample sample(double total_ms, uint32_t draws = 0, uint32_t tris = 0)
{
	FrameSample s;
	s.total_ms = total_ms;
	s.logic_ms = total_ms * 0.25;
	s.render_ms = total_ms * 0.5;
	s.swap_ms = total_ms * 0.25;
	s.draws = draws;
	s.tris = tris;
	return s;
}

TEST_SUITE("FrameStats")
{

TEST_CASE("percentile uses the nearest-rank rule")
{
	std::vector<double> v;
	for (int i = 1; i <= 100; ++i)
		v.push_back((double)i);

	CHECK(frame_stats::percentile(v, 50.0) == doctest::Approx(50.0));
	CHECK(frame_stats::percentile(v, 95.0) == doctest::Approx(95.0));
	CHECK(frame_stats::percentile(v, 99.0) == doctest::Approx(99.0));
	CHECK(frame_stats::percentile(v, 100.0) == doctest::Approx(100.0));
	CHECK(frame_stats::percentile(v, 0.0) == doctest::Approx(1.0));
}

TEST_CASE("percentile does not need sorted input and rounds the rank up")
{
	const std::vector<double> v = { 40.0, 10.0, 30.0, 20.0 };
	// rank = ceil(0.5 * 4) = 2 -> 20; ceil(0.9 * 4) = 4 -> 40
	CHECK(frame_stats::percentile(v, 50.0) == doctest::Approx(20.0));
	CHECK(frame_stats::percentile(v, 90.0) == doctest::Approx(40.0));
}

TEST_CASE("percentile of nothing is zero")
{
	CHECK(frame_stats::percentile(std::vector<double>(), 50.0) == 0.0);
}

TEST_CASE("summary averages every column and finds the worst frame")
{
	std::vector<FrameSample> frames;
	frames.push_back(sample(10.0, 100, 1000));
	frames.push_back(sample(20.0, 200, 2000));
	frames.push_back(sample(30.0, 300, 3000));
	frames.push_back(sample(40.0, 400, 4000));

	const frame_stats::Summary s = frame_stats::summarize(frames);
	CHECK(s.frames == 4);
	CHECK(s.avg_ms == doctest::Approx(25.0));
	CHECK(s.fps == doctest::Approx(40.0));      // 4 frames in 0.1 s
	CHECK(s.p50_ms == doctest::Approx(20.0));
	CHECK(s.p95_ms == doctest::Approx(40.0));
	CHECK(s.max_ms == doctest::Approx(40.0));
	CHECK(s.logic_ms == doctest::Approx(6.25));
	CHECK(s.render_ms == doctest::Approx(12.5));
	CHECK(s.swap_ms == doctest::Approx(6.25));
	CHECK(s.draws == doctest::Approx(250.0));
	CHECK(s.tris == doctest::Approx(2500.0));
}

TEST_CASE("one slow frame in 100 shows in max, not in p99, barely in the average")
{
	std::vector<FrameSample> frames(99, sample(10.0));
	frames.push_back(sample(110.0));

	const frame_stats::Summary s = frame_stats::summarize(frames);
	CHECK(s.avg_ms == doctest::Approx(11.0));
	CHECK(s.p50_ms == doctest::Approx(10.0));
	CHECK(s.p99_ms == doctest::Approx(10.0));   // rank 99 of 100
	CHECK(s.max_ms == doctest::Approx(110.0));
}

TEST_CASE("summary of no frames is all zeros")
{
	const frame_stats::Summary s = frame_stats::summarize(std::vector<FrameSample>());
	CHECK(s.frames == 0);
	CHECK(s.avg_ms == 0.0);
	CHECK(s.fps == 0.0);
	CHECK(s.max_ms == 0.0);
}

TEST_CASE("CSV header and row have the same columns")
{
	const std::string header = frame_stats::csvHeader();
	const std::string row = frame_stats::csvRow(7, sample(16.0, 12, 3456));

	CHECK(header == "frame,total_ms,logic_ms,render_ms,swap_ms,draws,tris,segment,state");
	CHECK(row == "7,16.000,4.000,8.000,4.000,12,3456,0,00000000");

	FrameSample s = sample(16.0, 12, 3456);
	s.segment = 3;
	s.state = 0xBEEF;
	CHECK(frame_stats::csvRow(7, s) == "7,16.000,4.000,8.000,4.000,12,3456,3,0000beef");
}

TEST_CASE("summary line is one parseable [BENCH] line")
{
	std::vector<FrameSample> frames;
	frames.push_back(sample(10.0, 100, 1000));
	frames.push_back(sample(30.0, 300, 3000));

	const std::string line = frame_stats::summaryLine("all", frame_stats::summarize(frames));
	CHECK(line == "[BENCH] segment=all frames=2 fps=50.0 avg_ms=20.00 p50_ms=10.00"
	              " p95_ms=30.00 p99_ms=30.00 max_ms=30.00 logic_ms=5.00"
	              " render_ms=10.00 swap_ms=5.00 draws=200 tris=2000");
}

TEST_CASE("stage slots: one per name, in first-use order, until full")
{
	std::vector<std::string> names;
	CHECK(frame_stats::stageSlot(names, "camera") == 0);
	CHECK(frame_stats::stageSlot(names, "terrain_geom") == 1);
	CHECK(frame_stats::stageSlot(names, "camera") == 0);
	CHECK(names.size() == 2);

	CHECK(frame_stats::stageSlot(names, nullptr) == -1);
	CHECK(frame_stats::stageSlot(names, "") == -1);

	for (size_t i = names.size(); i < frame_stats::kMaxStages; ++i)
		CHECK(frame_stats::stageSlot(names, ("s" + std::to_string(i)).c_str()) == (int)i);
	CHECK(frame_stats::stageSlot(names, "one_too_many") == -1);
	CHECK(names.size() == frame_stats::kMaxStages);
}

TEST_CASE("stages are averaged and become extra columns and keys")
{
	const std::vector<std::string> stages = { "camera", "terrain_geom" };
	std::vector<FrameSample> frames;
	frames.push_back(sample(10.0, 100, 1000));
	frames.push_back(sample(30.0, 300, 3000));
	frames[0].stage_ms[0] = 1.0;
	frames[0].stage_ms[1] = 2.5;
	frames[1].stage_ms[0] = 3.0;
	frames[1].stage_ms[1] = 0.5;

	const frame_stats::Summary s = frame_stats::summarize(frames);
	CHECK(s.stage_ms[0] == doctest::Approx(2.0));
	CHECK(s.stage_ms[1] == doctest::Approx(1.5));

	CHECK(frame_stats::csvHeader(stages) ==
			"frame,total_ms,logic_ms,render_ms,swap_ms,draws,tris,segment,state,camera_ms,terrain_geom_ms");
	CHECK(frame_stats::csvRow(7, frames[0], stages.size()) ==
			"7,10.000,2.500,5.000,2.500,100,1000,0,00000000,1.000,2.500");
	CHECK(frame_stats::summaryLine("all", s, stages) ==
			"[BENCH] segment=all frames=2 fps=50.0 avg_ms=20.00 p50_ms=10.00"
			" p95_ms=30.00 p99_ms=30.00 max_ms=30.00 logic_ms=5.00"
			" render_ms=10.00 swap_ms=5.00 draws=200 tris=2000"
			" camera_ms=2.00 terrain_geom_ms=1.50");
}

}
