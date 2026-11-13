"""The Papyrus scripts compile against the player's own Fallout 4 and F4SE sources alone.

F3NV_LevelUp still reads the FNV conversion's F3NV_CharacterRules and F3NV_CharacterEvents (README "Next" 2), so it
is the one script that needs them on the import path.
"""

import re

import pytest

from crui import REPO, papyrus

#: Scripts that still need the converter's scripts to compile.
NEEDS_CONVERTER = {'F3NV_LevelUp'}


@pytest.mark.skipif(not papyrus.available(), reason='the Fallout 4 Papyrus compiler or Base sources are missing')
def test_every_other_script_compiles_alone(tmp_path):
    """Every script but those in NEEDS_CONVERTER compiles with only the game's and F4SE's sources."""
    built, failed, log = papyrus.compile_scripts(tmp_path)
    assert set(failed) == NEEDS_CONVERTER, log
    assert len(built) == len(list(papyrus.SCRIPTS.glob('*.psc'))) - len(NEEDS_CONVERTER)


def test_the_converter_dependency_is_only_the_character_scripts():
    """The level-up host names no converter script but the character rules and events."""
    source = (REPO / 'papyrus' / 'F3NV_LevelUp.psc').read_text(encoding='utf-8')
    code = '\n'.join(line for line in source.splitlines() if not line.lstrip().startswith(('{', ';')))
    named = set(re.findall(r'\b(F3NV_\w+|TES4\w+)\b', code)) - {p.stem for p in papyrus.SCRIPTS.glob('*.psc')}
    assert named <= {'F3NV_CharacterRules', 'F3NV_CharacterEvents', 'F3NV_LevelUpTake', 'F3NV_LevelUpRaise',
                     'F3NV_LevelUpClose', 'F3NV_LevelUpLog', 'F3NV_LevelUpTags', 'F3NV_LevelUpTraits'}, named
