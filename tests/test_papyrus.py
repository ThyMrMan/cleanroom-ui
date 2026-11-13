"""The Papyrus scripts compile against the player's own Fallout 4 and F4SE sources alone, naming nothing else."""

import re

import pytest

from crui import papyrus

OURS = {p.stem for p in papyrus.SCRIPTS.glob('*.psc')}
#: Names in code that are not scripts: the level-up movie's external events.
EVENTS = {'CRUI_LevelUpTake', 'CRUI_LevelUpRaise', 'CRUI_LevelUpClose', 'CRUI_LevelUpLog', 'CRUI_LevelUpTags',
          'CRUI_LevelUpTraits'}


@pytest.mark.skipif(not papyrus.available(), reason='the Fallout 4 Papyrus compiler or Base sources are missing')
def test_every_script_compiles_alone(tmp_path):
    """Every script compiles with only the game's and F4SE's sources."""
    built, failed, log = papyrus.compile_scripts(tmp_path)
    assert not failed and set(built) == OURS, log


def test_no_script_names_another_mods_script():
    """Outside comments and doc strings, every CRUI_/F3NV_/TES4 name a script uses is one of ours (or a movie
    event)."""
    for path in papyrus.SCRIPTS.glob('*.psc'):
        source = re.sub(r'\{.*?\}', '', path.read_text(encoding='utf-8'), flags=re.S)
        code = '\n'.join(line.split(';')[0] for line in source.splitlines())
        named = set(re.findall(r'\b(CRUI_\w+|F3NV_\w+|TES4\w+)\b', code)) - OURS - EVENTS
        assert not named, (path.name, named)
