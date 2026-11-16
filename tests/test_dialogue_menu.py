"""Our dialogue menu's movie and the CleanroomDialogue.dll hooks it relies on.

See: docs/swf_framework.md#have
"""

import re

import pytest

from crui import REPO, menus, mxmlc
from crui.swf import Swf

MENU = menus.BY_NAME['dialogue']
MENU_AS = MENU.sources / 'DialogueMenu.as'
DIALOGUE_CPP = REPO / 'dll' / 'dialogue' / 'dialogue.cpp'
LIBRARY = REPO / 'as3' / 'crui'


def test_movie_draws_only_white_so_the_hud_color_shows():
    """Every fill is white, never a black backdrop (it would draw opaque): the menu fills nothing itself and leaves
    Theme.color white, and the library's outline and list fill only in Theme.color (the outline takes another color
    only when one is passed, which the menu and the list never do)."""
    menu = MENU_AS.read_text(encoding='utf-8')
    assert 'beginFill' not in menu and 'Theme.color' not in menu
    assert 'public static var color:uint = WHITE;' in (LIBRARY / 'Theme.as').read_text(encoding='utf-8')
    for name in ('ScrollList.as', 'Draw.as'):
        source = (LIBRARY / name).read_text(encoding='utf-8')
        body = source if name == 'ScrollList.as' else source[source.index('function outline'):source.index('function box')]
        fills = re.findall(r'beginFill\(([^,]+),', body)
        assert fills and {fill.split(' : ')[-1] for fill in fills} == {'Theme.color'}, name
    for source in (menu, (LIBRARY / 'ScrollList.as').read_text(encoding='utf-8')):
        assert all(call.count(',') == 6 for call in re.findall(r'Draw\.outline\(([^;]+)\);', source))


def test_backdrop_is_the_games_shaded_background():
    """The backdrop is the engine's shaded background, never a fill."""
    source = DIALOGUE_CPP.read_text(encoding='utf-8')
    assert 'dll.ShadeList(on)' in MENU_AS.read_text(encoding='utf-8')
    assert 'ENABLE_SHADED_BACKGROUND = Both(278402, 2287022);' in source and 'MENU_QUESTION_TARGET = 0xF8' in source


def test_failed_skill_checks_show_fnv_tags_in_the_hard_tint():
    """FNV's tags ([Speech 25] / [Sneak 17/25], two spaces after), failed rows in the Negative holder's Hard tint."""
    source, menu = DIALOGUE_CPP.read_text(encoding='utf-8'), MENU_AS.read_text(encoding='utf-8')
    assert '"[{} {}/{}]  "' in source and '"[{} {}]  "' in source
    assert 'TINT_CHALLENGE = Both(1432530, 2248685);' in source and 'CHALLENGE_HARD = 3' in source
    assert 'MENU_NEGATIVE_TARGET = 0xE8' in source
    assert 'FadeHolder_mc.Negative.addChild(this.failedRows)' in menu and 'dll.TintFailed(tint)' in menu


def test_a_said_option_dims_only_its_own_row():
    """The game dims a said button's whole holder, and every row lives in the Question holder, so one said option
    dimmed them all (played 2026-11-15, Codsworth's "What happened?"). The DLL takes over that tint call (0xD9 into
    the SetButtonText handler in every checked build), keeps each button's said flag and challenge, tints undimmed
    (the Question holder in the HUD color); the movie dims only said rows and puts challenges in the tinted layer."""
    source, menu = DIALOGUE_CPP.read_text(encoding='utf-8'), MENU_AS.read_text(encoding='utf-8')
    assert 'BUTTON_TINT_CALL = Both(651751, 2248678, 0xD9);' in source
    assert 'g_engine.tintChallenge(menu, target, button == question ? 0 : type, false);' in source
    assert 'trampoline.write_call<5>(tintCall, TintButton);' in source
    assert 'RegisterFunction<ButtonsNative>(plugin, view, "GameButtons");' in source
    assert '"said":said, "dim":said, "challenge":challenge' in menu and 'this.dll.GameButtons()' in menu
    assert 'entry.failed == true || entry.challenge > 0 ? this.failedRows : null' in menu


def test_movie_shows_escaped_prompts_as_written():
    """The links' &lt; &gt; &amp; show as < > & in the menu."""
    menu = MENU_AS.read_text(encoding='utf-8')
    assert 'split("&lt;").join("<").split("&gt;").join(">").split("&amp;").join("&")' in menu
    assert 'this.unescaped(String(options[i].text))' in menu and 'text = this.unescaped(text);' in menu


def test_npc_slot_gets_the_same_answer_on_every_ask():
    """The engine asks the NPC slot each frame until the NPC can speak; each ask of that action gets the picked line."""
    source = DIALOGUE_CPP.read_text(encoding='utf-8')
    assert 'if (picked.line) return Again(picked, action, scene);' in source
    assert 'if (picked.answeredBy != action) return nullptr;' in source and 'return picked.line;' in source
    assert 'REFR_MODEL_STATE = 0x10A' in source and 'answered = true' not in source


def test_prompts_write_out_the_players_macros_and_gray_on_the_answer_line():
    """&PCName; / &PCRace; (any case, or &amp;-escaped) become the player's; said follows the answer line."""
    source = DIALOGUE_CPP.read_text(encoding='utf-8')
    assert 'if (name == "pcname" && npc)' in source and 'if (name == "pcrace" && player->race)' in source
    assert 'if (name == "amp") name = MacroName(text, end + 1, &end);' in source
    assert 'Expanded(own && *own ? own : g_engine.prompt(info))' in source
    assert 'Field<std::uint8_t>(line, INFO_SAID) != 0' in source


def test_unused_rows_leave_the_holder_so_the_backdrop_fits():
    """A hidden row would still count in the holder's bounds that size the backdrop, so it leaves the holder."""
    rows = (LIBRARY / 'ScrollList.as').read_text(encoding='utf-8')
    assert 'var layer:Sprite = i < shown ? layerFor(items[first + i]) : null;' in rows
    assert 'field.parent.removeChild(field);' in rows
    assert 'scroll.show(this.entries' in MENU_AS.read_text(encoding='utf-8')


def test_wheel_scrolls_the_list_and_skips_the_camera():
    """The movie moves on ZoomIn/ZoomOut (Input maps them to up and down); CleanroomDialogue.dll refuses them at the first- and
    third-person cameras' tests."""
    source = DIALOGUE_CPP.read_text(encoding='utf-8')
    assert 'ZoomIn: UP' in (LIBRARY / 'Input.as').read_text(encoding='utf-8')
    assert 'this.input.control(name, down)' in MENU_AS.read_text(encoding='utf-8')
    for rva in ('0x25AAB08', '0x10213E0', '0x2522A88', '0x102DFC0'):
        assert rva in source
    assert 'CAMERA_ZOOM' not in source


def test_rows_take_text_only_through_crui():
    """Row text goes in through Text.show, never row.text = (a "$" loses the font)."""
    rows = (LIBRARY / 'ScrollList.as').read_text(encoding='utf-8')
    assert 'Text.show(text, ' in rows and not re.search(r'\.text\s*=', rows)
    assert not re.search(r'\.text\s*=', MENU_AS.read_text(encoding='utf-8'))


@pytest.mark.skipif(not mxmlc.available(), reason='the Flex SDK is not installed')
def test_movie_builds_the_same_twice_with_the_menu_at_root_menu_mc(tmp_path):
    """Two uncompressed, undated builds are identical; Menu_mc registers the holders, calls the DLL, uses crui."""
    raw = menus.compile_movie(MENU, tmp_path / 'a.swf').read_bytes()
    assert raw == menus.compile_movie(MENU, tmp_path / 'b.swf').read_bytes() and raw[:3] == b'FWS'
    assert not {t.code for t in Swf.parse(raw).tags} & set(mxmlc.DATED)
    for name in (b'DialogueMenuRoot', b'Menu_mc', b'registerObjects', b'GetDialogueOptions', b'SelectDialogueOption',
                 b'onF4SEObjCreated', b'QuickkeyUp', b'crui'):
        assert name in raw
