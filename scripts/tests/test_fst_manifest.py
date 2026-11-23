#!/usr/bin/env python3
"""Tests for scripts/fst_manifest.py on small made-up archives.

Run: python3 -m unittest discover -s scripts/tests
"""

import hashlib
import os
import struct
import sys
import tempfile
import unittest

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..'))
import fst_manifest as fm  # noqa: E402


def make_fst(path, files, version=0xCADDECAF):
    """An FST with files (name, data) stored in the given order."""
    table = 8 + len(files) * fm.ENTRY.size
    head = struct.pack('<II', version, len(files))
    entries, blobs, offset = b'', b'', table
    for name, data in files:
        entries += fm.ENTRY.pack(offset, len(data), len(data), 0, name.encode())
        blobs += data
        offset += len(data)
    with open(path, 'wb') as f:
        f.write(head + entries + blobs)


class FstManifest(unittest.TestCase):
    def setUp(self):
        self.dir = tempfile.TemporaryDirectory()

    def tearDown(self):
        self.dir.cleanup()

    def path(self, name):
        return os.path.join(self.dir.name, name)

    def test_lists_each_entry_with_its_hash(self):
        make_fst(self.path('a.fst'), [('data/x.ini', b'one'), ('data/y.tga', b'two')])
        got = dict(fm.entries(self.path('a.fst')))
        self.assertEqual(got, {'data/x.ini': hashlib.sha256(b'one').hexdigest(),
                               'data/y.tga': hashlib.sha256(b'two').hexdigest()})

    def test_order_inside_the_archive_does_not_matter(self):
        files = [('data/b.tgl', b'bb'), ('data/a.tgl', b'aaa')]
        make_fst(self.path('one.fst'), files)
        make_fst(self.path('two.fst'), list(reversed(files)), version=0xFADDECAF)
        self.assertEqual(sorted(fm.entries(self.path('one.fst'))),
                         sorted(fm.entries(self.path('two.fst'))))

    def test_rejects_other_files(self):
        with open(self.path('bad.fst'), 'wb') as f:
            f.write(struct.pack('<II', 0x12345678, 0))
        with self.assertRaises(ValueError):
            list(fm.entries(self.path('bad.fst')))

    def test_rejects_an_entry_past_the_end(self):
        make_fst(self.path('cut.fst'), [('data/x.ini', b'one')])
        with open(self.path('cut.fst'), 'r+b') as f:
            f.truncate(os.path.getsize(self.path('cut.fst')) - 1)
        with self.assertRaises(ValueError):
            list(fm.entries(self.path('cut.fst')))


if __name__ == '__main__':
    unittest.main()
