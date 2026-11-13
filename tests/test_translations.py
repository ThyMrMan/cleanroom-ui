"""Our translation keys: every $CRUI_ key a release shows is in its own English source, written as F4SE reads it.

See: docs/swf_framework.md#translations
"""

import pytest

from crui import translations as T


@pytest.mark.parametrize('release', T.RELEASES)
def test_every_key_a_release_shows_is_in_its_english_and_none_is_spare(release):
    """The `"$CRUI_..."` keys in a release's code are exactly its own en.txt's keys."""
    english = T.entries(T.SOURCES / release / 'en.txt')
    used = T.used_keys(release)
    assert sorted(set(used) - set(english)) == [] and sorted(set(english) - set(used)) == []


def test_the_dialogue_release_needs_none_of_the_frameworks_words():
    """The dialogue menu's words are in CleanroomDialogue's own file, which its DLL loads without any plugin."""
    assert set(T.used_keys(T.DIALOGUE)) == {'$CRUI_LEAVE', '$CRUI_HINT_SELECT', '$CRUI_HINT_LEAVE'}


def test_install_writes_every_release_and_language_as_utf16_with_a_bom(tmp_path):
    """One <release>_<language>.txt per release and game language: BOM, UTF-16 LE, CRLF `$KEY<tab>text` lines."""
    assert T.install(tmp_path) == len(T.RELEASES) * len(T.LANGUAGES)
    for release in T.RELEASES:
        raw = (tmp_path / 'Interface' / 'Translations' / f'{release}_de.txt').read_bytes()
        text = raw.decode('utf-16-le')
        assert raw[:2] == b'\xff\xfe' and text[1:].startswith('$CRUI_') and '\r\n' in text
