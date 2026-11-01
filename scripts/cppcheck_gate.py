#!/usr/bin/env python3
"""cppcheck with a baseline: fail only on findings that are not already known.

The legacy code has ~2000 existing cppcheck findings. They are recorded in a
baseline (scripts/cppcheck-baseline.txt) keyed by file + check id + message,
not line number, so unrelated edits do not shift them. A run fails when any
key occurs more often than the baseline allows.

usage:
  scripts/cppcheck_gate.py --build-dir build                 # full check
  scripts/cppcheck_gate.py --build-dir build --files a.cpp b.cpp   # only these
  scripts/cppcheck_gate.py --build-dir build --update        # rewrite baseline

--build-dir must hold compile_commands.json (a CMake configure is enough; no
build needed). The baseline depends on the cppcheck version: CI generates it
(see .github/workflows/static.yml), so update it from CI, not locally.

Exit status: 0 nothing new, 1 new findings, 2 cppcheck itself failed.
"""
import argparse
import collections
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BASELINE = os.path.join(ROOT, "scripts", "cppcheck-baseline.txt")
SEP = "\x1f"  # unit separator: never appears in cppcheck output
TEMPLATE = SEP.join(["{file}", "{line}", "{severity}", "{id}", "{message}"])
EXCLUDE = ["tests", "thirdparty", "3rdparty"]

# Microsoft's "Stuff" class registration: every class in mclib/stuff, mlr,
# gosfx and the object types declares its own static InitializeClass /
# TerminateClass / DefaultData / Make / TestInstance, hiding the parent's on
# purpose. Every game object type likewise declares its own init() overloads.
# The Stuff and gosFX libraries go further: their math types (Point3D,
# Normal3D, UnitVector3D, LinearMatrix4D...) and effect classes redefine
# operators, Load/Save, GetSpecification etc. per class so they return the
# derived type. duplInheritedMember stays on for every other name and the rest
# of the tree, where hiding a parent's member is a likely bug.
DELIBERATE_HIDING = {"InitializeClass", "TerminateClass", "DefaultData",
                     "Make", "TestInstance", "init"}
DELIBERATE_HIDING_DIRS = ("mclib/stuff/", "mclib/gosfx/")
HIDDEN_NAME = re.compile(r"with name '([^']+)' also defined in its parent class")
# Every game object, appearance, GUI and file class calls its own init() from
# its constructor and its own destroy()/close() from its destructor. Each level
# cleans up only its own members and nulls what it frees, so static binding is
# what the code wants and repeated calls down the hierarchy are safe. Virtual
# calls of any other name from a constructor or destructor are still reported.
CTOR_DTOR_CALLS = {"init", "destroy", "close"}
VIRTUAL_CALL = re.compile(r"^Virtual function '([^']+)' is called from (con|de)structor")
# memset() with a zero fill on a struct holding floats: all-bits-zero is 0.0f
# on IEEE 754, which every target is (x86-64, arm64), so only a non-zero fill
# (which makes NaNs) is reported. The fill is read from the source line.
ZERO_FILL_MEMSET = re.compile(r"\bmemset\s*\([^,]+,\s*(0+|0[xX]0+|'\\0')\s*,")
# some messages name a line ("called from constructor 'Camera()' at line
# 389"); keys must not move when unrelated edits shift the code
LINE_IN_MESSAGE = re.compile(r"\bat line \d+")


def key(path, severity, check, message):
    return SEP.join([path, severity, check,
                     LINE_IN_MESSAGE.sub("at line N", message)])


def source_line(path, line):
    try:
        with open(os.path.join(ROOT, path), encoding="latin-1") as f:
            for n, text in enumerate(f, 1):
                if n == line:
                    return text
    except (OSError, ValueError):
        pass
    return ""


def ignored(path, line, check, message):
    if check == "memsetClassFloat":
        return bool(ZERO_FILL_MEMSET.search(source_line(path, line)))
    if check == "duplInheritedMember":
        if path.startswith(DELIBERATE_HIDING_DIRS):
            return True
        m = HIDDEN_NAME.search(message)
        return bool(m) and m.group(1) in DELIBERATE_HIDING
    if check == "virtualCallInConstructor":
        m = VIRTUAL_CALL.search(message)
        return bool(m) and m.group(1) in CTOR_DTOR_CALLS
    return False


def run_cppcheck(build_dir, files, jobs):
    cmd = ["cppcheck", "--project=" + os.path.join(build_dir, "compile_commands.json"),
           "--enable=warning,portability", "--inline-suppr", "--quiet",
           "-j", str(jobs), "--template=" + TEMPLATE]
    cmd += ["-i" + os.path.join(ROOT, d) for d in EXCLUDE]
    cmd += ["--file-filter=*" + f.replace("\\", "/").lstrip("./") for f in files]
    r = subprocess.run(cmd, capture_output=True, text=True)
    findings = collections.Counter()
    for line in r.stderr.splitlines():
        parts = line.split(SEP)
        if len(parts) != 5:
            continue
        path, line_no, severity, check, message = parts
        path = os.path.relpath(os.path.abspath(path), ROOT).replace("\\", "/")
        if path.startswith(".."):
            continue  # system headers etc.
        if path.split("/", 1)[0] in EXCLUDE:
            continue  # -i is not honoured in --project mode by every version
        if severity in ("information", "debug"):
            continue
        if ignored(path, int(line_no) if line_no.isdigit() else 0, check, message):
            continue
        findings[key(path, severity, check, message)] += 1
    # Without --error-exitcode cppcheck exits non-zero only when it failed
    # itself (no file matched the filter, bad project file, crash). Its
    # findings would then be missing, not zero. Exit 2 (1 means new findings).
    if r.returncode != 0 and not findings:
        # the error can be on either stream (--quiet sends some to stdout)
        tail = "\n".join((r.stdout + "\n" + r.stderr).strip().splitlines()[-5:])
        print("cppcheck failed (exit %d):\n%s" % (r.returncode, tail))
        sys.exit(2)
    return findings


def load_baseline():
    base = collections.Counter()
    if os.path.exists(BASELINE):
        with open(BASELINE, encoding="utf-8") as f:
            for line in f:
                if line.startswith("#") or not line.strip():
                    continue
                count, rest = line.rstrip("\n").split("\t", 1)
                base[key(*rest.split("\t"))] += int(count)
    return base


def write_baseline(findings, version):
    with open(BASELINE, "w", encoding="utf-8", newline="\n") as f:
        f.write("# cppcheck baseline: count<TAB>file<TAB>severity<TAB>id<TAB>message\n")
        f.write("# generated by scripts/cppcheck_gate.py --update with %s\n" % version)
        f.write("# Lower counts by fixing code; never raise them to hide a new finding.\n")
        for key in sorted(findings):
            f.write("%d\t%s\n" % (findings[key], key.replace(SEP, "\t")))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--build-dir", required=True)
    ap.add_argument("--files", nargs="*", default=[])
    ap.add_argument("--update", action="store_true")
    ap.add_argument("-j", type=int, default=os.cpu_count() or 2)
    args = ap.parse_args()

    version = subprocess.run(["cppcheck", "--version"], capture_output=True,
                             text=True).stdout.strip()
    findings = run_cppcheck(args.build_dir, args.files, args.j)

    if args.update:
        if args.files:
            sys.exit("--update needs a full run (no --files)")
        write_baseline(findings, version)
        print("baseline written: %d findings (%s)" % (sum(findings.values()), version))
        return 0

    base = load_baseline()
    checked = {f.replace("\\", "/").lstrip("./") for f in args.files}
    new, fixed = [], 0
    for key, n in sorted(findings.items()):
        extra = n - base.get(key, 0)
        if extra > 0:
            new.append((extra, key))
    for key, n in base.items():
        path = key.split(SEP, 1)[0]
        if (not checked or path in checked) and findings.get(key, 0) < n:
            fixed += n - findings.get(key, 0)

    scope = "%d file(s)" % len(checked) if checked else "full tree"
    print("cppcheck (%s, %s): %d findings, %d new, %d fewer than baseline"
          % (version, scope, sum(findings.values()), sum(e for e, _ in new), fixed))
    for extra, key in new:
        path, severity, check, message = key.split(SEP)
        print("NEW %s: %s [%s/%s]%s" % (path, message, severity, check,
                                        " (x%d)" % extra if extra > 1 else ""))
    if fixed and not new:
        print("Findings went down: consider lowering the baseline "
              "(Actions > Static analysis > Run workflow, update_baseline).")
    return 1 if new else 0


if __name__ == "__main__":
    sys.exit(main())
