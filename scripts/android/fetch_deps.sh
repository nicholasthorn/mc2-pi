#!/bin/bash
# Download and unpack the SDL2 sources the Android build compiles, and the
# OpenXR loader and ARCore it links, into
# android/deps (git-ignored). Each archive is checked against its SHA-256
# (pinned from the first download, 2026-11-01). What is already unpacked is
# left as it is, so adding a dependency fetches only that one.
#
# usage: scripts/android/fetch_deps.sh
set -e
. "$(dirname "$0")/env.sh"
DEPS="$ROOT/android/deps"
mkdir -p "$DEPS"

fetch() {   # dir url sha256
    local dir=$1 url=$2 sum=$3
    [ -d "$DEPS/$dir" ] && return 0
    local archive="$DEPS/$(basename "$url")"
    if [ ! -f "$archive" ]; then
        echo "fetch: $url"
        curl -fL --retry 3 -o "$archive.part" "$url"
        mv "$archive.part" "$archive"
    fi
    echo "$sum  $archive" | sha256sum -c --quiet -
    # --force-local: GNU tar would take the C: of a Windows path for a remote host.
    # Xcode: symlinks that Git Bash cannot make, in files this build does not use.
    tar --force-local --exclude='*/Xcode/*' -xzf "$archive" -C "$DEPS"
}

fetch $SDL2_DIR \
    https://github.com/libsdl-org/SDL/releases/download/release-2.30.11/$SDL2_DIR.tar.gz \
    8b8d4aef2038533da814965220f88f77d60dfa0f32685f80ead65e501337da7f
fetch $SDL2_MIXER_DIR \
    https://github.com/libsdl-org/SDL_mixer/releases/download/release-2.8.0/$SDL2_MIXER_DIR.tar.gz \
    1cfb34c87b26dbdbc7afd68c4f545c0116ab5f90bbfecc5aebe2a9cb4bb31549
# the OpenXR loader: an AAR (a zip) from Maven Central, of which the build
# uses the arm64 library and the headers
if [ ! -d "$DEPS/$OPENXR_DIR" ]; then
    V=${OPENXR_DIR#openxr_loader-}
    AAR="$DEPS/openxr_loader_for_android-$V.aar"
    if [ ! -f "$AAR" ]; then
        echo "fetch: openxr_loader_for_android $V"
        curl -fL --retry 3 -o "$AAR.part" \
            "https://repo1.maven.org/maven2/org/khronos/openxr/openxr_loader_for_android/$V/openxr_loader_for_android-$V.aar"
        mv "$AAR.part" "$AAR"
    fi
    echo "a298c326085b09181c85235fb04d37510939bd30938ed4322c093c09780e8643  $AAR" | sha256sum -c --quiet -
    unzip -q "$AAR" -d "$DEPS/$OPENXR_DIR"
fi
# ARCore, for the phone's tabletop: the AAR from Google's Maven (its C and
# JNI libraries, the Java classes they call, the resources of its install
# dialog) and the C API's header from the SDK's repository, at the same version
if [ ! -d "$DEPS/$ARCORE_DIR" ]; then
    V=${ARCORE_DIR#arcore-}
    AR_AAR="$DEPS/core-$V.aar"
    AR_H="$DEPS/arcore_c_api-$V.h"
    if [ ! -f "$AR_AAR" ]; then
        echo "fetch: ARCore $V"
        curl -fL --retry 3 -o "$AR_AAR.part" "https://dl.google.com/android/maven2/com/google/ar/core/$V/core-$V.aar"
        mv "$AR_AAR.part" "$AR_AAR"
    fi
    if [ ! -f "$AR_H" ]; then
        curl -fL --retry 3 -o "$AR_H.part" \
            "https://raw.githubusercontent.com/google-ar/arcore-android-sdk/v$V/libraries/include/arcore_c_api.h"
        mv "$AR_H.part" "$AR_H"
    fi
    echo "df53bb8ea8e3a9f32e256b91909959d60b2e4c60d8fcc56b905b4c3e926f5c72  $AR_AAR" | sha256sum -c --quiet -
    echo "0dfc2ae3f70acaca7b1fa28e9303a2ce8f1212836809f21306e9226a4d383b50  $AR_H" | sha256sum -c --quiet -
    unzip -q "$AR_AAR" -d "$DEPS/$ARCORE_DIR"
    mkdir -p "$DEPS/$ARCORE_DIR/include"
    cp "$AR_H" "$DEPS/$ARCORE_DIR/include/arcore_c_api.h"
fi
echo "fetch: sources in $DEPS"
