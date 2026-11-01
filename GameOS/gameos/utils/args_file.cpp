#include "args_file.h"

namespace args_file {

std::vector<std::string> parse(const std::string& text)
{
	std::vector<std::string> args;
	std::string token;
	bool inToken = false;
	bool inQuotes = false;
	bool inComment = false;

	for (char c : text) {
		if (inComment) {
			if (c == '\n')
				inComment = false;
			continue;
		}
		if (inQuotes) {
			if (c == '"')
				inQuotes = false;
			else
				token += c;
			continue;
		}
		if (c == '"') {
			inQuotes = true;
			inToken = true;
		} else if (c == '#') {
			inComment = true;
		} else if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
			if (inToken) {
				args.push_back(token);
				token.clear();
				inToken = false;
			}
		} else {
			token += c;
			inToken = true;
		}
	}
	if (inToken)
		args.push_back(token);
	return args;
}

// NAME=value with NAME a letter or '_' followed by letters, digits or '_'
static bool isSetting(const std::string& token, size_t* eq)
{
	*eq = token.find('=');
	if (*eq == std::string::npos || *eq == 0)
		return false;
	for (size_t i = 0; i < *eq; ++i) {
		const char c = token[i];
		const bool letter = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_';
		const bool digit = c >= '0' && c <= '9';
		if (!letter && !(digit && i > 0))
			return false;
	}
	return true;
}

std::vector<std::pair<std::string, std::string>> takeEnv(std::vector<std::string>& args)
{
	std::vector<std::pair<std::string, std::string>> env;
	size_t n = 0, eq = 0;
	while (n < args.size() && isSetting(args[n], &eq)) {
		env.push_back(std::make_pair(args[n].substr(0, eq), args[n].substr(eq + 1)));
		++n;
	}
	args.erase(args.begin(), args.begin() + n);
	return env;
}

}
