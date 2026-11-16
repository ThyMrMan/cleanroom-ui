"""docs/api.md: its generated reference matches the sources, and its own words stay outside the markers.

See: docs/api.md#reference
"""

import pytest

from crui import api


def test_the_page_holds_the_current_reference():
    """The text between the markers is what `python -m crui api` would write now."""
    text = api.PAGE.read_text(encoding='utf-8')
    assert text.count(api.START) == 1 and text.count(api.END) == 1
    assert api.reference() in text, 'docs/api.md is out of date: run python -m crui api'


def test_the_reference_covers_scripts_natives_and_library():
    """Each part names something it must: a script, a native and a library class."""
    reference = api.reference()
    for name in ('## Papyrus', '### CRUI_Hud', '## Movie natives', '`Settings`', '### Bridge', '### ScrollList'):
        assert name in reference


def test_write_keeps_the_words_around_the_markers(tmp_path):
    """write() replaces only what is between the markers, then reports no change."""
    page = tmp_path / 'api.md'
    page.write_text(f'Intro\n{api.START}\nSTALE-TEXT\n{api.END}\nOutro\n', encoding='utf-8')
    assert api.write(page) and not api.write(page)
    text = page.read_text(encoding='utf-8')
    assert text.startswith('Intro\n') and text.endswith('\nOutro\n') and 'STALE-TEXT' not in text


def test_write_needs_both_markers(tmp_path):
    """A page without the markers is refused."""
    page = tmp_path / 'api.md'
    page.write_text('no markers', encoding='utf-8')
    with pytest.raises(ValueError):
        api.write(page)
