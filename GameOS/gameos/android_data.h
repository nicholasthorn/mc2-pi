#ifndef ANDROID_DATA_H
#define ANDROID_DATA_H

// Android: the game data an APK can carry in its assets (mc2data/, see
// scripts/android/build.sh), copied into the current folder (the app's files
// folder) on the first start and again when the APK brings different data.
// Elsewhere these do nothing.
namespace android_data {

// Copies the data if the APK has some and it is not installed yet. Returns
// false if the copy failed; the player has been told why.
bool install();

// The game's first start without options.cfg only runs the old hardware
// sniffer and quits: start from the minimum settings instead, as
// scripts/smoke.sh does.
void firstStartSettings();

}

#endif // ANDROID_DATA_H
