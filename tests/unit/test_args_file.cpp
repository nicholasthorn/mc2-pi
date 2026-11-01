// tests/unit/test_args_file.cpp
//
// args_file: the command line of an Android run comes from mc2args.txt in the
// app's files folder (there is no shell to type it into). Tokens split on
// whitespace and line breaks, '#' comments run to the end of the line, and
// double quotes keep a token with spaces together.

#include "doctest.h"

#include "utils/args_file.h"

#include <string>
#include <vector>

using args_file::parse;
using Args = std::vector<std::string>;

TEST_SUITE("ArgsFile")
{

TEST_CASE("tokens split on spaces, tabs and line breaks, CRLF included")
{
	CHECK(parse("-mission mc2_19\t-autoexit 3600\r\n-bench frames.csv\n") ==
			Args{ "-mission", "mc2_19", "-autoexit", "3600", "-bench", "frames.csv" });
}

TEST_CASE("comments run to the end of the line")
{
	CHECK(parse("# a bench run\n-mission mc2_19 # the battle map\n-stereo 40\n") ==
			Args{ "-mission", "mc2_19", "-stereo", "40" });
}

TEST_CASE("double quotes keep spaces, and a # inside them is not a comment")
{
	CHECK(parse("-label \"fan on # 2\" -x") == Args{ "-label", "fan on # 2", "-x" });
	CHECK(parse("\"\"") == Args{ "" });
}

TEST_CASE("empty input and comment-only input give no arguments")
{
	CHECK(parse("").empty());
	CHECK(parse("  \n# nothing here\n\t").empty());
}

TEST_CASE("an unclosed quote takes the rest of the input")
{
	CHECK(parse("-a \"b c") == Args{ "-a", "b c" });
}

TEST_CASE("leading NAME=value tokens are environment settings, the rest are arguments")
{
	Args args = parse("MC2_VSYNC=0 MC2_STOP_EXITS=1\n-mission mc2_19 -label A=B");
	const std::vector<std::pair<std::string, std::string>> env = args_file::takeEnv(args);
	REQUIRE(env.size() == 2);
	CHECK(env[0].first == "MC2_VSYNC");
	CHECK(env[0].second == "0");
	CHECK(env[1].first == "MC2_STOP_EXITS");
	CHECK(env[1].second == "1");
	CHECK(args == Args{ "-mission", "mc2_19", "-label", "A=B" });   // only leading ones
}

TEST_CASE("a token is a setting only with a valid name before '='")
{
	Args args = parse("=x 1A=2 -a");
	CHECK(args_file::takeEnv(args).empty());
	CHECK(args == Args{ "=x", "1A=2", "-a" });

	Args empty = parse("EMPTY=");
	const auto env = args_file::takeEnv(empty);
	REQUIRE(env.size() == 1);
	CHECK(env[0].first == "EMPTY");
	CHECK(env[0].second == "");
	CHECK(empty.empty());
}

}
