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
| `dll`: CleanroomUI.dll and CleanroomDialogue.dll (`dll/xmake.lua`) | xmake 3.1 and Visual Studio; Dear-Modding's CommonLibF4 (from `commonlibf4-template`, submodules included), which `crui` patches from `dll/patches/` first | xmake: `CRUI_XMAKE`, else `PATH`, else `C:\Program Files\xmake`; CommonLibF4: `CRUI_COMMONLIBF4`, else `../tools_ext/commonlibf4-template/lib/commonlibf4` |

The game's `Base.zip` is unpacked into `out/cache/` when the Creation Kit
hasn't unpacked it yet. xmake keeps its downloaded libraries in
`out/xmake/` unless `XMAKE_GLOBALDIR` is set
([why](docs/spin_offs.md#port-status)).

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
| `papyrus/` | The level-up host, its holder and its per-plugin tables (`CRUI_LevelUp`, `CRUI_LevelUpHolder`, `CRUI_LevelUpTable`), `CRUI_Array`, and the scripts declaring CleanroomUI.dll's natives (`CRUI_Condition`, `CRUI_Framework`, `CRUI_HudNative`, `CRUI_ItemCard`, `CRUI_LevelUpNative`, `CRUI_Pipboy`, `CRUI_PipboyAmmo`, `CRUI_Sex`), and `CRUI_Hud`, which puts widgets on HUDFramework when installed, else on our host |
| `dll/` | `xmake.lua` building both DLLs on CommonLibF4: `framework/` (CleanroomUI.dll: condition API, Pip-Boy overlay, card lines, tabs, repair and ammo lists, transfer menus, `ChangeSex`, the game's perk points) and `dialogue/` (CleanroomDialogue.dll), both with `base/` (Flash helpers and the natives every movie may call); `patches/` (ours to CommonLibF4) |
| `docs/` | The framework, level-up and spin-off plans, as written for the converter |
| `tests/` | Contract tests between movies, scripts and the DLL, and the reproducible builds |

Stayed in the converter: the FNV table and icon builders (they read FNV
records), the level-up MCM page (it sets the converter's rule rates), the
Pip-Boy tab quests, the dialogue links writer and test mod, the casino, and
FNV's own rules in its F3NV.dll (Damage Threshold, Hardcore's HUD and ammo
weight, weapons' spawn condition, no combat sprint).

## Next

Status as of 2026-11-14. The full list, in order:
[next](docs/swf_framework.md#next).

1. **Play everything built since 2026-11-12.** None of it has been played
   in Cleanroom UI's own build: the Pip-Boy movie (CND line, lists,
   item-card lines, under vanilla and FallUI), the level-up menu (opened in
   the converter on 2026-11-12; the full check and the holder without the
   converter are left), `Input`, `HintBar`, `ScrollList`, `Modal` and
   `Images`, the dialogue menu's own build, words and links, and the whole
   HUD: the host (also under HUDFramework), saved places and the edit menu
   (mouse, keys, gamepad), DEF_HUD.xml layouts, the FNV preset and its
   weapon condition bar, hiding and recoloring, and parts shown on
   conditions (combat, weapon drawn, V.A.T.S., a key, a value changing or
   low, the location moving, aiming, sneaking; `[Hud] sShow=Immersive`;
   each part's rule in the edit menu; other mods' conditions and rules
   through `CRUI_Hud.SetCondition` and `SetShowRule`). And both DLLs as
   ported to CommonLibF4 on 2026-11-13 and 14: every native and hook of
   CleanroomUI.dll (item condition, repair and vendor repair, the
   transfer and barter CND lines, the Pip-Boy lines, `ChangeSex`, the
   perk points) and the dialogue menu's reference lookup
   ([port](docs/spin_offs.md#port-status)). Then Cleanroom UI alone on a
   stock Fallout 4, no converter ([stock check](docs/swf_framework.md#stock-check)).
2. **This repo as the converter's submodule** in `external/`. The converter
   already runs on Cleanroom UI (on its `fo4-2026-10-31`, not played): its
   scripts on the `CRUI_` natives, its F3NV.dll cut to FNV's rules, shipping
   our two DLLs from a pinned commit, its dialogue links in
   `CleanroomDialogue\` ([split](docs/spin_offs.md#split-status)).
3. **The shared base, other game versions**: both DLLs are on CommonLibF4
   (built 2026-11-14, not played) and run on 1.11.240 only; next, the
   other game versions' layouts (old-gen IDs matched 2026-11-14)
   ([port](docs/spin_offs.md#port-status)). Upstream to CommonLibF4: our
   two patches (the version check, `IDDB::has`) and its two wrong old-gen
   IDs ([which](docs/spin_offs.md#og-match)).
4. **The container and barter CND line** as a movie, retiring that C++, and
   the movie check in preflight.
5. **The HUD's rest, as play asks**: fades, hiding a part's pieces
   (`flags`), rotating and opacity in the edit menu, a key to open it
   without an MCM, the power armor HUD, `color2`, `SafeX`/`SafeY`,
   `globalscale`, FNV's CND arrows and ammo type label, aiming in third
   person, a pad button for the show key.
6. **The level-up rest**: skills of other mods; an MCM page for the host's
   own options (`SkillCap`, `ShowAllPerks`).
7. **The DLL side, translations, controller**: injection by menu URL,
   a settings INI to the movies, GFx log routing; the Papyrus key check
   and a test language; the Pip-Boy "more" button on a pad.
8. **Release**: an example menu for authors and the API in this README;
   CI that fetches the Flex SDK and builds the movies; release assets;
   Nexus.

## License

GPL-3.0 with the modding and linking exceptions in [EXCEPTIONS](EXCEPTIONS),
so menus and mods built on it may use any license.
