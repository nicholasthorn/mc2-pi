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

PKG=com.nicholasthorn.mc2
APK="$ROOT/android/mc2-quest-test.apk"
