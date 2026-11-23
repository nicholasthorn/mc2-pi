#!/usr/bin/env python3
"""Compile-check the game's GLSL shaders without running the game.

Mirrors how gameos builds them at runtime: #include lines are expanded
relative to the including file, then the header buildShaderHeader() picks
(gameos_graphics.cpp) and each material-variant #define are prepended.
Every *.vert / *.frag is checked as GLSL ES 3.10 (the MC2_GLES / Raspberry Pi
path), desktop GLSL 4.20, GLSL 3.30 with 420pack, and GLSL 3.30 / 4.10
without it (macOS), with glslangValidator.

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
    # GLSL 3.30+ without 420pack (macOS GL 4.1 reports 410): no
    # layout(binding=); again the driver's own version, 330 the lowest.
    "gl330": "#version 330\n#define MC2_NO_LAYOUT_BINDING 1\n",
    "gl410": "#version 410\n#define MC2_NO_LAYOUT_BINDING 1\n",
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


COMMENT = re.compile(r'//[^\n]*|/\*.*?\*/', re.S)
FRAG_INPUT = re.compile(
    r'^\s*(?:(?:flat|smooth|noperspective|centroid)\s+)*in\s+[^;(]*?\b(\w+)\s*(?:\[[^\]]*\])?\s*;',
    re.M)


def unwritten_inputs(vert_src, frag_src):
    """Fragment inputs the vertex shader never assigns. Mesa (Linux, the Pi)
    links such a pair; Apple's GL (macOS) refuses: "Input of fragment shader
    'X' not written by vertex shader" (issue #183). Text based: an assignment
    in any #if branch counts as written."""
    vert = COMMENT.sub('', vert_src)
    missing = []
    for name in FRAG_INPUT.findall(COMMENT.sub('', frag_src)):
        written = re.compile(r'\b%s\b(?:\s*\[[^\]]*\])?(?:\.\w+)?\s*[-+*/]?=(?!=)' % re.escape(name))
        if not written.search(vert):
            missing.append(name)
    return missing


def check_interfaces(sdir):
    """Each .vert/.frag pair: every fragment input is written. Returns the
    number of failing pairs."""
    failed = 0
    for name in sorted(os.listdir(sdir)):
        base, ext = os.path.splitext(name)
        frag = os.path.join(sdir, base + ".frag")
        if ext != ".vert" or not os.path.exists(frag):
            continue
        missing = unwritten_inputs(expand(os.path.join(sdir, name)), expand(frag))
        if missing:
            failed += 1
            print("FAIL %s: fragment input(s) %s never written by the vertex shader"
                  " (Apple's GL refuses to link)" % (base, ", ".join(missing)))
    return failed


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
    bad_pairs = check_interfaces(sdir)
    return 1 if failed or bad_pairs else 0


if __name__ == "__main__":
    sys.exit(main())
