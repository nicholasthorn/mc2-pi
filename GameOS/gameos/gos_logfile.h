#ifndef GOS_LOGFILE_H
#define GOS_LOGFILE_H

// The player's log, mc2.log (gos_logfile.cpp). Not used by automated runs
// (smoke, bench, UI walks), which take the game's output from the terminal.

// Sends stdout and stderr to mc2.log in the config folder, the last run's log
// kept as mc2.previous.log, and copies both on to where stdout was going, so
// the terminal still shows them. MC2_LOG_FILE=0 turns it off. For the start
// of main, before anything is printed.
void gos_LogFileStart();

// Android, where stdout goes to mc2.log already (android_prepare): keeps the
// last run's log as mc2.previous.log. Call before the file is opened.
void gos_LogFileKeepPrevious();

// Writes a line with the game's version and the time, for telling logs apart.
void gos_LogFileHeader();

// A crash adds the signal (or Windows exception) and the stack to the log,
// then carries on to whatever would have happened without it.
void gos_LogFileCatchCrashes();

#endif // GOS_LOGFILE_H
