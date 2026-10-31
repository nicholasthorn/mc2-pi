#!/usr/bin/env python3
"""Compile-check the game's GLSL shaders without running the game.

Mirrors how gameos builds them at runtime: #include lines are expanded
relative to the including file, then the header buildShaderHeader() picks
(gameos_graphics.cpp) and each material-variant #define are prepended.
Every *.vert / *.frag is checked as GLSL ES 3.10 (the MC2_GLES / Raspberry Pi
path) and desktop GLSL 4.20, with glslangValidator.

usage: scripts/validate_shaders.py [shaders_dir]     (exit 1 on any failure)
"""
import os
import re
import subprocess
import sys
import tempfile

# keep in sync with buildShaderHeader() in GameOS/gameos/gameos_graphics.cpp
HEADERS = {
    "es310": "#version 310 es\nprecision highp float;\nprecision highp int;\n"
             "precision highp sampler2DArray;\nprecision highp sampler3D;\n"
             "precision highp sampler2DShadow;\n",
    "gl420": "#version 420\n",
    # GLSL 3.30..4.10 drivers with 420pack (e.g. WSLg's GL 4.1): the engine
    # emits the driver's own version; 330 is the lowest it accepts.
    "gl330_420pack": "#version 330\n"
                     "#extension GL_ARB_shading_language_420pack : require\n",
}
# material variants compiled by gosRenderer::init()
VARIANTS = [[], ["ALPHA_TEST"]]
INCLUDE = re.compile(r'#include\s*[<"]([^>"]+)[>"]')
VERSION = re.compile(r'^\s*#version', re.M)


def expand(path):
    base = os.path.dirname(path)
    with open(path) as f:
        src = f.read()
    return INCLUDE.sub(lambda m: expand(os.path.join(base, m.group(1))), src)


def check(name, ext, src):
    with tempfile.NamedTemporaryFile("w", suffix=ext, delete=False) as f:
        f.write(src)
    try:
        r = subprocess.run(["glslangValidator", f.name], capture_output=True, text=True)
    finally:
        os.unlink(f.name)
    if r.returncode == 0:
        return True
    lines = src.splitlines()
    for l in r.stdout.splitlines():
        if not l.startswith("ERROR"):
            continue
        print("    " + l)
        m = re.match(r"ERROR: 0:(\d+):", l)
        if m and int(m.group(1)) <= len(lines):
            print("      > " + lines[int(m.group(1)) - 1].strip())
    return False


def main():
    sdir = sys.argv[1] if len(sys.argv) > 1 else os.path.join(
        os.path.dirname(os.path.abspath(__file__)), "..", "shaders")
    failed = checked = 0
    for name in sorted(os.listdir(sdir)):
        ext = os.path.splitext(name)[1]
        if ext not in (".vert", ".frag"):
            continue
        body = expand(os.path.join(sdir, name))
        if VERSION.search(body):
            # buildShaderHeader() injects #version at runtime; a second one
            # breaks both profiles (.claude/rules/shader-conventions.md)
            failed += 1
            print("FAIL %s: has its own #version line" % name)
            continue
        for profile, header in HEADERS.items():
            for defs in VARIANTS:
                checked += 1
                src = header + "".join("#define %s = 1\n" % d for d in defs) + "\n" + body
                if not check(name, ext, src):
                    failed += 1
                    print("FAIL %s [%s] %s" % (name, profile, " ".join(defs)))
    print("%d shader variants checked, %d failed" % (checked, failed))
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
