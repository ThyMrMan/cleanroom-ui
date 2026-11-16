"""The DLL split: CleanroomUI.dll (the framework) and CleanroomDialogue.dll, each under its own name, the movies
asking each by that name, and the framework changing nothing until a mod asks.

See: docs/spin_offs.md#split
"""

import re

from crui import REPO

DLL = REPO / 'dll'
FRAMEWORK, DIALOGUE = DLL / 'framework', DLL / 'dialogue'


def _scaleform_name(entry):
    """The f4se.plugins name `entry` (a plugin.cpp) registers under."""
    source = entry.read_text(encoding='utf-8')
    name = re.search(r'scaleform->Register\((\w+|"\w+")', source).group(1)
    return name.strip('"') if name.startswith('"') else re.search(rf'{name} = "(\w+)"', source).group(1)


def test_each_dll_is_built_and_named_as_its_movies_ask():
    """xmake.lua builds CleanroomUI.dll and CleanroomDialogue.dll (CommonLibF4), each under the F4SE plugin name it
    gives and the Scaleform name its movie asks for; the F4SE 0.7.9 build is gone."""
    xmake = (DLL / 'xmake.lua').read_text(encoding='utf-8')
    assert not (DLL / 'build.bat').exists() and not (DLL / 'shared').exists()
    for name in ('CleanroomUI', 'CleanroomDialogue'):
        assert f'target("{name}")' in xmake and f'name = "{name}"' in xmake
    assert 'author = "cleanroom-ui"' in xmake
    for entry, name, movie in ((FRAMEWORK / 'plugin.cpp', 'CleanroomUI', REPO / 'menus/pipboy/as3/CRUI_Pipboy.as'),
                               (DIALOGUE / 'plugin.cpp', 'CleanroomDialogue',
                                REPO / 'menus/dialogue/as3/DialogueMenu.as')):
        assert _scaleform_name(entry) == name
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
    """The C++ function at `signature` in `source`, to its column-0 brace."""
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
    base = (DLL / 'base' / 'base.cpp').read_text(encoding='utf-8')
    assert '"Keys"' in keys and 'MCM\\\\Settings\\\\" + mod + ".ini"' in base
    assert 'return IniText("CleanroomUI", section, name);' in (FRAMEWORK / 'condition.h').read_text(encoding='utf-8')
    assert all(f'"{name}"' in keys for name in ('sRepair', 'sAmmo', 'sHudReset'))
    pipboy = (FRAMEWORK / 'pipboy.cpp').read_text(encoding='utf-8')
    ammo = (FRAMEWORK / 'ammo.cpp').read_text(encoding='utf-8')
    for source in (pipboy, ammo):
        assert not re.search(r'KEY_[GB]\b|HINT_KEY', source)
    assert 'KeyName(OurKey::Repair)' in pipboy and 'KeyName(OurKey::Ammo)' in ammo
    assert 'ReadKeys();' in _body(pipboy, 'class OpenSink')
    plugin = (FRAMEWORK / 'plugin.cpp').read_text(encoding='utf-8')
    assert 'ReadKeys();' in plugin and 'RegisterKeys(movie, plugin);' in plugin
    assert '"framework/*.cpp"' in (DLL / 'xmake.lua').read_text(encoding='utf-8')
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
    """GetVersion, the version query and F4SE's plugin version (VERSION.0.0, read by xmake.lua) all come from the
    one VERSION constant."""
    plugin = (FRAMEWORK / 'plugin.cpp').read_text(encoding='utf-8')
    assert 'return VERSION;' in plugin and 'std::uint32_t version = VERSION;' in plugin
    assert re.search(r'constexpr std::uint32_t VERSION = \d+;', (FRAMEWORK / 'condition.h').read_text(encoding='utf-8'))
    xmake = (DLL / 'xmake.lua').read_text(encoding='utf-8')
    assert 'header:match("constexpr std::uint32_t VERSION = (%d+);")' in xmake


def test_both_dlls_log_at_the_ini_level():
    """Each DLL logs at its own INI's [Log] iLevel (LogLevel into F4SE::Init)."""
    for entry in (FRAMEWORK / 'plugin.cpp', DIALOGUE / 'plugin.cpp'):
        assert '.logLevel = LogLevel(NAME), .logName = NAME' in entry.read_text(encoding='utf-8')
    assert '"iLevel"' in (DLL / 'base' / 'base.cpp').read_text(encoding='utf-8')


def test_the_dialogue_dll_hooks_only_where_its_layouts_were_read():
    """Every address in dialogue.cpp is an old-gen and a next-gen Address Library ID (its 1.11.240 RVA beside it),
    resolved without ending the game when a database lacks it; the hooks wait for the checked runtime."""
    dialogue = (DIALOGUE / 'dialogue.cpp').read_text(encoding='utf-8')
    ids = re.findall(r'= Both\(\d+, \d+(, 0x[0-9A-F]+){0,2}\);\s+// 0x', dialogue)
    assert 'RelocAddr' not in dialogue and 'Next(' not in dialogue and len(ids) == 24
    assert 'if (!CheckedRuntime())' in dialogue and 'if (!ResolveEngine())' in dialogue
    base = (DLL / 'base' / 'base.cpp').read_text(encoding='utf-8')
    assert 'if (!id || !db->has(id)) return 0;' in base
    checked = re.search(r'CHECKED\[\] = \{(.+?)\};', base, re.S).group(1)
    assert set(re.findall(r'\{1, (\d+), (\d+), 0\}', checked)) == {
        ('11', '240'), ('10', '163'), ('10', '984'), ('11', '137'), ('11', '159'), ('11', '169'), ('11', '191'),
        ('11', '221')}
    assert 'GetModuleHandleW(L"steam_api64.dll")' in _body(base, 'bool CheckedRuntime(')
    assert 'bool has(std::uint64_t a_id) const noexcept;' in (DLL / 'patches' / 'commonlib-shared.patch').read_text(
        encoding='utf-8')


def test_the_dialogue_release_stands_alone():
    """CleanroomDialogue.dll loads its own words (no plugin of ours needed) and reads its links from its own folder,
    the converter's old F3NV_Menus folder still read until the converter writes ours."""
    plugin = (DIALOGUE / 'plugin.cpp').read_text(encoding='utf-8')
    assert 'translator->AddTranslationsMod(NAME);' in plugin and 'kInputLoaded' in plugin
    dialogue = (DIALOGUE / 'dialogue.cpp').read_text(encoding='utf-8')
    assert re.search(r'LINKS = "Data.+CleanroomDialogue.+";', dialogue)
    assert 'ReadFolder(LINKS)' in dialogue and 'ReadFolder(LEGACY_LINKS)' in dialogue


def test_other_mods_name_movies_for_menus():
    """Each Menus/*.ini section names a menu or its movie file, with sMovie and sParent (root by default); the movie
    loads once per open menu, the lists read as the game loads."""
    inject = (FRAMEWORK / 'inject.cpp').read_text(encoding='utf-8')
    assert 'CleanroomUI' + chr(92) * 2 + 'Menus"' in inject
    assert '"sMovie"' in inject and '"sParent"' in inject and '"root"' in inject
    assert 'ends_with(".swf")' in inject and 'GetFileURL()' in inject
    assert 'g_loaded.emplace(name, i).second' in inject and 'std::erase_if(g_loaded' in inject
    assert 'InstallInjections();' in _body((FRAMEWORK / 'plugin.cpp').read_text(encoding='utf-8'), 'void OnMessage(')


def test_movies_read_a_mods_settings():
    """Every movie gets Settings(mod): a plain file name only, both INI files, values typed by prefix; Bridge
    reaches it, and injected movies find F4SE through Bridge.attach."""
    base = (DLL / 'base' / 'base.cpp').read_text(encoding='utf-8')
    assert 'RegisterFunction<SettingsNative>(plugin, movie, "Settings");' in base
    assert 'if (!PlainName(mod)) return;' in base and 'mod.find("..")' in base
    assert all(f"case '{prefix}'" in _body(base, 'Value Typed(') for prefix in 'biuf')
    bridge = (REPO / 'as3' / 'crui' / 'Bridge.as').read_text(encoding='utf-8')
    assert 'dll.Settings(mod)' in bridge and 'public function attach(stage:Stage)' in bridge
    for movie in ('pipboy/as3/CRUI_Pipboy.as', 'transfer/as3/CRUI_Transfer.as', 'hud/as3/CRUI_HudStats.as'):
        assert 'bridge.attach(stage);' in (REPO / 'menus' / movie).read_text(encoding='utf-8')
