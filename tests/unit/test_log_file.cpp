// tests/unit/test_log_file.cpp
//
// log_file: where a player's mc2.log goes, the name the last run's log is kept
// under, and the crash lines, which a signal handler builds without printf.

#include "doctest.h"

#include "utils/log_file.h"

#include <cstdint>
#include <string>

TEST_SUITE("LogFile")
{

TEST_CASE("the log folder is the game's config folder, as the saved settings use")
{
	CHECK(log_file::folder("/xdg", "/home/me") == "/xdg/.mechcommander2");
	CHECK(log_file::folder(nullptr, "/home/me") == "/home/me/.config/.mechcommander2");
	CHECK(log_file::folder("", "/home/me") == "/home/me/.config/.mechcommander2");
}

TEST_CASE("with no home to go by (a Windows double-click), the log stays in the game folder")
{
	CHECK(log_file::folder(nullptr, nullptr) == "");
	CHECK(log_file::folder("", "") == "");
}

TEST_CASE("log paths")
{
	CHECK(log_file::logPath("") == "mc2.log");
	CHECK(log_file::previousPath("") == "mc2.previous.log");
	CHECK(log_file::logPath("/a/b") == "/a/b/mc2.log");
	CHECK(log_file::previousPath("/a/b") == "/a/b/mc2.previous.log");
}

TEST_CASE("the crash line names the signal and the address")
{
	char line[128];
	const size_t len = log_file::formatCrash(line, sizeof(line), "signal", 11, 0x1a2bu);
	CHECK(std::string(line) == "CRASH: signal 11 at 0x1a2b\n");
	CHECK(len == std::string(line).size());
}

TEST_CASE("a Windows exception code prints in full as the unsigned value it is")
{
	char line[128];
	log_file::formatCrash(line, sizeof(line), "exception", 0xC0000005u, 0u);
	CHECK(std::string(line) == "CRASH: exception 3221225477 at 0x0\n");
}

TEST_CASE("a stack line gives what addr2line needs: the module's bare name and the offset")
{
	char line[128];
	log_file::formatFrame(line, sizeof(line), 3, "/opt/game/libmain.so", 0xdeadbeefu);
	CHECK(std::string(line) == "CRASH:   #3 libmain.so+0xdeadbeef\n");
	log_file::formatFrame(line, sizeof(line), 0, "C:\\Games\\mc2\\mc2.exe", 0x10u);
	CHECK(std::string(line) == "CRASH:   #0 mc2.exe+0x10\n");
	log_file::formatFrame(line, sizeof(line), 12, nullptr, 0x10u);
	CHECK(std::string(line) == "CRASH:   #12 ?+0x10\n");
}

TEST_CASE("64-bit addresses are not cut short")
{
	char line[128];
	log_file::formatCrash(line, sizeof(line), "signal", 6, UINT64_C(0xffffffffffffffff));
	CHECK(std::string(line) == "CRASH: signal 6 at 0xffffffffffffffff\n");
}

TEST_CASE("a short buffer cuts the line and keeps it terminated")
{
	char line[10];
	const size_t len = log_file::formatFrame(line, sizeof(line), 3, "/opt/game/libmain.so", 0xdeadbeefu);
	CHECK(len == 9);
	CHECK(std::string(line) == "CRASH:   ");
}

}
