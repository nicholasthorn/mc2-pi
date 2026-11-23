// GameOS/gameos/gos_wsl.h
//
// Whether the game runs in WSL (Windows Subsystem for Linux), where WSLg's
// windows and network differ from a real Linux machine. Header only: both
// libgameos and libgameos_main use it, and some tools link those two in an
// order where one could not call into the other.

#ifndef GOS_WSL_H
#define GOS_WSL_H

#include <cctype>
#include <cstdio>
#include <cstring>

inline bool gos_RunningInWsl()
{
#ifdef PLATFORM_WINDOWS
	return false;
#else
	std::FILE* file = std::fopen("/proc/sys/kernel/osrelease", "r");
	if (!file)
		return false;
	char text[256] = {};
	const bool read = std::fgets(text, sizeof(text), file) != nullptr;
	std::fclose(file);
	if (!read)
		return false;
	for (char* c = text; *c; c++)
		*c = static_cast<char>(std::tolower(static_cast<unsigned char>(*c)));
	return std::strstr(text, "microsoft") != nullptr;
#endif
}

#endif
