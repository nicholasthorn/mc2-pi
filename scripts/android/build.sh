#!/bin/bash
# Build an Android test APK with the GLES renderer, for the Meta Quest or,
# with MC2_ANDROID_FLAVOUR=phone, for phones and tablets: SDL2 and SDL2_mixer
# from android/deps (scripts/android/fetch_deps.sh), the game as libmain.so,
# then the APK, with the SDK's own tools instead of Gradle. Runs in Git Bash
# on Windows or bash on Linux. See android/README.md.
#
# usage: scripts/android/build.sh [deps|game|apk|all]   (default: all)
#
# Environment:
#   ANDROID_HOME  SDK (default: %LOCALAPPDATA%\Android\Sdk, or ~/Android/Sdk)
#   JAVA_HOME     a JDK 17 or newer (Android Studio's jbr is one)
#   MC2_ANDROID_FLAVOUR  quest (the default: VR through OpenXR) or phone
#                 (flat, for phones and tablets); see env.sh
#   MC2_ANDROID_DATA  a game data folder (scripts/build_gamedata.sh; use
#                 --no-movies for phones) to put in the APK with this
#                 checkout's shaders: the first start copies it to the app's
#                 files folder (GameOS/gameos/android_data.cpp). Without it
#                 the data is pushed with adb, as before.
#   MC2_VERSION   version for mc2.log and the APK's versionName (default:
#                 git describe, and the manifest's versionName)
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
    # a checkout from before the phone took ARCore
    [ -z "$AR_DEPS" ] || [ -d "$AR_DEPS" ] || "$ROOT/scripts/android/fetch_deps.sh"
    "$CMAKE" -S "$ROOT" -B "$GAME_BUILD" "${COMMON[@]}" -DMC2_GLES=ON -DMC2_BUILD_TESTS=OFF \
        "-DMC2_VERSION=${MC2_VERSION:-}" "${XR_CMAKE[@]}"
    "$CMAKE" --build "$GAME_BUILD"
}

# MC2_ANDROID_DATA and shaders/ into $1/assets/mc2data, with the list the
# game copies from (mc2data/files.txt, format: utils/data_manifest.h). The
# stamp is a hash of every file, so changed data is copied again. Files the
# player's runs make (settings, saves, logs) are left out, as the game's
# manifest check requires, and so are a desktop build's game and libraries
# (the APK has its own).
stage_data() {
    local out="$1/assets/mc2data" src="$MC2_ANDROID_DATA"
    [ -f "$src/minprefs.cfg" ] || { echo "android: MC2_ANDROID_DATA=$src is not a game data folder (no minprefs.cfg; see scripts/build_gamedata.sh)" >&2; exit 2; }
    mkdir -p "$out"
    (cd "$src" && find . -type f \
        ! -name options.cfg ! -name orgprefs.cfg ! -name mc2args.txt ! -name 'mc2*.log' \
        ! -name mc2data.installed ! -path './.*' ! -path './savegame/*' ! -path './shaders/*'         ! -path ./mc2 ! -path ./mc2.exe ! -path './*.so' ! -path './*.dll' ! -path './*.pdb' -print0) \
        | (cd "$src" && xargs -0 cp --parents -t "$out")
    mkdir -p "$out/shaders" && cp -r "$ROOT/shaders/." "$out/shaders/"
    local list stamp
    list=$(cd "$out" && find . -type f -printf '%s %P\n' | LC_ALL=C sort -k2)
    stamp=$(cd "$out" && find . -type f -print0 | LC_ALL=C sort -z | xargs -0 sha256sum | sha256sum | cut -c1-16)
    printf 'mc2data %s\n%s\n' "$stamp" "$list" > "$out/files.txt"
    echo "android: game data $stamp, $(echo "$list" | wc -l) files, $(du -sm "$out" | cut -f1) MB"
}

build_apk() {
    [ -n "$JAVA_HOME" ] || { echo "android: set JAVA_HOME to a JDK 17+ (e.g. Android Studio's jbr)" >&2; exit 2; }
    local J; J="$(native "$JAVA_HOME")/bin"
    local A="$BUILD/apk"
    rm -rf "$A"; mkdir -p "$A/classes" "$A/dex" "$A/stage/lib/arm64-v8a"

    # Java: SDL's activity and ours, and the flavour's libraries' classes
    # (ARCore's on the phone), which go into the dex as they are
    local sources classes classpath="$ANDROID_JAR" sep=":" jar
    [ $WIN = 1 ] && sep=";"
    for jar in "${JAVA_JARS[@]}"; do classpath="$classpath$sep$jar"; done
    mapfile -t sources < <(find "$DEPS/$SDL2_DIR/android-project/app/src/main/java" "$ROOT/android/java" -name '*.java')
    "$J/javac" -nowarn --release 11 -classpath "$classpath" -d "$A/classes" "${sources[@]}"
    mapfile -t classes < <(find "$A/classes" -name '*.class')
    "$BT/d8$BAT" --release --min-api $MIN_SDK --lib "$ANDROID_JAR" --output "$A/dex" "${classes[@]}" "${JAVA_JARS[@]}"

    # the libraries' resources (ARCore's install dialog, which it finds by name)
    local res=() dir
    for dir in "${JAVA_RES[@]}"; do
        "$BT/aapt2" compile --dir "$dir" -o "$A/res-$(basename "$(dirname "$dir")").zip"
        res+=(-R "$A/res-$(basename "$(dirname "$dir")").zip")
    done

    # manifest, then dex and native libraries: stripped copies (the unstripped
    # ones stay in android/build for reading crash stacks), stored uncompressed
    # and page-aligned so they load straight from the APK (extractNativeLibs false)
    local version=()
    [ -z "$MC2_VERSION" ] || version=(--version-name "$MC2_VERSION" --replace-version)
    "$BT/aapt2" link -o "$A/base.apk" --manifest "$MANIFEST" "${res[@]}" --auto-add-overlay \
        -I "$ANDROID_JAR" --min-sdk-version $MIN_SDK --target-sdk-version $TARGET_SDK "${version[@]}"
    cp "$A/dex/classes.dex" "$A/stage/"
    local lib libs=("$PREFIX/lib/libSDL2.so" "$PREFIX/lib/libSDL2_mixer.so"
                    "$GAME_BUILD/libmain.so" "$GAME_BUILD/out/res/libmc2res_64.so" "${XR_LIBS[@]}")
    for lib in "${libs[@]}"; do
        "$STRIP" --strip-unneeded -o "$A/stage/lib/arm64-v8a/$(basename "$lib")" "$lib"
    done
    local data=()
    if [ -n "$MC2_ANDROID_DATA" ]; then stage_data "$A/stage"; data=(assets); fi
    (cd "$A/stage" && "$J/jar" -u -M -0 -f ../base.apk classes.dex lib "${data[@]}")
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
