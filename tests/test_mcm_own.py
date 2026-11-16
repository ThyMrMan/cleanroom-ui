"""Our own MCM page: the HUD edit menu from a button and a hotkey, and only settings the DLL reads.

See: docs/swf_framework.md#mcm-page
"""

import configparser
import json
import re

from crui import REPO, mcm

FOLDER = mcm.OWN
CONFIG = json.loads((FOLDER / 'config.json').read_text(encoding='utf-8'))
KEYS = json.loads((FOLDER / 'keybinds.json').read_text(encoding='utf-8'))
DLL = '\n'.join(p.read_text(encoding='utf-8') for p in (REPO / 'dll' / 'framework').glob('*.cpp'))
CONTROLS = [entry for entry in CONFIG['content'] if 'id' in entry and entry['type'] != 'hotkey']


def test_the_button_and_hotkey_open_the_edit_menu():
    """Both call CRUI_Hud.EditLayout, a Global function; the hotkey's id is the same in both files."""
    actions = [e['action'] for e in CONFIG['content'] if e['type'] == 'button'] + [KEYS['keybinds'][0]['action']]
    assert all(a == {'type': 'CallGlobalFunction', 'script': 'CRUI_Hud', 'function': 'EditLayout', 'params': []}
               for a in actions) and len(actions) == 2
    assert 'Function EditLayout() Global' in (REPO / 'papyrus' / 'CRUI_Hud.psc').read_text(encoding='utf-8')
    hotkeys = [e['id'] for e in CONFIG['content'] if e['type'] == 'hotkey']
    assert hotkeys == [KEYS['keybinds'][0]['id']] and CONFIG['modName'] == KEYS['modName'] == mcm.OWN_NAME


def test_every_setting_is_one_the_dll_reads_with_its_default():
    """Each control's name:Section is read by the DLL, and settings.ini gives each one, as the DLL defaults it."""
    defaults = configparser.ConfigParser(inline_comment_prefixes=None)
    defaults.optionxform = str
    defaults.read(FOLDER / 'settings.ini', encoding='utf-8')
    keys = dict(re.findall(r'\{"(s\w+)", "(\w+)", "\w*", \'\w+\'\}', DLL))
    for entry in CONTROLS:
        name, section = entry['id'].split(':')
        read = f'IniText("{section}", "{name}")' in DLL or (section == 'Keys' and name in keys)
        assert read, entry['id']
        assert defaults[section][name] == keys.get(name, defaults[section][name]), entry['id']
    assert 'DEFAULT_LINGER = 3.0f' in DLL and defaults['Hud']['fShowLinger'] == '3.0'
    assert 'g_perSave = setting.empty() ||' in DLL and defaults['Hud']['bLayoutPerSave'] == '1'


def test_every_word_on_the_page_is_translated():
    """Each $CRUI_MCM_ key the page shows is in our English words."""
    words = (REPO / 'translations' / 'CleanroomUI' / 'en.txt').read_text(encoding='utf-8')
    shown = set(re.findall(r'\$CRUI_MCM_\w+', json.dumps(CONFIG)))
    assert shown and all(f'{key}\t' in words for key in shown)


def test_the_build_installs_the_page(tmp_path):
    """The mcm step writes the three files under MCM/Config/CleanroomUI."""
    folder = mcm.install_own(tmp_path)
    assert sorted(p.name for p in folder.iterdir()) == ['config.json', 'keybinds.json', 'settings.ini']
    assert folder == tmp_path / 'MCM' / 'Config' / 'CleanroomUI'
