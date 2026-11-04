#!/bin/bash
# Turn a release package (scripts/package.sh) into a full one: the same files
# plus the built game data and settings, ready to play once unpacked
# (planning/future-features/EASY-INSTALL.md). Runs on Linux for every platform:
# the data is platform independent.
#
# usage: scripts/package_full.sh --package FILE --data DIR --data-version SHA
#                                --prefs NAME [--out DIR]
#   --package FILE       mc2-<ver>-<platform>.tar.gz or .zip
#   --data DIR           built game data (scripts/build_gamedata.sh)
#   --data-version SHA   the alariq/mc2srcdata commit it was built from
#   --prefs NAME         settings to start with: hiprefs or minprefs
#   --out DIR            where the archive goes (default: dist)
#
# Writes <out>/mc2-<ver>-<platform>-full.tar.gz (or .zip).
set -eu

PACKAGE=""; DATA=""; DATA_VERSION=""; PREFS=""; OUT="dist"
while [ $# -gt 0 ]; do
    case "$1" in
        --package)      PACKAGE="$2"; shift 2 ;;
        --data)         DATA="$2"; shift 2 ;;
        --data-version) DATA_VERSION="$2"; shift 2 ;;
        --prefs)        PREFS="$2"; shift 2 ;;
        --out)          OUT="$2"; shift 2 ;;
        -h|--help)      sed -n '2,14p' "$0"; exit 0 ;;
        *) echo "package_full: unknown option $1" >&2; exit 2 ;;
    esac
done
[ -f "$PACKAGE" ] && [ -f "$DATA/minprefs.cfg" ] && [ -f "$DATA/$PREFS.cfg" ] && [ -n "$DATA_VERSION" ] || {
    echo "package_full: needs --package, --data-version, --data with built game data, and --prefs naming a .cfg in it" >&2
    exit 2
}

SRC="$(cd "$(dirname "$0")/.." && pwd)"
mkdir -p "$OUT"
OUT="$(cd "$OUT" && pwd)"
PACKAGE="$(cd "$(dirname "$PACKAGE")" && pwd)/$(basename "$PACKAGE")"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

case "$PACKAGE" in
    *.tar.gz) NAME="$(basename "$PACKAGE" .tar.gz)"; tar -xzf "$PACKAGE" -C "$TMP" ;;
    *.zip)    NAME="$(basename "$PACKAGE" .zip)"; unzip -q "$PACKAGE" -d "$TMP" ;;
    *) echo "package_full: $PACKAGE is neither .tar.gz nor .zip" >&2; exit 2 ;;
esac
STAGE="$TMP/$NAME"
[ -d "$STAGE" ] || { echo "package_full: no $NAME folder in $PACKAGE" >&2; exit 2; }

cp -r "$DATA/." "$STAGE/"
# The game takes a missing options.cfg as its first run: it only "sniffs" the
# hardware and exits. It also resets options.cfg when orgprefs.cfg is missing
# (code/mechcmd2.cpp), so both go in
cp "$STAGE/$PREFS.cfg" "$STAGE/options.cfg"
cp "$STAGE/minprefs.cfg" "$STAGE/orgprefs.cfg"
# the licence the game data is distributed under
cp "$SRC/EULA.txt" "$STAGE/"
echo "alariq/mc2srcdata $DATA_VERSION" > "$STAGE/DATA-VERSION"

case "$PACKAGE" in
    *.tar.gz) tar -C "$TMP" -czf "$OUT/$NAME-full.tar.gz" "$NAME"; RESULT="$OUT/$NAME-full.tar.gz" ;;
    *.zip)    RESULT="$OUT/$NAME-full.zip"; rm -f "$RESULT"; (cd "$TMP" && zip -q -r -X "$RESULT" "$NAME") ;;
esac
echo "package_full: $RESULT ($(du -h "$RESULT" | cut -f1))"
