# Cleanroom UI

A menu framework for Fallout 4, built from scratch: no decompiled or copied
Bethesda interface files. Mod authors get their own menus, Pip-Boy tabs and
item-card lines without shipping anyone else's movie.

**Status: private, pre-release.** The code was split out of the FO3/FNV →
Fallout 4 conversion (`ThyMrMan/fo3fnv-in-fo4`) with its history.

Prefix for scripts, translation keys and files: **`CRUI_`** (the `crui` AS3
package, `CRUI_*` scripts and movies, `$CRUI_*` keys). The two DLLs are
`CleanroomUI.dll` and `CleanroomDialogue.dll`, which movies reach as
`f4se.plugins.CleanroomUI` and `f4se.plugins.CleanroomDialogue`.

| Release | Holds | Needs |
|---|---|---|
| **Cleanroom UI** | `CleanroomUI.esl`, `CleanroomUI.dll` (item condition API, the Pip-Boy's item-card lines, repair and ammo lists and STAT tabs, transfer-menu CND lines, `ChangeSex`), the level-up menu, the Pip-Boy movie, the scripts | F4SE |
| **Dialogue menu** | `CleanroomDialogue.dll`, `Interface/DialogueMenu.swf` and `CleanroomDialogue_<language>.txt`; options and their answers from `F4SE/Plugins/CleanroomDialogue/*.txt` | F4SE |

## Build

```
python -m crui all --out dist/CleanroomUI
python -m pytest
```

`all` runs five steps, each also on its own (`plugin`, `translations`,
`movies`, `scripts`, `dll`), and writes a mod folder: `CleanroomUI.esl` (the
shared forms), `Interface/Translations/CleanroomUI_<language>.txt` and
`CleanroomDialogue_<language>.txt` (our words, from `translations/`), `Interface/` (movies), `Scripts/` (Papyrus),
`F4SE/Plugins/` (both DLLs). Built
files never go in git.

| Step | Needs | Found at |
|---|---|---|
| `plugin`, `translations` | Python alone | |
| `movies` | Java and the Apache Flex SDK 4.16.1 with playerglobal 11.2 ([setup](docs/swf_framework.md#mxmlc-setup)) | `CRUI_FLEX`, else `../tools_ext/FlexSDK` |
| `scripts` | Fallout 4's Creation Kit (the Papyrus compiler and `Base.zip`) and F4SE's script sources | `CRUI_FALLOUT4`, else the registered install, else Steam's default; F4SE's in `CRUI_F4SE_SCRIPTS`, else `Data/Scripts/Source` |
| `dll` | Visual Studio with the C++ x64 tools (found by vswhere); F4SE 0.7.9's source and ianpatt's `common`, built as static libraries | `TOOLS_EXT` (default `../tools_ext`), or `F4SE_ROOT` and `COMMON_ROOT` |

The game's `Base.zip` is unpacked into `out/cache/` when the Creation Kit
hasn't unpacked it yet.

Every script, movie and DLL builds here alone. The level-up host's
partner, `CRUI_LevelUpHolder`, keeps the tagged skills, is where `Get()`
finds the host, and decides what opening each menu does; a mod with rules of
its own extends it ([holder](docs/level_up_plan.md#holder)). Every host
also announces itself to the shared holder in `CleanroomUI.esl`, so other
mods reach the running one with that plugin as their only master
([plugin](docs/level_up_plan.md#plugin)).

## What's here

| Folder | Holds |
|---|---|
| `crui/` | The build: `plugin.py` (`CleanroomUI.esl`), `mxmlc.py` (byte-stable movies), `swf.py` (SWF reader and writer), `papyrus.py`, `menus.py` (every movie), the `python -m crui` command |
| `as3/crui/` | The AS3 library: `Host`, `Bridge`, `Theme`, `Draw`, `Text`, `ScrollList`, `Modal`, `Images`, `Input`, `HintBar` |
| `translations/` | Our words, `$CRUI_KEY<tab>text` per line: a folder per release (`CleanroomUI/`, `CleanroomDialogue/`), one UTF-8 file per language (`en.txt` holds every key) |
| `menus/level_up/` | The level-up, tag-skill and trait menu (`Interface/CRUI_LevelUp.swf`) |
| `menus/pipboy/` | The Pip-Boy overlay CleanroomUI.dll loads (`Interface/CRUI_Pipboy.swf`): item-card lines, the list panel |
| `menus/hud/` | The HUD host CleanroomUI.dll loads (`Interface/CRUI_Hud.swf`): mods' widget movies and the built-in stats line; the stats widget HUDFramework hosts when installed (`Interface/CRUI_HudStats.swf`); the edit menu for our host's widgets (`Interface/CRUI_HudEdit.swf`); and the layout of the game's own HUD parts from `DEF_HUD.xml` (`PartLayout.as`) |
| `menus/dialogue/` | The dialogue menu (`Interface/DialogueMenu.swf`) |
| `papyrus/` | The level-up host, its holder and its per-plugin tables (`CRUI_LevelUp`, `CRUI_LevelUpHolder`, `CRUI_LevelUpTable`), `CRUI_Array`, and the scripts declaring CleanroomUI.dll's natives (`CRUI_Condition`, `CRUI_Framework`, `CRUI_HudNative`, `CRUI_ItemCard`, `CRUI_Pipboy`, `CRUI_PipboyAmmo`, `CRUI_Sex`), and `CRUI_Hud`, which puts widgets on HUDFramework when installed, else on our host |
| `dll/` | `build.bat` (both DLLs); `framework/` (CleanroomUI.dll: condition API, Pip-Boy overlay, card lines, tabs, repair and ammo lists, transfer menus, `ChangeSex`); `dialogue/` (CleanroomDialogue.dll); `shared/` (Flash helpers and the natives every movie may call, compiled into both) |
| `docs/` | The framework, level-up and spin-off plans, as written for the converter |
| `tests/` | Contract tests between movies, scripts and the DLL, and the reproducible builds |

Stayed in the converter: the FNV table and icon builders (they read FNV
records), the level-up MCM page (it sets the converter's rule rates), the
Pip-Boy tab quests, the dialogue links writer and test mod, the casino, and
FNV's own rules in its F3NV.dll (Damage Threshold, Hardcore's HUD and ammo
weight, weapons' spawn condition, no combat sprint).

## Next

1. **Level-up host, the rest.** Play `CleanroomUI.esl` in game; skills of
   other mods; an MCM page for the host's own options (`SkillCap`,
   `ShowAllPerks`).
2. **The converter on the `CRUI_` scripts: play it.** Built 2026-11-12, not
   played (`fo3fnv-in-fo4`, branch `claude/fo4-dialogue-own-movie`): it
   ships copies of ours and a byte-identical `CleanroomUI.esl`, and moves an
   older save's `F3NV_LevelUp` points, added perks and options into
   `CRUI_LevelUp` once. Its level-up movie still builds on its own `f3ui`
   copy until it takes this repo as a submodule.
3. **The DLL split, the rest** ([split](docs/spin_offs.md#split-status)):
   the converter on the two DLLs, its F3NV.dll cut to FNV's rules, its
   dialogue links in `CleanroomDialogue\` (the
   [framework rules](docs/spin_offs.md#framework-rules) and the dialogue
   release's own words and links folder are built, not played). Then the shared base:
   CommonLibF4 (Dear-Modding), Address Library, xmake
   ([runtime](docs/spin_offs.md#runtime)).
4. **The HUD, the rest** ([host](docs/swf_framework.md#hud-built), with
   HUDFramework as host when installed, and our own
   [saved places and edit menu](docs/swf_framework.md#hud-edit), and the
   [layout module](docs/swf_framework.md#hud-layout) reading `DEF_HUD.xml`,
   with the HUD's parts moved and scaled in the edit menu too, built, not
   played), and the [FNV layout preset](docs/swf_framework.md#fnv-preset)
   with its [weapon condition bar](docs/swf_framework.md#fnv-widgets)
   (built, not played), and [hiding and recoloring](docs/swf_framework.md#hud-look)
   any part in the edit menu, and parts shown on
   [conditions](docs/swf_framework.md#hud-conditions) (in combat, with a
   weapon out, in V.A.T.S., on a key, as a value changes or runs low, as
   the location moves, aiming, sneaking; `[Hud] sShow=Immersive`) (built,
   not played), each part's rule set in the edit menu, and other mods'
   conditions and rules (`CRUI_Hud.SetCondition`, `SetShowRule`). The
   edit menu works with a gamepad too (built, not played).
5. **An example menu** for authors, and the API in this README.
6. **CI** that fetches the Flex SDK and builds the movies; release assets.
7. **The converter uses this repo** as a pinned submodule in `external/`.

## License

GPL-3.0 with the modding and linking exceptions in [EXCEPTIONS](EXCEPTIONS),
so menus and mods built on it may use any license.
