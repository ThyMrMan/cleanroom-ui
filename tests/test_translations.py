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
    """The dialogue menu's words are all in CleanroomDialogue's own file."""
    assert set(T.used_keys(T.DIALOGUE)) == {'$CRUI_LEAVE', '$CRUI_HINT_SELECT', '$CRUI_HINT_LEAVE'}


def test_install_writes_every_release_and_language_as_utf16_with_a_bom(tmp_path):
    """One <release>_<language>.txt per release and game language: BOM, UTF-16 LE, CRLF `$KEY<tab>text` lines."""
    assert T.install(tmp_path) == len(T.RELEASES) * len(T.LANGUAGES)
    for release in T.RELEASES:
        raw = (tmp_path / 'Interface' / 'Translations' / f'{release}_de.txt').read_bytes()
        text = raw.decode('utf-16-le')
        assert raw[:2] == b'\xff\xfe' and text[1:].startswith('$CRUI_') and '\r\n' in text


def _sources(tmp_path, german):
    """A copy of our sources with `german` ({key: text}) as CleanroomUI's de.txt."""
    for release in T.RELEASES:
        (tmp_path / release).mkdir(parents=True)
        (tmp_path / release / 'en.txt').write_bytes((T.SOURCES / release / 'en.txt').read_bytes())
    lines = ''.join(f'{key}\t{text}\n' for key, text in german.items())
    (tmp_path / T.PLUGIN / 'de.txt').write_text(lines, encoding='utf-8')
    return tmp_path


def test_a_translation_keeps_englishs_slots_and_adds_no_key(tmp_path):
    """A wrong slot or an unknown key is a problem, and install writes nothing then."""
    sources = _sources(tmp_path / 'src', {'$CRUI_LU_LEVEL': 'Stufe {1}', '$CRUI_NOPE': 'x'})
    found = T.problems(sources)
    assert len(found) == 2 and "['{1}']" in found[0] and '$CRUI_NOPE' in found[1]
    with pytest.raises(ValueError):
        T.install(tmp_path / 'out', sources)


def test_untranslated_counts_each_language(tmp_path):
    """Each language but English counts the English keys its own source lacks."""
    sources = _sources(tmp_path, {'$CRUI_LU_LEVEL': 'Stufe {0}'})
    counts = T.untranslated(sources)[T.PLUGIN]
    english = len(T.entries(T.SOURCES / T.PLUGIN / 'en.txt'))
    assert 'en' not in counts and counts['de'] == english - 1 and counts['fr'] == english


def test_the_test_language_stretches_and_accents_and_keeps_slots(tmp_path):
    """Every file holds every key's English text in the test language, its slots unchanged."""
    assert T.pseudo('Level {0}') == '[Lëvël {0}~~~]'
    T.install(tmp_path, test=True)
    raw = (tmp_path / 'Interface' / 'Translations' / f'{T.PLUGIN}_fr.txt').read_bytes().decode('utf-16-le')
    assert '$CRUI_LU_LEVEL\t[Lëvël {0}~~~]\r\n' in raw


def test_papyrus_comment_lines_hold_no_keys(tmp_path):
    """Our scripts are read for keys, `;` comment lines left out."""
    (tmp_path / 'A.psc').write_text('; "$CRUI_SKIPPED"\nShow("$CRUI_KEPT")\n', encoding='utf-8')
    assert set(T.used_keys(T.PLUGIN, ((tmp_path, '*.psc'),))) == {'$CRUI_KEPT'}
