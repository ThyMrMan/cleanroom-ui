"""The example menu for authors: its own mod folder, never in our release, and it builds.

See: docs/api.md#example
"""

import configparser

import pytest

from crui import menus, mxmlc, papyrus
from crui.__main__ import build_example

EXAMPLE = menus.EXAMPLE
FOLDER = EXAMPLE.sources.parent


def test_the_example_is_not_part_of_the_release():
    """The release's menus leave the example out."""
    assert EXAMPLE not in menus.MENUS and EXAMPLE.name not in menus.BY_NAME
    assert EXAMPLE.sources == menus.EXAMPLES_DIR / 'example_menu' / 'as3'


def test_the_ini_is_valid_and_has_the_title():
    """The settings parse, with the title the movie reads, and no comment after a value."""
    ini = configparser.ConfigParser(inline_comment_prefixes=None)
    ini.read(FOLDER / 'ExampleMenu.ini', encoding='utf-8')
    assert ini['General']['sTitle'] == 'Example Menu'
    movie = (EXAMPLE.sources / 'ExampleMenu.as').read_text(encoding='utf-8')
    assert 'settings("ExampleMenu")' in movie and 'sTitle' in movie


def test_movie_and_script_agree_on_events():
    """The events the movie sends are the ones the script listens for."""
    movie = (EXAMPLE.sources / 'ExampleMenu.as').read_text(encoding='utf-8')
    script = (FOLDER / 'papyrus' / 'ExampleMenu.psc').read_text(encoding='utf-8')
    for event in ('ExampleMenu_Pick', 'ExampleMenu_Close'):
        assert f'"{event}"' in movie and f'RegisterForExternalEvent("{event}"' in script


@pytest.mark.skipif(not mxmlc.available(), reason='the Flex SDK is not installed')
def test_the_movie_builds_the_same_twice(tmp_path):
    """Two builds of the example are identical."""
    raw = menus.compile_movie(EXAMPLE, tmp_path / 'a.swf').read_bytes()
    assert raw == menus.compile_movie(EXAMPLE, tmp_path / 'b.swf').read_bytes() and raw[:3] == b'FWS'


@pytest.mark.skipif(not (mxmlc.available() and papyrus.available()), reason='needs the Flex SDK and Papyrus')
def test_build_example_writes_its_mod_folder(tmp_path):
    """`crui example` writes the movie, the script and the INI."""
    assert build_example(tmp_path, [])
    for path in ('Interface/ExampleMenu.swf', 'Scripts/ExampleMenu.pex', 'F4SE/Plugins/ExampleMenu.ini'):
        assert (tmp_path / path).is_file()
