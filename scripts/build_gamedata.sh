#!/bin/bash
# Build the game data from an alariq/mc2srcdata checkout with this repo's data
# tools, and assemble a game directory (data/, assets/, *.fst, *.cfg).
# The result is platform independent: build it once, copy it anywhere (Pi).
#
# usage: scripts/build_gamedata.sh --src-data DIR --build-dir DIR --out DIR [--no-movies] [-j N]
#   --src-data DIR   mc2srcdata checkout (its build_scripts/ is used in place,
#                    as upstream intends; outputs land there before copying)
#   --build-dir DIR  CMake build tree with the data tools built
#                    (targets: aseconv makefst makersp pak text_tool)
#   --out DIR        game directory to fill (created if missing)
#   --no-movies      skip the movie targets. In mc2srcdata only movies/ is
#                    stored in Git LFS, so a clone with GIT_LFS_SKIP_SMUDGE=1
#                    plus --no-movies needs no LFS download at all. Missions
#                    run without movies.
#   -j N             parallel make jobs (default: nproc)

set -eu

SRC_DATA=""; BUILD_DIR=""; OUT=""; NO_MOVIES=0; JOBS=$(nproc)
while [ $# -gt 0 ]; do
    case "$1" in
        --src-data)  SRC_DATA="$2"; shift 2 ;;
        --build-dir) BUILD_DIR="$2"; shift 2 ;;
        --out)       OUT="$2"; shift 2 ;;
        --no-movies) NO_MOVIES=1; shift ;;
        -j)          JOBS="$2"; shift 2 ;;
        -h|--help)   sed -n '2,19p' "$0"; exit 0 ;;
        *) echo "build_gamedata: unknown option $1" >&2; exit 2 ;;
    esac
done
[ -d "$SRC_DATA/build_scripts" ] || { echo "build_gamedata: no build_scripts/ in '$SRC_DATA'" >&2; exit 2; }
[ -n "$BUILD_DIR" ] && [ -n "$OUT" ] || { echo "build_gamedata: --build-dir and --out are required" >&2; exit 2; }

BS="$SRC_DATA/build_scripts"
for t in out/data_tools/aseconv out/data_tools/makefst out/data_tools/makersp out/data_tools/pak out/text_tool/text_tool; do
    [ -x "$BUILD_DIR/$t" ] || { echo "build_gamedata: missing tool $BUILD_DIR/$t" >&2; exit 2; }
    cp "$BUILD_DIR/$t" "$BS/"
done

MAKE_ARGS=(all BUILD_PLATFORM=linux -j "$JOBS")
# command-line variables override the makefile's own assignments
[ $NO_MOVIES -eq 1 ] && MAKE_ARGS+=(ALL_MOVIE_TARGETS=)

echo "build_gamedata: make ${MAKE_ARGS[*]} in $BS"
make -C "$BS" "${MAKE_ARGS[@]}"

mkdir -p "$OUT"
cp -r "$BS/data" "$BS/assets" "$OUT/"
cp "$BS"/*.fst "$BS"/*.cfg "$BS/testtxm.tga" "$OUT/"
echo "build_gamedata: game data in $OUT ($(du -sh "$OUT" | cut -f1))"
