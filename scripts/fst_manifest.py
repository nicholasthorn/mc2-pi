#!/usr/bin/env python3
"""List the files inside FST archives with a sha256 of each, sorted by name.

Two builds of the game data can pack the same files in a different order
(makersp takes them in the order the file system lists them), so archives
that differ byte for byte may still hold identical files. This compares
what is inside. Format: mclib/ffile.h (version, count, FILEENTRY table).

usage: scripts/fst_manifest.py FILE.fst [...]   (one "sha256  archive:name" per entry)
"""
import hashlib
import os
import struct
import sys

VERSIONS = (0xCADDECAF, 0xFADDECAF)  # FASTFILE_VERSION, FASTFILE_VERSION_LZ
ENTRY = struct.Struct("<IIII250s")    # offset, size, realSize, hash, name


def entries(path):
    with open(path, "rb") as f:
        data = f.read()
    version, count = struct.unpack_from("<II", data, 0)
    if version not in VERSIONS:
        raise ValueError("%s: not an FST archive (version %#x)" % (path, version))
    for i in range(count):
        offset, size, _real, _hash, name = ENTRY.unpack_from(data, 8 + i * ENTRY.size)
        name = name.split(b"\0", 1)[0].decode("latin-1")
        if offset + size > len(data):
            raise ValueError("%s: entry %s runs past the end" % (path, name))
        yield name, hashlib.sha256(data[offset:offset + size]).hexdigest()


def main(paths):
    if not paths:
        print(__doc__.strip().splitlines()[-1], file=sys.stderr)
        return 2
    lines = []
    for path in paths:
        base = os.path.basename(path)
        lines += ["%s  %s:%s" % (digest, base, name) for name, digest in entries(path)]
    for line in sorted(lines, key=lambda l: l.split("  ", 1)[1].lower()):
        print(line)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
