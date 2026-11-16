"""The Pip-Boy on a pad: View opens our action menu over the Pip-Boy, which lists the actions in use and then works
the open list as the Pip-Boy's own keys.

See: docs/swf_framework.md#pipboy-pad
"""

import re

from crui import REPO, menus

PAD_CPP = REPO / 'dll' / 'framework' / 'pad.cpp'
MENU = menus.BY_NAME['pipboy_actions']


def _source(path):
    """The text of `path`."""
    return path.read_text(encoding='utf-8')


def test_view_released_in_the_pipboy_opens_the_menu_through_its_script():
    """The handler takes only View's release, while the Pip-Boy is open and an action is in use, and calls the
    script that opens the menu the build installs."""
    pad = _source(PAD_CPP)
    assert 'BS_BUTTON_CODE::kBack && button->QReleased()' in pad
    assert '!ui->GetMenuOpen(PIPBOY) || ui->GetMenuOpen(ACTIONS) || (!RepairInUse() && !AmmoInUse())' in pad
    assert 'DispatchStaticCall(ACTIONS, "Open", none)' in pad and 'RegisterHandler(&g_padHandler)' in pad
    script = _source(REPO / 'papyrus' / 'CRUI_PipboyActions.psc')
    assert 'UI.RegisterCustomMenu(menu, menu, "root1", data)' in script and 'UI.OpenMenu(menu)' in script
    assert f'"{MENU.document}"' in script and MENU.movie == f'Interface/{MENU.document}.swf'
    assert 'InstallPad();' in _source(REPO / 'dll' / 'framework' / 'pipboy.cpp')


def test_the_menu_calls_every_native_pad_cpp_registers():
    """Each native pad.cpp registers is called by the action menu."""
    natives = re.findall(r'RegisterFunction<\w+>\(plugin, movie, "(\w+)"\);', _source(PAD_CPP))
    movie = _source(MENU.sources / f'{MENU.document}.as')
    assert sorted(natives) == ['PipboyActions', 'PipboyActionsDone', 'PipboyListOpen', 'PipboyPress']
    assert all(f'dll.{name}(' in movie for name in natives)


def test_the_pad_works_the_lists_through_the_pipboys_own_keys():
    """The menu turns the pad's moves, A and B into the keys the lists already take; the Pip-Boy's key handler and
    the menu share one function."""
    movie = _source(MENU.sources / f'{MENU.document}.as')
    assert 'KEYS:Object = {up: 38, down: 40, accept: 13, back: 27}' in movie
    pipboy = _source(REPO / 'dll' / 'framework' / 'pipboy.cpp')
    assert 'PipboyKey(params.movie, int(Num(code)));' in pipboy
    assert 'PipboyKey(pipboy, int(Num(params.args[0])));' in _source(PAD_CPP)
