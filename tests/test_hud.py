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
    show = _source(REPO / 'dll' / 'framework' / 'show.cpp')
    assert re.search(r'SCRIPT = "(\w+)"', show).group(1) == script_name and 'RegisterShow(vm);' in source
    registered = set(re.findall(r'>\(\s*"(\w+)", SCRIPT,', (source + show).replace('\n', ' ').replace('  ', '')))
    script = _source(REPO / 'papyrus' / f'{script_name}.psc')
    declared = set(re.findall(r'^\w*\s?Function (\w+)\(.*\) Global Native$', script, re.M))
    assert registered == declared == {'AddWidget', 'AddStats', 'RemoveWidget', 'Send', 'HostOf', 'MovieOf',
                                      'SetCondition', 'SetConditionGlobal', 'RemoveCondition', 'SetShowRule'}


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
    for name in (b'CRUI_Hud', b'StatsLine', b'PartLayout', b'FnvPreset', b'ConditionBar', b'HudWidgets',
                 b'HudMessages', b'HudStats', b'HudPartLayout', b'HudWeaponCondition', b'processMessage', b'crui'):
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
    assert sorted(natives) == ['HudEditDone', 'HudHide', 'HudLayout', 'HudMove', 'HudPlaceOf', 'HudReport',
                               'HudReset', 'HudScale', 'HudSetShow', 'HudTint']
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
    registered = re.findall(r'RegisterFunction<\w+>\(plugin, movie, "(\w+)"\);', parts)
    assert registered == ['HudPartLayout', 'HudWeaponCondition']
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
    assert 'Number(entry.scaleX) != 1' in layout and 'uint(part.entry.color) != 0xFFFFFF' in layout
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
    order = [body.index(step) for step in ('fnv.add(hud);', 'lay(part);', 'fnv.arrange(found, stage);',
                                           'finish(part, dll, stage);')]
    assert order == sorted(order)
    preset = _source(MENU.sources / 'FnvPreset.as')
    assert 'public static const NAME:String = "FNV";' in preset
    assert re.findall(r'\w+\.addChild\(\w+\)', preset) == ['group.addChild(condition)']
    assert 'removeChild' not in preset
    rules = preset[preset.index('public function arrange('):]
    named = set(re.findall(r'"(\w+_mc)"', rules))
    table = layout[layout.index('GROUPS:Object = {'):layout.index('};', layout.index('GROUPS:Object = {'))]
    assert named and named <= set(re.findall(r'"(\w+_mc)"', table))


def test_the_fnv_preset_draws_the_weapon_condition_bar_fnv_has_and_fo4_has_not():
    """With the FNV preset, our own CND bar joins the HUD group RightMeters_mc (so it hides and fades with it) as a
    part of the layout (laid out, arranged, placed and in the edit menu as "part:" + its name); the host feeds it each
    frame from HudWeaponCondition: the equipped weapon's condition, 1 while none is set, -1 (hidden) for no weapon
    or one that doesn't wear."""
    bar = _source(MENU.sources / 'ConditionBar.as')
    assert 'public static const NAME:String = "CRUI_WeaponCondition_mc";' in bar
    assert 'Text.show(label, "$CRUI_CND");' in bar and 'visible = shown && health >= 0;' in bar
    assert re.search(r'^\$CRUI_CND\t', _source(REPO / 'translations' / 'CleanroomUI' / 'en.txt'), re.M)
    preset = _source(MENU.sources / 'FnvPreset.as')
    assert 'WIDGETS:Object = {CRUI_WeaponCondition_mc: "RightMeters_mc"};' in preset
    assert 'dll.HudWeaponCondition()' in preset
    layout = _source(MENU.sources / 'PartLayout.as')
    assert 'for (var widget:String in FnvPreset.WIDGETS)' in layout
    assert 'parts.draw(hud, dll, rules);' in _source(MENU.sources / f'{MENU.document}.as')
    parts = _source(REPO / 'dll' / 'framework' / 'parts.cpp')
    native = parts[parts.index('class WeaponConditionNative'):parts.index('}  // namespace')]
    assert 'WeaponCondition(player, EquippedWeapon(player))' in native
    condition = parts[parts.index('double WeaponCondition('):]
    assert 'g_tracked.count(weapon->formID)' in condition and 'return set < 0.0f ? 1.0 : double(set);' in condition


def test_the_player_hides_and_colors_widgets_and_parts_without_fighting_the_game():
    """A place carries hidden and a color (saved as two more columns; a line without them still loads), toggled and
    stepped in the edit menu by keys from [Keys]. The game's parts are hidden by a mask (never their visible or
    alpha, which the engine sets) and colored by their color transform's multipliers, put on again every frame; the
    player's color goes over DEF_HUD.xml's. Our own widgets are hidden by alpha; a stats line draws in the color."""
    layout = _source(REPO / 'dll' / 'framework' / 'layout.cpp')
    assert "(place.hidden ? 1 : 0)" in layout and 'color != "-"' in layout and 'if (fields >> hidden)' in layout
    keys = _source(REPO / 'dll' / 'framework' / 'keys.cpp')
    assert '{"sHudHide", "H", "", \'H\'}' in keys and '{"sHudColor", "C", "", \'C\'}' in keys
    look = _source(MENU.sources / 'Look.as')
    assert 'view.mask = state.mask;' in look and '.visible' not in look and '.alpha' not in look
    parts = _source(MENU.sources / 'PartLayout.as')
    assert 'looks.apply(part.view, hidden,' in parts and 'part.hidden == true ||' in parts
    assert 'part.color = at != null && Number(at.color) >= 0 ? Number(at.color) : own;' in parts
    host = _source(MENU.sources / f'{MENU.document}.as')
    assert 'widget.view.alpha = widget.hidden ||' in host and 'parts.draw(hud, dll, rules);' in host
    edit_menu = menus.BY_NAME['hud_edit']
    edit = _source(edit_menu.sources / f'{edit_menu.document}.as')
    assert 'e.keyCode == hideKey' in edit and 'e.keyCode == colorKey' in edit and 'keys.hideName' in edit
    words = _source(REPO / 'translations' / 'CleanroomUI' / 'en.txt')
    hint = re.search(r'^\$CRUI_HUD_EDIT_HINT\t(.*)$', words, re.M).group(1)
    assert all(slot in hint for slot in ('{0}', '{1}', '{2}')) and re.search(r'^\$CRUI_HUD_EDIT_HIDDEN\t', words, re.M)


def test_parts_and_widgets_show_on_conditions():
    """Each box's rule (the conditions it shows under) is the player's from the layout file, else the preset's
    ([Hud] sShow=Immersive); HudState gives the conditions each frame (combat, drawn, vats, the show key) and the
    linger. A box shows while one of its conditions holds and for the linger after; with no rule, or a condition the
    DLL doesn't know, it shows. The settings are read as each HUD loads."""
    show = _source(REPO / 'dll' / 'framework' / 'show.cpp')
    assert re.findall(r'RegisterFunction<\w+>\(plugin, movie, "(\w+)"\);', show) == ['HudState', 'HudValues',
                                                                                    'HudConditions']
    assert all(f'SetMember("{name}"' in show for name in ('combat', 'drawn', 'vats', 'peek', 'linger'))
    assert 'IsInCombat()' in show and 'actorState.IsWeaponDrawn()' in show and '"VATSMenu"' in show
    assert 'GetAsyncKeyState(key)' in show and 'GameInFront()' in show
    assert 'Setting("sShow")' in show and 'Setting("fShowLinger")' in show
    rules = dict(re.findall(r'\{"(part:\w+)", "([\w,]+)"\}', show))
    assert rules['part:CompassWidget_mc'] == 'moved,sneaking,peek' and 'combat' in rules['part:HPMeter_mc']
    assert all('peek' in rule for rule in rules.values())
    known = {'combat', 'drawn', 'vats', 'peek', 'changed', 'low', 'moved', 'aiming', 'sneaking'}
    assert set(','.join(rules.values()).split(',')) <= known
    hud = _source(HUD_CPP)
    assert 'RegisterHudShow(movie, plugin);' in hud
    assert hud.index('ReadShowSettings();') < hud.index('if (root.HasMember(HOOKED)) return;')
    assert 'show.cpp' in _source(REPO / 'dll' / 'build.bat')
    layout = _source(REPO / 'dll' / 'framework' / 'layout.cpp')
    assert '(place.show.empty() ? "-" : place.show)' in layout and 'if (fields >> show && show != "-")' in layout
    assert '{"sHudPeek", "Y", "", \'Y\'}' in _source(REPO / 'dll' / 'framework' / 'keys.cpp')
    movie = _source(MENU.sources / 'ShowRules.as')
    assert 'state[word] === undefined' in movie and 'Number(state.linger) * 1000' in movie
    host = _source(MENU.sources / f'{MENU.document}.as')
    assert 'rules.update(dll, parts.watched());' in host and 'dll.HudState' in movie


def test_parts_show_as_their_values_change_run_low_or_the_location_moves():
    """Step 2: changed (a meter's value from HudValues, else the part's text), low (under a threshold, low:F to set it;
    the ammo count's magazine against the fullest seen with this weapon) and moved (the location text or the quest
    updates changed, a cleared text not counting) are worked out by the host; HudValues reads the player's health and
    AP as fractions, rads, the weapon and its condition."""
    show = _source(REPO / 'dll' / 'framework' / 'show.cpp')
    assert 'HEALTH = 0x2D4, ACTION_POINTS = 0x2D5, RADS = 0x2E1' in show
    assert all(f'SetMember("{name}"' in show for name in ('health', 'ap', 'rads', 'condition', 'weapon'))
    assert 'WeaponCondition(player, weapon)' in show
    rules = _source(MENU.sources / 'ShowRules.as')
    assert '"part:HPMeter_mc": "health"' in rules and '"part:CRUI_WeaponCondition_mc": "condition"' in rules
    assert 'WATCHED:Array = ["LocationText_mc", "QuestUpdates_mc", "StealthMeter_mc"];' in rules
    assert 'condition.split(":")' in rules and 'dll.HudValues()' in rules
    assert 'if (weapon != fullestWeapon)' in rules and 'movedFrame == frames' in rules
    start = rules.index('WATCHED:Array')
    named = set(re.findall(r'"(\w+_mc)"', rules[start:rules.index(';', start)]))
    layout = _source(MENU.sources / 'PartLayout.as')
    table = layout[layout.index('GROUPS:Object = {'):layout.index('};', layout.index('GROUPS:Object = {'))]
    assert named <= set(re.findall(r'"(\w+_mc)"', table))
    assert 'rules.shows(PREFIX + part.name, part.show, part.view)' in layout


def test_parts_show_while_aiming_or_sneaking():
    """Step 3: aiming is the DLL's (the player camera in its iron sights state, or the scope menu open); sneaking is the
    host's, the game's sneak meter on screen (it and every clip above it visible and not faded), which the engine
    shows only while sneaking and our mask doesn't change."""
    show = _source(REPO / 'dll' / 'framework' / 'show.cpp')
    assert 'PlayerCamera::kCameraState_IronSights' in show and '"ScopeMenu"' in show
    assert 'SetMember("aiming", &aiming);' in show
    rules = _source(MENU.sources / 'ShowRules.as')
    assert 'sneaking = watched != null && onScreen(watched[MOVED]);' in rules and 'word == "sneaking"' in rules
    on_screen = rules[rules.index('function onScreen('):rules.index('function textOf(')]
    assert 'if (!at.visible)' in on_screen and 'alpha *= at.alpha;' in on_screen and '.mask' not in on_screen


def test_the_edit_menu_sets_when_each_box_shows():
    """Step 4: the show key (V, [Keys] sHudShow) over a box opens a list in a pop-up: the preset's rule, always, each
    condition on or off, Done. Each pick goes to HudSetShow at once ("" the preset's, "always" none); while it is open
    the menu's own keys and mouse leave it alone. Every condition the DLL and the host know is on the list."""
    edit_menu = menus.BY_NAME['hud_edit']
    edit = _source(edit_menu.sources / f'{edit_menu.document}.as')
    assert 'e.keyCode == showKey' in edit and 'dll.HudSetShow(box, rule);' in edit and 'keys.showName' in edit
    on_key = edit[edit.index('private function onKey('):]
    assert on_key.index('if (modal.isOpen)') < on_key.index('var name:String = under();')
    rules = _source(edit_menu.sources / 'RuleList.as')
    listed = re.search(r'CONDITIONS:Array = \[([^\]]+)\]', rules).group(1)
    listed = set(re.findall(r'"(\w+)"', listed))
    show = _source(REPO / 'dll' / 'framework' / 'show.cpp')
    state = set(re.findall(r'SetMember\("(\w+)", &\w+\);', show[show.index('class StateNative'):]))
    host = _source(MENU.sources / 'ShowRules.as')
    assert listed == (state - {'linger', 'health', 'ap', 'rads', 'condition', 'weapon'}) | {
        word for word in ('changed', 'low', 'moved', 'sneaking') if f'word == "{word}"' in host}
    layout = _source(REPO / 'dll' / 'framework' / 'layout.cpp')
    assert 'entry.SetMember("showOwn", &own);' in layout and 'g_places[Arg(args, 0)].show = rule;' in layout
    assert '{"sHudShow", "V", "", \'V\'}' in _source(REPO / 'dll' / 'framework' / 'keys.cpp')
    hint = re.search(r'^\$CRUI_HUD_EDIT_HINT\t(.*)$', _source(REPO / 'translations' / 'CleanroomUI' / 'en.txt'), re.M)
    assert '{3}' in hint.group(1)


def test_other_mods_set_their_own_conditions_and_rules():
    """Step 5: a mod sets a condition (on, or while a global is not 0) under its own name, which HudState gives the host
    beside ours and the edit menu lists under its label; a name of ours is refused. A mod's rule for a box goes
    between the player's and the preset's. Both are forgotten as a save loads."""
    show = _source(REPO / 'dll' / 'framework' / 'show.cpp')
    state = show[show.index('class StateNative'):]
    assert 'for (const auto &[name, condition] : g_conditions)' in state
    assert 'condition.global->value != 0.0f' in state
    start = show.index('BUILT_IN[] = {')
    built_in = set(re.findall(r'"(\w+)"', show[start:show.index('};', start)]))
    rules = _source(menus.BY_NAME['hud_edit'].sources / 'RuleList.as')
    listed = set(re.findall(r'"(\w+)"', re.search(r'CONDITIONS:Array = \[([^\]]+)\]', rules).group(1)))
    assert listed <= built_in and {'always', 'linger'} <= built_in
    rule = show[show.index('std::string ShowRule('):]
    assert rule.index('place.show') < rule.index('g_defaults.find(name)') < rule.index('g_preset')
    hud = _source(HUD_CPP)
    forget = hud[hud.index('void ForgetHud()'):hud.index('void RegisterHudMenu(')]
    assert 'ForgetShow();' in forget
    edit = _source(menus.BY_NAME['hud_edit'].sources / 'CRUI_HudEdit.as')
    assert 'dll.HudConditions()' in edit and 'labels[condition]' in rules
