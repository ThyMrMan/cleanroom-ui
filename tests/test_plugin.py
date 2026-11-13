"""CleanroomUI.esl: a light master holding the shared level-up holder quest the scripts find by its id.

See: docs/level_up_plan.md#plugin
"""

import re
import struct

from crui import REPO, plugin as P

PAPYRUS = REPO / 'papyrus'


def _subrecords(data: bytes) -> dict:
    """{signature: data} of a record's subrecords (each signature once here)."""
    out, pos = {}, 0
    while pos < len(data):
        sig, size = struct.unpack_from('<4sH', data, pos)
        out[sig.decode()] = data[pos + 6:pos + 6 + size]
        pos += 6 + size
    return out


def _parse(raw: bytes) -> tuple:
    """(header flags, header subrecords, [(signature, FormID, subrecords)] of the plugin's records)."""
    sig, size, flags = struct.unpack_from('<4sII', raw)
    assert sig == b'TES4'
    head, pos, records = _subrecords(raw[24:24 + size]), 24 + size, []
    while pos < len(raw):
        assert raw[pos:pos + 4] == b'GRUP'
        end, inner = pos + struct.unpack_from('<I', raw, pos + 4)[0], pos + 24
        while inner < end:
            rsig, rsize, _, fid = struct.unpack_from('<4sIII', raw, inner)
            records.append((rsig.decode(), fid, _subrecords(raw[inner + 24:inner + 24 + rsize])))
            inner += 24 + rsize
        pos = end
    return flags, head, records


def test_the_plugin_is_a_light_master_without_masters(tmp_path):
    """ESM and ESL flags, no MAST, HEDR counting the record and its group, the next id past the holder."""
    flags, head, records = _parse(P.build(tmp_path).read_bytes())
    assert flags == P.FLAG_ESM | P.FLAG_ESL and 'MAST' not in head
    assert struct.unpack('<fII', head['HEDR']) == (1.0, 2, P.HOLDER_ID + 1)
    assert len(records) == 1 and (tmp_path / P.PLUGIN).is_file()


def test_the_holder_quest_starts_with_the_game_and_carries_the_holder():
    """The quest at HOLDER_ID is start-game enabled and carries F3NV_LevelUpHolder, which exists."""
    _, _, [(sig, fid, subs)] = _parse(P.plugin_bytes())
    assert (sig, fid) == ('QUST', P.HOLDER_ID) and subs['EDID'] == P.HOLDER_EDID.encode() + b'\0'
    assert struct.unpack_from('<H', subs['DNAM'])[0] & P.START_GAME_ENABLED
    assert subs['VMAD'] == struct.pack('<HHHH', 6, 2, 1, len(P.HOLDER_SCRIPT)) + P.HOLDER_SCRIPT.encode() + b'\0\0\0'
    assert (PAPYRUS / f'{P.HOLDER_SCRIPT}.psc').is_file()


def test_the_scripts_find_the_holder_where_the_plugin_puts_it():
    """F3NV_LevelUpHolder.Shared reads PLUGIN at HOLDER_ID; Get reads it first and every host announces to it."""
    holder = (PAPYRUS / 'F3NV_LevelUpHolder.psc').read_text(encoding='utf-8')
    shared = re.search(r'Function Shared\(\) Global.*?EndFunction', holder, re.S).group(0)
    assert f'Game.GetFormFromFile(0x{P.HOLDER_ID:X}, "{P.PLUGIN}")' in shared
    assert f'Game.IsPluginInstalled("{P.PLUGIN}")' in shared
    host = (PAPYRUS / 'F3NV_LevelUp.psc').read_text(encoding='utf-8')
    get = re.search(r'Function Get\(\) Global.*?EndFunction', host, re.S).group(0)
    assert get.index('= F3NV_LevelUpHolder.Shared()') < get.index('GetFormFromFile(0x901, "FalloutHelpers.esl")')
    announce = re.search(r'Function Announce\(\).*?EndFunction', host, re.S).group(0)
    assert 'shared.Hold(Self)' in announce and 'holder.Hold(Self)' in announce
