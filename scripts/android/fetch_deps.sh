#!/bin/bash
# Download and unpack the SDL2 sources the Android build compiles, into
# android/deps (git-ignored). Each archive is checked against its SHA-256
# (pinned from the first download, 2026-11-01).
#
# usage: scripts/android/fetch_deps.sh
set -e
. "$(dirname "$0")/env.sh"
DEPS="$ROOT/android/deps"
mkdir -p "$DEPS"

fetch() {   # dir url sha256
    local dir=$1 url=$2 sum=$3
    local archive="$DEPS/$(basename "$url")"
    if [ ! -f "$archive" ]; then
        echo "fetch: $url"
        curl -fL --retry 3 -o "$archive.part" "$url"
        mv "$archive.part" "$archive"
    fi
    echo "$sum  $archive" | sha256sum -c --quiet -
    [ -d "$DEPS/$dir" ] || tar -xzf "$archive" -C "$DEPS"
}

fetch $SDL2_DIR \
    https://github.com/libsdl-org/SDL/releases/download/release-2.30.11/$SDL2_DIR.tar.gz \
    8b8d4aef2038533da814965220f88f77d60dfa0f32685f80ead65e501337da7f
fetch $SDL2_MIXER_DIR \
    https://github.com/libsdl-org/SDL_mixer/releases/download/release-2.8.0/$SDL2_MIXER_DIR.tar.gz \
    1cfb34c87b26dbdbc7afd68c4f545c0116ab5f90bbfecc5aebe2a9cb4bb31549
echo "fetch: sources in $DEPS"
