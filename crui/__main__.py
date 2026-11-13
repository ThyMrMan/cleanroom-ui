"""Build Cleanroom UI into a mod folder: Interface/ (movies), Scripts/ (Papyrus), F4SE/Plugins/ (the DLL).

    python -m crui all --out dist/CleanroomUI
    python -m crui scripts --imports <folder>    # extra Papyrus import folders, searched before the game's
"""

import argparse
import subprocess
import sys
from pathlib import Path

from crui import POPEN_FLAGS, REPO, menus, mxmlc, papyrus

STEPS = ('movies', 'scripts', 'dll')
DEFAULT_OUT = REPO / 'dist' / 'CleanroomUI'
DLL_BUILD = REPO / 'dll' / 'f3nv' / 'build.bat'


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


def build_dll(out: Path) -> bool:
    """F3NV.dll into <out>/F4SE/Plugins with dll/f3nv/build.bat; False when it fails or off Windows."""
    if sys.platform != 'win32':
        print('[dll] needs Windows and Visual Studio')
        return False
    plugins = out / 'F4SE' / 'Plugins'
    result = subprocess.run(['cmd', '/c', str(DLL_BUILD), str(plugins)], capture_output=True, text=True, **POPEN_FLAGS)
    print(result.stdout + result.stderr, end='')
    return result.returncode == 0


def main(argv=None) -> int:
    """Run the steps named (or all); 0 when every one succeeded."""
    parser = argparse.ArgumentParser(prog='python -m crui', description=__doc__.splitlines()[0])
    parser.add_argument('steps', nargs='*', choices=(*STEPS, 'all'), default=['all'])
    parser.add_argument('--out', type=Path, default=DEFAULT_OUT, help='the mod folder to write')
    parser.add_argument('--imports', type=Path, action='append', default=[], help='extra Papyrus import folder')
    args = parser.parse_args(argv)
    steps = STEPS if 'all' in args.steps else args.steps
    ok = {'movies': lambda: build_movies(args.out), 'scripts': lambda: build_scripts(args.out, args.imports),
          'dll': lambda: build_dll(args.out)}
    results = [ok[step]() for step in steps]
    return 0 if all(results) else 1


if __name__ == '__main__':
    sys.exit(main())
