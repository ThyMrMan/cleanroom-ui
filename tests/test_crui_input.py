"""crui's Input and HintBar: the game's controls and glyphs they rely on, and the menus taking input only through them.

See: docs/swf_framework.md#input
"""

import re

from crui import REPO
from crui.menus import BY_NAME

LIBRARY = REPO / 'as3' / 'crui'
#: The menus that take keys and controls (the Pip-Boy overlay rides the Pip-Boy's own input).
MENUS = (BY_NAME['level_up'].sources / 'CRUI_LevelUp.as', BY_NAME['dialogue'].sources / 'DialogueMenu.as')
RUNTIME = REPO / 'dll'
#: FO4's menu-mode controls (the control map in Fallout4.exe) and the action each should be.
MENU_CONTROLS = {'Accept': 'ACCEPT', 'Cancel': 'BACK', 'Up': 'UP', 'Down': 'DOWN', 'Left': 'LEFT', 'Right': 'RIGHT',
                 'LShoulder': 'PREV', 'RShoulder': 'NEXT', 'LTrigger': 'PREV', 'RTrigger': 'NEXT', 'XButton': 'X',
                 'YButton': 'Y', 'ZoomIn': 'UP', 'ZoomOut': 'DOWN'}
#: The controller font's letter per button (the game's button hint map), Xbox then PlayStation.
XBOX = {'accept': 'A', 'back': 'B', 'x': 'C', 'y': 'D', 'prev': 'G', 'next': 'L', 'up': 'W', 'down': 'V', 'left': 'T',
        'right': 'U'}
PLAYSTATION = {'accept': 'a', 'back': 'd', 'x': 'c', 'y': 'b', 'prev': 'g', 'next': 'm', 'up': 'w', 'down': 'v',
               'left': 't', 'right': 'u'}


def _object(source: str, name: str) -> dict:
    """The AS3 object literal `name` in `source` as {key: value}, its values strings or bare names."""
    body = re.search(rf'{name}:Object = \{{(.*?)\}};', source, re.S).group(1)
    return {k: v.strip('"') for k, v in re.findall(r'(\w+):\s*("?[\w.]+"?)', body)}


def test_the_hint_glyphs_are_the_controller_fonts_letters():
    """Each action's glyph on Xbox and PlayStation is the letter the game's controller font draws that button with."""
    source = (LIBRARY / 'HintBar.as').read_text(encoding='utf-8')
    assert _object(source, 'XBOX') == XBOX and _object(source, 'PLAYSTATION') == PLAYSTATION
    assert 'GLYPHS:String = "$Controller_Buttons"' in source


def test_input_maps_every_menu_mode_control():
    """The game's menu-mode controls each map to their action; moves fire on the press, accept and back on release."""
    source = (LIBRARY / 'Input.as').read_text(encoding='utf-8')
    assert _object(source, 'names') == MENU_CONTROLS
    assert set(_object(source, 'MOVES')) == {'up', 'down', 'left', 'right', 'prev', 'next'}


def test_no_movie_calls_flash_11s_remove_children():
    """removeChildren() is Flash Player 11's; Scaleform may lack it, so our AS3 empties a container a child at a time.

    See: docs/swf_framework.md#scroll-list
    """
    for path in [*(REPO / 'menus').rglob('*.as'), *LIBRARY.rglob('*.as')]:
        assert 'removeChildren(' not in path.read_text(encoding='utf-8'), path.name


def test_images_asks_the_dll_for_a_texture_by_the_name_it_registers():
    """Each DLL of ours registers TextureExists on every movie; Images asks whichever is there (Bridge.shared) with
    the path MountImage takes, and treats a DLL without it as found.

    See: docs/swf_framework.md#images
    """
    for shared in (RUNTIME / 'shared' / 'gfx.cpp', RUNTIME / 'base' / 'base.cpp'):
        gfx = shared.read_text(encoding='utf-8')
        assert 'RegisterFunction<TextureExistsNative>(plugin, movie, "TextureExists")' in gfx
        assert 'std::string("Textures\\\\")' in gfx and 'BSResourceNiBinaryStream stream(path.c_str());' in gfx
    assert 'RegisterSharedNatives(movie, plugin);' in (RUNTIME / 'framework' / 'plugin.cpp').read_text(encoding='utf-8')
    assert 'RegisterSharedNatives(view, plugin);' in (RUNTIME / 'dialogue' / 'dialogue.cpp').read_text(encoding='utf-8')
    images = (LIBRARY / 'Images.as').read_text(encoding='utf-8')
    assert 'bridge.shared()' in images
    assert 'dll.TextureExists == null || dll.TextureExists(texturePath(path)) == true' in images


def test_both_lists_are_the_librarys():
    """The Pip-Boy's list panel is a Modal round a ScrollList, and the dialogue's rows are a ScrollList."""
    panel = (BY_NAME['pipboy'].sources / 'ListPanel.as').read_text(encoding='utf-8')
    assert 'new Modal()' in panel and 'new ScrollList()' in panel and 'modal.open(body' in panel
    assert 'new ScrollList()' in MENUS[1].read_text(encoding='utf-8')


def test_the_menus_take_keys_and_controls_only_through_input():
    """No menu listens for keys itself; each hands the game's controls and platform to Input and shows a HintBar."""
    for menu in MENUS:
        source = menu.read_text(encoding='utf-8')
        assert 'KeyboardEvent' not in source, menu.name
        assert 'input.control(' in source and 'input.setPlatform(platform)' in source, menu.name
        assert 'new HintBar(' in source, menu.name


def test_movie_sources_use_no_bethesda_classes():
    """Every import in every movie is Flash's, our crui's or the movie's own package: nothing from the game's."""
    for menu in BY_NAME.values():
        own = {p.parent.relative_to(menu.sources).as_posix().replace('/', '.') for p in menu.sources.rglob('*.as')}
        for path in menu.sources.rglob('*.as'):
            imports = re.findall(r'^\s*import ([\w.]+);', path.read_text(encoding='utf-8'), re.M)
            assert all(name.startswith(('flash.', 'crui.')) or name.rsplit('.', 1)[0] in own for name in imports), \
                (path.name, imports)
