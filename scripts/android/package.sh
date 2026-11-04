#!/bin/bash
# Release package for the Meta Quest build, after scripts/android/build.sh:
# the APK as it is (so it can be downloaded and sideloaded on its own), the
# files that go beside it on the headset, and the unstripped libraries.
#
# usage: scripts/android/package.sh --version VER [--out DIR]
#
# Writes to <out> (default: dist):
#   mc2-<ver>-quest-arm64.apk
#   mc2-<ver>-quest-arm64-files.tar.gz    shaders/, INSTALL.md, licences, VERSION
#   mc2-<ver>-quest-arm64-symbols.tar.gz  for reading crash stacks
set -e
. "$(dirname "$0")/env.sh"

VERSION=""; OUT="dist"
while [ $# -gt 0 ]; do
    case "$1" in
        --version) VERSION="$2"; shift 2 ;;
        --out)     OUT="$2"; shift 2 ;;
        -h|--help) sed -n '2,11p' "$0"; exit 0 ;;
        *) echo "package: unknown option $1" >&2; exit 2 ;;
    esac
done
[ -n "$VERSION" ] || { echo "package: --version is required" >&2; exit 2; }
[ -f "$APK" ] || { echo "package: no $APK; run scripts/android/build.sh first" >&2; exit 2; }

BUILD="$ROOT/android/build"
DEPS="$ROOT/android/deps"
NAME="mc2-$VERSION-quest-arm64"
mkdir -p "$OUT"
OUT="$(cd "$OUT" && pwd)"
STAGE="$OUT/$NAME-files"
SYMS="$OUT/$NAME-symbols"
rm -rf "$STAGE" "$SYMS"
mkdir -p "$STAGE/licenses" "$SYMS"

cp "$APK" "$OUT/$NAME.apk"

cp -r "$ROOT/shaders" "$STAGE/"
cp "$ROOT/license.txt" "$ROOT/dist/INSTALL.md" "$STAGE/"
cp "$ROOT/dist/licenses/README.md" "$ROOT/dist/licenses/pl_mpeg.txt" "$ROOT/dist/licenses/stb.txt" "$STAGE/licenses/"
# what the APK bundles: SDL2 and SDL2_mixer (build.sh turns off every codec but
# the built-in minimp3 and stb_vorbis), and the OpenXR loader, whose AAR holds
# its licence
for f in SDL2.txt SDL2_mixer.txt SDL2_mixer-minimp3.txt SDL2_mixer-stb_vorbis.txt; do
    cp "$ROOT/dist/licenses/windows/$f" "$STAGE/licenses/"
done
cp "$DEPS/$OPENXR_DIR/META-INF/LICENSE" "$STAGE/licenses/OpenXR-loader.txt"
echo "$VERSION" > "$STAGE/VERSION"

cp "$BUILD/game/libmain.so" "$BUILD/game/out/res/libmc2res_64.so" \
   "$BUILD/prefix/lib/libSDL2.so" "$BUILD/prefix/lib/libSDL2_mixer.so" "$SYMS/"

tar -C "$OUT" -czf "$STAGE.tar.gz" "$NAME-files"
tar -C "$OUT" -czf "$SYMS.tar.gz" "$NAME-symbols"
rm -rf "$SYMS"
echo "package: $(ls "$OUT/$NAME".apk "$STAGE.tar.gz" "$SYMS.tar.gz" | tr '\n' ' ')"
