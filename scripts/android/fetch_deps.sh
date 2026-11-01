#!/bin/bash
# Download and unpack the SDL2 sources the Android build compiles, into
# android/deps (git-ignored). Same versions as the Windows build
# (BUILD-WIN.md). Each archive is checked against its SHA-256 (pinned from
# the first download, 2026-11-01).
#
# usage: scripts/android/fetch_deps.sh
set -e
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
DEPS="$ROOT/android/deps"
mkdir -p "$DEPS"

fetch() {   # name url sha256
    local name=$1 url=$2 sum=$3
    local archive="$DEPS/$(basename "$url")"
    if [ ! -f "$archive" ]; then
        echo "fetch: $url"
        curl -fL --retry 3 -o "$archive.part" "$url"
        mv "$archive.part" "$archive"
    fi
    local got
    got=$(sha256sum "$archive" | cut -d' ' -f1)
    if [ -n "$sum" ] && [ "$got" != "$sum" ]; then
        echo "fetch: $archive: SHA-256 $got, expected $sum" >&2
        exit 1
    fi

    if [ ! -d "$DEPS/$name" ]; then
        tar -xzf "$archive" -C "$DEPS"
    fi
}

fetch SDL2-2.30.11 \
    https://github.com/libsdl-org/SDL/releases/download/release-2.30.11/SDL2-2.30.11.tar.gz \
    8b8d4aef2038533da814965220f88f77d60dfa0f32685f80ead65e501337da7f
fetch SDL2_mixer-2.8.0 \
    https://github.com/libsdl-org/SDL_mixer/releases/download/release-2.8.0/SDL2_mixer-2.8.0.tar.gz \
    1cfb34c87b26dbdbc7afd68c4f545c0116ab5f90bbfecc5aebe2a9cb4bb31549
echo "fetch: sources in $DEPS"
