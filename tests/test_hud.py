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
    opens)."""
    source = _source(HUD_CPP)
    assert 'Interface/' + re.search(r'HOST = "([^"]+)"', source).group(1) == MENU.movie
    hook = source[source.index('void HookMovie()'):source.index('void Hook()')]
    assert hook.index('if (root.HasMember(HOOKED)) return;') < hook.index('LoadChild(movie, root, HOST)')
    assert 'Ours()' not in hook and 'HasPartLayout' not in source


def test_the_movie_calls_every_native_the_dll_registers_for_it():
    """Each native hud.cpp registers for the host movie is called by it, with the shared HudColor and Log."""
    source = _source(HUD_CPP)
    movie = _source(MENU.sources / f'{MENU.document}.as')
    natives = re.findall(r'RegisterFunction<\w+>\(plugin, movie, "(\w+)"\);', source)
    assert sorted(natives) == ['HudMessages', 'HudStats', 'HudWidgets']
    assert all(f'dll.{name}(' in movie for name in natives + ['HudColor', 'Log'])
    assert 'RegisterFunction<LogNative>(plugin, movie, "Log");' in _source(REPO / 'dll' / 'base' / 'base.cpp')


def test_widgets_take_messages_through_process_message():
    """Widgets get processMessage(command, params)."""
    assert 'shown.content.processMessage(String(message.command), message.params as Array);' in _source(
        MENU.sources / f'{MENU.document}.as')


def test_the_native_script_declares_every_native_hud_cpp_registers():
    """CRUI_HudNative.psc declares each Papyrus native hud.cpp registers on its script, and no other."""
    source = _source(HUD_CPP)
    script_name = re.search(r'SCRIPT = "(\w+)"', source).group(1)
    show = _source(REPO / 'dll' / 'framework' / 'show.cpp')
    assert re.search(r'SCRIPT = "(\w+)"', show).group(1) == script_name and 'RegisterShow(vm);' in source
    registered = set(re.findall(r'BindNativeMethod\(SCRIPT, "(\w+)"', source + show))
    script = _source(REPO / 'papyrus' / f'{script_name}.psc')
    declared = set(re.findall(r'^\w*\s?Function (\w+)\(.*\) Global Native$', script, re.M))
    assert registered == declared == {'AddWidget', 'AddStats', 'RemoveWidget', 'Send',
                                      'SetCondition', 'SetConditionGlobal', 'RemoveCondition', 'SetShowRule'}


def test_the_hud_stands_alone():
    """Every widget is on our host: no other HUD framework is reached, and no host is chosen."""
    for path in (REPO / 'papyrus' / 'CRUI_Hud.psc', REPO / 'papyrus' / 'CRUI_HudNative.psc', HUD_CPP,
                 MENU.sources / f'{MENU.document}.as'):
        source = _source(path)
        assert 'HUDFramework' not in source and 'host ==' not in source and 'asHost' not in source, path.name
    assert 'hud_stats' not in menus.BY_NAME and not (MENU.sources / 'CRUI_HudStats.as').exists()


@pytest.mark.skipif(not mxmlc.available(), reason='the Flex SDK is not installed')
def test_the_movie_builds_the_same_twice(tmp_path):
    """Two uncompressed, undated builds are identical, with the host, its stats line and the natives it asks for."""
    raw = menus.compile_movie(MENU, tmp_path / 'a.swf').read_bytes()
    assert raw == menus.compile_movie(MENU, tmp_path / 'b.swf').read_bytes() and raw[:3] == b'FWS'
    assert not {t.code for t in Swf.parse(raw).tags} & set(mxmlc.DATED)
    for name in (b'CRUI_Hud', b'StatsLine', b'PartLayout', b'FnvPreset', b'ConditionBar', b'Clock', b'HudExtras',
                 b'HudWidgets',
                 b'HudMessages', b'HudStats', b'HudPartLayout', b'HudWeaponCondition', b'processMessage', b'crui'):
        assert name in raw


def test_the_edit_menu_and_host_use_every_layout_native():
    """layout.cpp's natives are each called: HudReport by the host, which applies a widget's place; HudLayout,
    HudMove, HudScale, HudReset, HudBackup, HudRestore and HudEditDone by the edit menu, which CRUI_Hud.EditLayout
    opens by its movie's name; the build compiles layout.cpp and HudWidgets hands each widget its place."""
    layout = _source(REPO / 'dll' / 'framework' / 'layout.cpp')
    natives = re.findall(r'RegisterFunction<\w+>\(plugin, movie, "(\w+)"\);', layout)
    assert sorted(natives) == ['HudBackup', 'HudEditDone', 'HudHide', 'HudLayout', 'HudLayoutPerSave', 'HudMove',
                               'HudPlaceOf', 'HudReport', 'HudReset', 'HudRestore', 'HudScale', 'HudSetShow',
                               'HudTint']
    host = _source(MENU.sources / f'{MENU.document}.as')
    parts = _source(MENU.sources / 'PartLayout.as')
    edit_menu = menus.BY_NAME['hud_edit']
    edit = _source(edit_menu.sources / f'{edit_menu.document}.as')
    assert 'dll.HudReport(' in host and 'view.scaleX = view.scaleY = Number(at.scale);' in host
    assert 'dll.HudReport(name,' in parts and 'dll.HudPlaceOf(name)' in parts
    assert all(f'dll.{name}(' in edit for name in natives if name not in ('HudReport', 'HudPlaceOf'))
    menu = edit_menu.movie.removeprefix('Interface/').removesuffix('.swf')
    assert f'String menu = "{menu}"' in _source(REPO / 'papyrus' / 'CRUI_Hud.psc')
    assert f'EDIT_MENU = "{menu}"' in layout
    assert 'entry.SetMember("place", place);' in _source(HUD_CPP)


def test_places_belong_to_each_save_with_a_file_as_backup():
    """By default places belong to each save (asked for 2026-11-15): the co-save's HUDL record, cleared for a new game
    and a load; [Hud] bLayoutPerSave=0 shares the file under Documents as before. That file is the backup the edit
    menu's keys write and restore (sHudBackup K, sHudRestore L). HudLayout lists only widgets on our host and the
    HUD's own parts, whose places the host keeps under the same prefix the DLL lists."""
    layout = _source(REPO / 'dll' / 'framework' / 'layout.cpp')
    plugin = _source(REPO / 'dll' / 'framework' / 'plugin.cpp')
    assert 'IniText("Hud", "bLayoutPerSave")' in layout and "RECORD = 'HUDL'" in layout
    for callback in ('RevertLayout', 'SaveLayoutRecord', 'LoadLayoutRecord'):
        assert f'void {callback}(const F4SE::SerializationInterface' in layout and f'({callback});' in plugin
    assert "SAVE_ID = 'CRUI'" in plugin and 'saves->SetUniqueID(SAVE_ID);' in plugin
    keys = _source(REPO / 'dll' / 'framework' / 'keys.cpp')
    assert '{"sHudBackup", "K", "", \'K\'}' in keys and '{"sHudRestore", "L", "", \'L\'}' in keys
    edit = _source(menus.BY_NAME['hud_edit'].sources / 'CRUI_HudEdit.as')
    assert 'e.keyCode == backupKey || e.keyCode == restoreKey' in edit
    assert 'CleanroomUI_Hud.txt' in layout and 'FOLDERID_Documents' in layout
    assert 'if (!OurWidget(name) && !name.starts_with(PART)) continue;' in layout
    prefix = re.search(r'PART = "([^"]+)"', layout).group(1)
    assert f'PREFIX:String = "{prefix}"' in _source(MENU.sources / 'PartLayout.as')
    assert f'name.indexOf("{prefix}") == 0' in _source(menus.BY_NAME['hud_edit'].sources / 'CRUI_HudEdit.as')
    assert 'LoadLayout();' in _source(HUD_CPP)
    done = layout[layout.index('class DoneNative'):layout.index('}  // namespace')]
    assert done.index('if (!g_perSave) Save();') < done.index('UI_MESSAGE_TYPE::kHide')


def test_a_placed_part_is_compared_with_what_it_reads_back():
    """A part is compared with its read-back place (swf_framework.md#hud-edit-fixes)."""
    parts = _source(MENU.sources / 'PartLayout.as')
    assert 'part.set = {x: view.x, y: view.y, scaleX: view.scaleX, scaleY: view.scaleY};' in parts
    assert 'part.set = target;' not in parts


def test_boxes_keep_one_size_while_their_part_animates():
    """An animated part (the Vault Boy, the hit arrows) reported changing bounds, so its box grew, shrank and slid
    (played 2026-11-15); parts and widgets report every bounds they have had, joined in their own space (Outline)."""
    parts, host = _source(MENU.sources / 'PartLayout.as'), _source(MENU.sources / f'{MENU.document}.as')
    outline = _source(MENU.sources / 'Outline.as')
    assert 'part.outline.measure(view, stage)' in parts and 'shown.outline.measure(view, stage)' in host
    assert 'view.getBounds(stage)' not in parts and 'joined.union(now)' in outline


def test_a_drag_never_loses_its_box():
    """A drag ends when the button is found up or the cursor leaves the movie, and a moved or scaled box stays on
    the screen (KeepOnScreen, for the mouse and the pad alike)."""
    edit = _source(menus.BY_NAME['hud_edit'].sources / 'CRUI_HudEdit.as')
    layout = _source(REPO / 'dll' / 'framework' / 'layout.cpp')
    assert 'if (dragging != null && !e.buttonDown)' in edit and 'Event.MOUSE_LEAVE, onLeave' in edit
    assert layout.count('KeepOnScreen(name, &place);') == 2


def test_a_box_behind_another_can_be_picked():
    """The cursor takes the smallest box under it, and a click where the last one was takes the next box under it
    (played 2026-11-16: the compass couldn't be picked under the subtitles); the box it works on is drawn bright."""
    edit = _source(menus.BY_NAME['hud_edit'].sources / 'CRUI_HudEdit.as')
    assert 'found.sortOn(["area", "order"], Array.NUMERIC);' in edit
    assert 'String(names[(at + 1) % names.length])' in edit and 'dragging = cycled;' in edit
    assert 'layout[i].name == hovered' in edit


def test_a_hidden_part_takes_its_shaded_background_with_it():
    """A hidden part keeps only the mask (no scrollRect, which left parts hidden once shown again); the engine's
    shaded background behind it goes off through its background-quad flag, back on only where we took it off
    (swf_framework.md#hud-look)."""
    look, parts = _source(MENU.sources / 'Look.as'), _source(MENU.sources / 'PartLayout.as')
    native = _source(REPO / 'dll' / 'framework' / 'parts.cpp')
    assert 'scrollRect' not in look and 'hiddenBounds' not in parts
    assert 'dll.HudBackgrounds(hidden);' in parts and 'var hidden:Array = looks.hidden();' in parts
    assert 'states.reset(Flags::kBackgroundQuad);' in native and 'g_quadless.insert(target);' in native
    assert '} else if (g_quadless.erase(target)) {' in native and 'states.set(Flags::kBackgroundQuad);' in native
    assert 'Get(at, "parent", &parent)' in native


def test_the_host_lays_out_the_hud_parts_from_def_hud_xml():
    """parts.cpp reads DEF_HUD.xml from where DEF_HUD's editor saves it and hands it to the host (HudPartLayout),
    which lays out and places the parts through PartLayout on its first frame, as a place changes and every few
    frames; the parts are the vanilla HUD's, in the seven groups the game pins to the safe area."""
    parts = _source(REPO / 'dll' / 'framework' / 'parts.cpp')
    assert r'".\\Data\\Interface\\DEF_CONF\\DEF_HUD.xml"' in parts
    registered = re.findall(r'RegisterFunction<\w+>\(plugin, movie, "(\w+)"\);', parts)
    assert registered == ['HudPartLayout', 'HudWeaponCondition', 'HudExtras', 'HudBackgrounds']
    assert 'RegisterHudParts(movie, plugin);' in _source(HUD_CPP)
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
    assert 'IniText("Hud", "sPreset")' in parts
    assert 'params.retVal->SetMember("preset", String(movie, preset));' in parts
    base = _source(REPO / 'dll' / 'base' / 'base.cpp')
    assert 'std::string IniText(' in base and 'MCM' in base
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
    assert 'g_tracked.contains(weapon->GetFormID())' in condition and 'return set < 0.0f ? 1.0 : double(set);' in condition


def test_a_clock_of_ours_shows_the_game_time_and_the_real_time():
    """[Hud] bClock adds our clock to TopRightGroup_mc as a part of ours (placed, colored and shown on rules like the
    game's, not logged missing while it is off); HudExtras gives its settings and the game's hour from
    Fallout4.esm's GameHour global, read again as the HUD loads and as the pause menu closes."""
    clock = _source(MENU.sources / 'Clock.as')
    assert 'NAME:String = "CRUI_Clock_mc";' in clock and 'GROUP:String = "TopRightGroup_mc";' in clock
    assert '"$CRUI_CLOCK_REAL\\t" + realText' in clock
    parts = _source(REPO / 'dll' / 'framework' / 'parts.cpp')
    assert 'GAME_HOUR = 0x38;' in parts and 'As<RE::TESGlobal>()' in parts
    assert 'RegisterFunction<ExtrasNative>(plugin, movie, "HudExtras");' in parts
    for name in ('bClock', 'bClockRealTime', 'bClock24', 'bPowerArmorMeters'):
        assert f'IniText("Hud", "{name}") == "1"' in parts
    hud = _source(HUD_CPP)
    assert hud.count('ReadPartSettings();') == 2
    layout = _source(MENU.sources / 'PartLayout.as')
    assert 'parts[Clock.NAME] = {group: Clock.GROUP, name: Clock.NAME, entry: null, ours: true};' in layout
    assert 'showClock(hud);' in layout and 'if (found == null && part.ours != true)' in layout
    assert 'dll.HudExtras()' in layout
    words = _source(REPO / 'translations' / 'CleanroomUI' / 'en.txt')
    assert re.search(r'^\$CRUI_CLOCK_REAL\t.*\{0\}', words, re.M)


def test_clock_times_read_as_the_game_shows_them():
    """12-hour as the Pip-Boy shows it (12 AM at midnight, 12 PM at noon), or 24-hour with leading zeros."""
    clock = _source(MENU.sources / 'Clock.as')
    assert 'var twelve:int = hours % 12 == 0 ? 12 : hours % 12;' in clock
    assert '(hours < 12 ? " AM" : " PM")' in clock and '(hours < 10 ? "0" : "") + String(hours)' in clock
    assert 'int(Math.floor(hour * 60)) % (24 * 60)' in clock


def test_power_armor_is_a_condition_and_its_meters_an_option():
    """powerarmor holds while the HUD is in its power armor mode (the active effects widget's bInPowerArmorMode);
    with [Hud] bPowerArmorMeters the host keeps the HP and AP meters, and the ammo count with a weapon out, visible
    in power armor in play, the one engine visibility we set, and hides one it stops keeping."""
    rules = _source(MENU.sources / 'ShowRules.as')
    assert 'powerArmor = watched != null && inPowerArmor(watched[POWER]);' in rules
    assert 'Object(view).bInPowerArmorMode == true' in rules and 'word == "powerarmor"' in rules
    show = _source(REPO / 'dll' / 'framework' / 'show.cpp')
    assert '"powerarmor"' in show[show.index('BUILT_IN[]'):show.index('};', show.index('BUILT_IN[]'))]
    layout = _source(MENU.sources / 'PartLayout.as')
    assert 'ARMOR_METERS:Object = {HPMeter_mc: "", ActionPointMeter_mc: "", AmmoCount_mc: "drawn"};' in layout
    keep = layout[layout.index('private function keepArmorMeters('):layout.index('private static function crosshairShown(')]
    assert 'extras.powerArmorMeters == true && armor' in keep and 'view.visible = true;' in keep
    assert 'else if (kept[name] == true && armor)' in keep
    assert layout.count('.visible = ') == 2
    words = _source(REPO / 'translations' / 'CleanroomUI' / 'en.txt')
    assert re.search(r'^\$CRUI_SHOW_POWERARMOR\t', words, re.M)


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
    assert 'IsInCombat()' in show and 'GetWeaponMagicDrawn()' in show and 'MenuOpen("VATSMenu")' in show
    assert 'GetAsyncKeyState(key)' in show and 'GameInFront()' in show
    assert 'IniText("Hud", "sShow")' in show and 'IniText("Hud", "fShowLinger")' in show
    rules = dict(re.findall(r'\{"(part:\w+)", "([\w,]+)"\}', show))
    assert rules['part:CompassWidget_mc'] == 'moved,sneaking,peek' and 'combat' in rules['part:HPMeter_mc']
    assert all('peek' in rule for rule in rules.values())
    known = {'combat', 'drawn', 'vats', 'peek', 'changed', 'low', 'moved', 'aiming', 'sneaking'}
    assert set(','.join(rules.values()).split(',')) <= known
    hud = _source(HUD_CPP)
    assert 'RegisterHudShow(movie, plugin);' in hud
    assert hud.index('ReadShowSettings();') < hud.index('if (root.HasMember(HOOKED)) return;')
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
    assert re.search(r'WATCHED:Array = \["LocationText_mc", "QuestUpdates_mc", "StealthMeter_mc",\s+'
                     r'"HUDActiveEffectsWidget_mc"\];', rules)
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
    assert 'cameraStates[RE::CameraStates::kIronSights]' in show and 'MenuOpen("ScopeMenu")' in show
    assert 'SetMember("aiming", Aiming());' in show
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
    state = set(re.findall(r'SetMember\("(\w+)", ', show[show.index('class StateNative'):]))
    host = _source(MENU.sources / 'ShowRules.as')
    assert listed == (state - {'linger', 'health', 'ap', 'rads', 'condition', 'weapon'}) | {
        word for word in ('changed', 'low', 'moved', 'sneaking', 'powerarmor') if f'word == "{word}"' in host}
    layout = _source(REPO / 'dll' / 'framework' / 'layout.cpp')
    assert 'entry.SetMember("showOwn", String(movie, place.show));' in layout
    assert 'g_places[Arg(params, 0)].show = rule;' in layout
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
    assert 'condition.global->GetValue() != 0.0f' in state
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


def test_the_edit_menu_works_with_a_gamepad():
    """With a pad one box is picked: LB/RB pick, the held D-pad moves it, LT/RT scale, X hides, Y colors, A opens its
    rule list, View resets, B saves and closes. The pad's buttons are mapped only while a pad is in use, since FO4
    sends some keyboard keys (R, C, Q, V, T) as the same controls; the D-pad goes to the list while it is open."""
    edit_menu = menus.BY_NAME['hud_edit']
    edit = _source(edit_menu.sources / f'{edit_menu.document}.as')
    pad = dict(re.findall(r'input\.mapControl\("(\w+)", ([\w.]+), true\);', edit))
    assert pad == {'LShoulder': 'Input.PREV', 'RShoulder': 'Input.NEXT', 'LTrigger': 'SMALLER', 'RTrigger': 'BIGGER',
                   'XButton': 'Input.X', 'YButton': 'Input.Y', 'Select': 'RESET'}
    assert 'public function ProcessUserEvent(control:String, down:Boolean) : Boolean' in edit
    assert 'public function SetPlatform(platform:uint, ps3Switch:Boolean) : void' in edit
    process = edit[edit.index('public function ProcessUserEvent('):edit.index('public function SetPlatform(')]
    assert 'input.gamepad && !modal.isOpen && DIRECTIONS[control] != null' in process
    on_action = edit[edit.index('private function onAction('):edit.index('private function recolor(')]
    assert on_action.index('if (modal.isOpen)') < on_action.index('if (!input.gamepad')
    for call in ('dll.HudHide(picked)', 'recolor(dll, picked)', 'dll.HudReset(picked)', 'openRules(picked)',
                 'dll.HudEditDone()', 'dll.HudScale(picked,'):
        assert call in on_action
    words = _source(REPO / 'translations' / 'CleanroomUI' / 'en.txt')
    assert re.search(r'^\$CRUI_HUD_EDIT_PAD_HINT\t', words, re.M)
