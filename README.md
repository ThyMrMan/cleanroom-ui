# Cleanroom UI

A menu framework for Fallout 4, built from scratch: no decompiled or copied
Bethesda interface files. Mod authors get their own menus, Pip-Boy tabs and
item-card lines without shipping anyone else's movie.

**Status: private, pre-release.** The code was split out of the FO3/FNV →
Fallout 4 conversion (`ThyMrMan/fo3fnv-in-fo4`) with its history, and still
carries that project's names and imports. Nothing here builds on its own yet.

Prefix for scripts, translation keys and files: **`CRUI_`** (not applied yet).

## What's here

| Folder | Holds | Came from |
|---|---|---|
| `as3/f3ui/` | The AS3 library: `Host`, `Bridge`, `Theme`, `Draw`, `Text`, `ScrollList`, `Modal`, `Images`, `Input`, `HintBar` | `fo4_import/base/as3/f3ui/` |
| `build/` | The `mxmlc` build (byte-stable movies) and our SWF reader and writer | `fo4_import/base/mxmlc.py`, `asset_convert/ui/swf.py` |
| `menus/level_up/` | The level-up, tag-skill and trait menu: movie source, MCM page, options, table builder and icons | `fo4_import/level_up/`, `fo4_import/spike/level_up*.py` |
| `papyrus/` | `F3NV_LevelUp`, `F3NV_LevelUpTable` (the menu host and per-plugin tables), `F3NV_Array` | `script_convert/static_scripts/fallout4/` |
| `dll/f3nv/` | F3NV.dll as it is today: GFx helpers, Pip-Boy overlay, tabs, card lines, ammo list, condition API, dialogue menu, plus the FNV-only rules | `tes_runtime/fo4_condition/` |
| `docs/` | The framework, level-up and spin-off plans | `docs/fork/` |
| `tests/` | The menu, input, Pip-Boy and level-up tests | `tests/` |

## Next

1. **Stand alone.** The Python still imports the converter (`fo4_import`,
   `asset_convert`, `script_convert`, `core`). Fix imports to this layout,
   take the converter-only parts out of the table and icon builders, and
   make the tests run here.
2. **Rename** `f3ui` → `crui`, `F3NV_` → `CRUI_`, and the `$F3NV_`/`$F3UI_`
   translation keys. The converter keeps thin `F3NV_*` wrappers until its
   scripts are regenerated, so saves keep working.
3. **Split the DLL** as [spin_offs.md](docs/spin_offs.md#split) lays out:
   the framework (condition API, card lines, tabs, GFx helpers,
   `ChangeSex`), the dialogue menu as its own DLL, and the FNV-only rules
   (Damage Threshold, Hardcore, combat sprint, spawn condition) back to the
   converter. Then the shared base: CommonLibF4 (Dear-Modding), Address
   Library, xmake ([runtime](docs/spin_offs.md#runtime)).
4. **An example menu** for authors, and the API in this README.
5. **CI** that fetches the Apache Flex SDK and builds the movies. Built
   `.swf` files ship as release assets, never in git.
6. **The converter uses this repo** as a pinned submodule in `external/`.

## License

GPL-3.0 with the modding and linking exceptions in [EXCEPTIONS](EXCEPTIONS),
so menus and mods built on it may use any license.
