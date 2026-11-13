"""CleanroomUI.esl: Cleanroom UI's own plugin, the forms its scripts find by a fixed id.

One start-game quest, HOLDER_ID, carrying CRUI_LevelUpHolder: every level-up host announces itself there, so
CRUI_LevelUp.Get() finds the running one with no master but this plugin. A light master (ESM + ESL flags) with no
masters of its own. Object ids are a save contract: append only, never reuse.

See: docs/level_up_plan.md#plugin
"""

import struct
from pathlib import Path

PLUGIN = 'CleanroomUI.esl'
#: The holder quest's object id and editor id, and the script on it.
HOLDER_ID, HOLDER_EDID, HOLDER_SCRIPT = 0x800, 'CleanroomUI_LevelUpHolder', 'CRUI_LevelUpHolder'
AUTHOR, DESCRIPTION = 'Cleanroom UI', 'Cleanroom UI: shared forms of the menu framework'
#: Fallout 4's record form version, HEDR version and INTV.
FORM_VERSION, HEDR_VERSION, INTV = 131, 1.0, 1
FLAG_ESM, FLAG_ESL = 0x1, 0x200
#: Quest DNAM flags: Start Game Enabled, Run Once.
START_GAME_ENABLED, RUN_ONCE = 0x0001, 0x0010
#: VMAD version and object format.
VMAD_VERSION, VMAD_OBJECT_FORMAT = 6, 2
GROUP_HEADER = 24


def subrecord(sig: str, data: bytes) -> bytes:
    """One subrecord (no XXXX: every one here is small)."""
    return sig.encode('ascii') + struct.pack('<H', len(data)) + data


def zstring(sig: str, text: str) -> bytes:
    """A null-terminated cp1252 string subrecord."""
    return subrecord(sig, text.encode('cp1252') + b'\0')


def record(sig: str, form_id: int, flags: int, subrecords: bytes) -> bytes:
    """A record: its 24-byte header, then `subrecords`."""
    return struct.pack('<4sIIIIHH', sig.encode('ascii'), len(subrecords), flags, form_id, 0, FORM_VERSION, 0) \
        + subrecords


def top_group(sig: str, records: bytes) -> bytes:
    """The top-level group of `sig` records."""
    return struct.pack('<4sI4siII', b'GRUP', GROUP_HEADER + len(records), sig.encode('ascii'), 0, 0, 0) + records


def wstring(text: str) -> bytes:
    """A VMAD string: u16 length, UTF-8 bytes."""
    data = text.encode('utf-8')
    return struct.pack('<H', len(data)) + data


def script_vmad(script: str) -> bytes:
    """A VMAD attaching `script` with no properties set."""
    return struct.pack('<HHH', VMAD_VERSION, VMAD_OBJECT_FORMAT, 1) + wstring(script) + struct.pack('<BH', 0, 0)


def script_quest(form_id: int, edid: str, script: str) -> bytes:
    """A start-game, run-once quest carrying `script`, no stages or aliases."""
    subs = zstring('EDID', edid) + subrecord('VMAD', script_vmad(script))
    subs += subrecord('DNAM', struct.pack('<HBxfB3x', START_GAME_ENABLED | RUN_ONCE, 0, 0.0, 0))
    subs += subrecord('NEXT', b'') + subrecord('ANAM', struct.pack('<I', 0))
    return record('QUST', form_id, 0, subs)


def header(num_records: int, next_object_id: int) -> bytes:
    """The TES4 file header: a light master with no masters and no interior cells."""
    subs = subrecord('HEDR', struct.pack('<fII', HEDR_VERSION, num_records, next_object_id))
    subs += zstring('CNAM', AUTHOR) + zstring('SNAM', DESCRIPTION)
    subs += subrecord('INTV', struct.pack('<I', INTV)) + subrecord('INCC', struct.pack('<I', 0))
    return record('TES4', 0, FLAG_ESM | FLAG_ESL, subs)


def plugin_bytes() -> bytes:
    """The whole plugin. HEDR counts records and groups."""
    quests = script_quest(HOLDER_ID, HOLDER_EDID, HOLDER_SCRIPT)
    return header(2, HOLDER_ID + 1) + top_group('QUST', quests)


def build(data_dir: Path) -> Path:
    """Write <data_dir>/CleanroomUI.esl; its path."""
    out = Path(data_dir) / PLUGIN
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_bytes(plugin_bytes())
    return out
