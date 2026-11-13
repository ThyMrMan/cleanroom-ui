# Cleanroom UI

A menu framework for Fallout 4, built from scratch: no decompiled or copied
Bethesda interface files. Mod authors get their own menus, Pip-Boy tabs and
item-card lines without shipping anyone else's movie.

**Status: private, pre-release.** The code was split out of the FO3/FNV →
Fallout 4 conversion (`ThyMrMan/fo3fnv-in-fo4`) with its history and still
carries that project's names (`F3NV_`, `f3ui`).

Prefix for scripts, translation keys and files: **`CRUI_`** (not applied yet).

## Build

```
python -m crui all --out dist/CleanroomUI
python -m pytest
```

`all` runs four steps, each also on its own (`plugin`, `movies`, `scripts`,
`dll`), and writes a mod folder: `CleanroomUI.esl` (the shared forms),
`Interface/` (movies), `Scripts/` (Papyrus), `F4SE/Plugins/` (the DLL). Built
files never go in git.

| Step | Needs | Found at |
|---|---|---|
| `plugin` | Python alone | |
| `movies` | Java and the Apache Flex SDK 4.16.1 with playerglobal 11.2 ([setup](docs/swf_framework.md#mxmlc-setup)) | `CRUI_FLEX`, else `../tools_ext/FlexSDK` |
| `scripts` | Fallout 4's Creation Kit (the Papyrus compiler and `Base.zip`) and F4SE's script sources | `CRUI_FALLOUT4`, else the registered install, else Steam's default; F4SE's in `CRUI_F4SE_SCRIPTS`, else `Data/Scripts/Source` |
| `dll` | Visual Studio with the C++ x64 tools (found by vswhere); F4SE 0.7.9's source and ianpatt's `common`, built as static libraries | `TOOLS_EXT` (default `../tools_ext`), or `F4SE_ROOT` and `COMMON_ROOT` |

The game's `Base.zip` is unpacked into `out/cache/` when the Creation Kit
hasn't unpacked it yet.

Every script, movie and the DLL builds here alone. The level-up host's
partner, `F3NV_LevelUpHolder`, keeps the tagged skills, is where `Get()`
finds the host, and decides what opening each menu does; a mod with rules of
its own extends it ([holder](docs/level_up_plan.md#holder)). Every host
also announces itself to the shared holder in `CleanroomUI.esl`, so other
mods reach the running one with that plugin as their only master
([plugin](docs/level_up_plan.md#plugin)).

## What's here

| Folder | Holds |
|---|---|
| `crui/` | The build: `plugin.py` (`CleanroomUI.esl`), `mxmlc.py` (byte-stable movies), `swf.py` (SWF reader and writer), `papyrus.py`, `menus.py` (every movie), the `python -m crui` command |
| `as3/f3ui/` | The AS3 library: `Host`, `Bridge`, `Theme`, `Draw`, `Text`, `ScrollList`, `Modal`, `Images`, `Input`, `HintBar` |
| `menus/level_up/` | The level-up, tag-skill and trait menu (`Interface/F3NV_LevelUp.swf`) |
| `menus/pipboy/` | The Pip-Boy overlay F3NV.dll loads (`Interface/F3NV_Pipboy.swf`): item-card lines, the list panel |
| `menus/dialogue/` | The dialogue menu (`Interface/DialogueMenu.swf`) |
| `papyrus/` | The level-up host, its holder and its per-plugin tables (`F3NV_LevelUp`, `F3NV_LevelUpHolder`, `F3NV_LevelUpTable`), `F3NV_Array`, and the scripts declaring the DLL's natives (`F3NV_Condition`, `F3NV_ItemCard`, `F3NV_Pipboy`, `F3NV_PipboyAmmo`, `F3NV_Sex`, `F3NV_Damage`, `F3NV_HardcoreNative`) |
| `dll/f3nv/` | F3NV.dll as it is today: GFx helpers, Pip-Boy overlay, tabs, card lines, ammo list, condition API, dialogue menu, plus the FNV-only rules |
| `docs/` | The framework, level-up and spin-off plans, as written for the converter |
| `tests/` | Contract tests between movies, scripts and the DLL, and the reproducible builds |

Stayed in the converter: the FNV table and icon builders (they read FNV
records), the level-up MCM page (it sets the converter's rule rates), the
Pip-Boy tab quests, the dialogue links writer and test mod, and the casino.

## Next

1. **Level-up host, the rest.** Play `CleanroomUI.esl` in game; the
   converter adopts the holder
   ([its change](docs/level_up_plan.md#holder-converter)) and ships
   `CleanroomUI.esl`, after which `Get()` drops its `FalloutHelpers.esl`
   fallback; skills of other mods; an MCM page for the host's own options
   (`SkillCap`, `ShowAllPerks`).
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
5. **CI** that fetches the Flex SDK and builds the movies; release assets.
6. **The converter uses this repo** as a pinned submodule in `external/`.

## License

GPL-3.0 with the modding and linking exceptions in [EXCEPTIONS](EXCEPTIONS),
so menus and mods built on it may use any license.
