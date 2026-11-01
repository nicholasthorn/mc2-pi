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
. "$(dirname "$0")/env.sh"

STEP=${1:-all}
NDK_VERSION=${NDK_VERSION:-28.2.13676358}
CMAKE_VERSION=${CMAKE_VERSION:-3.31.6}
BUILD_TOOLS=${BUILD_TOOLS:-36.0.0}
PLATFORM=${PLATFORM:-android-36}
MIN_SDK=29          # Quest 2 runs Android 12 or later
TARGET_SDK=32

NDK="$ANDROID_HOME/ndk/$NDK_VERSION"
CMAKE="$ANDROID_HOME/cmake/$CMAKE_VERSION/bin/cmake"
NINJA="$ANDROID_HOME/cmake/$CMAKE_VERSION/bin/ninja"
BT="$ANDROID_HOME/build-tools/$BUILD_TOOLS"
ANDROID_JAR="$ANDROID_HOME/platforms/$PLATFORM/android.jar"
STRIP="$(ls -d "$NDK"/toolchains/llvm/prebuilt/*/bin | head -n 1)/llvm-strip"
DEPS="$ROOT/android/deps"
BUILD="$ROOT/android/build"
PREFIX="$BUILD/prefix"

for f in "$NDK/build/cmake/android.toolchain.cmake" "$ANDROID_JAR"; do
    [ -e "$f" ] || { echo "android: missing $f" >&2; exit 2; }
done

COMMON=(-G Ninja "-DCMAKE_MAKE_PROGRAM=$NINJA"
    "-DCMAKE_TOOLCHAIN_FILE=$NDK/build/cmake/android.toolchain.cmake"
    -DANDROID_ABI=arm64-v8a "-DANDROID_PLATFORM=android-$MIN_SDK"
    -DCMAKE_BUILD_TYPE=RelWithDebInfo
    "-DCMAKE_PREFIX_PATH=$PREFIX" "-DCMAKE_FIND_ROOT_PATH=$PREFIX")

cmake_dep() {   # source dir name, extra configure options...
    local src="$DEPS/$1" bin="$BUILD/$1"; shift
    "$CMAKE" -S "$src" -B "$bin" "${COMMON[@]}" "-DCMAKE_INSTALL_PREFIX=$PREFIX" "$@"
    "$CMAKE" --build "$bin"
    "$CMAKE" --install "$bin"
}

build_deps() {
    [ -d "$DEPS/$SDL2_DIR" ] || "$ROOT/scripts/android/fetch_deps.sh"
    cmake_dep $SDL2_DIR -DSDL_SHARED=ON -DSDL_STATIC=OFF -DSDL_TEST=OFF
    # WAV is built in; MP3 (minimp3) and Ogg (stb_vorbis) need no extra sources
    cmake_dep $SDL2_MIXER_DIR -DBUILD_SHARED_LIBS=ON \
        -DSDL2MIXER_VENDORED=OFF -DSDL2MIXER_SAMPLES=OFF -DSDL2MIXER_OPUS=OFF \
        -DSDL2MIXER_FLAC=OFF -DSDL2MIXER_MOD=OFF -DSDL2MIXER_MIDI=OFF \
        -DSDL2MIXER_WAVPACK=OFF -DSDL2MIXER_GME=OFF
}

build_game() {
    "$CMAKE" -S "$ROOT" -B "$BUILD/game" "${COMMON[@]}" -DMC2_GLES=ON -DMC2_BUILD_TESTS=OFF \
        "-DOPENXR_PREFAB=$DEPS/$OPENXR_DIR/prefab/modules"
    "$CMAKE" --build "$BUILD/game"
}

build_apk() {
    [ -n "$JAVA_HOME" ] || { echo "android: set JAVA_HOME to a JDK 17+ (e.g. Android Studio's jbr)" >&2; exit 2; }
    local J; J="$(native "$JAVA_HOME")/bin"
    local A="$BUILD/apk"
    rm -rf "$A"; mkdir -p "$A/classes" "$A/dex" "$A/stage/lib/arm64-v8a"

    # Java: SDL's activity and ours
    local sources classes
    mapfile -t sources < <(find "$DEPS/$SDL2_DIR/android-project/app/src/main/java" "$ROOT/android/java" -name '*.java')
    "$J/javac" -nowarn --release 11 -classpath "$ANDROID_JAR" -d "$A/classes" "${sources[@]}"
    mapfile -t classes < <(find "$A/classes" -name '*.class')
    "$BT/d8$BAT" --release --min-api $MIN_SDK --lib "$ANDROID_JAR" --output "$A/dex" "${classes[@]}"

    # manifest, then dex and native libraries: stripped copies (the unstripped
    # ones stay in android/build for reading crash stacks), stored uncompressed
    # and page-aligned so they load straight from the APK (extractNativeLibs false)
    "$BT/aapt2" link -o "$A/base.apk" --manifest "$ROOT/android/AndroidManifest.xml" \
        -I "$ANDROID_JAR" --min-sdk-version $MIN_SDK --target-sdk-version $TARGET_SDK
    cp "$A/dex/classes.dex" "$A/stage/"
    local lib
    for lib in "$PREFIX/lib/libSDL2.so" "$PREFIX/lib/libSDL2_mixer.so" \
               "$DEPS/$OPENXR_DIR/jni/arm64-v8a/libopenxr_loader.so" \
               "$BUILD/game/libmain.so" "$BUILD/game/out/res/libmc2res_64.so"; do
        "$STRIP" --strip-unneeded -o "$A/stage/lib/arm64-v8a/$(basename "$lib")" "$lib"
    done
    (cd "$A/stage" && "$J/jar" -u -M -0 -f ../base.apk classes.dex lib)
    "$BT/zipalign" -p -f 4 "$A/base.apk" "$A/aligned.apk"

    local KS="$ROOT/android/debug.keystore"
    [ -f "$KS" ] || "$J/keytool" -genkeypair -keystore "$KS" -storepass android -keypass android \
        -alias androiddebugkey -keyalg RSA -keysize 2048 -validity 10000 -dname "CN=MC2 test build"
    "$BT/apksigner$BAT" sign --ks "$KS" --ks-pass pass:android --out "$APK" "$A/aligned.apk"
    echo "android: $APK"
}

case "$STEP" in
    deps) build_deps ;;
    game) build_game ;;
    apk) build_apk ;;
    all) build_deps; build_game; build_apk ;;
    *) echo "usage: $0 [deps|game|apk|all]" >&2; exit 2 ;;
esac
