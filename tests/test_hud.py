"""The HUD host: CleanroomUI.dll loads our host movie into the HUD once a mod adds a widget, the movie asks for what
the DLL registers for it, and CRUI_Hud.psc declares the natives hud.cpp registers.

See: docs/swf_framework.md#hud
"""

import re

import pytest

from crui import REPO, menus, mxmlc
from crui.swf import Swf

MENU = menus.BY_NAME['hud']
HUD_CPP = REPO / 'dll' / 'framework' / 'hud.cpp'


def _source(path):
    """`path`'s text with Windows line ends as newlines."""
    return path.read_text(encoding='utf-8')


def test_the_dll_loads_the_host_the_build_installs_only_once_a_widget_is_added():
    """hud.cpp loads HOST (the movie the build installs under Interface) into the HUD's root, and its hook gives up
    while no widget has been added, so a game with no mod using the HUD loads nothing."""
    source = _source(HUD_CPP)
    assert 'Interface/' + re.search(r'HOST = "([^"]+)"', source).group(1) == MENU.movie
    hook = source[source.index('class HookTask'):source.index('void Hook()')]
    assert hook.index('if (g_widgets.empty()) return;') < hook.index('LoadChild(movie, root, HOST)')
    assert 'hud.cpp' in _source(REPO / 'dll' / 'build.bat')


def test_the_movie_calls_every_native_the_dll_registers_for_it():
    """Each native hud.cpp registers for the host movie is called by it, with the shared HudColor and Log."""
    source = _source(HUD_CPP)
    movie = _source(MENU.sources / f'{MENU.document}.as')
    natives = re.findall(r'RegisterFunction<\w+>\(plugin, movie, "(\w+)"\);', source)
    assert sorted(natives) == ['HudMessages', 'HudStats', 'HudWidgets']
    assert all(f'dll.{name}(' in movie for name in natives + ['HudColor', 'Log'])
    assert 'RegisterFunction<LogNative>(plugin, movie, "Log");' in _source(REPO / 'dll' / 'shared' / 'gfx.cpp')


def test_widgets_take_hudframeworks_method():
    """A widget movie is handed messages through processMessage(command, params), HUDFramework's widget method, so
    one widget runs under either host."""
    assert 'shown.content.processMessage(String(message.command), message.params as Array);' in _source(
        MENU.sources / f'{MENU.document}.as')


def test_the_script_declares_every_native_hud_cpp_registers():
    """CRUI_Hud.psc declares each Papyrus native hud.cpp registers on its script, and no other."""
    source = _source(HUD_CPP)
    script_name = re.search(r'SCRIPT = "(\w+)"', source).group(1)
    registered = set(re.findall(r'>\("(\w+)", SCRIPT,', source.replace('\n', ' ').replace('  ', '')))
    script = _source(REPO / 'papyrus' / f'{script_name}.psc')
    declared = set(re.findall(r'^Function (\w+)\(.*\) Global Native$', script, re.M))
    assert registered == declared == {'AddWidget', 'AddStats', 'RemoveWidget', 'Send'}


@pytest.mark.skipif(not mxmlc.available(), reason='the Flex SDK is not installed')
def test_the_movie_builds_the_same_twice(tmp_path):
    """Two uncompressed, undated builds are identical, with the host, its stats line and the natives it asks for."""
    raw = menus.compile_movie(MENU, tmp_path / 'a.swf').read_bytes()
    assert raw == menus.compile_movie(MENU, tmp_path / 'b.swf').read_bytes() and raw[:3] == b'FWS'
    assert not {t.code for t in Swf.parse(raw).tags} & set(mxmlc.DATED)
    for name in (b'CRUI_Hud', b'StatsLine', b'HudWidgets', b'HudMessages', b'HudStats', b'processMessage', b'crui'):
        assert name in raw
