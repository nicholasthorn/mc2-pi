#!/usr/bin/env python3
"""Tests for scripts/mc2x_import.py on small made-up files (no MC2X data).

Run: python3 -m unittest discover -s scripts/tests
"""

import bz2
import io
import os
import struct
import sys
import tempfile
import unittest
import zipfile
import zlib

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..'))
import mc2x_import as mi  # noqa: E402


def make_clickteam(path, files, list_compression=2):
    """A minimal PE with a Clickteam overlay holding files: (name, data,
    compression byte)."""
    pe = 0x40
    head = bytearray(0x200)
    head[0:2] = b'MZ'
    struct.pack_into('<I', head, 0x3c, pe)
    head[pe:pe + 4] = b'PE\0\0'
    struct.pack_into('<H', head, pe + 6, 1)       # one section
    struct.pack_into('<H', head, pe + 20, 0)      # no optional header
    struct.pack_into('<II', head, pe + 24 + 16, 0x100, 0x200)  # raw size, pointer
    body = bytes(head) + bytes(0x100)             # the section's raw data
    data = bytearray()
    entries = bytearray()
    for name, content, kind in files:
        stream = {0: content, 1: zlib.compress(content), 2: bz2.compress(content)}[kind]
        offset = len(data)
        data += bytes([kind]) + stream
        e = bytearray(62)
        struct.pack_into('<IIIII', e, 6, offset, 1 + len(stream), 0, len(content), 0)
        e += name.encode('cp1252') + b'\0'
        struct.pack_into('<HH', e, 0, len(e), 0)
        entries += e
    # a non-file entry, which is skipped
    other = bytearray(10)
    struct.pack_into('<HH', other, 0, 10, 3)
    file_list = struct.pack('<I', len(files) + 1) + bytes(entries) + bytes(other)
    packed = bz2.compress(file_list) if list_compression == 2 else zlib.compress(file_list)
    list_block = struct.pack('<IB', len(file_list), list_compression) + packed
    data_block = struct.pack('<I', 0) + bytes(data)
    overlay = mi.CLICKTEAM_SIGNATURE
    overlay += struct.pack('<HHI', 0x1234, 0, 3) + b'abc'
    overlay += struct.pack('<HHI', mi.BLOCK_FILE_LIST, 0, len(list_block)) + list_block
    overlay += struct.pack('<HHI', mi.BLOCK_DATA, 0, len(data_block)) + data_block
    with open(path, 'wb') as f:
        f.write(body + overlay)


def lzw_codes(codes):
    """9-bit codes packed least significant bit first, as the game reads them."""
    buf = count = 0
    out = bytearray()
    for c in codes:
        buf |= c << count
        count += 9
        while count >= 8:
            out.append(buf & 0xff)
            buf >>= 8
            count -= 8
    if count:
        out.append(buf & 0xff)
    return bytes(out)


def make_fst(path, files):
    """files: (name, stored bytes, size)."""
    header = struct.pack('<II', 0, len(files))
    table_size = len(files) * (16 + 250)
    offset = len(header) + table_size
    table = bytearray()
    blob = bytearray()
    for name, stored, size in files:
        table += struct.pack('<IIII', offset + len(blob), len(stored), size, 0)
        table += name.encode('cp1252').ljust(250, b'\0')
        blob += stored
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, 'wb') as f:
        f.write(header + bytes(table) + bytes(blob))


def write(path, data=b'x'):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, 'wb') as f:
        f.write(data)


class Clickteam(unittest.TestCase):
    def test_lists_and_extracts_every_compression(self):
        with tempfile.TemporaryDirectory() as t:
            exe = os.path.join(t, 'setup.exe')
            make_clickteam(exe, [('art.fst', b'stored', 0),
                                 ('Data\\Art\\MC2X_MM.fit', b'zlib zlib', 1),
                                 ('mc2xres.dll', b'bzip2' * 50, 2)])
            files = mi.clickteam_files(exe)
            self.assertEqual([f[0] for f in files], ['art.fst', 'Data/Art/MC2X_MM.fit', 'mc2xres.dll'])
            out = os.path.join(t, 'out')
            mi.extract_clickteam(exe, out)
            with open(os.path.join(out, 'Data', 'Art', 'MC2X_MM.fit'), 'rb') as f:
                self.assertEqual(f.read(), b'zlib zlib')
            with open(os.path.join(out, 'mc2xres.dll'), 'rb') as f:
                self.assertEqual(f.read(), b'bzip2' * 50)

    def test_extracts_a_chunk_at_a_time(self):
        with tempfile.TemporaryDirectory() as t:
            exe = os.path.join(t, 'setup.exe')
            content = bytes(range(256)) * 40
            make_clickteam(exe, [('s', content, 0), ('z', content, 1), ('b', content, 2)])
            out = os.path.join(t, 'out')
            mi.extract_clickteam(exe, out, chunk=7)
            for n in ('s', 'z', 'b'):
                with open(os.path.join(out, n), 'rb') as f:
                    self.assertEqual(f.read(), content)

    def test_refuses_names_that_leave_the_folder(self):
        for name in ('..\\escape.txt', 'data\\..\\..\\x', 'C:\\x', '\\root'):
            with tempfile.TemporaryDirectory() as t:
                exe = os.path.join(t, 'setup.exe')
                make_clickteam(exe, [(name, b'x', 0)])
                with self.assertRaises(mi.ImportError_, msg=name):
                    mi.extract_clickteam(exe, os.path.join(t, 'out'))

    def test_zlib_file_list(self):
        with tempfile.TemporaryDirectory() as t:
            exe = os.path.join(t, 'setup.exe')
            make_clickteam(exe, [('a.txt', b'a', 1)], list_compression=1)
            self.assertEqual(mi.clickteam_files(exe)[0][0], 'a.txt')

    def test_refuses_other_files(self):
        with tempfile.TemporaryDirectory() as t:
            p = os.path.join(t, 'x.exe')
            write(p, b'not an executable at all')
            with self.assertRaises(mi.ImportError_):
                mi.clickteam_files(p)


class FastFiles(unittest.TestCase):
    def test_stored_zlib_and_lzw(self):
        with tempfile.TemporaryDirectory() as t:
            fst = os.path.join(t, 'mission.fst')
            # LZW "ABAB": A, B, then code 258 (= "AB"), end
            lzw = lzw_codes([65, 66, 258, 257])
            make_fst(fst, [('Data\\Missions\\Plain.fit', b'plain', 5),
                           ('data\\missions\\z.fit', zlib.compress(b'zz' * 20), 40),
                           ('data\\missions\\l.fit', lzw, 4)])
            files = mi.fst_files(fst)
            self.assertEqual([f[0] for f in files],
                             ['data/missions/plain.fit', 'data/missions/z.fit', 'data/missions/l.fit'])
            with open(fst, 'rb') as f:
                self.assertEqual([mi.fst_data(f, *e[1:]) for e in files], [b'plain', b'zz' * 20, b'ABAB'])

    def test_bad_lzw_is_refused(self):
        self.assertIsNone(mi._lzw(lzw_codes([300, 257]), 10))
        with tempfile.TemporaryDirectory() as t:
            fst = os.path.join(t, 'a.fst')
            make_fst(fst, [('data\\a', lzw_codes([300, 257]), 10)])
            with open(fst, 'rb') as f, self.assertRaises(mi.ImportError_):
                mi.fst_data(f, *mi.fst_files(fst)[0][1:])


class Identify(unittest.TestCase):
    def test_size_then_md5(self):
        known = [mi.Download('k', 'k.exe', 3, 'good', 'clickteam', 'f')]
        with tempfile.TemporaryDirectory() as t:
            p = os.path.join(t, 'k.exe')
            write(p, b'abc')
            self.assertEqual(mi.identify(p, known, md5=lambda _: 'good').key, 'k')
            with self.assertRaises(mi.ImportError_):
                mi.identify(p, known, md5=lambda _: 'bad')
            write(p, b'abcd')
            self.assertIsNone(mi.identify(p, known, md5=lambda _: 'good'))


class Folders(unittest.TestCase):
    def test_lowercase_tree(self):
        with tempfile.TemporaryDirectory() as t:
            write(os.path.join(t, 'Data', 'Art', 'A.FIT'))
            write(os.path.join(t, 'Data', 'Art', 'b.fit'))
            write(os.path.join(t, 'Data', 'art2', 'C.tga'), b'upper')
            write(os.path.join(t, 'Data', 'art2', 'c.tga'), b'lower')
            # a case-insensitive file system (Windows) keeps one file for both
            case_sensitive = len(os.listdir(os.path.join(t, 'Data', 'art2'))) == 2
            clashes = mi.lowercase_tree(t)
            self.assertIn('a.fit', os.listdir(os.path.join(t, 'data', 'art')))
            self.assertEqual(sorted(os.listdir(t)), ['data'])
            self.assertEqual([os.path.basename(c) for c in clashes], ['C.tga'] if case_sensitive else [])

    def test_raise_resolution(self):
        text = 'FITini\r\nl ResolutionX = 800\r\nl ResolutionY = 600\r\nl Other = 1\r\n'
        self.assertEqual(mi.raise_resolution(text),
                         'FITini\r\nl ResolutionX = 1024\r\nl ResolutionY = 768\r\nl Other = 1\r\n')
        high = 'l ResolutionX = 1920\nl ResolutionY = 1080\n'
        self.assertEqual(mi.raise_resolution(high), high)
        # stock's prefs files have neither line
        stock = 'FITini\r\n\r\n[MechCommander2]\r\nl DigitalMasterVolume = 192\r\nl Resolution = 0\r\n'
        self.assertEqual(mi.raise_resolution(stock),
                         'FITini\r\n\r\n[MechCommander2]\r\nl ResolutionX = 1024\r\nl ResolutionY = 768\r\n'
                         'l DigitalMasterVolume = 192\r\nl Resolution = 0\r\n')
        self.assertEqual(mi.raise_resolution('no block\n'), 'no block\n')

    def test_setup_folder(self):
        with tempfile.TemporaryDirectory() as t:
            folder = os.path.join(t, 'game')
            stock = os.path.join(t, 'stock')
            write(os.path.join(folder, 'prefs.cfg'), b'mc2x prefs')
            write(os.path.join(folder, 'options.cfg'), b'mc2x options')
            write(os.path.join(stock, 'prefs.cfg'), b'stock prefs')
            write(os.path.join(stock, 'minprefs.cfg'), b'l ResolutionX = 640\nl ResolutionY = 480\n')
            write(os.path.join(stock, 'assets', 'graphics', 'arial8.bmp'), b'font')
            write(os.path.join(folder, 'assets', 'binaries', 'blade.dll'), b'mc2x')
            engine = os.path.join(t, 'engine')
            write(os.path.join(engine, 'mc2'), b'elf')
            write(os.path.join(engine, 'libmc2res_64.so'), b'so')
            write(os.path.join(engine, 'readme.txt'), b'not copied')
            write(os.path.join(engine, 'shaders', 'a.frag'), b'glsl')
            mi.setup_folder(folder, stock, engine)
            with open(os.path.join(folder, 'mc2x-cfg', 'options.cfg'), 'rb') as f:
                self.assertEqual(f.read(), b'mc2x options')
            self.assertFalse(os.path.exists(os.path.join(folder, 'options.cfg')))
            with open(os.path.join(folder, 'prefs.cfg'), 'rb') as f:
                self.assertEqual(f.read(), b'stock prefs')
            with open(os.path.join(folder, 'minprefs.cfg'), 'rb') as f:
                self.assertEqual(f.read(), b'l ResolutionX = 1024\nl ResolutionY = 768\n')
            self.assertTrue(os.path.isfile(os.path.join(folder, 'assets', 'graphics', 'arial8.bmp')))
            self.assertTrue(os.path.isfile(os.path.join(folder, 'assets', 'binaries', 'blade.dll')))
            self.assertTrue(os.path.isfile(os.path.join(folder, 'mc2')))
            self.assertTrue(os.path.isfile(os.path.join(folder, 'shaders', 'a.frag')))
            self.assertFalse(os.path.exists(os.path.join(folder, 'readme.txt')))

    def test_engine_copy_does_not_write_through_links(self):
        with tempfile.TemporaryDirectory() as t:
            engine = os.path.join(t, 'engine')
            write(os.path.join(engine, 'mc2.exe'), b'new exe')
            write(os.path.join(engine, 'SDL2.dll'), b'new dll')
            base = os.path.join(t, 'base')
            write(os.path.join(base, 'SDL2.dll'), b'old dll')
            linked = os.path.join(t, 'linked')
            mi.link_or_copy_tree(base, linked)
            mi.copy_engine(engine, linked)
            with open(os.path.join(linked, 'SDL2.dll'), 'rb') as f:
                self.assertEqual(f.read(), b'new dll')
            with open(os.path.join(base, 'SDL2.dll'), 'rb') as f:
                self.assertEqual(f.read(), b'old dll')

    def test_add_own_files_keeps_shared_ones(self):
        with tempfile.TemporaryDirectory() as t:
            base = os.path.join(t, 'base')
            make_fst(os.path.join(base, 'art.fst'), [('data\\art\\shared.fit', b'base', 4)])
            write(os.path.join(base, 'data', 'missions', 'loose.fit'), b'base loose')
            overlay = os.path.join(t, 'overlay')
            make_fst(os.path.join(overlay, 'art.fst'), [('data\\art\\shared.fit', b'over', 4),
                                                        ('data\\art\\own.fit', b'own', 3)])
            write(os.path.join(overlay, 'data', 'missions', 'loose.fit'), b'over loose')
            write(os.path.join(overlay, 'data', 'missions', 'dg.fit'), b'dg')
            dest = os.path.join(t, 'dest')
            added = mi.add_own_files(mi.game_view(base), overlay, dest)
            self.assertEqual(added, 2)
            with open(os.path.join(dest, 'data', 'art', 'own.fit'), 'rb') as f:
                self.assertEqual(f.read(), b'own')
            self.assertTrue(os.path.isfile(os.path.join(dest, 'data', 'missions', 'dg.fit')))
            self.assertFalse(os.path.exists(os.path.join(dest, 'data', 'art', 'shared.fit')))
            self.assertFalse(os.path.exists(os.path.join(dest, 'data', 'missions', 'loose.fit')))

    def test_zip_install_and_unsafe_paths(self):
        with tempfile.TemporaryDirectory() as t:
            z = os.path.join(t, 'dg.zip')
            with zipfile.ZipFile(z, 'w') as w:
                w.writestr('Dark Gray/MC2X/data/missions/a.fit', b'a')
                w.writestr('Dark Gray/readme.txt', b'r')
            out = os.path.join(t, 'out')
            mi.extract_zip_install(z, out)
            self.assertTrue(os.path.isfile(os.path.join(out, 'data', 'missions', 'a.fit')))
            self.assertFalse(os.path.exists(os.path.join(out, 'readme.txt')))
            # a folder whose name only contains "mc2x" is not the install
            z2 = os.path.join(t, 'dg2.zip')
            with zipfile.ZipFile(z2, 'w') as w:
                w.writestr('wolfman-mc2x/MC2X/data/b.fit', b'b')
            out3 = os.path.join(t, 'out3')
            mi.extract_zip_install(z2, out3)
            self.assertTrue(os.path.isfile(os.path.join(out3, 'data', 'b.fit')))
            bad = os.path.join(t, 'bad.zip')
            with zipfile.ZipFile(bad, 'w') as w:
                w.writestr('MC2X/../../escape.txt', b'x')
            with self.assertRaises(mi.ImportError_):
                mi.extract_zip_install(bad, os.path.join(t, 'out2'))


class Build(unittest.TestCase):
    def test_folders_from_downloads(self):
        with tempfile.TemporaryDirectory() as t:
            downloads = os.path.join(t, 'dl')
            exe = os.path.join(downloads, 'setup.exe')
            os.makedirs(downloads)
            make_clickteam(exe, [('Data\\Missions\\M1.fit', b'm1', 1), ('Prefs.cfg', b'mc2x', 0)])
            write(os.path.join(downloads, 'notes.txt'), b'unrelated')
            zp = os.path.join(downloads, 'dg.zip')
            with zipfile.ZipFile(zp, 'w') as w:
                w.writestr('MC2X/data/missions/darkgray_1.fit', b'dg')
                w.writestr('MC2X/data/missions/m1.fit', b'dg version')
            stock = os.path.join(t, 'stock')
            write(os.path.join(stock, 'system.cfg'), b'sys')
            write(os.path.join(stock, 'prefs.cfg'), b'stock')
            known = [mi.Download('reap3', 'setup.exe', os.path.getsize(exe), 'm', 'clickteam', 'mc2x-reap3'),
                     mi.Download('darkgray', 'dg.zip', os.path.getsize(zp), 'm', 'zip', 'mc2x-darkgray')]
            saved_downloads, saved_md5 = mi.DOWNLOADS, mi.md5_of
            mi.DOWNLOADS, mi.md5_of = known, (lambda _p: 'm')
            try:
                out = os.path.join(t, 'out')
                log = io.StringIO()
                made = mi.build(downloads, stock, out, log=lambda s: log.write(s + '\n'))
                self.assertIn('skipped notes.txt', log.getvalue())
                with self.assertRaises(mi.ImportError_):
                    mi.build(downloads, stock, out, log=lambda s: None)
                mi.build(downloads, stock, out, force=True, log=lambda s: None)
            finally:
                mi.DOWNLOADS, mi.md5_of = saved_downloads, saved_md5
            reap3 = os.path.join(out, 'mc2x-reap3')
            dg = os.path.join(out, 'mc2x-darkgray')
            self.assertEqual(made, [reap3, dg])
            with open(os.path.join(reap3, 'data', 'missions', 'm1.fit'), 'rb') as f:
                self.assertEqual(f.read(), b'm1')
            with open(os.path.join(reap3, 'prefs.cfg'), 'rb') as f:
                self.assertEqual(f.read(), b'stock')
            # Dark Gray's own mission added, the shared one kept as MC2X's
            with open(os.path.join(dg, 'data', 'missions', 'darkgray_1.fit'), 'rb') as f:
                self.assertEqual(f.read(), b'dg')
            with open(os.path.join(dg, 'data', 'missions', 'm1.fit'), 'rb') as f:
                self.assertEqual(f.read(), b'm1')
            self.assertFalse(os.path.exists(os.path.join(out, '.mc2x-import')))


if __name__ == '__main__':
    unittest.main()
