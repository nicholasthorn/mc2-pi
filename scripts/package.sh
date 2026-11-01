#!/bin/bash
# Build a release package from a CMake build tree: a game directory without
# game data (planning/RELEASES.md). Players unpack it and build the data into
# it with the bundled build_gamedata.sh (see dist/INSTALL.md).
#
# usage: scripts/package.sh --build-dir DIR --platform NAME --version VER
#                           [--out DIR] [--vcredist DIR] [--thirdparty DIR]
#   --build-dir DIR   CMake build tree (Linux: Ninja; Windows: Visual Studio,
#                     RelWithDebInfo) with mc2, the resource library and, on
#                     Linux, the data tools built
#   --platform NAME   rpi-arm64, linux-x86_64 or windows-x64: names the package
#                     and picks what goes in (see below)
#   --version VER     e.g. v0.1.0-rc1; names the package and goes in VERSION
#   --out DIR         where the archives go (default: dist)
#   --vcredist DIR    Windows: folder with the MSVC runtime DLLs
#                     (...\VC\Redist\MSVC\<ver>\x64\Microsoft.VC143.CRT)
#   --thirdparty DIR  Windows: 3rdparty/lib/x64 with the SDL2 and GLEW DLLs
#
# Writes <out>/mc2-<ver>-<platform>.tar.gz (Linux) or .zip (Windows), and
# <out>/mc2-<ver>-<platform>-symbols.tar.gz with the debug information.
#
# Linux packages use the system SDL2, GLEW and zlib (INSTALL.md has the apt
# line) and ship the data tools; the Windows zip bundles its DLLs and the C++
# runtime, and has no data tools: the data is built with a Linux package (WSL
# or any Linux machine, a Pi included) and copied over.

set -eu

BUILD_DIR=""; PLATFORM=""; VERSION=""; OUT="dist"; VCREDIST=""; THIRDPARTY=""
while [ $# -gt 0 ]; do
    case "$1" in
        --build-dir)  BUILD_DIR="$2"; shift 2 ;;
        --platform)   PLATFORM="$2"; shift 2 ;;
        --version)    VERSION="$2"; shift 2 ;;
        --out)        OUT="$2"; shift 2 ;;
        --vcredist)   VCREDIST="$2"; shift 2 ;;
        --thirdparty) THIRDPARTY="$2"; shift 2 ;;
        -h|--help)    sed -n '2,25p' "$0"; exit 0 ;;
        *) echo "package: unknown option $1" >&2; exit 2 ;;
    esac
done
[ -d "$BUILD_DIR" ] && [ -n "$VERSION" ] || { echo "package: --build-dir and --version are required" >&2; exit 2; }
case "$PLATFORM" in
    rpi-arm64|linux-x86_64) WINDOWS=0 ;;
    windows-x64)            WINDOWS=1 ;;
    *) echo "package: --platform must be rpi-arm64, linux-x86_64 or windows-x64" >&2; exit 2 ;;
esac

SRC="$(cd "$(dirname "$0")/.." && pwd)"
NAME="mc2-$VERSION-$PLATFORM"
mkdir -p "$OUT"
OUT="$(cd "$OUT" && pwd)"
STAGE="$OUT/$NAME"
SYMS="$OUT/$NAME-symbols"
rm -rf "$STAGE" "$SYMS"
mkdir -p "$STAGE" "$SYMS"

# the built file with this name. Visual Studio puts outputs in
# per-configuration folders: only RelWithDebInfo's count, so a tree that also
# holds Debug or Release builds cannot mix them into one package
find_built() {
    local f n
    if [ $WINDOWS -eq 1 ]; then
        f=$(find "$BUILD_DIR" -name "$1" -type f -path "*/RelWithDebInfo/*" -not -path "*/CMakeFiles/*")
    else
        f=$(find "$BUILD_DIR" -name "$1" -type f -not -path "*/CMakeFiles/*")
    fi
    n=$(printf '%s' "$f" | grep -c . || true)
    [ "$n" -eq 1 ] || { echo "package: expected one $1 under $BUILD_DIR, found $n" >&2; exit 2; }
    echo "$f"
}

# Linux: split the debug information off into the symbols archive and leave a
# debuglink, so a crash report can be symbolised against the released binary
add_elf() {
    local src="$1" dest="$2"
    cp "$src" "$dest"
    objcopy --only-keep-debug "$dest" "$SYMS/$(basename "$dest").debug"
    strip --strip-debug --strip-unneeded "$dest"
    (cd "$SYMS" && objcopy --add-gnu-debuglink="$(basename "$dest").debug" "$dest")
}

if [ $WINDOWS -eq 0 ]; then
    add_elf "$(find_built mc2)" "$STAGE/mc2"
    add_elf "$(find_built libmc2res_64.so)" "$STAGE/libmc2res_64.so"
    mkdir -p "$STAGE/tools"
    for t in aseconv makefst makersp pak text_tool; do
        add_elf "$(find_built "$t")" "$STAGE/tools/$t"
    done
    cp "$SRC/scripts/build_gamedata.sh" "$STAGE/"
else
    [ -d "$VCREDIST" ] && [ -d "$THIRDPARTY" ] || { echo "package: windows-x64 needs --vcredist and --thirdparty" >&2; exit 2; }
    exe=$(find_built mc2.exe)
    res=$(find_built mc2res_64.dll)
    cp "$exe" "$res" "$STAGE/"
    for pdb in "${exe%.exe}.pdb" "${res%.dll}.pdb"; do
        [ -f "$pdb" ] && cp "$pdb" "$SYMS/"
    done
    # what mc2.exe imports: zlib is linked statically (zlib.lib) and SDL2_ttf is
    # only used by text_tool, which the Windows package does not carry
    for dll in SDL2.dll SDL2_mixer.dll glew32.dll; do
        cp "$THIRDPARTY/$dll" "$STAGE/" || { echo "package: $dll missing from $THIRDPARTY" >&2; exit 2; }
    done
    for dll in vcruntime140.dll vcruntime140_1.dll msvcp140.dll; do
        cp "$VCREDIST/$dll" "$STAGE/" || { echo "package: $dll missing from $VCREDIST" >&2; exit 2; }
    done
fi

cp -r "$SRC/shaders" "$STAGE/"
cp "$SRC/license.txt" "$SRC/dist/INSTALL.md" "$STAGE/"
mkdir -p "$STAGE/licenses"
cp "$SRC/dist/licenses/README.md" "$SRC/dist/licenses/pl_mpeg.txt" "$SRC/dist/licenses/stb.txt" "$STAGE/licenses/"
if [ $WINDOWS -eq 1 ]; then
    cp "$SRC"/dist/licenses/windows/*.txt "$STAGE/licenses/"
fi
echo "$VERSION" > "$STAGE/VERSION"

if [ $WINDOWS -eq 0 ]; then
    tar -C "$OUT" -czf "$OUT/$NAME.tar.gz" "$NAME"
else
    rm -f "$OUT/$NAME.zip"
    (cd "$OUT" && 7z a -tzip -bso0 "$NAME.zip" "$NAME")
fi
tar -C "$OUT" -czf "$OUT/$NAME-symbols.tar.gz" "$NAME-symbols"
rm -rf "$SYMS"
echo "package: $(ls "$OUT/$NAME".* "$OUT/$NAME-symbols.tar.gz" | tr '\n' ' ')"
echo "package: staged in $STAGE"
