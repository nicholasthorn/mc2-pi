# Shared settings for scripts/android/*.sh (sourced, not run).
# Paths come out in the native form (C:/... in Git Bash on Windows), which
# both Git Bash's tools and the Windows SDK executables accept.

case "$(uname -s)" in
    MINGW*|MSYS*|CYGWIN*) WIN=1 ;;
    *) WIN=0 ;;
esac
native() { if [ $WIN = 1 ]; then cygpath -m "$1"; else echo "$1"; fi; }

ROOT="$(native "$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)")"
if [ -z "$ANDROID_HOME" ]; then
    if [ $WIN = 1 ]; then ANDROID_HOME="$LOCALAPPDATA/Android/Sdk"; else ANDROID_HOME="$HOME/Android/Sdk"; fi
fi
ANDROID_HOME="$(native "$ANDROID_HOME")"
BAT=""; [ $WIN = 1 ] && BAT=".bat"   # d8 and apksigner are .bat scripts on Windows

# the Windows build's versions (BUILD-WIN.md)
SDL2_DIR=SDL2-2.30.11
SDL2_MIXER_DIR=SDL2_mixer-2.8.0
OPENXR_DIR=openxr_loader-1.1.65   # the Khronos loader AAR, unpacked

# MC2_ANDROID_FLAVOUR picks the app:
#   quest  Meta Quest, in VR through OpenXR (the default)
#   phone  phones and tablets: flat, no OpenXR, its own package name, so both
#          can be installed on one device (planning/future-features/MOBILE.md)
FLAVOUR=${MC2_ANDROID_FLAVOUR:-quest}
case "$FLAVOUR" in
    quest) PKG=com.nicholasthorn.mc2
           MANIFEST="$ROOT/android/AndroidManifest.xml" ;;
    phone) PKG=com.nicholasthorn.mc2.phone
           MANIFEST="$ROOT/android/phone/AndroidManifest.xml" ;;
    *) echo "android: MC2_ANDROID_FLAVOUR is quest or phone, not '$FLAVOUR'" >&2; exit 2 ;;
esac
APK="$ROOT/android/mc2-$FLAVOUR-test.apk"
# the activity's class keeps the Quest package's name in both apps
ACTIVITY="$PKG/com.nicholasthorn.mc2.MC2Activity"
# the game's build folder: one per flavour, as MC2_OPENXR differs
GAME_BUILD="$ROOT/android/build/game"
[ "$FLAVOUR" = quest ] || GAME_BUILD="$ROOT/android/build/game-$FLAVOUR"
