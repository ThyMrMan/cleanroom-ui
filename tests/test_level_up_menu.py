"""Our level-up menu: the movie mxmlc builds, and the contract between it, F3NV_LevelUp.psc and F3NV_LevelUpTable.psc.

See: docs/level_up_plan.md#menu
"""

import re

import pytest

from crui import REPO, menus, mxmlc
from crui.swf import Swf

MENU = menus.BY_NAME['level_up']
PAPYRUS = REPO / 'papyrus'
SCRIPT = (PAPYRUS / 'F3NV_LevelUp.psc').read_text(encoding='utf-8')
TABLE = (PAPYRUS / 'F3NV_LevelUpTable.psc').read_text(encoding='utf-8')
MOVIE = (MENU.sources / 'F3NV_LevelUp.as').read_text(encoding='utf-8')
CHARACTER = (MENU.sources / 'f3nv' / 'levelup' / 'Character.as').read_text(encoding='utf-8')


def _function(source: str, name: str) -> str:
    """The body of Papyrus function `name` in `source`."""
    return re.search(rf'Function {name}\(.*?EndFunction', source, re.S).group(0)


def test_the_script_opens_the_movie_the_build_installs():
    """The menu name F3NV_LevelUp.psc registers is the movie's file and document class."""
    name = re.search(r'String Property MenuName = "(\w+)"', SCRIPT).group(1)
    assert MENU.movie == f'Interface/{name}.swf' and MENU.document == name and f'MENU:String = "{name}"' in MOVIE


def test_the_script_and_movie_agree_on_events_and_calls():
    """Every event the movie sends is one the script registers, and each root1 call the script makes is a public
    function of the movie."""
    sent = set(re.findall(r'send\("(F3NV_LevelUp\w+)"', MOVIE))
    registered = set(re.findall(r'RegisterForExternalEvent\("(\w+)"', SCRIPT))
    assert sent == registered == {'F3NV_LevelUpTake', 'F3NV_LevelUpRaise', 'F3NV_LevelUpClose', 'F3NV_LevelUpLog',
                                  'F3NV_LevelUpTags', 'F3NV_LevelUpTraits'}
    for call in re.findall(r'"root1\.(\w+)"', SCRIPT):
        assert re.search(rf'public function {call}\(', MOVIE), call
    assert 'Function OnMenuTags(Int aiMask)' in SCRIPT and 'Function OnMenuTraits(Int aiMask, Int aiShown)' in SCRIPT
    assert 'send("F3NV_LevelUpTraits", Character.mask(who.traits, "picked"), who.traits.length)' in MOVIE


def test_built_hands_over_what_the_movie_reads():
    """Built fills args[0] to args[34] and Send hands them over; Character reads 17 values, 8 requirement columns
    (17-24), then the mode, count, bonus, the trait tables (25-31), the show-all option (32) and each table's perk
    and row counts (33, 34), which move the rows' owners and perk targets."""
    built = _function(SCRIPT, 'Built')
    assert sorted(int(n) for n in re.findall(r'args\[(\d+)\] =', built)) == list(range(35)) and 'new Var[35]' in built
    assert 'UI.Invoke(MenuName, "root1.Open", args)' in _function(SCRIPT, 'Send')
    assert 'args[16]' in CHARACTER and 'readRows(args.slice(17, 25), split(args[33]), split(args[34]))' in CHARACTER
    assert re.findall(r'split\(columns\[(\d)\]\)', CHARACTER) == [str(i) for i in range(8)]
    assert 'for each (var at:String in split(columns[7]))' in CHARACTER
    assert sorted(int(n) for n in re.findall(r'args\[(2[5-9]|3\d)\]', CHARACTER)) == list(range(25, 35))
    assert 'args[32] = ShowAllPerks' in built and 'showAll = who.showAll;' in MOVIE


def test_built_merges_each_text_the_movie_reads_where_the_table_keeps_it():
    """Each Merged(n, ...) in Built is the table's Text n, a property written whole (an open joins none), and fills
    the argument the movie reads that table from; the rows' held tests are gathered beside the ranks."""
    built = _function(SCRIPT, 'Built')
    text = re.search(r'String Function Text\(.*?EndFunction', TABLE, re.S).group(0)
    branches = {**dict(re.findall(r'aiWhich == (\d+)\n\s+Return (\w+)\n', text)), '14': 'RequirementLabels'}
    assert text.endswith('Return RequirementLabels\nEndFunction') and 'Joined(' not in text
    wanted = {5: 'PerkNames', 6: 'PerkDescriptions', 7: 'PerkImages', 8: 'PerkLevels', 9: 'PerkRanksText',
              17: 'RequirementOwners', 18: 'RequirementKinds', 19: 'RequirementTargets', 20: 'RequirementCompares',
              21: 'RequirementValues', 22: 'RequirementOr', 23: 'RequirementLabels',
              28: 'TraitNames', 29: 'TraitDescriptions', 30: 'TraitImages'}
    merged = {int(arg): branches[n] for arg, n in re.findall(r'args\[(\d+)\] = Merged\((\d+),', built)}
    assert merged == wanted and 'args[24] = unlisted' in built
    assert 'args[33] = GroupCounts(perkCounts)' in built and 'args[34] = GroupCounts(rowCounts)' in built


def test_open_gathers_again_only_for_a_different_open():
    """Prepare registers the menu and gathers; Open gathers again only for another mode, count or bonus."""
    assert 'Register()' in _function(SCRIPT, 'Prepare') and 'If !ready || mode != aiMode' in _function(SCRIPT, 'Open')
    modes = {m: int(v) for m, v in re.findall(r'Int Property (\w+)Mode = (\d)', SCRIPT)}
    assert modes == {'LevelUp': 0, 'Tags': 1, 'Traits': 2}
    assert all(f'{name}:int = {modes[mode]};' in CHARACTER
               for name, mode in (('LEVEL_UP', 'LevelUp'), ('TAGS', 'Tags'), ('TRAITS', 'Traits')))


def test_the_api_sends_its_events():
    """SkillRaised as points raise a skill, MenuClosed as our menu closes, each declared."""
    for event, function in (('SkillRaised', 'RaiseSkill'), ('MenuClosed', None)):
        assert f'CustomEvent {event}' in SCRIPT
        body = _function(SCRIPT, function) if function else re.search(
            r'Event OnMenuOpenCloseEvent\(.*?EndEvent', SCRIPT, re.S).group(0)
        assert f'SendCustomEvent("{event}", args)' in body


def test_an_added_perk_is_described_and_tested_by_the_game():
    """DescribeAdded fills every perk and row text (Text's 0-4, 8-14) for an added perk, its one row of kind 4,
    which the movie's GAME kind is, listed as passed after every table's rows when the game's test passes."""
    described = _function(SCRIPT, 'DescribeAdded')
    filled = sorted(int(n) for n in re.findall(r'AddPart\((\d+),', described))
    assert filled == [*range(5), *range(8, 15)] and 'AddPart(9, "4", j)' in described
    assert '"" + (tableRows + j)' in described
    assert 'public static const GAME:int = 4;' in CHARACTER and '"$F3NV_LU_OWN_TEST"' in CHARACTER


HOLDER = (PAPYRUS / 'F3NV_LevelUpHolder.psc').read_text(encoding='utf-8')


def test_the_api_opens_each_menu_through_the_holder():
    """OpenLevelUpMenu, OpenTagMenu and OpenTraitMenu announce the host, then call the holder's opener, whose own
    version shows our menu in the matching mode."""
    for api, opener, mode in (('OpenLevelUpMenu', 'OpenLevelUp()', 'LevelUpMode'), ('OpenTagMenu', 'OpenTags(aiTags)',
                              'TagsMode'), ('OpenTraitMenu', 'OpenTraits(aiTraits)', 'TraitsMode')):
        body = _function(SCRIPT, api)
        assert body.index('Announce()') < body.index(f'Holder().{opener}'), api
        assert f'Running().Show(Running().{mode},' in _function(HOLDER, opener.split('(')[0])


def test_every_holder_call_is_a_holder_function():
    """Each function the host calls on the holder is one the holder defines, so a holder of another mod's overrides
    exactly these."""
    called = set(re.findall(r'(?:Holder\(\)|holder)\.(\w+)\(', SCRIPT))
    defined = set(re.findall(r'^\S*\s*Function (\w+)\(', HOLDER, re.M))
    assert called == {'Hold', 'IsTagged', 'IsSuggested', 'Tag', 'Untag', 'OpenLevelUp', 'OpenTags', 'OpenTraits',
                      'Running'}
    assert called <= defined, called - defined


def test_tags_go_through_the_holder_and_send_the_hosts_events():
    """Tag and Untag change the holder's tags and send SkillTagged and SkillUntagged; the tag menu's picks use them."""
    for name, event in (('Tag', 'SkillTagged'), ('Untag', 'SkillUntagged')):
        body = _function(SCRIPT, name)
        assert f'Holder().{name}(akSkill)' in body and f'SendCustomEvent("{event}"' in body
        assert f'CustomEvent {event}' in SCRIPT
    picks = _function(SCRIPT, 'OnMenuTags')
    assert '      Tag(skill)' in picks and '      Untag(skill)' in picks


def test_show_gathers_while_menus_close_then_opens():
    """Show prepares first, waits out every menu (ours, the Pip-Boy) without a settle, then opens."""
    show = _function(SCRIPT, 'Show')
    assert show.index('Prepare(') < show.index('While Utility.IsInMenuMode()') < show.index('Open(aiMode')
    assert 'Utility.Wait(' not in show


def test_level_up_menu_ex_is_never_named():
    """Neither script compiles against LevelUpMenuEx."""
    assert 'PRKF:' not in SCRIPT and 'PRKF:' not in TABLE


@pytest.mark.skipif(not mxmlc.available(), reason='the Flex SDK is not installed')
def test_the_movie_builds_the_same_twice(tmp_path):
    """Two uncompressed, undated builds are identical and hold the menu's calls and the library."""
    raw = menus.compile_movie(MENU, tmp_path / 'a.swf').read_bytes()
    assert raw == menus.compile_movie(MENU, tmp_path / 'b.swf').read_bytes() and raw[:3] == b'FWS'
    assert not {t.code for t in Swf.parse(raw).tags} & set(mxmlc.DATED)
    for name in (b'F3NV_LevelUp', b'Open', b'Taken', b'Raised', b'ProcessUserEvent', b'Character', b'ScrollList'):
        assert name in raw


@pytest.mark.skipif(not mxmlc.available(), reason='the Flex SDK is not installed')
def test_install_writes_the_movie_under_interface(tmp_path):
    """install() copies the built movie to <data>/Interface/F3NV_LevelUp.swf."""
    assert menus.install(MENU, tmp_path).read_bytes()[:3] == b'FWS' and (tmp_path / MENU.movie).is_file()
