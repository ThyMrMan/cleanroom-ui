"""The movie check: our classes only, no imported assets, mapped font names, byte-stable builds.

See: docs/swf_framework.md#testing
"""

import pytest

from crui import check, menus, mxmlc
from crui.swf import Swf, Tag

TRANSFER, PIPBOY = menus.BY_NAME['transfer'], menus.BY_NAME['pipboy']
FONTS = {'$MAIN_Font', '$MAIN_Font_Bold', '$Controller_Buttons'}
needs_flex = pytest.mark.skipif(not mxmlc.available(), reason='the Flex SDK is not installed')


@pytest.fixture(scope='module')
def transfer(tmp_path_factory):
    """The transfer movie, built once."""
    return menus.compile_movie(TRANSFER, tmp_path_factory.mktemp('check') / 'a.swf').read_bytes()


@needs_flex
def test_the_check_reads_classes_and_strings(transfer):
    """The ABC reader finds the movie's own class, the library's Bridge, and the CND key."""
    classes, strings, codes = check.movie_contents(transfer)
    assert sorted(classes) == ['CRUI_Transfer', 'crui.Bridge'] and '$CRUI_CND' in strings
    assert check.DO_ABC in codes and check.problems(TRANSFER, transfer, transfer, FONTS) == []


@needs_flex
def test_the_check_names_each_problem(transfer, monkeypatch):
    """A class from other sources, an import tag, an unmapped font and differing builds are each reported; a key's
    arguments after a tab and a bare $ are not names."""
    assert check.problems(PIPBOY, transfer, transfer, None) == ['class CRUI_Transfer is not from our sources']
    movie = Swf.parse(transfer)
    movie.tags.insert(1, Tag(57, b'other.swf\0\0\0'))
    imported = movie.serialize(compress=False)
    assert "tag 57 imports another movie's assets" in check.problems(TRANSFER, imported, imported, None)
    read = check.movie_contents
    monkeypatch.setattr(check, 'movie_contents', lambda raw: (
        read(raw)[0], read(raw)[1] | {'$Odd_Font', '$MAIN_Font', '$CRUI_LU_LEVEL\targs', '$'}, read(raw)[2]))
    assert check.problems(TRANSFER, transfer, transfer, FONTS) == [
        '$Odd_Font is neither our key nor a font FontConfig.txt maps']
    assert check.problems(TRANSFER, transfer, transfer + b'\0', None) == ['two builds differ']


def test_font_names_need_the_archive(tmp_path):
    """Without the game's Interface archive, font names are not read (and so not checked)."""
    assert check.font_names(tmp_path) is None
