"""Fallout 4's own Papyrus compiler, against the game's script sources in the player's install: F4SE's, then Base.

The compiler and the sources come with the Creation Kit and F4SE (its .psc files in Data/Scripts/Source, or
Source/F4SE); nothing of Bethesda's is in this repo. The Creation Kit ships Base as Source/Base/Base.zip and unpacks
it on first run; until then the build unpacks it into its own cache. The compiler fails on a path over 260 characters
in its working directory, so it runs from the output folder.
"""

import os
import subprocess
import zipfile
from pathlib import Path

from crui import CACHE, POPEN_FLAGS, REPO

#: Steam's default Fallout 4 folder, when neither CRUI_FALLOUT4 nor the registry names one.
STEAM_DEFAULT = Path(r'C:\Program Files (x86)\Steam\steamapps\common\Fallout 4')
REGISTRY_KEY = r'SOFTWARE\WOW6432Node\Bethesda Softworks\Fallout4'
SCRIPTS = REPO / 'papyrus'
#: Flags file of the vanilla scripts, found on the import path.
FLAGS = 'Institute_Papyrus_Flags.flg'


def _registry_dir() -> 'Path | None':
    """The install folder Steam registered for Fallout 4; None off Windows or when not registered."""
    try:
        import winreg
        with winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, REGISTRY_KEY) as key:
            return Path(winreg.QueryValueEx(key, 'installed path')[0])
    except (ImportError, OSError):
        return None


def fallout4_dir() -> Path:
    """The game folder: CRUI_FALLOUT4, else the registered install, else Steam's default."""
    named = os.environ.get('CRUI_FALLOUT4')
    return Path(named) if named else (_registry_dir() or STEAM_DEFAULT)


def compiler() -> Path:
    """PapyrusCompiler.exe of the Creation Kit."""
    return fallout4_dir() / 'Papyrus Compiler' / 'PapyrusCompiler.exe'


def base_dir(source: Path) -> 'Path | None':
    """The vanilla sources: Source/Base when unpacked, else its Base.zip unpacked into the cache; None without
    either."""
    base = source / 'Base'
    if (base / 'Form.psc').is_file():
        return base
    archive, unpacked = base / 'Base.zip', CACHE / 'papyrus_base'
    if not archive.is_file():
        return None
    stamp = unpacked / '.from'
    key = f'{archive.stat().st_size} {archive.stat().st_mtime_ns}'
    if not stamp.is_file() or stamp.read_text() != key:
        with zipfile.ZipFile(archive) as zipped:
            zipped.extractall(unpacked)
        stamp.write_text(key)
    found = next(unpacked.rglob('Form.psc'), None)
    return found.parent if found else None


def header_dirs() -> list:
    """The import folders after ours: F4SE's sources (CRUI_F4SE_SCRIPTS, Source/F4SE, else Source), then Base;
    empty without Base."""
    source = fallout4_dir() / 'Data' / 'Scripts' / 'Source'
    f4se = os.environ.get('CRUI_F4SE_SCRIPTS')
    f4se = Path(f4se) if f4se else next((d for d in (source / 'F4SE', source) if (d / 'F4SE.psc').is_file()), source)
    base = base_dir(source)
    return [f4se, base] if base else []


def available() -> bool:
    """Whether the compiler and the Base sources are installed."""
    return compiler().is_file() and bool(header_dirs())


def compile_folder(folder: Path, out_dir: Path, imports=()) -> str:
    """Compile every .psc in `folder` into `out_dir`, importing `folder`, `imports`, then header_dirs(); the
    compiler's combined output."""
    out_dir.mkdir(parents=True, exist_ok=True)
    path = ';'.join(str(p) for p in (folder, *imports, *header_dirs()))
    result = subprocess.run([str(compiler()), str(folder), '-all', '-q', f'-i={path}', f'-o={out_dir}', f'-f={FLAGS}'],
                            capture_output=True, text=True, cwd=str(out_dir), **POPEN_FLAGS)
    return result.stdout + result.stderr


def compile_scripts(out_dir: Path, imports=()) -> tuple:
    """Compile papyrus/ into `out_dir`: (names built, names that failed, the compiler's output)."""
    wanted = {p.stem for p in SCRIPTS.glob('*.psc')}
    for name in wanted:
        (out_dir / f'{name}.pex').unlink(missing_ok=True)
    log = compile_folder(SCRIPTS, out_dir, imports)
    built = {p.stem for p in out_dir.glob('*.pex')} & wanted
    return sorted(built), sorted(wanted - built), log
