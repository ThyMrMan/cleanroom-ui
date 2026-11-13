"""Our movies' translation files: translations/<language>.txt (UTF-8) written as the files F4SE loads.

F4SE loads `Interface/Translations/<plugin>_<sLanguage>.txt` only for each plugin active in plugins.txt, so ours
are named after CleanroomUI.esl. Each source holds `$KEY<tab>text` lines; the installed file is UTF-16 LE with a
BOM and CRLF lines. Every game language gets a file, its missing keys in English, since F4SE has no fallback of
its own. A key's text may hold `{0}`, `{1}` slots, which crui's Text.fill fills.

See: docs/swf_framework.md#translations
"""

import re
from pathlib import Path

from crui import REPO

#: The UTF-8 sources, one `<language>.txt` per language.
SOURCES = REPO / 'translations'
#: The plugin the files are named after.
PLUGIN = 'CleanroomUI'
#: F4SE's line buffer, in UTF-16 units: a longer line is cut.
LINE_LIMIT = 511
#: Fallout 4's sLanguage values (its own Interface/translate_<language>.txt files).
LANGUAGES = ('cn', 'de', 'en', 'es', 'esmx', 'fr', 'it', 'ja', 'pl', 'ptbr', 'ru')
#: The code that shows our keys: the AS3 library, every menu and our DLLs.
CODE = ((REPO / 'as3', '**/*.as'), (REPO / 'menus', '**/*.as'), (REPO / 'dll', '**/*.cpp'))
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
    """Write <data_dir>/Interface/Translations/<PLUGIN>_<language>.txt for every game language (English under the
    language's own source); how many were written."""
    english = entries(Path(sources) / 'en.txt')
    folder = Path(data_dir) / 'Interface' / 'Translations'
    folder.mkdir(parents=True, exist_ok=True)
    for language in LANGUAGES:
        own = Path(sources) / f'{language}.txt'
        table = {**english, **(entries(own) if own.is_file() else {})}
        (folder / f'{PLUGIN}_{language}.txt').write_bytes(encoded(table))
    return len(LANGUAGES)


def used_keys(code=CODE) -> dict:
    """{key: [files]} of the `"$CRUI_KEY"` strings in `code` ((folder, glob) pairs), `//` comment lines left out."""
    out = {}
    for folder, pattern in code:
        for path in sorted(Path(folder).glob(pattern)):
            lines = path.read_text(encoding='utf-8').splitlines()
            text = '\n'.join(line for line in lines if not line.lstrip().startswith('//'))
            for key in KEY_LITERAL.findall(text):
                out.setdefault(key, []).append(path.name)
    return out
