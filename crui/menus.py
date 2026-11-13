"""Every menu movie Cleanroom UI builds: its AS3 sources, document class and path in a mod's data folder.

See: docs/swf_framework.md#have
"""

from dataclasses import dataclass
from pathlib import Path

from crui import CACHE, REPO, mxmlc

MENUS_DIR = REPO / 'menus'


@dataclass(frozen=True)
class Menu:
    """One movie: built from menus/<folder, else name>/as3 with document class `document`, installed at `movie`."""
    name: str
    document: str
    movie: str
    folder: str = ''

    @property
    def sources(self) -> Path:
        """The movie's AS3 source folder."""
        return MENUS_DIR / (self.folder or self.name) / 'as3'


#: The level-up, tag and trait menu (CRUI_LevelUp.psc opens it); the Pip-Boy overlay (pipboy.cpp loads it); the HUD
#: host (hud.cpp loads it), the stats widget HUDFramework hosts (CRUI_Hud.psc registers it) and the host's edit menu
#: (CRUI_Hud.EditLayout opens it), from one folder; the dialogue menu (dialogue.cpp drives it).
MENUS = (Menu('level_up', 'CRUI_LevelUp', 'Interface/CRUI_LevelUp.swf'),
         Menu('pipboy', 'CRUI_Pipboy', 'Interface/CRUI_Pipboy.swf'),
         Menu('hud', 'CRUI_Hud', 'Interface/CRUI_Hud.swf'),
         Menu('hud_stats', 'CRUI_HudStats', 'Interface/CRUI_HudStats.swf', 'hud'),
         Menu('hud_edit', 'CRUI_HudEdit', 'Interface/CRUI_HudEdit.swf', 'hud'),
         Menu('dialogue', 'DialogueMenuRoot', 'Interface/DialogueMenu.swf'))
BY_NAME = {menu.name: menu for menu in MENUS}


def compile_movie(menu: Menu, out: Path) -> Path:
    """`menu`'s movie, compiled to `out`."""
    return mxmlc.compile_movie(out, menu.sources, menu.document)


def install(menu: Menu, data_dir: Path) -> 'Path | None':
    """`menu`'s movie (cached by its sources' hash) copied to <data_dir>/<movie>; None without the Flex SDK."""
    return mxmlc.install(data_dir, menu.movie, menu.sources, menu.document, CACHE / menu.name)
