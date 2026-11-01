#ifndef ARGS_FILE_H
#define ARGS_FILE_H

#include <string>
#include <utility>
#include <vector>

// The command line of an Android run, read from a text file (mc2args.txt)
// because there is no shell to type it into. GL- and SDL-free for the unit
// tests.
namespace args_file {

// Tokens split on whitespace (line breaks included); '#' starts a comment to
// the end of the line; double quotes keep a token's spaces (and #) together.
std::vector<std::string> parse(const std::string& text);

// Removes the leading NAME=value tokens from args (NAME: a letter or '_',
// then letters, digits or '_') and returns them as environment settings, in
// order, as a shell line "A=1 B=2 cmd args" would.
std::vector<std::pair<std::string, std::string>> takeEnv(std::vector<std::string>& args);

}

#endif // ARGS_FILE_H
