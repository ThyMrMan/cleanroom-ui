"""Cleanroom UI's one download: a built mod folder split into the installer's parts (the framework, always installed;
the dialogue menu, an option ticked unless XDI.esm is active), the FOMOD's two files, and the zip.

    python -m crui all fomod     # builds dist/CleanroomUI, then dist/CleanroomUI-<version>.zip

See: docs/swf_framework.md#fomod
"""

import re
import shutil
import zipfile
from fnmatch import fnmatch
from pathlib import Path

from crui import REPO

NAME = 'Cleanroom UI'
AUTHOR = 'ThyMrMan'
WEBSITE = 'https://github.com/ThyMrMan/cleanroom-ui'
#: The download's version, which dll/xmake.lua also gives both DLLs.
XMAKE = REPO / 'dll' / 'xmake.lua'
#: Each part's folder in the download; CORE takes every file DIALOGUE_FILES doesn't.
CORE = '00 Cleanroom UI'
DIALOGUE = '10 Dialogue Menu'
DIALOGUE_FILES = ('F4SE/Plugins/CleanroomDialogue.dll', 'Interface/DialogueMenu.swf',
                  'Interface/Translations/CleanroomDialogue_*.txt')
#: Files a download can't go without: a mod folder missing one was built without a tool.
REQUIRED = ('CleanroomUI.esl', 'F4SE/Plugins/CleanroomUI.dll', 'F4SE/Plugins/CleanroomDialogue.dll',
            'Interface/CRUI_Hud.swf', 'Interface/DialogueMenu.swf', 'Scripts/CRUI_Hud.pex',
            'MCM/Config/CleanroomUI/config.json')
#: XDI's plugin: while it is active the dialogue menu is offered unticked. MO2 tests only plugins, not its DLL.
XDI = 'XDI.esm'
#: One fixed date for every zip entry, so the same files give the same zip.
ZIP_DATE = (2026, 1, 1, 0, 0, 0)

DIALOGUE_TEXT = ('Replaces the four-button dialogue wheel with a list of every option. Leave it off if you use '
                 'another dialogue menu; it turns itself off when XDI is installed.')
DESCRIPTION = ('Move, resize, hide and recolor every part of your HUD, and get item condition, repair and ammo lists '
               'in your Pip-Boy. A menu framework for other mods, written from scratch. Needs F4SE.')


def version() -> str:
    """The version dll/xmake.lua sets."""
    return re.search(r'set_version\("([^"]+)"\)', XMAKE.read_text(encoding='utf-8')).group(1)


def part(rel: str) -> str:
    """The part folder the file at `rel` (posix, in the mod folder) goes in."""
    return DIALOGUE if any(fnmatch(rel, pattern) for pattern in DIALOGUE_FILES) else CORE


def missing(built: Path) -> list:
    """REQUIRED files absent from the mod folder `built`."""
    return [rel for rel in REQUIRED if not (built / rel).is_file()]


def module_config() -> str:
    """fomod/ModuleConfig.xml: CORE always installed, DIALOGUE an option recommended unless XDI is there."""
    return f'''<?xml version="1.0" encoding="UTF-8"?>
<config xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:noNamespaceSchemaLocation="http://qconsulting.ca/fo3/ModConfig5.0.xsd">
  <moduleName>{NAME}</moduleName>
  <requiredInstallFiles>
    <folder source="{CORE}" destination="" />
  </requiredInstallFiles>
  <installSteps order="Explicit">
    <installStep name="Options">
      <optionalFileGroups order="Explicit">
        <group name="Dialogue" type="SelectAny">
          <plugins order="Explicit">
            <plugin name="Dialogue menu">
              <description>{DIALOGUE_TEXT}</description>
              <files>
                <folder source="{DIALOGUE}" destination="" />
              </files>
              <typeDescriptor>
                <dependencyType>
                  <defaultType name="Recommended" />
                  <patterns>
                    <pattern>
                      <dependencies operator="And">
                        <fileDependency file="{XDI}" state="Active" />
                      </dependencies>
                      <type name="Optional" />
                    </pattern>
                  </patterns>
                </dependencyType>
              </typeDescriptor>
            </plugin>
          </plugins>
        </group>
      </optionalFileGroups>
    </installStep>
  </installSteps>
</config>
'''


def info() -> str:
    """fomod/info.xml: the name, author, version, website and description mod managers show."""
    return f'''<?xml version="1.0" encoding="UTF-8"?>
<fomod>
  <Name>{NAME}</Name>
  <Author>{AUTHOR}</Author>
  <Version>{version()}</Version>
  <Website>{WEBSITE}</Website>
  <Description>{DESCRIPTION}</Description>
</fomod>
'''


def zip_folder(folder: Path, path: Path) -> Path:
    """Every file under `folder` into the zip `path`, sorted, with fixed dates."""
    with zipfile.ZipFile(path, 'w', zipfile.ZIP_DEFLATED) as archive:
        for file in sorted(p for p in folder.rglob('*') if p.is_file()):
            entry = zipfile.ZipInfo(file.relative_to(folder).as_posix(), ZIP_DATE)
            entry.compress_type = zipfile.ZIP_DEFLATED
            archive.writestr(entry, file.read_bytes())
    return path


def write(built: Path, stage: Path, dist: Path) -> Path:
    """The download from the mod folder `built`: its files split into parts in `stage` (emptied first), the FOMOD
    files beside them, zipped into <dist>/CleanroomUI-<version>.zip; the zip."""
    shutil.rmtree(stage, ignore_errors=True)
    for file in sorted(p for p in built.rglob('*') if p.is_file()):
        rel = file.relative_to(built).as_posix()
        target = stage / part(rel) / rel
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(file, target)
    (stage / 'fomod').mkdir()
    (stage / 'fomod' / 'ModuleConfig.xml').write_text(module_config(), encoding='utf-8')
    (stage / 'fomod' / 'info.xml').write_text(info(), encoding='utf-8')
    dist.mkdir(parents=True, exist_ok=True)
    return zip_folder(stage, dist / f'CleanroomUI-{version()}.zip')
