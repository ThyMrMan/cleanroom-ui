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


def test_the_dll_loads_the_host_the_build_installs_into_every_hud():
    """hud.cpp loads HOST (the movie the build installs under Interface) into the HUD's root once per HUD movie, with
    or without widgets of ours (it also places the game's own parts, which the edit menu needs measured before it
    opens); the host skips HUDFramework's widgets."""
    source = _source(HUD_CPP)
    assert 'Interface/' + re.search(r'HOST = "([^"]+)"', source).group(1) == MENU.movie
    hook = source[source.index('class HookTask'):source.index('void Hook()')]
    assert hook.index('if (root.HasMember(HOOKED)) return;') < hook.index('LoadChild(movie, root, HOST)')
    assert 'Ours()' not in hook and 'HasPartLayout' not in source
    assert 'hud.cpp' in _source(REPO / 'dll' / 'build.bat')
    assert 'if (entry.host != "")' in _source(MENU.sources / f'{MENU.document}.as')


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


def test_the_native_script_declares_every_native_hud_cpp_registers():
    """CRUI_HudNative.psc declares each Papyrus native hud.cpp registers on its script, and no other."""
    source = _source(HUD_CPP)
    script_name = re.search(r'SCRIPT = "(\w+)"', source).group(1)
    registered = set(re.findall(r'>\("(\w+)", SCRIPT,', source.replace('\n', ' ').replace('  ', '')))
    script = _source(REPO / 'papyrus' / f'{script_name}.psc')
    declared = set(re.findall(r'^\w*\s?Function (\w+)\(.*\) Global Native$', script, re.M))
    assert registered == declared == {'AddWidget', 'AddStats', 'RemoveWidget', 'Send', 'HostOf', 'MovieOf'}


def test_hudframework_hosts_when_installed_and_keeps_the_players_place():
    """CRUI_Hud reaches HUDFramework only through vanilla calls (no compile dependency), registers a widget only
    when it is not registered yet (registering again resets the place the player gave it), sends through its
    string channel with the params joined as it splits them, and hosts every stats line in our stats widget."""
    script = _source(REPO / 'papyrus' / 'CRUI_Hud.psc')
    code = re.sub(r'\{.*?\}', '', script, flags=re.S).replace('"HUDFramework.esm"', '').replace('"HUDFramework.esp"', '')
    assert 'Utility.CallGlobalFunction("HUDFramework", "GetInstance", new Var[0]) as ScriptObject' in script
    assert 'HUDFramework.' not in code and 'As HUDFramework' not in code.replace('as', 'As')
    register = script[script.index('Function Register('):script.index('String Function Joined')]
    assert register.index('"IsWidgetRegistered"') < register.index('"RegisterWidget"')
    assert 'hud.CallFunction("SendMessageString", args)' in script and 'out += " | "' in script
    stats = menus.BY_NAME['hud_stats']
    assert f'Register(hud, akOwner, "{stats.movie.removeprefix("Interface/")}", afX, afY)' in script
    widget = _source(stats.sources / f'{stats.document}.as')
    assert 'entry.movie == "" && entry.host == "HUDFramework"' in widget
    assert 'public function processMessage(command:String, params:Array) : void' in widget


@pytest.mark.skipif(not mxmlc.available(), reason='the Flex SDK is not installed')
def test_the_movie_builds_the_same_twice(tmp_path):
    """Two uncompressed, undated builds are identical, with the host, its stats line and the natives it asks for."""
    raw = menus.compile_movie(MENU, tmp_path / 'a.swf').read_bytes()
    assert raw == menus.compile_movie(MENU, tmp_path / 'b.swf').read_bytes() and raw[:3] == b'FWS'
    assert not {t.code for t in Swf.parse(raw).tags} & set(mxmlc.DATED)
    for name in (b'CRUI_Hud', b'StatsLine', b'PartLayout', b'FnvPreset', b'HudWidgets', b'HudMessages', b'HudStats',
                 b'HudPartLayout', b'processMessage', b'crui'):
        assert name in raw


@pytest.mark.skipif(not mxmlc.available(), reason='the Flex SDK is not installed')
def test_the_stats_widget_builds_from_the_hosts_folder(tmp_path):
    """The stats widget HUDFramework hosts builds from the host's folder, with the same StatsLine."""
    stats = menus.BY_NAME['hud_stats']
    assert stats.sources == MENU.sources
    raw = menus.compile_movie(stats, tmp_path / 'stats.swf').read_bytes()
    for name in (b'CRUI_HudStats', b'StatsLine', b'HudWidgets', b'HudStats', b'processMessage'):
        assert name in raw


def test_the_edit_menu_and_host_use_every_layout_native():
    """layout.cpp's natives are each called: HudReport by the host, which applies a widget's place; HudLayout,
    HudMove, HudScale, HudReset and HudEditDone by the edit menu, which CRUI_Hud.EditLayout opens by its movie's
    name; the build compiles layout.cpp and HudWidgets hands each widget its place."""
    layout = _source(REPO / 'dll' / 'framework' / 'layout.cpp')
    natives = re.findall(r'RegisterFunction<\w+>\(plugin, movie, "(\w+)"\);', layout)
    assert sorted(natives) == ['HudEditDone', 'HudLayout', 'HudMove', 'HudPlaceOf', 'HudReport', 'HudReset',
                               'HudScale']
    host = _source(MENU.sources / f'{MENU.document}.as')
    parts = _source(MENU.sources / 'PartLayout.as')
    edit_menu = menus.BY_NAME['hud_edit']
    edit = _source(edit_menu.sources / f'{edit_menu.document}.as')
    assert 'dll.HudReport(' in host and 'view.scaleX = view.scaleY = Number(at.scale);' in host
    assert 'dll.HudReport(name,' in parts and 'dll.HudPlaceOf(name)' in parts
    assert all(f'dll.{name}(' in edit for name in natives if name not in ('HudReport', 'HudPlaceOf'))
    menu = edit_menu.movie.removeprefix('Interface/').removesuffix('.swf')
    assert f'String menu = "{menu}"' in _source(REPO / 'papyrus' / 'CRUI_Hud.psc')
    assert f'EDIT_MENU = "{menu}"' in layout and 'layout.cpp' in _source(REPO / 'dll' / 'build.bat')
    assert 'entry.SetMember("place", &place);' in _source(HUD_CPP)


def test_places_are_kept_for_every_save_and_only_for_our_host():
    """Places live in a file under Documents (read once as the game loads, written when the edit menu is done), not
    in a save; HudLayout lists only widgets on our host and the HUD's own parts, whose places the host keeps under
    the same prefix the DLL lists."""
    layout = _source(REPO / 'dll' / 'framework' / 'layout.cpp')
    assert 'CleanroomUI_Hud.txt' in layout and 'CSIDL_MYDOCUMENTS' in layout
    assert 'if (!OurWidget(name) && name.rfind(PART, 0) != 0) continue;' in layout
    prefix = re.search(r'PART = "([^"]+)"', layout).group(1)
    assert f'PREFIX:String = "{prefix}"' in _source(MENU.sources / 'PartLayout.as')
    assert f'name.indexOf("{prefix}") == 0' in _source(menus.BY_NAME['hud_edit'].sources / 'CRUI_HudEdit.as')
    assert 'LoadLayout();' in _source(HUD_CPP)
    done = layout[layout.index('class DoneNative'):layout.index('}  // namespace')]
    assert done.index('Save();') < done.index('kMessage_Close')


def test_the_host_lays_out_the_hud_parts_from_def_hud_xml():
    """parts.cpp reads DEF_HUD.xml from where DEF_HUD's editor saves it and hands it to the host (HudPartLayout),
    which lays out and places the parts through PartLayout on its first frame, as a place changes and every few
    frames; the parts are the vanilla HUD's, in the seven groups the game pins to the safe area."""
    parts = _source(REPO / 'dll' / 'framework' / 'parts.cpp')
    assert r'".\\Data\\Interface\\DEF_CONF\\DEF_HUD.xml"' in parts
    assert re.findall(r'RegisterFunction<\w+>\(plugin, movie, "(\w+)"\);', parts) == ['HudPartLayout']
    assert 'RegisterHudParts(movie, plugin);' in _source(HUD_CPP)
    assert 'parts.cpp' in _source(REPO / 'dll' / 'build.bat')
    host = _source(MENU.sources / f'{MENU.document}.as')
    assert 'new PartLayout(dll.HudPartLayout != null ? dll.HudPartLayout() : null, log);' in host
    assert 'if (list != null || frames % REPLACE_FRAMES == 0)' in host and 'parts.place(hud, dll, stage);' in host
    layout = _source(MENU.sources / 'PartLayout.as')
    table = layout[layout.index('GROUPS:Object = {'):layout.index('};', layout.index('GROUPS:Object = {'))]
    groups = set(re.findall(r'(\w+_mc): \[', table))
    assert len(re.findall(r'"\w+_mc"', table)) == 27
    assert groups == {'HUDNotificationsGroup_mc', 'TopCenterGroup_mc', 'TopRightGroup_mc', 'CenterGroup_mc',
                      'LeftMeters_mc', 'BottomCenterGroup_mc', 'RightMeters_mc'}


def test_only_what_the_layout_changes_is_set():
    """A position is set only when it differs from the game's (defx, defy), a scale other than 1, a color other than
    white; a value already right is left alone, so DEF_HUD's own movie and ours agree; a part the game moved since
    takes that as its new base."""
    layout = _source(MENU.sources / 'PartLayout.as')
    assert 'Number(entry.x) != Number(entry.defx)' in layout and 'Number(entry.y) != Number(entry.defy)' in layout
    assert 'Number(entry.scaleX) != 1' in layout and 'uint(entry.color) != 0xFFFFFF' in layout
    assert 'if (!near(Number(view[property]), Number(at[property])))' in layout
    assert 'part.base = {x: view.x, y: view.y, scaleX: view.scaleX, scaleY: view.scaleY};' in layout


def test_the_fnv_preset_is_a_setting_run_between_def_and_the_places():
    """[Hud] sPreset (our INI, then MCM's copy) picks the preset; PartLayout lays each part at its base, arranges them
    by the preset, then reports and places them; with a preset DEF_HUD.xml is left aside. The preset moves parts only
    within their own groups."""
    parts = _source(REPO / 'dll' / 'framework' / 'parts.cpp')
    assert 'GetPrivateProfileStringA("Hud", "sPreset"' in parts and 'MCM' in parts
    assert 'args->result->SetMember("preset", &presetText);' in parts
    layout = _source(MENU.sources / 'PartLayout.as')
    body = layout[layout.index('public function place('):layout.index('private function find(')]
    order = [body.index(step) for step in ('lay(part);', 'FnvPreset.arrange(found, stage);', 'finish(part, dll, stage);')]
    assert order == sorted(order)
    preset = _source(MENU.sources / 'FnvPreset.as')
    assert 'public static const NAME:String = "FNV";' in preset
    assert 'addChild' not in preset and 'removeChild' not in preset
    named = set(re.findall(r'"(\w+_mc)"', preset))
    table = layout[layout.index('GROUPS:Object = {'):layout.index('};', layout.index('GROUPS:Object = {'))]
    assert named and named <= set(re.findall(r'"(\w+_mc)"', table))
