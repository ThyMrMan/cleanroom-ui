# Spin-off mods from F3NV.dll

A plan to release parts of the converter's runtime as Fallout 4 mods that
anyone can use, with or without the FNV conversion. Status lives in the
[roadmap](ROADMAP.md#parked). The dialogue menu has its own plan:
[dialogue menu mod](dialogue_menu_mod.md).

## <a id="why"></a>Why

F3NV.dll already does several things FO4 modders ask for: item condition per
inventory stack, readable from Papyrus; extra Pip-Boy STAT tabs; a dialogue
menu that lists any number of options. Released on their own, they help
people who will never run the converter. The converter then uses the same
releases, so there is one copy of each piece.

## <a id="today"></a>What F3NV.dll is today

One F4SE plugin (`tes_runtime/fo4_condition/`, about 3,200 lines). Each
feature is one file. All features share `condition.h`, and `plugin.cpp`
installs all of them at load.

| File | Feature | Papyrus | Installs |
|---|---|---|---|
| `plugin.cpp`, `repair.cpp`, `vendor.cpp`, `transfer.cpp` | Item condition per stack, repair from spares, vendor repair, CND line in the transfer and barter menus | `F3NV_Condition` (18 natives) | Spawn-health call redirect; transfer and barter menu listeners |
| `pipboy.cpp` | CND line, repair list and ammo list on the Pip-Boy item card | `F3NV_Condition.Pipboy*`, `F3NV_PipboyAmmo` | Pip-Boy menu listener |
| `pages.cpp` | Extra STAT tabs | `F3NV_Pipboy.SetTab` | (through the Pip-Boy listener) |
| `ammo.cpp` | Ammo choice list for the highlighted gun | `F3NV_PipboyAmmo` | (through the Pip-Boy listener) |
| `damage.cpp` | Flat Damage Threshold, per pellet | `F3NV_Damage.SetRules` | Four resistance-curve call redirects; off until `SetRules` |
| `hardcore.cpp` | Ammo weight outside Survival, HUD needs | `F3NV_HardcoreNative.Track` | One call redirect; off until its global is set |
| `sex.cpp` | Change an actor's sex through the game's own handler | `F3NV_Sex.ChangeSex` | Nothing |
| `perks.cpp` | Set the game's own perk points (the Pip-Boy's LEVEL UP notice) | `CRUI_LevelUpNative.SetGamePerkPoints` | Nothing |
| `dialogue.cpp` | Dialogue menu with any number of options | Scaleform natives `F3NV` | Three call redirects at PostLoad; stays out when `XDI.dll` is loaded |
| `plugin.cpp` | No combat sprinting | none | Turns `bCombatPathSprinting` off for every actor, always |

Hardcoded: about 35 `RelocAddr` addresses for Fallout4.exe 1.11.240, built on
raw F4SE 0.7.9 with no Address Library.

## <a id="split"></a>The split

| Release | Contains | For |
|---|---|---|
| **Framework DLL** (`CleanroomUI.dll`) | Item condition API, Pip-Boy and menu CND lines, Pip-Boy tabs API, `ChangeSex`, the Papyrus array library (`CRUI_Array.psc`) | Mod authors. Does nothing on its own |
| **Dialogue menu** (`CleanroomDialogue.dll`) | `dialogue.cpp` and its menu file, separate DLL | Players replacing XDI. No framework needed. See [its plan](dialogue_menu_mod.md) |
| **NV Weapon Condition & Repair** (content mod) | ESP + scripts that track weapons and armor, repair lists, vendors | Players. Requires the framework |
| **NV Ammo Switching** (content mod, optional) | ESP + scripts for the ammo list | Players. Requires the framework. Many alternatives exist; low priority |
| **Dynamic collision** | New DLL ([plan](dynamic_collision_mod.md)), from [the research](research/fo4_dynamic_collision.md) | Players and mod authors. No FO4 equivalent exists. No code yet |
| **F3NV.dll** | The converter's own rules: Damage Threshold, Hardcore, combat sprint, spawn condition | The FNV build only. Requires the framework and the dialogue menu |

Not released on their own:
- **Damage Threshold.** Zoltanite's mod (Nexus 109168) already does this, and ours follows its approach. Keep the credit in the code.
- **Ammo weight and combat sprint.** These are FNV rules, not general features.

### <a id="split-status"></a>Where the split stands (built 2026-11-12, not played)

In Cleanroom UI (`dll/`, `python -m crui dll`), from F3NV.dll's sources:
- **`CleanroomUI.dll`** (`dll/framework/`): condition API, Pip-Boy overlay,
  item-card lines, STAT tabs, repair and vendor repair lists, ammo list,
  transfer and barter CND lines, `ChangeSex`, the game's perk points. Natives on
  `CRUI_Condition`, `CRUI_ItemCard`, `CRUI_LevelUpNative`, `CRUI_Pipboy`,
  `CRUI_PipboyAmmo`, `CRUI_Sex`; movies
  reach it as `f4se.plugins.CleanroomUI`; external events
  `CRUI_ConditionRepair`, `CRUI_AmmoSwitch`; log `CleanroomUI.log`. It takes
  no trampoline and no longer sets the spawn condition or combat sprint. On
  CommonLibF4 since 2026-11-14 ([port](#port-status)).
- **`CleanroomDialogue.dll`** (`dll/dialogue/`): the dialogue menu alone,
  on CommonLibF4 since 2026-11-13 ([port](#port-status));
  `f4se.plugins.CleanroomDialogue`; log `CleanroomDialogue.log`.
- **`dll/base/`** in both DLLs: the Flash helpers and the natives
  every movie may call (`TextureExists`, `HudColor`). crui's `Bridge.shared()`
  asks `CleanroomUI`, then `CleanroomDialogue`, so the level-up menu and
  `Images` work with either installed.
- **Not here:** Damage Threshold, Hardcore (HUD needs, ammo weight), weapons'
  spawn condition and combat sprint: the converter's F3NV.dll keeps them
  (its `tes_runtime/fo4_condition`).

The converter on Cleanroom UI (its branch `fo4-2026-10-31`, checked
2026-11-14, not played):
- **Its F3NV.dll keeps only FNV's rules**: Damage Threshold, Hardcore, and
  the Goodbye line ending the player's talk. Its converted scripts call the
  `CRUI_` natives.
- **It ships our two DLLs** from `tes_runtime/dist`, built from the commit
  in `tes_runtime/dist/cleanroom_ui.txt` (`fo4_import/cleanroom.py`).
- **Its dialogue links** go to `F4SE\Plugins\CleanroomDialogue\`; each
  build deletes its own file from the old `F3NV_Menus\`.

Not done yet:
- **This repo as the converter's submodule** in `external/`, in place of
  the copied DLLs and their pin.
- **Dropping the `F3NV_Menus\` read** from `CleanroomDialogue.dll`, once
  converter builds from before 2026-11-13 are gone.

The dialogue release stands alone (built 2026-11-13, not played):
- **Its words** are `$CRUI_LEAVE`, `$CRUI_HINT_SELECT` and `$CRUI_HINT_LEAVE`
  in `CleanroomDialogue_<language>.txt` (`translations/CleanroomDialogue/`),
  every game language written. F4SE reads one translation file per active
  plugin and this release ships none, so `CleanroomDialogue.dll` loads its
  own: when input loads (else when the game has loaded) it takes the
  translator from the game's Scaleform manager and reads the file with F4SE's
  own `Translation::ParseTranslation`.
- **Its links** are read from `Data\F4SE\Plugins\CleanroomDialogue\*.txt`,
  one file per plugin with options, in the same format. The converter's old
  `F3NV_Menus\` folder is still read after it, logged `DEPRECATED`, for
  converter builds from before it wrote ours.

### <a id="framework-rules"></a>Rules for the framework

Built 2026-11-13, not played (`CleanroomUI.dll` version 2):
- **Changes nothing on load.** A hook changes game behavior only after a mod
  asks for it through a Papyrus call. Combat sprinting and the spawn-health
  redirect moved to F3NV.dll. The REPAIR hint and G key appear only once a
  mod tracks an item (`CRUI_Condition.Track`); the AMMO hint and B key only
  once a mod gives ammo lists (`CRUI_PipboyAmmo.SetLists`).
- **Renames.** The Papyrus scripts get the framework's own prefix. F3NV
  keeps thin `F3NV_*` wrappers until the converter's scripts are regenerated.
- **Several mods at once.** Each call adds to what other mods gave:
  - `Track`: the tracked items.
  - `SetTab`: tabs, by name (a name given again replaces that tab).
  - `TrackRepair`: an item repairs from every list it was given, and with
    Jury Rigging from any item sharing one of its groups. Group numbers are
    shared by every mod; the last Repair skill given is used.
  - `SetLists`: ammo lists, by keyword (a list given again replaces its
    rounds), forgotten as a save loads, so each mod gives its own on every
    load. `ChosenList` names the chosen round's list, so a mod takes only
    its own choices; `TakeChoice` is the index in that mod's rounds.
- **Version and API checks.** `CRUI_Framework.GetVersion()`, F4SE's plugin
  version, and an F4SE message: another plugin dispatches type `'CRUV'` to
  `CleanroomUI` and gets the version back (a `UInt32`, same type). All read
  `VERSION` in `dll/framework/condition.h`, raised whenever a native, event
  or rule changes.
- **Log** to `Documents\My Games\Fallout4\F4SE\<name>.log` at the level in
  `Data\F4SE\Plugins\<name>.ini`, both DLLs (`OpenLog`):
  ```ini
  [Log]
  ; 0 errors, 1 warnings, 2 messages (the default), 3 every open, list and key
  iLevel=2
  ```
  The framework's INI also holds its [keys](swf_framework.md#keys)
  (`[Keys]`: the Pip-Boy's repair and ammo keys, the HUD edit menu's reset
  key) and the HUD's [preset](swf_framework.md#fnv-preset) (`[Hud]
  sPreset=FNV`).
  Per-open and per-key lines (Pip-Boy lists, repairs, the dialogue menu's
  picks and timings) are level 3; a menu not laid out as expected is a
  warning.

## <a id="runtime"></a>One base for both game versions

Players are split between old-gen 1.10.163 and next-gen 1.10.984 / 1.11.x.
Every DLL above needs both versions, so the port is done once:

1. Move from raw F4SE 0.7.9 to **Dear-Modding-FO4/commonlibf4**
   (GPL-3.0-or-later with the modding and linking exceptions), one DLL for
   1.10.163, 1.10.980/984 and 1.11.137–1.11.240. Chosen 2026-11-11 over
   LucaDotGit's "AV" fork: 24 contributors against one, active daily,
   our license family, and the base of Addictol and DearModdingUI
   ([comparison](research/commonlibf4_forks.md)). No VR; see
   [platforms](#platforms).
2. **Fix its version check first.** It accepts only an exact list of game
   versions and terminates the game on any other, so a game update would
   crash every DLL built on it until we rebuild. Change it to treat a
   newer version as the latest known one (what AV does), offer that
   upstream as a pull request, and carry it locally if refused
   ([why](research/commonlibf4_forks.md#verdict)).
   Read 2026-11-13: the check is `GetRuntimeForVersion` in
   `lib/commonlib-shared/src/REX/FModule.cpp`, an exact list per runtime;
   `GetRuntimeIndex` calls `REX::FAIL` and `std::terminate` on any other,
   and its own `static_assert` makes 1.11.241 unsupported.
3. Replace each `RelocAddr` with an Address Library ID (next-gen IDs for
   all of ours found 2026-11-13: [ids](#ids)). The fork names
   about half of F3NV.dll's ([coverage](research/commonlibf4_forks.md#ours))
   and the dialogue plan lists IDs for its functions. Find the rest by
   matching each function in the 1.10.163 exe, and send them upstream.
4. Build with the fork's own xmake (start from
   `Dear-Modding-FO4/commonlibf4-template`, GPL-3.0) in place of
   `build.bat`. xmake finds Visual Studio itself, so the Community
   `vcvars64` workaround ([runtime DLL build](../../tes_runtime/common/msvc.bat))
   should stop being needed.
   Checked 2026-11-13: the unchanged template (`tools_ext/commonlibf4-template`,
   xmake 3.1.2) builds in 20 s with `xmake f -p windows -a x64 -m release`,
   finding Visual Studio itself; without `-p windows`, under Git Bash, xmake
   picks MinGW and fails on spdlog.
5. Write a check for each hooked call site: when the bytes there aren't what
   we expect, log it and leave that feature off. The game should keep
   running.

### <a id="port-status"></a>Where the port stands (2026-11-14, not played)

- **Both DLLs are on CommonLibF4**, built by `dll/xmake.lua` through
  `python -m crui dll`: CleanroomDialogue.dll since 2026-11-13,
  CleanroomUI.dll since 2026-11-14. `build.bat`, `msvc.bat` and
  `dll/shared/` are gone; both share `dll/base/`. The C runtime is static,
  so each DLL imports only system DLLs.
- **CleanroomUI.dll's F4SE plugin version is now 17.0.0**, packed as F4SE
  packs versions (0x11000000), where the old build gave the raw 17.
  `xmake.lua` reads it from `VERSION` in `framework/condition.h`; the
  Papyrus `GetVersion` and the `CRUV` message still give 17.
- **Its 25 addresses are next-gen IDs** (each with its 1.11.240 RVA beside
  it). An ID a database lacks resolves to 0, and the hooks stay off, in
  place of CommonLibF4 ending the game.
- **The hooks run only on 1.11.240** (`CheckedRuntime`), the one build whose
  struct layouts were read. They also stay off when XDI.dll is loaded or a
  call site doesn't call what we expect (step 5). CleanroomUI.dll goes
  further: on any other build it registers no natives and installs no menu
  hook at all, and logs that it is off.
- **Our two patches to commonlib-shared** are in `dll/patches/`, applied by
  `crui` before each build: the version check (step 2) and `IDDB::has`.
  Both are meant for upstream.
- **xmake under the `py` launcher:** `py` is a packaged Windows app, and its
  children can't see `%LOCALAPPDATA%\.xmake`. So `crui` points
  `XMAKE_GLOBALDIR` at `out/xmake` (unless it is already set) and
  reconfigures with `xmake f -c`. The first build downloads the packages
  there.
- **Old-gen IDs are in** for both DLLs (2026-11-14, [ids](#ids)), but
  nothing runs there yet: the hooks wait for old-gen's struct layouts.
- **Not done:** the other game versions' layouts, and seven of F3NV.dll's
  old-gen IDs ([open](#og-match)).

### <a id="ids"></a>Our addresses as Address Library IDs (2026-11-13, old-gen 2026-11-14)

Every hard-coded 1.11.240 address in `CleanroomUI.dll`, `CleanroomDialogue.dll`
and the converter's `F3NV.dll`, as its next-gen ID, read from the Address
Library databases (`fo3fnv-in-fo4`'s `tools/disasm/address_lib.py --rvas
... --builds ...`). An ID with `+0x...` is a call site or a vtable slot
inside that function or vtable: the ID moves with the build, the offset
inside it may not, so each of those is a byte-checked patch (step 5).

- **All 51 have an ID in 1.11.240**, and 48 exist in every next-gen
  database (1.10.980, 1.10.984, 1.11.137, 1.11.191, 1.11.221).
- **Three start at 1.11.137:** the topic manager, the Pip-Boy data and the
  inventory interface globals (IDs 4796064, 4796372, 4796586). 1.10.980 and
  984 number those globals elsewhere; find them there by matching.
- **Old-gen (1.10.163)** numbers its IDs apart. Every Cleanroom UI address
  was matched in the 1.10.163 exe on 2026-11-14 ([how](#og-match)); seven
  of F3NV.dll's are still open. `same` is a vtable, which keeps one ID; an
  old-gen `+0x...` is that build's own offset.

| DLL, file, name | 1.11.240 RVA | Next-gen ID | In next-gen | Old-gen ID |
|---|---|---|---|---|
| Dialogue `dialogue.cpp` AliasHandle | 0x671fa0 | 2207810 | all | 688914 |
| Dialogue `dialogue.cpp` CAMERA_TO_PLAYER | 0x83266c | 2214898+0xadc | all | 24497+0x940 |
| Dialogue `dialogue.cpp` EnableShadedBackground | 0x1b04350 | 2287022 | all | 278402 |
| Dialogue `dialogue.cpp` FIRST_PERSON_SHOULD | 0x10213e0 | 2248264 | all | 895531 |
| Dialogue `dialogue.cpp` FIRST_PERSON_VTABLE | 0x25aab08 | 246953 | all | same |
| Dialogue `dialogue.cpp` HandleRef | 0x21e550 | 2188369 | all | 967277 (see below) |
| Dialogue `dialogue.cpp` MarkSaid | 0x69f8d0 | 2208467 | all | 1160364 |
| Dialogue `dialogue.cpp` NPC_SLOT_CALL | 0x415a21 | 2196817+0x7a1 | all | 1439851+0x777 |
| Dialogue `dialogue.cpp` OptionInfo | 0x416600 | 2196825 | all | 781358 |
| Dialogue `dialogue.cpp` PLAYER_DIALOGUE_VTABLE | 0x2488978 | 388825 | all | same |
| Dialogue `dialogue.cpp` PLAYER_SLOT_CALL | 0x41568a | 2196817+0x40a | all | 1439851+0x3e1 |
| Dialogue `dialogue.cpp` PLAYER_SPEAKS | 0x4158d2 | 2196817+0x652 | all | 1439851+0x62e |
| Dialogue `dialogue.cpp` Passes | 0x69f490 | 2208464 | all | 244581 |
| Dialogue `dialogue.cpp` Pick | 0x698db0 | 2208361 | all | 433331 |
| Dialogue `dialogue.cpp` Prompt | 0x69e740 | 2208446 | all | 1435199 |
| Dialogue `dialogue.cpp` RESPONSE_CALL | 0x413da8 | 2196800+0x388 | all | 574707+0x388 |
| Dialogue `dialogue.cpp` ResponseInfo | 0x414180 | 2196806 | all | 1369299 |
| Dialogue `dialogue.cpp` SHADER_TARGET_VTABLE | 0x27236d0 | 298683 | all | same |
| Dialogue `dialogue.cpp` SelectOption | 0xd76130 | 2233190 | all | 696117 |
| Dialogue `dialogue.cpp` StopScene | 0x638d60 | 2206866 | all | 904456 |
| Dialogue `dialogue.cpp` THIRD_PERSON_SHOULD | 0x102dfc0 | 2248466 | all | 908569 |
| Dialogue `dialogue.cpp` THIRD_PERSON_VTABLE | 0x2522a88 | 728085 | all | same |
| Dialogue `dialogue.cpp` TintChallenge | 0x1043ef0 | 2248685 | all | 1432530 |
| Dialogue `dialogue.cpp` ZoomInName | 0xb25620 | 2224896 | all | 1282473 |
| Dialogue `dialogue.cpp` ZoomOutName | 0xa7a080 | 2223332 | all | 750808 |
| F3NV `damage.cpp` HitResist | 0xe38a00 | 2236862 | all | open |
| F3NV `damage.cpp` HitTyped | 0xe387b0 | 2236861 | all | open |
| F3NV `damage.cpp` PHYSICAL_CALL | 0xe38c3d | 2236862+0x23d | all | open |
| F3NV `damage.cpp` ResistCurve | 0x6cb2e0 | 2209007 | all | open (see below) |
| F3NV `damage.cpp` SHOT_RESIST_CALL | 0xe37472 | 2236846+0x252 | all | open |
| F3NV `damage.cpp` SHOT_TYPED_CALL | 0xe373bf | 2236846+0x19f | all | open |
| F3NV `damage.cpp` TYPED_CALL | 0xe3af4a | 2236870+0xca | all | 308245+? |
| F3NV `hardcore.cpp` AMMO_DIFFICULTY_CALL | 0x3220e0 | 2193446+0x110 | all | 1321341+? |
| F3NV `hardcore.cpp` Difficulty | 0xd6d600 | 2233056 | all | 922962 (CommonLibF4, unchecked) |
| F3NV `plugin.cpp` COMBAT_PATH_SPRINTING | 0x2f38a98 | 2664479 | all | open |
| F3NV `plugin.cpp` MaxHealth | 0x316940 | 2193227 | all | 1515099 |
| F3NV `plugin.cpp` SPAWN_HEALTH_CALL | 0x271611 | 2190031+0xa1 | all | 250701+? |
| F3NV `talk.cpp` EndTalkNative | 0xbb0720 | 2227155 | all | 674395 |
| F3NV `talk.cpp` HandleRef | 0x21e550 | 2188369 | all | 967277 (see below) |
| F3NV `talk.cpp` TopicManager | 0x30e6e50 | 4796064 | from 1.11.137 | 1391114 (CommonLibF4) |
| UI `perks.cpp` SetPlayerPerkCount | 0xd75ff0 | 2233187 | all | 616372 |
| UI `pipboy.cpp` g_inventoryDataVtable | 0x2544750 | 952887 | all | same |
| UI `pipboy.cpp` g_pipboyData | 0x30e8af8 | 4796372 | from 1.11.137 | 1178971 |
| UI `plugin.cpp` CHECK_STACK_ID_VTABLE | 0x247a038 | 168197 | all | same |
| UI `plugin.cpp` FindAndWriteStackData | 0x502560 | 2200937 | all | 1304040 |
| UI `plugin.cpp` SET_HEALTH_VTABLE | 0x252b680 | 606842 | all | same |
| UI `repair.cpp` RemoveFromStack | 0x34e020 | 2194169 | all | 400062 |
| UI `transfer.cpp` BARTER_LINE_SLOT | 0x2534710 | 910737+0xe0 | all | same+0xe0 |
| UI `transfer.cpp` CONTAINER_LINE_SLOT | 0x25ac7a8 | 1098621+0xe0 | all | same+0xe0 |
| UI `transfer.cpp` ItemOf | 0x347c10 | 2194009 | all | 1200959 |
| UI `transfer.cpp` g_inventoryInterface | 0x30ec470 | 4796586 | from 1.11.137 | 501899 |

**What CommonLibF4 already names** (`include/RE/IDs.h`, `IDs_VTABLE.h`,
read 2026-11-13 at the template's pin `a312a9f`): 26 of our 46
IDs, each with its old-gen ID (a `VariantID {old-gen, next-gen[, 1.11]}`),
so these need no matching. The three globals missing from 1.10.980/984
have their 1.10.98x IDs there too. Vtable IDs are one set across builds.

| Next-gen ID | CommonLibF4 name | Old-gen ID |
|---|---|---|
| 1098621 | vtable `ContainerMenu` | same ID |
| 168197 | vtable `BGSInventoryItem::CheckStackIDFunctor` | same ID |
| 2193227 | `TESHealthForm::GetFormHealth` | 1515099 |
| 2193446 | `TESWeightForm::GetFormWeight` | 1321341 |
| 2194009 | `BGSInventoryInterface::RequestInventoryItem` | 1200959 |
| 2194169 | `BGSInventoryList::RemoveItem1` | 400062 |
| 2196800 | `BGSSceneActionNPCResponseDialogue::UpdateAction2` | 574707 |
| 2196817 | `BGSSceneActionNPCResponseDialogue::UpdateAction1` | 1439851 |
| 2196825 | `BGSSceneActionNPCResponseDialogue::GetCurrentTopicInfo` | 1369299: wrong, it is 781358 ([why](#og-match)) |
| 2207810 | `TESQuest::GetAliasedRef` | 847223: wrong, it is 688914 ([why](#og-match)) |
| 2209007 | `CombatFormulas::CalcResistedPercentage` | 420470 |
| 2214898 | `DialogueCameraState::UpdateActorsAndAction` | 24497 |
| 2233056 | `PlayerCharacter::GetDifficultyLevel` | 922962 |
| 2233187 | `PlayerCharacter::SetPerkCount` | 616372 |
| 2233190 | `PlayerCharacter::SetLastDialogueInput` | 696117 |
| 2287022 | `BSGFxShaderFXTarget::EnableShadedBackground` | 278402 |
| 246953 | vtable `FirstPersonState` | same ID |
| 298683 | vtable `BSGFxShaderFXTarget` | same ID |
| 388825 | vtable `BGSSceneActionPlayerDialogue` | same ID |
| 4796064 | `MenuTopicManager::Singleton` | 1391114 (1.10.98x: 2688800) |
| 4796372 | `PipboyDataManager::Singleton` | 1178971 (1.10.98x: 2689086) |
| 4796586 | `BGSInventoryInterface::Singleton` | 501899 (1.10.98x: 2689299) |
| 606842 | vtable `BGSInventoryItem::SetHealthFunctor` | same ID |
| 728085 | vtable `ThirdPersonState` | same ID |
| 910737 | vtable `BarterMenu` | same ID |
| 952887 | vtable `PipboyInventoryData` | same ID |

### <a id="og-match"></a>How the old-gen IDs were matched (2026-11-14)

The 1.10.163 exe (Steam depot 377162, manifest 5847529232406005096) is
SteamStub-encrypted: its code section was decrypted from the stub's own
header (variant 3.1: AES-256-CBC, the key, IV and 16 stolen bytes in the
header 0xF0 bytes before the entry point). Each next-gen function was then
found there by its call graph: callers and callees CommonLibF4 already
pairs (846 functions and vtable slots), a caller's call list aligned
against its old-gen twin's, the neighbors a source file keeps together, a
vtable's same slot, and the code itself compared instruction by
instruction. Every match above was read side by side in both builds. The
scripts are kept outside the repo (`fo3fnv-in-fo4`'s `temp/og_ids/`).

What the old-gen code does differently:
- **The player-speaks patch:** old-gen orders that block otherwise, so its
  `mov rcx, rax` is at +0x62e and the jump past the call is `EB 14 90`, not
  `EB 1E 90` (`SILENT_OG` in `dialogue.cpp`).
- **A handle's reference:** next-gen's `2188369` takes (out, handle);
  old-gen has no such function and calls `967277`, CommonLibF4's
  `GetSmartPointer` (handle, out). `dialogue.cpp` now calls CommonLibF4's,
  paired in both builds (next-gen `2188681`), so `2188369` is no longer used.
- **ZoomIn:** old-gen's third-person test reads the static name inline, but
  the same name's getter exists (`1282473`, the same static string).

**CommonLibF4's wrong pairs** (include/RE/IDs.h at `a312a9f`), to send upstream:
- `TESQuest::GetAliasedRef {847223, 2207810}`: next-gen `2207810` is the
  locking wrapper, identical to old-gen `688914`; `847223` is the inner
  function the wrapper calls (next-gen `2207980`).
- `BGSSceneActionNPCResponseDialogue::GetCurrentTopicInfo {1369299,
  2196825}`: old-gen `1369299` is next-gen `2196806` (the same code, called
  from the same two functions); `2196825` is the player dialogue's
  (`{781358, 2196825}`, also in the file, is right).
- `CombatFormulas::CalcResistedPercentage {420470, 2209007}` (F3NV.dll's
  ResistCurve): old-gen `420470` is a short function that calls `1096523`,
  whose call list is next-gen `2209007`'s; not settled.

**Still open** (F3NV.dll only, which is on raw addresses until it moves to
CommonLibF4): `2236846`, `2236861`, `2236862` (the damage calls), the
`bCombatPathSprinting` setting `2664479`, `2209007` above, and the call-site
offsets inside `308245`, `1321341` and `250701`.

## <a id="platforms"></a>Platforms: PC versions, VR, consoles

From a survey on 2026-11-11 (**V** read in source or official notes,
**S** search results only).

### How Address Library fits in

- **The problem:** every game update moves the game's functions to new
  addresses. F3NV.dll calls about 35 functions by raw address for
  1.11.240, so every update breaks it until we rebuild.
- **What it does:** Address Library (Nexus 47327, by meh321) gives each
  function a **stable ID** and ships one small database per game version
  (`Data\F4SE\Plugins\version-1-11-240-0.bin`), which the player installs.
  A DLL asks for ID 2196825 and gets that function's address in whatever
  version is running. One DLL then works on every version that has a
  database, often including future ones, without a rebuild.
- **Telling F4SE:** a DLL declares in its version data that it uses
  Address Library (`addressIndependence`) and doesn't depend on struct
  layouts (`structureIndependence`), so F4SE loads it on new game versions
  instead of refusing it (F4SE `PluginAPI.h`, V).
- **What still breaks:** a changed class layout, a function the compiler
  inlined or rewrote, and our byte patches at call sites. That's why every
  hook keeps checking the bytes it patches and turns its feature off with a
  log line when they differ.
- **ID sets:** next-gen renumbered everything, so old-gen and next-gen have
  different IDs for the same function. CommonLibF4 handles this with
  `REL::RelocationID(oldGenId, nextGenId)`.
- **Speed:** for 1.11.240 (18 Aug 2026), F4SE shipped the same day and the
  Address Library database three days later (S). Players of an
  Address Library DLL wait for that update, not for us.

### Game versions

| Version | F4SE | Address Library | CommonLibF4 forks |
|---|---|---|---|
| Old-gen 1.10.163 | 0.6.23 | `.bin`, old IDs | Dear-Modding, LucaDotGit "AV", alandtse |
| Next-gen 1.10.984 | 0.7.2 | `.bin`, next-gen IDs | Dear-Modding, LucaDotGit "AV", alandtse |
| 1.11.x (current 1.11.240) | 0.7.9 | `.bin`, next-gen IDs | Dear-Modding (up to 1.11.240), LucaDotGit "AV" |
| VR 1.2.72 | F4SEVR 0.6.21 (beta) | **CSV** (alandtse's VR Address Library: old-gen IDs mapped to VR) | alandtse; ArthurHub's VR-only CommonLibF4VR; an untested path in "AV" |

**Decision:** no fork we'd use covers all four, so **flat first, VR
later**: Dear-Modding for the three PC versions
([why](research/commonlibf4_forks.md#verdict)). When VR work starts, choose
then between a second build on alandtse's fork and adding VR to ours.

### VR

- **Possible, but its own product.** VR menus are drawn on 3D panels, the
  Pip-Boy is on the wrist or projected (FRIK), input is motion controllers,
  and our menu offsets differ. Flat UI mods mostly break there, which is why
  PipboyTabsVR and an XDI VR beta (Neanka, 2025, S) exist as separate builds.
  HUDFramework reportedly doesn't work in VR (S).
- **We can't test it:** VR isn't installed, and a headset is needed. Building
  blind would repeat the VR community's main complaint, mods that claim
  support and don't work.
- **Plan:** keep the code VR-ready (VR IDs where the library offers them, no
  flat-only assumptions in the shared base), release flat first, then offer
  a VR build as a beta for VR players to test, ideally with a VR tester or
  co-maintainer. ArthurHub's F4VRCommonFramework (VR input, VR UI widgets;
  license unchecked) is the reference for VR UI.

### Consoles

No script extender, so no DLL, no custom menus and no way for a movie to
call Papyrus. What's left is Papyrus plus replaced vanilla movies.

| | Xbox | PlayStation |
|---|---|---|
| New scripts | Yes | Not in any patch notes; assume no (V: the Aug and Oct 2026 notes add meshes, textures, sounds, video, not `.pex` or `.swf`) |
| New or replaced interface movies | Yes (FDI, HUDFramework, DEF_UI ship there, S) | Assume no |
| Size | Up to 2 GB per upload | Smaller |

What each framework could be on Xbox:

| Framework | Xbox without a DLL | Worth it? |
|---|---|---|
| HUD widgets | Yes, as HUDFramework does: Papyrus sends text through `ShowPerkVaultBoyOnHUD` and `Message.Show` number slots, a replaced `HUDMenu.swf` catches it. One way only | **Yes, the cheap way:** our widget movies already speak HUDFramework's `processMessage`, and our scripts reach HUDFramework through `CastAs`/`CallFunction`, which work on console. So our widgets run under HUDFramework on Xbox with no console framework of our own |
| Dialogue menu | A replaced `DialogueMenu.swf` can only restyle the game's four options (as FDI does). No more options, no topic links, no odds | Low. Maybe a "look only" edition once our movie exists, since ours is free of Bethesda code and can ship there |
| Pip-Boy | A replaced movie can only restyle the game's own data; no new data, no tabs fed from Papyrus | No |
| Custom menus, casino | Not possible. Message boxes, terminals and holotapes only | No (the FNV conversion can't go to console anyway) |
| Settings (MCM) | No menu; a settings holotape or terminal is the console norm | Later idea: one settings definition that shows in MCM on PC and as a holotape on console |

Rules for anything on Bethesda.net: upload only our own work; ports of
others' work need their public permission and a credit link
([community standards](https://bethesda.net/en-US/news/bethesda-softworks-community-standards), V).
Nothing written covers patched vanilla movies; they ship in practice. Our
own rule (no Bethesda files) keeps us to movies built from scratch, so a
console HUD of our own would mean a whole new HUD movie; another reason to
ride on HUDFramework there.

**Summary:** consoles get a reduced, Xbox-only experience through
HUDFramework for HUD widgets, and possibly a look-only dialogue menu.
Everything else needs F4SE and stays PC (and later VR).

## <a id="repo"></a>Where the code lives

- A new repo per release, or one repo with a folder per DLL and a shared
  `common/`. **Choose one repo**: the shared base would otherwise be copied.
- License: **GPL-3.0 with the modding exception**, for every release and
  for our code in both converter repos ([license](#license)). Credit the
  upstream tes4skyrim authors where code came from there (none of F3NV.dll
  did; the game bridge recorder would); their code stays MIT inside.
- fo4_main uses the release as a git submodule or a pinned download in
  `external/`, so the converter build stops compiling F3NV's shared parts
  itself.
- The FO4 build session owns F3NV.dll builds ([build ownership](../../CLAUDE.md)).
  The split is done on its own branch and handed over as "ready", never
  built into a game folder by the side session.

## <a id="order"></a>Order

1. **Dialogue menu**: the most wanted. XDI still gets Nexus builds, but its
   source has no license and stops in 2020, it pairs answers by position,
   and each game update waits on its authors
   ([its plan](dialogue_menu_mod.md#xdi-today)).
2. **Shared base**: CommonLibF4 (Dear-Modding), Address Library, xmake. The dialogue menu
   is the first DLL on it (built 2026-11-13, not played); CleanroomUI.dll is next
   ([port](#port-status)).
3. **Framework**: carve out the condition, Pip-Boy and tabs APIs; F3NV.dll
   then requires it.
4. **NV Weapon Condition & Repair**: the first content mod, also the
   framework's example mod.
5. **Dynamic collision**: once its open engine questions are answered.

## <a id="strategy"></a>Strategy: open, drop-in replacements

Widely used FO4 UI mods by the same small group (reg2k, very likely the
Nexus user registrator2000, and Neanka, who co-wrote XDI and made DEF_UI and
PipboyTabs) share one state: source published without a license, updates
mostly for new game versions, no new features. Nobody else can legally
maintain their cores.

| Mod | Last update | Source |
|---|---|---|
| XDI (dialogue) | Sep 2026 | GitHub, stops 2020, no license |
| HUDFramework | Mar 2017 (source uploaded Sep 2026) | `references/HUDFramework Source Code 1.0f`, no license |
| MCM | Sep 2026 | GitHub, no license |
| DEF_UI (DEF_HUD + DEF_INV, Neanka) | Real work 2016; 2024 repack "as-is, not supported" | None. Permissions forbid changing its files ([DEF_UI](#def-ui)) |

**Decision:** aim to be the open-licensed, drop-in replacement, one
framework at a time, the way Open Animation Replacer took over from Dynamic
Animation Replacer in Skyrim:
1. **Drop-in on day one:** read the old mod's config files and offer its
   Papyrus script names and functions, so mods built for it keep working.
2. **Clearly better**, not only open ([what we add](dialogue_menu_mod.md#beyond-xdi)).
3. **Not hostile:** credit the original, present ours as an alternative or
   successor.

**Order:**

| | Why this place | Effort |
|---|---|---|
| 1. XDI → our [dialogue menu](dialogue_menu_mod.md) | We need it anyway; the most to add | Large, half built |
| 2. Pip-Boy framework ([plan](swf_framework.md#pipboy)) | No incumbent to replace; builds the injection code the HUD needs | Medium |
| 3. HUDFramework → HUD on the same injection ([plan](swf_framework.md#hud)) | Ends the `HUDMenu.swf` conflict; instant messages on PC; about 25 Papyrus functions and one widget method to match. The host is built ([host](swf_framework.md#hud-built), not played); the HUDFramework-compatible API waits on its author | Medium once 2 exists |
| 3b. DEF_HUD layouts on the same HUD host ([reading built](swf_framework.md#hud-layout), not played) | Move, scale, recolor and hide the game's own HUD parts without replacing `HUDMenu.swf`; reads `DEF_HUD.xml`; ends the DEF_UI + HUDFramework merge patch | Small on top of 3 |
| 4. MCM | Works, most depended on, still updated; no clear gain yet | Largest; keep using its file format |
| 5. DEF_INV-style inventory | Icons, tabs and key-ring roll-ups from sorting mods' tags; FallUI Inventory already holds this space | Large; maybe never |

### <a id="def-ui"></a>DEF_UI

Read 2026-11-11 from `references/DEF_UI 1.6.0-…` (gitignored; Nexus page
included).
- **What it is:** Neanka's DEF_HUD (move, rotate, resize and recolor every
  HUD element) and DEF_INV (bigger container and barter screens, icons and
  tabs from sorting mods' name tags, key-ring roll-ups), repacked in 2024 by
  Valdacil "as-is … not monitored or necessarily supported". 1.4M unique
  downloads, 210 mods depend on it.
- **How it's built:** replaced vanilla movies, one set per screen shape
  (16:9, 16:10, 21:9, 4:3, 5:4): `HUDMenu.swf`, `PipboyMenu.swf`,
  `Pipboy_InvPage.swf`, `ContainerMenu.swf`, `BarterMenu.swf`,
  `ExamineMenu.swf`, `Workshop.swf`. Its movies date from 2016. Settings are
  edited outside the game with two Windows programs (`HUDMenuSet.exe`,
  `settings.exe`) that write XML into `Interface\DEF_CONF\`.
- **Its conflicts:** every other HUD mod, including HUDFramework, which needs
  a merged copy (shipped pre-made for HUDFramework 1.0f). It also ships its
  own copies of the game's `Translate_<lang>.txt`, which must be redone after
  each game update (1.6.0: "Updated Translate_en.txt for Next Gen update").
- **Permissions:** no changes or reuse of its files without asking; no
  source. Its icons (`iconlibs2.swf`) are its own assets and can't be used.
- **What other mods depend on is its config formats:** sorting mods (VIS,
  Better Item Sorting, and others) ship `DEF_INV_TAGS.xml` (name tag →
  icon) and `DEF_INV_TABS.xml` (tabs by filter and tag), and players keep
  `DEF_HUD.xml` layouts (per element: position, scale, rotation, colors,
  parent clip, shown parts).

**Where it fits:** not a framework, so not a target on its own. Its HUD
half is one module of our HUD host: an injected movie that applies a
`DEF_HUD.xml` layout to the game's own HUD parts, with an in-game drag
editor instead of an outside program and one layout for every screen shape.
Its inventory half competes with FallUI Inventory, which already replaced it
for most players; reading sorting mods' tag files is possible later, but
not planned.

**Rules:**
- **Formats and APIs, never code.** We've read their sources, so each
  compatibility spec is written in our own words from observed behavior and
  tested against real mods that use the original.
- **Stand-in plugins are the sensitive step.** Mods master `XDI.esm` and
  `HUDFramework.esm`, so a drop-in ships files with those names and record
  IDs. Ask the author before releasing one.
- **Author contact comes first.** If the author adds licenses, contribute to
  his mods instead (HUDFramework's unfinished F4SE fast path is a ready
  first contribution). If he declines or doesn't answer, release clean,
  credited replacements.
- **Finish the shared core first** (one DLL for every game version, the
  shared Flash library): each replacement is a promise to keep up with game
  updates, and only a shared core makes that affordable.

**Status:** the HUDFramework license question was sent to the author on
Nexus; waiting for an answer.

## <a id="license"></a>License

**Decision (2026-11-11): GPL-3.0 with the modding exception** for all our
releases and for our own code in both converter repos (`fo4_main`,
`tes4skyrim_standalone`).

- **Why:** XDI, HUDFramework and MCM are stuck because they have *no*
  license, so nobody may continue them. Any open license fixes that; GPL also
  stops anyone taking our work closed (a closed fork, or a closed rival
  converter, folding our code in), so every version built on ours stays
  open too.
- **The modding exception** (`EXCEPTIONS`, the text DCA and others ship):
  linking with the game and other proprietary game code, and with modding
  libraries under other licenses, doesn't pull that code under the GPL. Mods
  that only use our APIs (Papyrus calls, widgets, links files) are their own
  works under any license.
- **Files:** `LICENSE` (GPL-3.0 text), `EXCEPTIONS`, `NOTICE` at each repo's
  root. `NOTICE` says what each license covers: upstream tes4skyrim code
  stays MIT (its text is in `NOTICE`), third-party code under `external/`
  keeps its own, converted game data is Bethesda's and covered by none.
- **Compatible:** MIT, BSD, Apache and MPL-2.0 code can be used inside
  (Caprica, the papyrus compiler); GPL-3.0 code too (CommonLibF4, PyNifly,
  OpenMW, DCA). Code with no license still can't, whatever ours is.
- **Upstream PRs** stay MIT: we offer our own changes to upstream under its
  license.
- **Every release also gets:** Nexus open permissions (upload, modify,
  convert without asking; Bethesda.net requires public permission for ports),
  source tagged to match each Nexus version, and the line "if this project
  goes unmaintained, anyone is welcome to continue it".

## <a id="nexus"></a>Before each Nexus release

- Played on both game versions, with and without the common UI mods
  (DEF_UI, FallUI, Full Dialogue Interface).
- Played once start to finish on a gamepad, mouse and keyboard untouched
  ([controller support](swf_framework.md#controller)).
- FOMOD installer; requirements listed (F4SE, Address Library).
- README with the Papyrus API and an example script.
- Source link and license on the page.
- No Bethesda files shipped. Menu files must be our own (see the dialogue plan).
