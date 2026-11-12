// tests/unit/test_fuzz_replay.cpp
//
// Replays the fuzz targets (fuzz/fuzz_targets.h) in every unit-test run: each
// seed in tests/unit/fuzz/corpus as it is, then a fixed set of mutations of
// it. The mutations lean on what broke the 2001 buffers: long runs of one
// character, long names, long lines, a string with no closing quote, header
// fields set to extremes. Under ASan + UBSan (CI) a read or write outside a
// buffer fails the run; elsewhere only a crash does. The libFuzzer builds
// (fuzz/README.md) search far wider; this keeps what they found fixed.
//
// MC2_FUZZ_REPLAY_MUTANTS sets the mutations per seed (default 150).
// MC2_FUZZ_REPLAY_LAST=<file> writes each input there before running it, so
// the one a crash leaves is the input that crashed (run it with mc2_fuzz_*).

#include "doctest.h"

#include "fuzz/fuzz_support.h"
#include "fuzz/fuzz_targets.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace
{

typedef int (*FuzzTarget)(const uint8_t*, size_t);

struct Seed
{
	std::string name;
	std::vector<uint8_t> bytes;
};

bool readFile(const std::string& path, std::vector<uint8_t>& out)
{
	FILE* f = fopen(path.c_str(), "rb");
	if (!f)
		return false;
	out.clear();
	uint8_t buffer[4096];
	size_t n;
	while ((n = fread(buffer, 1, sizeof(buffer), f)) > 0)
		out.insert(out.end(), buffer, buffer + n);
	fclose(f);
	return true;
}

// fuzz_corpus.txt (written by CMake): "<target> <path under fixtures/>" lines
std::vector<Seed> seedsFor(const char* target)
{
	std::vector<Seed> seeds;
	FILE* list = fopen("fuzz_corpus.txt", "r");
	if (!list)
		return seeds;
	char line[1024];
	const size_t targetLen = strlen(target);
	while (fgets(line, sizeof(line), list))
	{
		line[strcspn(line, "\r\n")] = '\0';
		if (strncmp(line, target, targetLen) != 0 || line[targetLen] != ' ')
			continue;
		Seed seed;
		seed.name = line + targetLen + 1;
		const bool opened = readFile(std::string(MC2_TEST_FIXTURE_DIR) + "/" + seed.name, seed.bytes);
		CHECK_MESSAGE(opened, "listed seed missing from the fixtures: ", seed.name);
		if (opened)
			seeds.push_back(seed);
	}
	fclose(list);
	return seeds;
}

// xorshift32: the same mutations on every machine and every run
struct Rng
{
	uint32_t state;
	uint32_t next()
	{
		state ^= state << 13;
		state ^= state >> 17;
		state ^= state << 5;
		return state;
	}
	uint32_t below(uint32_t n) { return n ? next() % n : 0; }
};

void mutate(std::vector<uint8_t>& b, Rng& rng, const std::vector<std::string>& tokens)
{
	static const uint32_t kRunLengths[] = { 20, 33, 49, 65, 129, 257, 300, 513, 1025, 2049, 4100 };
	static const char kRunChars[] = { 'A', 'z', '9', '"', '[', ']', '=', ' ', '\n', '\0', '\xff', '.' };
	const size_t at = b.empty() ? 0 : rng.below((uint32_t)b.size() + 1);
	switch (rng.below(8))
	{
	case 0: // one byte changed
		if (!b.empty())
			b[rng.below((uint32_t)b.size())] = (uint8_t)rng.next();
		break;
	case 1: // one byte inserted
		b.insert(b.begin() + at, (uint8_t)rng.next());
		break;
	case 2: // a few bytes removed
		if (!b.empty())
		{
			const size_t from = rng.below((uint32_t)b.size());
			const size_t len = 1 + rng.below(16);
			b.erase(b.begin() + from, b.begin() + (from + len < b.size() ? from + len : b.size()));
		}
		break;
	case 3: // a chunk repeated
		if (!b.empty())
		{
			const size_t from = rng.below((uint32_t)b.size());
			size_t len = 1 + rng.below(64);
			if (from + len > b.size())
				len = b.size() - from;
			std::vector<uint8_t> chunk(b.begin() + from, b.begin() + from + len);
			b.insert(b.begin() + at, chunk.begin(), chunk.end());
		}
		break;
	case 4: // a long run of one character: names, strings and lines past their buffers
	case 5:
	{
		const uint32_t len = kRunLengths[rng.below(sizeof(kRunLengths) / sizeof(kRunLengths[0]))];
		const char c = rng.below(4) == 0 ? (char)rng.next() : kRunChars[rng.below(sizeof(kRunChars))];
		b.insert(b.begin() + at, len, (uint8_t)c);
		break;
	}
	case 6: // a token of the format
		if (!tokens.empty())
		{
			const std::string& t = tokens[rng.below((uint32_t)tokens.size())];
			b.insert(b.begin() + at, t.begin(), t.end());
		}
		break;
	case 7: // a field set to an extreme, or the file cut short
		if (rng.below(3) == 0)
		{
			b.resize(at);
		}
		else if (b.size() >= 4)
		{
			static const uint32_t kExtremes[] = { 0, 1, 0x7f, 0x80, 0xff, 0x7fff, 0x8000, 0xffff, 0x7fffffff, 0x80000000u, 0xffffffffu };
			const uint32_t v = kExtremes[rng.below(sizeof(kExtremes) / sizeof(kExtremes[0]))];
			const size_t pos = rng.below((uint32_t)(b.size() - 3));
			const size_t width = 1 + rng.below(4);
			for (size_t i = 0; i < width && pos + i < b.size(); i++)
				b[pos + i] = (uint8_t)(v >> (8 * i));
		}
		break;
	}
}

void run(FuzzTarget fn, const std::vector<uint8_t>& bytes)
{
	const char* last = getenv("MC2_FUZZ_REPLAY_LAST");
	if (last)
	{
		FILE* f = fopen(last, "wb");
		if (f)
		{
			fwrite(bytes.data(), 1, bytes.size(), f);
			fclose(f);
		}
	}
	fn(bytes.data(), bytes.size());
}

int mutantsPerSeed()
{
	const char* env = getenv("MC2_FUZZ_REPLAY_MUTANTS");
	const int n = env ? atoi(env) : 150;
	return n < 0 ? 0 : n;
}

void replay(const char* target, FuzzTarget fn, const std::vector<std::string>& tokens, uint32_t seed)
{
	const std::vector<Seed> seeds = seedsFor(target);
	REQUIRE_MESSAGE(!seeds.empty(), "no corpus for ", target, " (fuzz_corpus.txt)");
	// an input the reader takes runs again with leak checking on
	struct Recheck
	{
		Recheck() { fuzzRecheckLeaks = fuzzLeakChecksAvailable(); }
		~Recheck() { fuzzRecheckLeaks = false; }
	} recheck;
	const int mutants = mutantsPerSeed();
	Rng rng = { seed };
	for (const Seed& s : seeds)
	{
		INFO("seed ", s.name);
		const unsigned long loaded = fuzzLoads();
		run(fn, s.bytes);
		// a bad-* seed is a broken file kept from a crash: turning it down is fine
		if (s.name.find("/bad-") == std::string::npos)
			CHECK_MESSAGE(fuzzLoads() > loaded, "the reader did not load seed ", s.name);
		for (int m = 0; m < mutants; m++)
		{
			std::vector<uint8_t> b = s.bytes;
			const uint32_t steps = 1 + rng.below(3);
			for (uint32_t i = 0; i < steps; i++)
				mutate(b, rng, tokens);
			if (!b.empty())
				run(fn, b);
		}
	}
}

} // namespace

TEST_SUITE("FuzzReplay")
{

TEST_CASE("FIT reader: corpus and mutations")
{
	replay("fit", fuzzFitFile,
		{ "FITini\n", "FITend\n", "[", "]", "\n[B]\n", "st Name = \"", "\"", "l Value = ", "f[3] Pos = ",
		  "l[3] Ids = ", "b Flag = TRUE", "c S = ", "uc[2] Values = ", ",", " = ", "0x", "1e38", "-" },
		0x1234567u);
}

TEST_CASE("CSV reader: corpus and mutations")
{
	replay("csv", fuzzCsvFile, { ",", "\"", "\r\n", "\n", ",,", "-2147483649", "1e38", "TRUE", "0x" },
		0x2345678u);
}

TEST_CASE("TGA readers: corpus and mutations")
{
	replay("tga", fuzzTga, { std::string("\x00\x00\x0a", 3), std::string("\x00\x00\x02", 3),
		std::string("\x00\x01\x01", 3), std::string("\xff\x7f", 2), std::string("\x80", 1) },
		0x3456789u);
}

TEST_CASE("ABL compiler: corpus and mutations")
{
	replay("abl", fuzzAblScript,
		{ "module ", "fsm ", "endmodule.", "endfsm.", "var\n", "integer ", "real ", "char[40] ", "boolean ",
		  "function ", "endfunction;", "state ", "endstate;", "if ", " then\n", "endif;", "while ", " do\n",
		  "endwhile;", "for ", "endfor;", "switch ", "case ", "endcase;", "endswitch;", "order ", "return(",
		  "\"", "concat(", "getmodule(", "fuzzname(", "fuzzint(", "fuzzreal(", "fuzzorder(", "#include \"",
		  "type ", "static ", "eternal ", "trans ", "1.5e38", "99999999999", "//", "(", ")", ";", ":=", "[", "]" },
		0x456789au);
}

} // TEST_SUITE("FuzzReplay")
