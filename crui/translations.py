"""Our translation files: translations/<release>/<language>.txt (UTF-8) written as the files F4SE reads.

Each release has its own file, `Interface/Translations/<release>_<sLanguage>.txt`. F4SE loads one per active
plugin, so CleanroomUI's is named after CleanroomUI.esl; the dialogue release ships no plugin, so
CleanroomDialogue.dll loads its own file through F4SE's reader. Each source holds `$KEY<tab>text` lines; the
installed file is UTF-16 LE with a BOM and CRLF lines. Every game language gets a file, its missing keys in
English, since F4SE has no fallback of its own. A key's text may hold `{0}`, `{1}` slots, which crui's Text.fill
fills.

See: docs/swf_framework.md#translations
"""

import re
from pathlib import Path

from crui import REPO

#: The UTF-8 sources: a folder per release, one `<language>.txt` each.
SOURCES = REPO / 'translations'
#: The framework's release, its file named after CleanroomUI.esl.
PLUGIN = 'CleanroomUI'
#: The dialogue release, its file loaded by CleanroomDialogue.dll.
DIALOGUE = 'CleanroomDialogue'
RELEASES = (PLUGIN, DIALOGUE)
#: The code only the dialogue release ships; every other file belongs to CleanroomUI, the AS3 library to both.
DIALOGUE_CODE = (REPO / 'menus' / 'dialogue', REPO / 'dll' / 'dialogue')
LIBRARY = REPO / 'as3'
#: F4SE's line buffer, in UTF-16 units: a longer line is cut.
LINE_LIMIT = 511
#: Fallout 4's sLanguage values (its own Interface/translate_<language>.txt files).
LANGUAGES = ('cn', 'de', 'en', 'es', 'esmx', 'fr', 'it', 'ja', 'pl', 'ptbr', 'ru')
#: The code that shows our keys: the AS3 library, every menu and our DLLs.
CODE = ((LIBRARY, '**/*.as'), (REPO / 'menus', '**/*.as'), (REPO / 'dll', '**/*.cpp'))
#: A `"$CRUI_KEY"` string literal, or a template's `"$CRUI_KEY\t`.
KEY_LITERAL = re.compile(r'"(\$CRUI_[A-Z0-9_]+)(?=["\\])')


def entries(source: Path) -> dict:
    """{key: text} of `source`; ValueError on a line that is not `$KEY<tab>text`, or too long."""
    out = {}
    for number, line in enumerate(Path(source).read_text(encoding='utf-8').splitlines(), 1):
        key, tab, text = line.partition('\t')
        if not (key.startswith('$') and len(key) > 1 and tab and text) or '\t' in text:
            raise ValueError(f'{source}:{number}: not a $KEY<tab>text line')
        if len(line.encode('utf-16-le')) // 2 > LINE_LIMIT:
            raise ValueError(f'{source}:{number}: longer than F4SE reads ({LINE_LIMIT} characters)')
        out[key] = text
    return out


def encoded(table: dict) -> bytes:
    """`table` as an F4SE file: BOM, UTF-16 LE, a CRLF line per key."""
    return ('﻿' + ''.join(f'{key}\t{text}\r\n' for key, text in table.items())).encode('utf-16-le')


def install(data_dir: Path, sources: Path = SOURCES) -> int:
    """Write <data_dir>/Interface/Translations/<release>_<language>.txt for every release and game language
    (English under the language's own source); how many were written."""
    folder = Path(data_dir) / 'Interface' / 'Translations'
    folder.mkdir(parents=True, exist_ok=True)
    for release in RELEASES:
        english = entries(Path(sources) / release / 'en.txt')
        for language in LANGUAGES:
            own = Path(sources) / release / f'{language}.txt'
            table = {**english, **(entries(own) if own.is_file() else {})}
            (folder / f'{release}_{language}.txt').write_bytes(encoded(table))
    return len(RELEASES) * len(LANGUAGES)


def releases_of(path: Path) -> tuple:
    """The releases that ship code file `path`: the dialogue release its own, CleanroomUI the rest, both the
    library."""
    if Path(path).is_relative_to(LIBRARY):
        return RELEASES
    return (DIALOGUE,) if any(Path(path).is_relative_to(folder) for folder in DIALOGUE_CODE) else (PLUGIN,)


def used_keys(release: str = PLUGIN, code=CODE) -> dict:
    """{key: [files]} of the `"$CRUI_KEY"` strings in the files of `code` ((folder, glob) pairs) that `release`
    ships, `//` comment lines left out."""
    out = {}
    for folder, pattern in code:
        for path in sorted(Path(folder).glob(pattern)):
            if release not in releases_of(path):
                continue
            lines = path.read_text(encoding='utf-8').splitlines()
            text = '\n'.join(line for line in lines if not line.lstrip().startswith('//'))
            for key in KEY_LITERAL.findall(text):
                out.setdefault(key, []).append(path.name)
    return out
