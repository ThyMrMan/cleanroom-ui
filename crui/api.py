"""docs/api.md's reference, written from the sources: each Papyrus script's documented functions, the natives our
DLLs give every movie, and the AS3 library's public classes and functions. The page's own words stay outside the
markers; `python -m crui api` rewrites what is between them.

See: docs/api.md#reference
"""

import re
from pathlib import Path

from crui import REPO

PAGE = REPO / 'docs' / 'api.md'
START, END = '<!-- api:start -->', '<!-- api:end -->'
SCRIPTS = REPO / 'papyrus'
LIBRARY = REPO / 'as3' / 'crui'
DLL = REPO / 'dll'
#: A script's name and its doc, the {} after its ScriptName line.
SCRIPT_DOC = re.compile(r'^ScriptName (\w+)[^\n]*\n\{(.*?)\}', re.S)
#: A Papyrus function's signature line and its {doc} on the next line.
FUNCTION = re.compile(r'^((?:[\w:]+(?:\[\])? )?Function \w+\([^\n]*\)[^\n]*)\n\s*\{(.*?)\}', re.M | re.S)
#: A movie native: its handler class and the name it is registered under.
REGISTERED = re.compile(r'RegisterFunction<(\w+)>\(plugin, movie, "(\w+)"\)')
#: The `//` lines right above a declaration.
COMMENT = r'((?:[ \t]*// [^\n]*\n)+)'
#: An AS3 public function, the comment above it, its parameters and type.
AS3_FUNCTION = re.compile(COMMENT + r'\s*public (?:static )?function (?:get )?(\w+)\(([^)]*)\)\s*:\s*([\w.*]+)')
AS3_CLASS = re.compile(COMMENT + r'\s*public class (\w+)')


def _prose(comment: str) -> str:
    """`//` comment lines as one line of text."""
    return ' '.join(line.strip().removeprefix('//').strip() for line in comment.splitlines())


def _flat(text: str) -> str:
    """`text` on one line."""
    return ' '.join(text.split())


def papyrus_section(folder: Path = SCRIPTS) -> list:
    """Markdown lines for every script in `folder`: its doc, then each documented function."""
    out = ['## Papyrus', '']
    for path in sorted(folder.glob('*.psc')):
        source = path.read_text(encoding='utf-8')
        head = SCRIPT_DOC.match(source)
        out += [f'### {path.stem}', '', _flat(head.group(2)) if head else '', '']
        out += [f'- `{_flat(signature)}`: {_flat(doc)}' for signature, doc in FUNCTION.findall(source)]
        out.append('')
    return out


def natives_section(folder: Path = DLL) -> list:
    """Markdown lines for each native a DLL registers on movies (f4se.plugins.<DLL>), with its handler's comment."""
    out = ['## Movie natives', '']
    for path in sorted(folder.rglob('*.cpp')):
        if 'build' in path.parts:
            continue
        source = path.read_text(encoding='utf-8')
        for handler, name in REGISTERED.findall(source):
            comment = re.search(COMMENT + rf'class {handler}\b', source)
            out.append(f'- `{name}` ({path.relative_to(folder).as_posix()}): {_prose(comment.group(1)) if comment else ""}')
    return out + ['']


def library_section(folder: Path = LIBRARY) -> list:
    """Markdown lines for each class of the AS3 library: its comment, then each public function's."""
    out = ['## The AS3 library (`crui`)', '']
    for path in sorted(folder.glob('*.as')):
        source = path.read_text(encoding='utf-8')
        head = AS3_CLASS.search(source)
        out += [f'### {path.stem}', '', _prose(head.group(1)) if head else '', '']
        for comment, name, params, kind in AS3_FUNCTION.findall(source):
            out.append(f'- `{name}({_flat(params)}) : {kind}`: {_prose(comment)}')
        out.append('')
    return out


def reference() -> str:
    """The generated part of the page, markers included."""
    lines = [START, '', *papyrus_section(), *natives_section(), *library_section(), END]
    return '\n'.join(lines)


def write(page: Path = PAGE) -> bool:
    """Put reference() between the page's markers; whether the page changed. ValueError without both markers."""
    text = Path(page).read_text(encoding='utf-8')
    if START not in text or END not in text:
        raise ValueError(f'{page}: no {START} ... {END}')
    new = text[:text.index(START)] + reference() + text[text.index(END) + len(END):]
    if new != text:
        Path(page).write_text(new, encoding='utf-8')
    return new != text
