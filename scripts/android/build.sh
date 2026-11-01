#!/bin/bash
# Build the Meta Quest test APK (a flat Android app with the GLES renderer):
# SDL2 and SDL2_mixer from android/deps (scripts/android/fetch_deps.sh), the
# game as libmain.so, then the APK, with the SDK's own tools instead of
# Gradle. Runs in Git Bash on Windows or bash on Linux. See android/README.md.
#
# usage: scripts/android/build.sh [deps|game|apk|all]   (default: all)
#
# Environment:
#   ANDROID_HOME  SDK (default: %LOCALAPPDATA%\Android\Sdk, or ~/Android/Sdk)
#   JAVA_HOME     a JDK 17 or newer (Android Studio's jbr is one)
#   NDK_VERSION, CMAKE_VERSION, BUILD_TOOLS, PLATFORM  override the defaults below
set -e

STEP=${1:-all}
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
case "$(uname -s)" in
    MINGW*|MSYS*|CYGWIN*) WIN=1 ;;
    *) WIN=0 ;;
esac
winpath() { if [ $WIN = 1 ]; then cygpath -m "$1"; else echo "$1"; fi; }
exe() { if [ $WIN = 1 ]; then echo "$1$2"; else echo "$1"; fi; }   # name, Windows suffix

if [ -z "$ANDROID_HOME" ]; then
    if [ $WIN = 1 ]; then ANDROID_HOME="$(cygpath -u "$LOCALAPPDATA")/Android/Sdk"; else ANDROID_HOME="$HOME/Android/Sdk"; fi
fi
NDK_VERSION=${NDK_VERSION:-28.2.13676358}
CMAKE_VERSION=${CMAKE_VERSION:-3.31.6}
BUILD_TOOLS=${BUILD_TOOLS:-36.0.0}
PLATFORM=${PLATFORM:-android-36}
MIN_SDK=29          # Quest 2 runs Android 12 or later
TARGET_SDK=32

NDK="$ANDROID_HOME/ndk/$NDK_VERSION"
CMAKE="$(exe "$ANDROID_HOME/cmake/$CMAKE_VERSION/bin/cmake" .exe)"
NINJA="$(exe "$ANDROID_HOME/cmake/$CMAKE_VERSION/bin/ninja" .exe)"
BT="$ANDROID_HOME/build-tools/$BUILD_TOOLS"
ANDROID_JAR="$ANDROID_HOME/platforms/$PLATFORM/android.jar"
DEPS="$ROOT/android/deps"
BUILD="$ROOT/android/build"
PREFIX="$BUILD/prefix"

for f in "$NDK/build/cmake/android.toolchain.cmake" "$CMAKE" "$NINJA" "$ANDROID_JAR"; do
    [ -e "$f" ] || { echo "android: missing $f" >&2; exit 2; }
done

COMMON=(-G Ninja "-DCMAKE_MAKE_PROGRAM=$(winpath "$NINJA")"
    "-DCMAKE_TOOLCHAIN_FILE=$(winpath "$NDK/build/cmake/android.toolchain.cmake")"
    -DANDROID_ABI=arm64-v8a "-DANDROID_PLATFORM=android-$MIN_SDK"
    -DCMAKE_BUILD_TYPE=RelWithDebInfo
    "-DCMAKE_PREFIX_PATH=$(winpath "$PREFIX")" "-DCMAKE_FIND_ROOT_PATH=$(winpath "$PREFIX")")

build_deps() {
    [ -d "$DEPS/SDL2-2.30.11" ] || "$ROOT/scripts/android/fetch_deps.sh"
    "$CMAKE" -S "$(winpath "$DEPS/SDL2-2.30.11")" -B "$(winpath "$BUILD/sdl2")" "${COMMON[@]}" \
        "-DCMAKE_INSTALL_PREFIX=$(winpath "$PREFIX")" -DSDL_SHARED=ON -DSDL_STATIC=OFF -DSDL_TEST=OFF
    "$CMAKE" --build "$(winpath "$BUILD/sdl2")"
    "$CMAKE" --install "$(winpath "$BUILD/sdl2")"
    # WAV is built in; MP3 (minimp3) and Ogg (stb_vorbis) need no extra sources
    "$CMAKE" -S "$(winpath "$DEPS/SDL2_mixer-2.8.0")" -B "$(winpath "$BUILD/sdl2_mixer")" "${COMMON[@]}" \
        "-DCMAKE_INSTALL_PREFIX=$(winpath "$PREFIX")" -DBUILD_SHARED_LIBS=ON \
        -DSDL2MIXER_VENDORED=OFF -DSDL2MIXER_SAMPLES=OFF -DSDL2MIXER_OPUS=OFF \
        -DSDL2MIXER_FLAC=OFF -DSDL2MIXER_MOD=OFF -DSDL2MIXER_MIDI=OFF \
        -DSDL2MIXER_WAVPACK=OFF -DSDL2MIXER_GME=OFF
    "$CMAKE" --build "$(winpath "$BUILD/sdl2_mixer")"
    "$CMAKE" --install "$(winpath "$BUILD/sdl2_mixer")"
}

build_game() {
    "$CMAKE" -S "$(winpath "$ROOT")" -B "$(winpath "$BUILD/game")" "${COMMON[@]}" \
        -DMC2_GLES=ON -DMC2_BUILD_TESTS=OFF
    "$CMAKE" --build "$(winpath "$BUILD/game")"
}

build_apk() {
    [ -n "$JAVA_HOME" ] || { echo "android: set JAVA_HOME to a JDK 17+ (e.g. Android Studio's jbr)" >&2; exit 2; }
    local J="$JAVA_HOME/bin"
    [ $WIN = 1 ] && J="$(cygpath -u "$JAVA_HOME")/bin"
    local A="$BUILD/apk"
    rm -rf "$A"; mkdir -p "$A/classes" "$A/dex" "$A/stage/lib/arm64-v8a"

    # Java: SDL's activity and ours
    local sources=()
    while IFS= read -r f; do sources+=("$(winpath "$f")"); done < <(
        find "$DEPS/SDL2-2.30.11/android-project/app/src/main/java" "$ROOT/android/java" -name '*.java')
    "$(exe "$J/javac" .exe)" -nowarn --release 11 -classpath "$(winpath "$ANDROID_JAR")" \
        -d "$(winpath "$A/classes")" "${sources[@]}"
    local classes=()
    while IFS= read -r f; do classes+=("$(winpath "$f")"); done < <(find "$A/classes" -name '*.class')
    "$(exe "$BT/d8" .bat)" --release --min-api $MIN_SDK --lib "$(winpath "$ANDROID_JAR")" \
        --output "$(winpath "$A/dex")" "${classes[@]}"

    # manifest, then dex and native libraries
    "$(exe "$BT/aapt2" .exe)" link -o "$(winpath "$A/base.apk")" \
        --manifest "$(winpath "$ROOT/android/AndroidManifest.xml")" -I "$(winpath "$ANDROID_JAR")" \
        --min-sdk-version $MIN_SDK --target-sdk-version $TARGET_SDK
    cp "$A/dex/classes.dex" "$A/stage/"
    cp "$PREFIX/lib/libSDL2.so" "$PREFIX/lib/libSDL2_mixer.so" \
        "$BUILD/game/libmain.so" "$BUILD/game/out/res/libmc2res_64.so" "$A/stage/lib/arm64-v8a/"
    (cd "$A/stage" && "$(exe "$J/jar" .exe)" -u -M -0 -f ../base.apk classes.dex lib)

    "$(exe "$BT/zipalign" .exe)" -p -f 4 "$(winpath "$A/base.apk")" "$(winpath "$A/aligned.apk")"
    local KS="$ROOT/android/debug.keystore"
    if [ ! -f "$KS" ]; then
        "$(exe "$J/keytool" .exe)" -genkeypair -keystore "$(winpath "$KS")" -storepass android \
            -keypass android -alias androiddebugkey -keyalg RSA -keysize 2048 -validity 10000 \
            -dname "CN=MC2 test build"
    fi
    "$(exe "$BT/apksigner" .bat)" sign --ks "$(winpath "$KS")" --ks-pass pass:android \
        --out "$(winpath "$ROOT/android/mc2-quest-test.apk")" "$(winpath "$A/aligned.apk")"
    echo "android: $ROOT/android/mc2-quest-test.apk"
}

case "$STEP" in
    deps) build_deps ;;
    game) build_game ;;
    apk) build_apk ;;
    all) build_deps; build_game; build_apk ;;
    *) echo "usage: $0 [deps|game|apk|all]" >&2; exit 2 ;;
esac
