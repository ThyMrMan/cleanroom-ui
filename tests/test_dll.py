"""The DLL split: CleanroomUI.dll (the framework) and CleanroomDialogue.dll, each under its own name, the movies
asking each by that name, and the framework changing nothing until a mod asks.

See: docs/spin_offs.md#split
"""

import re

from crui import REPO

DLL = REPO / 'dll'
FRAMEWORK, DIALOGUE = DLL / 'framework', DLL / 'dialogue'


def _scaleform_name(entry):
    """The name `entry` (a plugin.cpp) registers its movie natives under (f4se.plugins.<name>)."""
    source = entry.read_text(encoding='utf-8')
    name = re.search(r'scaleform->Register\((\w+|"\w+")', source).group(1)
    return name.strip('"') if name.startswith('"') else re.search(rf'{name} = "(\w+)"', source).group(1)


def test_each_dll_is_built_and_named_as_its_movies_ask():
    """build.bat builds each DLL under the F4SE plugin name it gives and the Scaleform name its movie asks for."""
    build = (DLL / 'build.bat').read_text(encoding='utf-8')
    for entry, name, movie in ((FRAMEWORK / 'plugin.cpp', 'CleanroomUI', REPO / 'menus/pipboy/as3/CRUI_Pipboy.as'),
                               (DIALOGUE / 'plugin.cpp', 'CleanroomDialogue',
                                REPO / 'menus/dialogue/as3/DialogueMenu.as')):
        source = entry.read_text(encoding='utf-8')
        assert f'call :plugin {name} {entry.parent.name} plugin.cpp' in build
        assert _scaleform_name(entry) == name and f'"{name}",\n    "cleanroom-ui",' in source.replace('\r\n', '\n')
        assert re.search(rf'OpenLog\(("{name}"|NAME)\);', source)
        assert f'private static const DLL:String = "{name}";' in movie.read_text(encoding='utf-8')
    bridge = (REPO / 'as3' / 'crui' / 'Bridge.as').read_text(encoding='utf-8')
    assert 'natives("CleanroomUI")' in bridge and 'natives("CleanroomDialogue")' in bridge


def test_the_framework_changes_nothing_at_load():
    """The framework takes no trampoline and writes no game setting; the FNV rules (Damage Threshold, Hardcore,
    spawn condition, combat sprint) are not in this repo."""
    framework = ''.join(p.read_text(encoding='utf-8') for p in FRAMEWORK.glob('*.*'))
    for gone in ('AllocateFromBranchPool', 'g_branchTrampoline', 'NoCombatSprint', 'SpawnMaxHealth', 'HookHud',
                 'HookDamage', 'HookAmmoWeight', 'HookDialogue'):
        assert gone not in framework, gone
    assert not list(DLL.rglob('damage.cpp')) and not list(DLL.rglob('hardcore.cpp'))


def test_every_native_script_the_dlls_register_exists():
    """Each script a DLL registers natives on (its SCRIPT constants) is one of ours, named CRUI_."""
    scripts = set()
    for source in DLL.rglob('*.cpp'):
        scripts |= set(re.findall(r'\b\w*SCRIPT = "(\w+)"', source.read_text(encoding='utf-8')))
    assert scripts == {'CRUI_Condition', 'CRUI_Framework', 'CRUI_HudNative', 'CRUI_ItemCard', 'CRUI_LevelUpNative',
                       'CRUI_Pipboy', 'CRUI_PipboyAmmo', 'CRUI_Sex'}
    assert all((REPO / 'papyrus' / f'{name}.psc').is_file() for name in scripts)


def _body(source, signature):
    """The text of the C++ function starting at `signature` in `source`, up to its closing brace at column 0."""
    start = source.index(signature)
    return source[start:source.index('\n}', start)]


def test_hints_and_keys_wait_for_a_mod():
    """The REPAIR hint and the repair key wait for a tracked item, the AMMO hint and the ammo key for a mod's ammo
    lists."""
    pipboy = (FRAMEWORK / 'pipboy.cpp').read_text(encoding='utf-8')
    ammo = (FRAMEWORK / 'ammo.cpp').read_text(encoding='utf-8')
    hint = _body(pipboy, 'void AddHint(')
    assert 'repair && MakeHint(' in hint and 'ammo && MakeAmmoHint(' in hint
    assert 'key == KeyCode(OurKey::Repair) && RepairInUse()' in pipboy
    assert "key != KeyCode(OurKey::Ammo) || !AmmoInUse()" in _body(ammo, 'bool AmmoKey(')


def test_our_keys_are_settings():
    """The Pip-Boy's keys and hints and the edit menu's reset key come from [Keys] in CleanroomUI.ini (MCM's saved
    copy over it), read again as the Pip-Boy opens; no fixed G, B or R is left."""
    keys = (FRAMEWORK / 'keys.cpp').read_text(encoding='utf-8')
    assert '"Keys"' in keys and 'Data\\\\MCM\\\\Settings\\\\CleanroomUI.ini' in keys
    assert all(f'"{name}"' in keys for name in ('sRepair', 'sAmmo', 'sHudReset'))
    pipboy = (FRAMEWORK / 'pipboy.cpp').read_text(encoding='utf-8')
    ammo = (FRAMEWORK / 'ammo.cpp').read_text(encoding='utf-8')
    for source in (pipboy, ammo):
        assert not re.search(r'KEY_[GB]\b|HINT_KEY', source)
    assert 'KeyName(OurKey::Repair)' in pipboy and 'KeyName(OurKey::Ammo)' in ammo
    assert 'ReadKeys();' in _body(pipboy, 'class OpenSink')
    plugin = (FRAMEWORK / 'plugin.cpp').read_text(encoding='utf-8')
    assert 'ReadKeys();' in plugin and 'RegisterKeys(movie, plugin);' in plugin
    assert 'keys.cpp' in (DLL / 'build.bat').read_text(encoding='utf-8')
    edit = (REPO / 'menus' / 'hud' / 'as3' / 'CRUI_HudEdit.as').read_text(encoding='utf-8')
    assert 'dll.HudKeys()' in edit and 'e.keyCode == resetKey' in edit and 'KEY_R' not in edit


def test_several_mods_add_their_lists():
    """A mod's ammo lists and repair lists add to other mods' (a list given again replaces only its own entry);
    ammo lists are forgotten as a save loads, so each mod gives its own again."""
    ammo = (FRAMEWORK / 'ammo.cpp').read_text(encoding='utf-8')
    assert '.clear()' not in _body(ammo, 'void SetLists(')
    assert 'ForgetAmmoLists();' in (FRAMEWORK / 'plugin.cpp').read_text(encoding='utf-8')
    repair = _body((FRAMEWORK / 'repair.cpp').read_text(encoding='utf-8'), 'void TrackRepair(')
    assert 'push_back(list)' in repair and '.insert(group)' in repair


def test_one_version_everywhere():
    """GetVersion, the version query and F4SE's plugin version all read the one VERSION constant."""
    plugin = (FRAMEWORK / 'plugin.cpp').read_text(encoding='utf-8')
    assert 'crui::VERSION,' in plugin and 'return VERSION;' in plugin and 'UInt32 version = VERSION;' in plugin
    assert re.search(r'constexpr UInt32 VERSION = \d+;', (FRAMEWORK / 'condition.h').read_text(encoding='utf-8'))


def test_both_dlls_log_at_the_ini_level():
    """Each DLL opens its log through OpenLog, which reads [Log] iLevel from its own INI."""
    for entry, name in ((FRAMEWORK / 'plugin.cpp', '"CleanroomUI"'), (DIALOGUE / 'plugin.cpp', 'NAME')):
        source = entry.read_text(encoding='utf-8')
        assert f'OpenLog({name});' in source and 'gLog.OpenRelative' not in source
    assert '"iLevel"' in (DLL / 'shared' / 'gfx.cpp').read_text(encoding='utf-8')


def test_the_dialogue_release_stands_alone():
    """CleanroomDialogue.dll loads its own words (no plugin of ours needed) and reads its links from its own folder,
    the converter's old F3NV_Menus folder still read until the converter writes ours."""
    plugin = (DIALOGUE / 'plugin.cpp').read_text(encoding='utf-8')
    assert 'Translation::ParseTranslation(translator, name);' in plugin and 'kMessage_InputLoaded' in plugin
    dialogue = (DIALOGUE / 'dialogue.cpp').read_text(encoding='utf-8')
    assert re.search(r'LINKS = "Data.+CleanroomDialogue.+";', dialogue)
    assert 'ReadFolder(LINKS)' in dialogue and 'ReadFolder(LEGACY_LINKS)' in dialogue
