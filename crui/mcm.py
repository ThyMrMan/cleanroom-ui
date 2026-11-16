"""Mod Configuration Menu pages. Our own (mcm/cleanroom_ui/, installed as Data/MCM/Config/CleanroomUI/): the HUD
edit menu's button and hotkey and our INI settings, which MCM saves to Data/MCM/Settings/CleanroomUI.ini, read over
CleanroomUI.ini. And the level-up host's own options, written by the mod that runs the host against its own quest:
Data/MCM/Config/<mod>/ (config.json, keybinds.json) from the templates in mcm/level_up/.

The page reads and writes the host's properties (CRUI_LevelUp's SkillCap and ShowAllPerks), which the save keeps,
and a hotkey calls its OpenLevelUpMenu. Its words are `$CRUI_MCM_` keys in CleanroomUI's translations, loaded with
CleanroomUI.esl. Nothing of ours calls MCM, so without it the options stay the mod's own values.

See: docs/level_up_plan.md#host-mcm
"""

import json
from pathlib import Path

from crui import REPO

#: The page's templates: {mod}, {title}, {form}, {script} and {hotkey} are filled per mod.
TEMPLATES = REPO / 'mcm' / 'level_up'
#: Our own page (config.json, keybinds.json, settings.ini with the DLL's defaults) and its MCM mod name.
OWN = REPO / 'mcm' / 'cleanroom_ui'
OWN_NAME = 'CleanroomUI'


def install_own(data_dir: Path) -> Path:
    """Our own page's files into <data_dir>/MCM/Config/CleanroomUI/; that folder."""
    folder = Path(data_dir) / 'MCM' / 'Config' / OWN_NAME
    folder.mkdir(parents=True, exist_ok=True)
    for path in sorted(OWN.iterdir()):
        (folder / path.name).write_bytes(path.read_bytes())
    return folder


def level_up_page(mod: str, title: str, form: str, script: str = 'CRUI_LevelUp') -> dict:
    """{file name: JSON} of the page for MCM mod name `mod` titled `title`, its host on quest `form`
    ("<plugin>|<object id hex>") under script `script` (CRUI_LevelUp or a script extending it)."""
    values = {'{mod}': mod, '{title}': title, '{form}': form, '{script}': script, '{hotkey}': f'{mod}_open_level_up'}

    def fill(node):
        """`node` with every placeholder string replaced."""
        if isinstance(node, dict):
            return {key: fill(value) for key, value in node.items()}
        if isinstance(node, list):
            return [fill(value) for value in node]
        return values.get(node, node)
    return {p.name: fill(json.loads(p.read_text(encoding='utf-8'))) for p in sorted(TEMPLATES.glob('*.json'))}


def install(data_dir: Path, mod: str, title: str, form: str, script: str = 'CRUI_LevelUp') -> Path:
    """Write level_up_page's files into <data_dir>/MCM/Config/<mod>/; that folder."""
    folder = Path(data_dir) / 'MCM' / 'Config' / mod
    folder.mkdir(parents=True, exist_ok=True)
    for name, page in level_up_page(mod, title, form, script).items():
        (folder / name).write_text(json.dumps(page, indent=2) + '\n', encoding='utf-8')
    return folder
