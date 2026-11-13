"""Our movies' translation keys: every $CRUI_ key they show is in translations/en.txt, written as F4SE reads it.

See: docs/swf_framework.md#translations
"""

from crui import translations as T


def test_every_key_the_movies_show_is_in_english_and_none_is_spare():
    """The `"$CRUI_..."` keys in our AS3 are exactly en.txt's keys."""
    english = T.entries(T.SOURCES / 'en.txt')
    used = T.used_keys()
    assert sorted(set(used) - set(english)) == [] and sorted(set(english) - set(used)) == []


def test_install_writes_every_language_as_utf16_with_a_bom(tmp_path):
    """One CleanroomUI_<language>.txt per game language: BOM, UTF-16 LE, CRLF `$KEY<tab>text` lines."""
    assert T.install(tmp_path) == len(T.LANGUAGES)
    raw = (tmp_path / 'Interface' / 'Translations' / f'{T.PLUGIN}_de.txt').read_bytes()
    text = raw.decode('utf-16-le')
    assert raw[:2] == b'\xff\xfe' and text[1:].startswith('$CRUI_') and '\r\n' in text
