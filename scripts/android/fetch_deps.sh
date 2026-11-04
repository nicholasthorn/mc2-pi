#!/bin/bash
# Download and unpack the SDL2 sources the Android build compiles, and the
# OpenXR loader it links, into
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
# the OpenXR loader: an AAR (a zip) from Maven Central, of which the build
# uses the arm64 library and the headers
V=${OPENXR_DIR#openxr_loader-}
AAR="$DEPS/openxr_loader_for_android-$V.aar"
if [ ! -f "$AAR" ]; then
    echo "fetch: openxr_loader_for_android $V"
    curl -fL --retry 3 -o "$AAR.part" \
        "https://repo1.maven.org/maven2/org/khronos/openxr/openxr_loader_for_android/$V/openxr_loader_for_android-$V.aar"
    mv "$AAR.part" "$AAR"
fi
echo "a298c326085b09181c85235fb04d37510939bd30938ed4322c093c09780e8643  $AAR" | sha256sum -c --quiet -
[ -d "$DEPS/$OPENXR_DIR" ] || unzip -q "$AAR" -d "$DEPS/$OPENXR_DIR"
echo "fetch: sources in $DEPS"
