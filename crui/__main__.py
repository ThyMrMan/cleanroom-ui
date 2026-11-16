"""Build Cleanroom UI into a mod folder: CleanroomUI.esl, Interface/ (movies and Translations/), Scripts/
(Papyrus), F4SE/Plugins/ (the DLLs).

    python -m crui all --out dist/CleanroomUI
    python -m crui scripts --imports <folder>    # extra Papyrus import folders, searched before the game's
    python -m crui check --game <Fallout 4>/Data   # the movie check (not a build step)
    python -m crui translations --test-language    # every language file stretched and accented, to look for cut text
"""

import argparse
import os
import shutil
import subprocess
import sys
from pathlib import Path

from crui import POPEN_FLAGS, REPO, check, menus, mxmlc, papyrus, plugin, translations

STEPS = ('plugin', 'translations', 'movies', 'scripts', 'dll')
DEFAULT_OUT = REPO / 'dist' / 'CleanroomUI'
#: CommonLibF4 (CRUI_COMMONLIBF4, else the template's beside this repo) and our patches to its commonlib-shared.
COMMONLIB = Path(os.environ.get('CRUI_COMMONLIBF4')
                 or REPO.parent / 'tools_ext' / 'commonlibf4-template' / 'lib' / 'commonlibf4')
PATCHES = REPO / 'dll' / 'patches'
#: The DLLs dll/xmake.lua builds, where it puts them, and where xmake is when it is not on PATH.
XMAKE_DLLS = ('CleanroomUI', 'CleanroomDialogue')
XMAKE_OUT = REPO / 'dll' / 'build' / 'windows' / 'x64' / 'release'
XMAKE_DEFAULT = Path('C:/Program Files/xmake/xmake.exe')
#: xmake's folder unless XMAKE_GLOBALDIR is set: children of the py launcher can't see %LOCALAPPDATA%\.xmake.
XMAKE_HOME = REPO / 'out' / 'xmake'


def build_plugin(out: Path) -> bool:
    """CleanroomUI.esl into <out>."""
    print(f'[plugin] {plugin.build(out)}')
    return True


def build_translations(out: Path, test: bool = False) -> bool:
    """Both releases' words in every game language (with `test`, the test language) into
    <out>/Interface/Translations, and each release's untranslated keys per language; False on a translation's
    problems."""
    try:
        written = translations.install(out, test=test)
    except ValueError as error:
        print(f'[translations] {error}')
        return False
    print(f'[translations] {written} files{" in the test language" if test else ""}')
    for release, missing in translations.untranslated().items():
        counts = {}
        for language, count in missing.items():
            counts.setdefault(count, []).append(language)
        print(f'[translations] {release} untranslated: ' + '; '.join(
            f'{count} in {", ".join(languages)}' for count, languages in sorted(counts.items())))
    return True


def build_movies(out: Path) -> bool:
    """Every menu's movie into <out>/Interface; False when the Flex SDK is missing."""
    if not mxmlc.available():
        print(f'[movies] the Flex SDK is not at {mxmlc.FLEX}; set CRUI_FLEX')
        return False
    for menu in menus.MENUS:
        print(f'[movies] {menus.install(menu, out)}')
    return True


def build_scripts(out: Path, imports: list) -> bool:
    """papyrus/ into <out>/Scripts; False when the compiler is missing or a script fails."""
    if not papyrus.available():
        print(f'[scripts] no Papyrus compiler or Base sources under {papyrus.fallout4_dir()}; set CRUI_FALLOUT4')
        return False
    built, failed, log = papyrus.compile_scripts(out / 'Scripts', imports)
    print(f'[scripts] built {len(built)}: {", ".join(built)}')
    if failed:
        print(f'[scripts] FAILED {len(failed)}: {", ".join(failed)}\n{log}')
    return not failed


def _run(command: list, cwd: Path = REPO, env: 'dict | None' = None) -> bool:
    """Run `command` in `cwd` (with `env`), printing its output; whether it succeeded."""
    result = subprocess.run(command, cwd=cwd, env=env, capture_output=True, text=True, **POPEN_FLAGS)
    print(result.stdout + result.stderr, end='')
    return result.returncode == 0


def patch_commonlib() -> bool:
    """Our patches (dll/patches) applied to COMMONLIB's commonlib-shared, each once; False when one applies to
    neither the patched nor the unpatched sources."""
    shared = COMMONLIB / 'lib' / 'commonlib-shared'
    for patch in sorted(PATCHES.glob('*.patch')):
        done = subprocess.run(['git', '-C', str(shared), 'apply', '--reverse', '--check', str(patch)],
                              capture_output=True, **POPEN_FLAGS).returncode == 0
        if not done and not _run(['git', '-C', str(shared), 'apply', str(patch)]):
            print(f'[dll] {patch.name} does not apply to {shared}')
            return False
    return True


def xmake() -> 'str | None':
    """CRUI_XMAKE, else xmake on PATH, else XMAKE_DEFAULT; None when there is none."""
    found = os.environ.get('CRUI_XMAKE') or shutil.which('xmake')
    return found or (str(XMAKE_DEFAULT) if XMAKE_DEFAULT.is_file() else None)


def build_dll(out: Path) -> bool:
    """XMAKE_DLLS built by dll/xmake.lua (CommonLibF4, patched first) into <out>/F4SE/Plugins; False when one fails,
    or off Windows."""
    if sys.platform != 'win32':
        print('[dll] needs Windows and Visual Studio')
        return False
    tool = xmake()
    if not tool:
        print('[dll] xmake not found; install it or set CRUI_XMAKE')
        return False
    dll, env = REPO / 'dll', {'XMAKE_GLOBALDIR': str(XMAKE_HOME), **os.environ}
    if not (patch_commonlib() and _run([tool, 'f', '-c', '-p', 'windows', '-a', 'x64', '-m', 'release', '-y'], dll, env)
            and all(_run([tool, 'build', '-y', name], dll, env) for name in XMAKE_DLLS)):
        return False
    plugins = out / 'F4SE' / 'Plugins'
    plugins.mkdir(parents=True, exist_ok=True)
    for name in XMAKE_DLLS:
        shutil.copyfile(XMAKE_OUT / f'{name}.dll', plugins / f'{name}.dll')
        print(f'[dll] {name}.dll -> {plugins}')
    return True


def main(argv=None) -> int:
    """Run the steps named (or all); 0 when every one succeeded."""
    parser = argparse.ArgumentParser(prog='python -m crui', description=__doc__.splitlines()[0])
    parser.add_argument('steps', nargs='*', choices=(*STEPS, 'all', 'check'), default=['all'])
    parser.add_argument('--out', type=Path, default=DEFAULT_OUT, help='the mod folder to write')
    parser.add_argument('--imports', type=Path, action='append', default=[], help='extra Papyrus import folder')
    parser.add_argument('--game', type=Path, help="the game's Data folder, for the movie check's font names")
    parser.add_argument('--test-language', action='store_true',
                        help='write every language file in the test language (stretched, accented English)')
    args = parser.parse_args(argv)
    steps = STEPS if 'all' in args.steps else args.steps
    ok = {'plugin': lambda: build_plugin(args.out),
          'translations': lambda: build_translations(args.out, args.test_language),
          'movies': lambda: build_movies(args.out), 'scripts': lambda: build_scripts(args.out, args.imports),
          'dll': lambda: build_dll(args.out), 'check': lambda: check.check(args.game)}
    results = [ok[step]() for step in steps]
    return 0 if all(results) else 1


if __name__ == '__main__':
    sys.exit(main())
