"""The level-up host's MCM page: filled for a mod's own quest, naming only properties and a function the host has.

See: docs/level_up_plan.md#host-mcm
"""

import json
import re

from crui import REPO, mcm

HOST = (REPO / 'papyrus' / 'CRUI_LevelUp.psc').read_text(encoding='utf-8')


def test_the_page_is_filled_for_the_mods_own_quest(tmp_path):
    """install() writes config.json and keybinds.json with no placeholder left, the hotkey's id the same in both."""
    folder = mcm.install(tmp_path, 'MyMod', 'My Mod', 'MyMod.esp|800')
    assert folder == tmp_path / 'MCM' / 'Config' / 'MyMod'
    config = json.loads((folder / 'config.json').read_text(encoding='utf-8'))
    keys = json.loads((folder / 'keybinds.json').read_text(encoding='utf-8'))
    assert not re.search(r'"\{\w+\}"', json.dumps([config, keys]))
    assert config['modName'] == keys['modName'] == 'MyMod' and config['displayName'] == 'My Mod'
    assert [e['id'] for e in config['content'] if 'id' in e] == [keys['keybinds'][0]['id']] == ['MyMod_open_level_up']
    assert keys['keybinds'][0]['action']['form'] == 'MyMod.esp|800'


def test_the_page_names_what_the_host_has():
    """Each property the page reads is a property of CRUI_LevelUp the save keeps (Auto, not Const), and the hotkey
    calls one of its functions."""
    page = mcm.level_up_page('MyMod', 'My Mod', 'MyMod.esp|800')
    options = [e['valueOptions'] for e in page['config.json']['content'] if 'valueOptions' in e]
    assert {o['scriptName'] for o in options} == {'CRUI_LevelUp'}
    for option in options:
        assert re.search(rf'^\w+ Property {option["propertyName"]} = \S+ Auto$', HOST, re.M), option
    called = page['keybinds.json']['keybinds'][0]['action']['function']
    assert re.search(rf'^Function {called}\(\)', HOST, re.M)
    assert mcm.level_up_page('M', 'M', 'M.esp|800', 'MyHost')['keybinds.json']['keybinds'][0]['action'][
        'scriptName'] == 'MyHost'
