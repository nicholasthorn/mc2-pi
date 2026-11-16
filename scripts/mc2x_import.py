#!/usr/bin/env python3
"""Build game folders for this port from wolfman-MC2X downloads.

MC2X is free for non-commercial use and may not be redistributed, so its
data is never in this repository, in CI or in a release. Players download it
from ModDB themselves; this script turns those downloads into game folders
for this port. It never runs an installer: MC2X's are Clickteam Install
Creator executables, read here as data.

usage: mc2x_import.py --downloads DIR --stock-game-dir DIR --out DIR
                      [--engine DIR] [--force]

  --downloads       folder holding the downloads (any of those listed below;
                    others are reported and left alone)
  --stock-game-dir  a stock game folder (scripts/build_gamedata.sh): its
                    config files replace MC2X's, whose blocks are renamed
  --out             where the game folders go: mc2x-r70, mc2x-reap3, and
                    mc2x-darkgray (Dark Gray's own files added to a copy of
                    an MC2X folder, which it needs)
  --engine          an unpacked release package of this port (or any folder
                    with mc2 / mc2.exe, its resource library and shaders/):
                    copied into each folder
  --force           replace game folders that are already there

Each file is recognised by its size and checked against its MD5, so a
damaged or unknown file is refused. See planning/future-features/
MC2X-COMPAT.md for what was tested with each.
"""

import argparse
import bz2
import hashlib
import os
import shutil
import struct
import sys
import zipfile
import zlib
from collections import namedtuple

Download = namedtuple('Download', 'key name size md5 kind folder')

# The downloads this was tested with (ModDB's MD5s).
DOWNLOADS = [
    Download('r70', 'MC2X.1.exe', 1005169355,
             'de0f450cab1f4382f437e9824ab62a9d', 'clickteam', 'mc2x-r70'),
    Download('reap3', 'MC2X-REAP3-G.4.exe', 1077278503,
             'ae9d81407f875699207d1b1e5083e6b9', 'clickteam', 'mc2x-reap3'),
    Download('darkgray', 'mc2_DarkGray_Borderworlds_v1.4.zip', 441880474,
             '6019fe37e48b9ff8d660ef01ab9d1458', 'zip', 'mc2x-darkgray'),
]

# The port's config files, from a stock game folder; MC2X's own use
# renamed blocks the game does not read.
STOCK_FILES = ['system.cfg', 'prefs.cfg', 'orgprefs.cfg',
               'minprefs.cfg', 'midprefs.cfg', 'hiprefs.cfg', 'h2prefs.cfg',
               'minprefs1.cfg', 'midprefs1.cfg', 'hiprefs1.cfg', 'h2prefs1.cfg',
               'testtxm.tga', 'sniff.dat']
MC2X_CONFIG = ['prefs.cfg', 'options.cfg', 'options.cfg.old']

# MC2X has no interface layout under 1024 wide.
MIN_RESOLUTION = (1024, 768)

# The port's engine files in a package or build: copied into each folder,
# and never hard-linked between folders
ENGINE_FILES = ('mc2', 'mc2.exe', 'libmc2res_64.so', 'mc2res_64.dll', 'mc2res.dll')

# The folders Dark Gray's own files can be added to, in order of preference
# (it was tested on REAP3-G's)
DARKGRAY_BASES = ('mc2x-reap3', 'mc2x-r70')

class Mc2xImportError(Exception):
    pass


# --------------------------------------------------------------------------
# Recognising downloads

def md5_of(path, chunk=1 << 22):
    h = hashlib.md5()
    with open(path, 'rb') as f:
        while True:
            b = f.read(chunk)
            if not b:
                return h.hexdigest()
            h.update(b)


def identify(path, known=None, md5=None):
    """The Download a file is, None if its size matches none, or
    Mc2xImportError if its size matches one but its MD5 does not."""
    known = DOWNLOADS if known is None else known
    md5 = md5_of if md5 is None else md5
    size = os.path.getsize(path)
    for d in known:
        if d.size == size:
            if md5(path) != d.md5:
                raise Mc2xImportError('%s: the size of %s but not its MD5 (damaged download?)'
                                   % (os.path.basename(path), d.name))
            return d
    return None


# --------------------------------------------------------------------------
# Clickteam Install Creator: a PE executable with the installer's data
# appended. After the last section: the signature, then blocks of
# (u16 id, u16, u32 size, data). Block 0x143a is the file list (u32 size,
# u8 compression, stream); block 0x7f7f the file data, each file a
# compression byte (0 stored, 1 zlib, 2 bzip2) and its stream.

CLICKTEAM_SIGNATURE = bytes.fromhex('777767542948')
BLOCK_FILE_LIST = 0x143a
BLOCK_DATA = 0x7f7f


def _overlay_offset(head):
    if head[:2] != b'MZ':
        raise Mc2xImportError('not an executable')
    pe = struct.unpack_from('<I', head, 0x3c)[0]
    if head[pe:pe + 4] != b'PE\0\0':
        raise Mc2xImportError('not a PE executable')
    sections = struct.unpack_from('<H', head, pe + 6)[0]
    optional = struct.unpack_from('<H', head, pe + 20)[0]
    end = 0
    for i in range(sections):
        o = pe + 24 + optional + 40 * i
        raw_size, raw_pointer = struct.unpack_from('<II', head, o + 16)
        end = max(end, raw_pointer + raw_size)
    return end


def _decompressor(kind):
    """A decompressor for a Clickteam compression byte; None for stored."""
    if kind == 0:
        return None
    if kind == 1:
        return zlib.decompressobj()
    if kind == 2:
        return bz2.BZ2Decompressor()
    raise Mc2xImportError('unknown compression %d' % kind)


def clickteam_files(path):
    """(name, offset, compressed size, size) of every file in the
    installer, the offset into the file; name with "/" separators."""
    with open(path, 'rb') as f:
        head = f.read(4 * 1024 * 1024)
        pos = _overlay_offset(head)
        f.seek(pos)
        if f.read(len(CLICKTEAM_SIGNATURE)) != CLICKTEAM_SIGNATURE:
            raise Mc2xImportError('not a Clickteam installer')
        pos += len(CLICKTEAM_SIGNATURE)
        end = os.fstat(f.fileno()).st_size
        blocks = {}
        while pos + 8 <= end:
            f.seek(pos)
            block, _unused, size = struct.unpack('<HHI', f.read(8))
            blocks[block] = (pos + 8, size)
            pos += 8 + size
        if BLOCK_FILE_LIST not in blocks or BLOCK_DATA not in blocks:
            raise Mc2xImportError('no file list in the installer')
        list_pos, list_size = blocks[BLOCK_FILE_LIST]
        f.seek(list_pos)
        raw = f.read(list_size)
        d = _decompressor(raw[4])
        file_list = raw[5:] if d is None else d.decompress(raw[5:])
    data_base = blocks[BLOCK_DATA][0] + 4
    count = struct.unpack_from('<I', file_list, 0)[0]
    p = 4
    files = []
    for _ in range(count):
        size, kind = struct.unpack_from('<HH', file_list, p)
        entry = file_list[p:p + size]
        p += size
        if kind != 0:
            continue  # not a file (shortcuts, registry entries)
        offset, csize, _z, usize, _crc = struct.unpack_from('<IIIII', entry, 6)
        name = entry[62:].split(b'\0')[0].decode('cp1252')
        files.append((name.replace('\\', '/'), data_base + offset, csize, usize))
    return files



def extract_clickteam(path, out, chunk=1 << 22):
    """Every file of the installer into out, a chunk at a time (its fast
    files run to hundreds of MB)."""
    with open(path, 'rb') as f:
        for name, offset, csize, usize in clickteam_files(path):
            f.seek(offset)
            d = _decompressor(f.read(1)[0])
            left = csize - 1
            written = 0
            with open(_target(out, name), 'wb') as w:
                while left > 0:
                    raw = f.read(min(chunk, left))
                    if not raw:
                        break
                    left -= len(raw)
                    data = raw if d is None else d.decompress(raw)
                    w.write(data)
                    written += len(data)
            if written != usize:
                raise Mc2xImportError('%s: %s decoded to %d bytes, not %d'
                                   % (os.path.basename(path), name, written, usize))


# --------------------------------------------------------------------------
# Fast files (.fst): u32 version, u32 count, then per file u32 offset,
# u32 stored size, u32 size, u32 hash, char[250] name; each file stored,
# zlib, or the game's 12-bit LZW.

def _lzw(src, size):
    chain = [0] * 4096
    suffix = [0] * 4096
    out = bytearray()
    pos = buf = count = 0
    bits, limit, free = 9, 512, 258
    started = False
    old = old_first = 0
    while True:
        while count < bits:
            if pos >= len(src):
                return None
            buf |= src[pos] << count
            pos += 1
            count += 8
        code = buf & ((1 << bits) - 1)
        buf >>= bits
        count -= bits
        if code == 257:
            return bytes(out)
        if code == 256:
            bits, limit, free, started = 9, 512, 258, False
            continue
        if not started:
            if code > 255:
                return None
            out.append(code)
            old = old_first = code
            started = True
            continue
        if code > free:
            return None
        stack = []
        c = code
        if code == free:
            stack.append(old_first)
            c = old
        while c >= 258:
            stack.append(suffix[c])
            c = chain[c]
        first = c
        stack.append(first)
        stack.reverse()
        out += bytes(stack)
        if len(out) > size:
            return None
        if free < 4096:
            chain[free] = old
            suffix[free] = first
            free += 1
        if free >= limit and bits < 12:
            bits += 1
            limit <<= 1
        old = code
        old_first = first


def fst_files(path):
    """(name, offset, stored size, size) of every file in a fast file,
    name lower case with "/" separators."""
    with open(path, 'rb') as f:
        _version, count = struct.unpack('<II', f.read(8))
        files = []
        for _ in range(count):
            offset, stored, size, _hash = struct.unpack('<IIII', f.read(16))
            name = f.read(250).split(b'\0')[0].decode('cp1252')
            files.append((name.replace('\\', '/').lower(), offset, stored, size))
    return files


def fst_data(f, offset, stored, size):
    f.seek(offset)
    raw = f.read(stored)
    if stored == size:
        return raw
    if raw[:1] == b'\x78':
        try:
            return zlib.decompress(raw)
        except zlib.error:
            pass
    data = _lzw(raw, size)
    if data is None or len(data) != size:
        raise Mc2xImportError('%s: a file does not decode' % os.path.basename(f.name))
    return data


def game_view(folder):
    """What the game sees in a folder: lower-case path -> ('disk', path)
    for loose files under data/, ('fst', fst path, offset, stored, size)
    for files in its fast files. Loose files win, as in the game."""
    view = {}
    for fst in sorted(os.listdir(folder)):
        if fst.lower().endswith('.fst'):
            full = os.path.join(folder, fst)
            for name, offset, stored, size in fst_files(full):
                view[name] = ('fst', full, offset, stored, size)
    data = os.path.join(folder, 'data')
    for root, _dirs, files in os.walk(data):
        for n in files:
            full = os.path.join(root, n)
            view[os.path.relpath(full, folder).replace(os.sep, '/').lower()] = ('disk', full)
    return view


# --------------------------------------------------------------------------
# Game folders

def _target(out, name):
    """out/name with its folders made; a name that would leave out (a ".."
    part, a drive or an absolute path) is refused."""
    parts = name.split('/')
    if any(p in ('', '.', '..') or ':' in p for p in parts) or os.path.isabs(name):
        raise Mc2xImportError('unsafe file name %r' % name)
    dst = os.path.join(out, *parts)
    os.makedirs(os.path.dirname(dst), exist_ok=True)
    return dst


def _write(out, name, data):
    with open(_target(out, name), 'wb') as w:
        w.write(data)


def lowercase_tree(root):
    """Lower-cases every name under root, deepest first. Returns the paths
    whose lower-case name was already taken (left as they are)."""
    clashes = []
    for parent, dirs, files in os.walk(root, topdown=False):
        # names as listed, not os.path.exists: on a case-insensitive file
        # system the file itself would count as taking its lower-case name
        taken = set(os.listdir(parent))
        for n in files + dirs:
            lower = n.lower()
            if lower == n:
                continue
            if lower in taken:
                clashes.append(os.path.join(parent, n))
                continue
            os.rename(os.path.join(parent, n), os.path.join(parent, lower))
            taken.discard(n)
            taken.add(lower)
    return clashes


def raise_resolution(text, minimum=MIN_RESOLUTION):
    """A prefs file's text with ResolutionX / ResolutionY raised to at
    least minimum, line endings kept. Stock's prefs files have neither (the
    game then takes 800x600): they are added under [MechCommander2]."""
    keys = (('ResolutionX', minimum[0]), ('ResolutionY', minimum[1]))
    least_of = dict(keys)
    lines = []
    seen = set()
    for line in text.splitlines(True):
        body = line.rstrip('\r\n')
        ending = line[len(body):]
        words = body.split()
        if len(words) == 4 and words[0] == 'l' and words[1] in least_of and words[2] == '=':
            key = words[1]
            seen.add(key)
            if words[3].isdigit() and int(words[3]) < least_of[key]:
                body = 'l %s = %d' % (key, least_of[key])
        lines.append([body, ending])
    for i, (body, ending) in enumerate(lines):
        if body.strip() == '[MechCommander2]':
            added = [['l %s = %d' % (key, least), ending or '\n'] for key, least in keys if key not in seen]
            lines[i + 1:i + 1] = added
            break
    return ''.join(body + ending for body, ending in lines)


def setup_folder(folder, stock_dir, engine_dir):
    os.makedirs(os.path.join(folder, 'mc2x-cfg'), exist_ok=True)
    for n in MC2X_CONFIG:
        src = os.path.join(folder, n)
        if os.path.exists(src):
            os.replace(src, os.path.join(folder, 'mc2x-cfg', n))
    for n in STOCK_FILES:
        src = os.path.join(stock_dir, n)
        if os.path.exists(src):
            shutil.copyfile(src, os.path.join(folder, n))
    # the port's own assets (its fonts, assets/graphics) over MC2X's
    assets = os.path.join(stock_dir, 'assets')
    if os.path.isdir(assets):
        shutil.copytree(assets, os.path.join(folder, 'assets'), dirs_exist_ok=True)
    for n in os.listdir(folder):
        if n.endswith('prefs.cfg') or n.endswith('prefs1.cfg'):
            p = os.path.join(folder, n)
            with open(p, 'r', encoding='latin-1', newline='') as f:
                text = f.read()
            raised = raise_resolution(text)
            if raised != text:
                with open(p, 'w', encoding='latin-1', newline='') as f:
                    f.write(raised)
    if engine_dir:
        copy_engine(engine_dir, folder)


def copy_engine(engine_dir, folder):
    """The engine files of a package or build (and, for Windows, its DLLs)
    and its shaders/ into folder."""
    windows = os.path.isfile(os.path.join(engine_dir, 'mc2.exe'))
    if not windows and not os.path.isfile(os.path.join(engine_dir, 'mc2')):
        raise Mc2xImportError('%s has no mc2 or mc2.exe' % engine_dir)
    for n in os.listdir(engine_dir):
        src = os.path.join(engine_dir, n)
        if os.path.isfile(src) and (n in ENGINE_FILES or (windows and n.lower().endswith('.dll'))):
            dst = os.path.join(folder, n)
            if os.path.exists(dst):
                # a new file, not a write through a hard link into another folder
                os.remove(dst)
            shutil.copy2(src, dst)
    shaders = os.path.join(engine_dir, 'shaders')
    if os.path.isdir(shaders):
        dst = os.path.join(folder, 'shaders')
        shutil.rmtree(dst, ignore_errors=True)
        shutil.copytree(shaders, dst)


def link_or_copy_tree(src, dst):
    """A copy of src at dst, hard-linked where the file system allows."""
    def link(s, d):
        try:
            os.link(s, d)
        except OSError:
            shutil.copy2(s, d)
    shutil.copytree(src, dst, copy_function=link)


def add_own_files(base_view, overlay, dest):
    """Puts into dest every data file the game sees in overlay but not in
    base_view (Dark Gray's missions, campaign and art): the MC2X folder's
    shared files are kept. Loose files are moved out of overlay, which is a
    staging folder. Returns how many were added."""
    own = [(name, where) for name, where in game_view(overlay).items()
           if name.startswith('data/') and name not in base_view]
    handles = {}
    try:
        # fast-file entries in file order, not name order
        for name, where in sorted(own, key=lambda e: (e[1][1], e[1][2] if e[1][0] == 'fst' else 0)):
            if where[0] == 'disk':
                os.replace(where[1], _target(dest, name))
                continue
            fst = handles.get(where[1])
            if fst is None:
                fst = handles[where[1]] = open(where[1], 'rb')
            _write(dest, name, fst_data(fst, *where[2:]))
    finally:
        for h in handles.values():
            h.close()
    return len(own)


def extract_zip_install(path, out):
    """Dark Gray's zip holds a whole MC2X install in a folder named mc2x:
    of it, what the game reads data from (its fast files and data/) goes
    to out, a file at a time."""
    with zipfile.ZipFile(path) as z:
        names = [n for n in z.namelist() if not n.endswith('/')]
        prefixes = set()
        for n in names:
            parts = n.split('/')
            for i, p in enumerate(parts[:-1]):
                if p.lower() == 'mc2x':
                    prefixes.add('/'.join(parts[:i + 1]) + '/')
                    break
        if not prefixes:
            raise Mc2xImportError('%s: no mc2x folder in it' % os.path.basename(path))
        prefix = sorted(prefixes, key=len)[0]
        for n in names:
            if not n.startswith(prefix):
                continue
            rest = n[len(prefix):]
            top_fst = '/' not in rest and rest.lower().endswith('.fst')
            if top_fst or rest.lower().startswith('data/'):
                with z.open(n) as src, open(_target(out, rest), 'wb') as dst:
                    shutil.copyfileobj(src, dst, 1 << 22)


def find_downloads(downloads, log):
    """{Download: path} of the known downloads in a folder."""
    found = {}
    for n in sorted(os.listdir(downloads)):
        p = os.path.join(downloads, n)
        if not os.path.isfile(p):
            continue
        d = identify(p)
        if d is None:
            log('skipped %s: not a download this script knows' % n)
            continue
        log('found %s (%s)' % (n, d.key))
        found[d] = p
    if not found:
        raise Mc2xImportError('no known download in %s' % downloads)
    return found


def _clear(folder, force):
    if os.path.exists(folder):
        if not force:
            raise Mc2xImportError('%s is already there (--force replaces it)' % folder)
        shutil.rmtree(folder)


def build(downloads, stock_dir, out, engine_dir=None, force=False, log=print):
    found = find_downloads(downloads, log)
    os.makedirs(out, exist_ok=True)
    work = os.path.join(out, '.mc2x-import')
    shutil.rmtree(work, ignore_errors=True)
    made = []
    try:
        for d in DOWNLOADS:
            if d not in found or d.kind != 'clickteam':
                continue
            folder = os.path.join(out, d.folder)
            _clear(folder, force)
            log('unpacking %s into %s' % (d.name, folder))
            staging = os.path.join(work, d.key)
            extract_clickteam(found[d], staging)
            clashes = lowercase_tree(staging)
            for c in clashes:
                log('  name clash, left as it is: %s' % c)
            setup_folder(staging, stock_dir, engine_dir)
            os.replace(staging, folder)
            made.append(folder)
        for d in found:
            if d.kind != 'zip':
                continue
            bases = [os.path.join(out, b) for b in DARKGRAY_BASES]
            base = next((b for b in bases if os.path.isdir(b)), None)
            if base is None:
                raise Mc2xImportError('Dark Gray needs an MC2X folder to add its files to: '
                                      'download MC2X R70 or REAP3-G as well')
            folder = os.path.join(out, d.folder)
            _clear(folder, force)
            log('unpacking %s and adding its own files to a copy of %s' % (d.name, base))
            staging = os.path.join(work, 'darkgray-install')
            extract_zip_install(found[d], staging)
            target = os.path.join(work, 'darkgray')
            link_or_copy_tree(base, target)
            for n in ENGINE_FILES + ('options.cfg',):
                p = os.path.join(target, n)
                if os.path.isfile(p):
                    # a later copy into the folder must not write through a link
                    os.remove(p)
            added = add_own_files(game_view(base), staging, target)
            log('  %d files added' % added)
            if engine_dir:
                copy_engine(engine_dir, target)
            os.replace(target, folder)
            made.append(folder)
    finally:
        shutil.rmtree(work, ignore_errors=True)
    return made


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('--downloads', required=True)
    ap.add_argument('--stock-game-dir', required=True)
    ap.add_argument('--out', required=True)
    ap.add_argument('--engine')
    ap.add_argument('--force', action='store_true')
    args = ap.parse_args(argv)
    if not os.path.isfile(os.path.join(args.stock_game_dir, 'system.cfg')):
        print('mc2x_import: %s is not a stock game folder (no system.cfg)' % args.stock_game_dir,
              file=sys.stderr)
        return 2
    try:
        made = build(args.downloads, args.stock_game_dir, args.out, args.engine, args.force)
    except Mc2xImportError as e:
        print('mc2x_import: %s' % e, file=sys.stderr)
        return 1
    for m in made:
        print('ready: %s' % m)
    return 0


if __name__ == '__main__':
    sys.exit(main())
