"""The movie check: every movie we build holds only classes from our sources, imports nothing from another movie,
names only fonts the game's FontConfig.txt maps, and builds byte-identical twice.

See: docs/swf_framework.md#testing
"""

import re
import struct
import tempfile
import zlib
from pathlib import Path

from crui import menus, mxmlc
from crui.swf import Swf

#: DoABC2; ImportAssets and ImportAssets2 (a tag that pulls characters from another movie, such as a Bethesda one).
DO_ABC, IMPORTS = 82, (57, 71)
#: The archive holding the game's FontConfig.txt, under Data.
INTERFACE_BA2, FONT_CONFIG = 'Fallout4 - Interface.ba2', 'interface\\fontconfig.txt'
MAPPED = re.compile(r'^map "(\$[^"]+)"', re.M)
OUR_KEY = re.compile(r'^\$CRUI_[A-Z0-9_]+$')
#: A `$` name at the start of a string (a key's arguments follow a tab).
DOLLAR = re.compile(r'^\$\w+')
PACKAGE = re.compile(r'^\s*package\s*([\w.]*)', re.M)


class _Reader:
    """A cursor over ABC bytes: u8, u30 (variable length) and length-prefixed strings."""

    def __init__(self, data: bytes, at: int = 0):
        self.data, self.at = data, at

    def u8(self) -> int:
        """One byte."""
        self.at += 1
        return self.data[self.at - 1]

    def u30(self) -> int:
        """A variable-length unsigned integer (7 bits a byte, low first)."""
        value, shift = 0, 0
        while True:
            byte = self.u8()
            value |= (byte & 0x7F) << shift
            if not byte & 0x80:
                return value
            shift += 7

    def skip(self, count: int) -> None:
        """`count` u30s, read and dropped."""
        for _ in range(count):
            self.u30()

    def string(self) -> str:
        """A u30 length, then that many UTF-8 bytes."""
        size = self.u30()
        self.at += size
        return self.data[self.at - size:self.at].decode('utf-8', 'replace')


def _multiname(r: _Reader) -> tuple:
    """One multiname: (namespace index, name index) for a QName, else (None, name index or 0)."""
    kind = r.u8()
    if kind in (0x07, 0x0D):
        return r.u30(), r.u30()
    if kind in (0x0F, 0x10):
        return None, r.u30()
    if kind in (0x09, 0x0E):
        name = r.u30()
        r.u30()
        return None, name
    if kind in (0x1B, 0x1C):
        r.u30()
    elif kind == 0x1D:
        r.u30()
        r.skip(r.u30())
    return None, 0


def _pool(r: _Reader) -> tuple:
    """The constant pool: (strings, namespace name indices, multinames); index 0 of each is the empty entry."""
    r.skip(max(r.u30() - 1, 0))
    r.skip(max(r.u30() - 1, 0))
    doubles = max(r.u30() - 1, 0)
    r.at += 8 * doubles
    strings = [''] + [r.string() for _ in range(max(r.u30() - 1, 0))]
    namespaces = [0]
    for _ in range(max(r.u30() - 1, 0)):
        r.u8()
        namespaces.append(r.u30())
    for _ in range(max(r.u30() - 1, 0)):
        r.skip(r.u30())
    multinames = [(None, 0)] + [_multiname(r) for _ in range(max(r.u30() - 1, 0))]
    return strings, namespaces, multinames


def _skip_methods_and_metadata(r: _Reader) -> None:
    """The method signatures and metadata entries, read past."""
    for _ in range(r.u30()):
        params = r.u30()
        r.skip(1 + params + 1)
        flags = r.u8()
        if flags & 0x08:
            for _ in range(r.u30()):
                r.u30()
                r.u8()
        if flags & 0x80:
            r.skip(params)
    for _ in range(r.u30()):
        r.u30()
        r.skip(2 * r.u30())


def _skip_traits(r: _Reader) -> None:
    """One trait list, read past."""
    for _ in range(r.u30()):
        r.u30()
        kind = r.u8()
        if kind & 0x0F in (0, 6):
            r.skip(2)
            if r.u30():
                r.u8()
        else:
            r.skip(2)
        if kind >> 4 & 0x04:
            r.skip(r.u30())


def abc_contents(abc: bytes) -> tuple:
    """(the classes `abc` defines, as 'package.Class', and every string in its constant pool)."""
    r = _Reader(abc, 4)
    strings, namespaces, multinames = _pool(r)
    _skip_methods_and_metadata(r)
    classes = []
    for _ in range(r.u30()):
        ns, name = multinames[r.u30()]
        package = strings[namespaces[ns]] if ns else ''
        classes.append(f'{package}.{strings[name]}' if package else strings[name])
        r.u30()
        if r.u8() & 0x08:
            r.u30()
        r.skip(r.u30())
        r.u30()
        _skip_traits(r)
    return classes, strings


def movie_contents(raw: bytes) -> tuple:
    """(the classes movie `raw` defines, the strings its code holds, the codes of its tags)."""
    tags = Swf.parse(raw).tags
    classes, strings = [], set()
    for tag in tags:
        if tag.code == DO_ABC:
            payload = tag.data
            name_end = payload.index(b'\0', 4)
            found, pool = abc_contents(payload[name_end + 1:])
            classes += found
            strings |= set(pool)
    return classes, strings, [tag.code for tag in tags]


def source_classes(menu: menus.Menu) -> set:
    """Every class `menu`'s sources and our AS3 library declare, as 'package.Class'."""
    out = set()
    for folder in (menu.sources, mxmlc.LIBRARY):
        for source in folder.rglob('*.as'):
            package = PACKAGE.search(source.read_text(encoding='utf-8'))
            name = package.group(1) if package else ''
            out.add(f'{name}.{source.stem}' if name else source.stem)
    return out


def font_names(game_data: Path) -> 'set | None':
    """The font names the game's FontConfig.txt maps, read from its Interface archive; None without it."""
    archive = game_data / INTERFACE_BA2
    if not archive.is_file():
        return None
    data = archive.read_bytes()
    count, table = struct.unpack_from('<IQ', data, 12)
    at = table
    for i in range(count):
        size, = struct.unpack_from('<H', data, at)
        name = data[at + 2:at + 2 + size].decode('latin-1')
        at += 2 + size
        if name.lower() == FONT_CONFIG:
            offset, packed, length = struct.unpack_from('<QII', data, 24 + 36 * i + 16)
            body = data[offset:offset + (packed or length)]
            return set(MAPPED.findall((zlib.decompress(body) if packed else body).decode('latin-1')))
    return None


def problems(menu: menus.Menu, raw: bytes, again: bytes, fonts: 'set | None') -> list:
    """What breaks the movie check in `menu`'s build `raw` (`again` its second build); empty when it passes."""
    classes, strings, codes = movie_contents(raw)
    out = [f'class {name} is not from our sources' for name in sorted(set(classes) - source_classes(menu))]
    out += [f'tag {code} imports another movie\'s assets' for code in sorted(set(codes) & set(IMPORTS))]
    if fonts is not None:
        names = {DOLLAR.match(s).group(0) for s in strings if DOLLAR.match(s)}
        out += [f'{name} is neither our key nor a font FontConfig.txt maps' for name in sorted(names)
                if not OUR_KEY.match(name) and name not in fonts]
    if raw != again:
        out.append('two builds differ')
    return out


def check(game_data: 'Path | None') -> bool:
    """Run the movie check on every menu, printing each problem; True when all pass. Needs the Flex SDK."""
    if not mxmlc.available():
        print(f'[check] the Flex SDK is not at {mxmlc.FLEX}; set CRUI_FLEX')
        return False
    fonts = font_names(game_data) if game_data else None
    if fonts is None:
        print('[check] no game Data folder with FontConfig.txt (--game); font names not checked')
    ok = True
    with tempfile.TemporaryDirectory() as tmp:
        for menu in menus.MENUS:
            builds = [menus.compile_movie(menu, Path(tmp) / f'{menu.name}{n}.swf').read_bytes() for n in (1, 2)]
            found = problems(menu, *builds, fonts)
            print(f'[check] {menu.movie}: ' + ('ok' if not found else '; '.join(found)))
            ok = ok and not found
    return ok
