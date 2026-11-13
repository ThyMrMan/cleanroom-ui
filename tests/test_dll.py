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
        assert f'\\\\{name}.log"' in source
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
    assert scripts == {'CRUI_Condition', 'CRUI_ItemCard', 'CRUI_Pipboy', 'CRUI_PipboyAmmo', 'CRUI_Sex'}
    assert all((REPO / 'papyrus' / f'{name}.psc').is_file() for name in scripts)
