# Cleanroom UI

Cleanroom UI is a menu framework for Fallout 4. With it you can:
- move, resize, hide and recolor every part of your HUD;
- get extra lines, lists and tabs in your Pip-Boy;
- use a dialogue menu that lists every option.

For mod authors, it adds menus, HUD widgets and Pip-Boy lines to your
mod without shipping a copy of Bethesda's interface files.

Every movie, script and DLL was written from scratch. Nothing is
decompiled from the game or copied from another mod. Mods built on
Cleanroom UI can share their menus freely, and they never fight over the
same replaced file.

> **Pre-release.** Not yet published. See [Roadmap](#roadmap).

## What you install

Cleanroom UI is one download with an installer (FOMOD), which works in
Mod Organizer 2, Vortex and other mod managers. It needs only
[F4SE](https://f4se.silverlock.org/).

| Option | What's in it | Pick it if |
|---|---|---|
| **Cleanroom UI** (always installed) | `CleanroomUI.dll`, `CleanroomUI.esl`, its movies and scripts | Always. On its own it changes nothing until you use it. |
| **Dialogue menu** | `CleanroomDialogue.dll` and its dialogue menu | You want every dialogue option listed instead of the four-button wheel. The installer suggests leaving it off if XDI is installed. |

## Features

### The HUD

Works with whichever HUD you use: vanilla, DEF_HUD or any other. Nothing
replaces `HUDMenu.swf`.

- **Edit your HUD.** Move, resize, hide or recolor any part of the HUD,
  with the mouse, keyboard or a gamepad. Open the editor from the MCM page
  or with its hotkey.
- **One layout per save.** Each save keeps its own layout, and you can
  back it up and restore it.
- **Keep your DEF_HUD layout.** Your saved `DEF_HUD.xml` is read as it
  is. A preset arranges the HUD like Fallout: New Vegas.
- **Show parts only when you need them.** Set any part to appear only:
  - in combat
  - with a weapon drawn
  - in V.A.T.S.
  - while aiming or sneaking
  - in power armor
  - when its value changes or runs low
  - while you hold a key

  Other mods can add their own conditions.
- **A clock** (optional): game time, real time, or both, placed and
  colored like any other part.
- **Health, AP and ammo in power armor** (optional). The game hides them
  in power armor; turn this on to keep them.

### The Pip-Boy

Works with the vanilla Pip-Boy and with FallUI.

- **Item condition.** Weapons and armor that wear down show a CND line in
  the Pip-Boy and in the container and barter menus.
- **Repair and ammo lists.** Repair an item from a spare copy, or pick a
  weapon's ammo type, from a list over the item card.
- **Extra item-card lines and STAT tabs**, added by the mods you use.
- **Gamepad.** The View button opens a small menu of item actions
  (Repair, Ammo).

### The dialogue menu (optional)

Replaces the four-button dialogue wheel with a list of every option. It
turns itself off when XDI is installed.

### Every language, mouse or gamepad

- All text is translated for every language the game ships.
- Every menu works with mouse, keyboard or gamepad, with button hints
  along the bottom.

## Compatibility

Cleanroom UI adds to the game's menus rather than replacing them, so most
UI mods work beside it. The HUD editor moves and hides the parts of
whichever HUD you use. It can't move anything another mod draws in **its
own menu** on top of the game; place those in that mod's own settings.

**HUDFramework widget mods** keep working with HUDFramework installed.
They don't appear in the HUD editor yet. Cleanroom UI has its own clock
and power armor meters, so you don't need HoloTime or Power Armor HUD for
those.

**Item condition and Pip-Boy mods:**

| Mod | With Cleanroom UI |
|---|---|
| **Equipment Condition Classic** | Works. When it's installed, Cleanroom UI's condition display and repair turn off and its own show instead, so you never get two CND bars or repair menus. Place its HUD bar in its own MCM page. |
| **Armor and Weapon Condition (AWC)** | Works. It shows condition in item names, and Cleanroom UI leaves that alone. |
| **Condition Boy** | Works. Place it in its own MCM page. To avoid two Vault Boys, hide the game's in the HUD editor. It may show over Cleanroom UI's menus. |

## For mod authors

Start with **[the API guide](docs/api.md)**. It covers what your mod
needs, how to build a menu from the example, and every script function,
movie native and library class.

What you can build:

- **Your own menus** in ActionScript 3, on the `crui` library. They match
  the game's look and the player's HUD color, and handle mouse, keyboard
  and gamepad for you. The library has lists, pop-ups, game textures and
  button hints.
- **Your movie inside any game menu.** Add an INI to
  `Data\F4SE\Plugins\CleanroomUI\Menus\` naming the menu and your movie.
- **HUD widgets** with `CRUI_Hud.AddWidget`, or a line of labels and
  actor values with `AddStats` and no movie at all. Players place them in
  the HUD editor.
- **Pip-Boy additions:** item-card lines (`CRUI_ItemCard.SetLine`) and
  STAT tabs (`CRUI_Pipboy.SetTab`).
- **Item condition per stack**, repair and vendor repair
  (`CRUI_Condition`).
- **A Fallout 3/New Vegas-style level-up menu** for mods that add
  skill-based leveling: skill points, tag skills, traits and perks, with
  every DLC's perks in one list and your own skills through `AddSkill`.
  Nothing opens it in plain Fallout 4, and the game's perk chart is left
  alone. The FO3/FNV conversion uses it.
- **Your mod's settings** in your movie, read from your INI with MCM's
  saved values first.
- **Script helpers:** `CRUI_Array` for arrays, and `CRUI_Sex.ChangeSex`.

[`examples/example_menu`](examples/example_menu) is a complete small
menu: the movie, a quest script that opens it, and an INI setting. Copy
and rename it to start your own.

Script names, movies and translation keys start with **`CRUI_`**. Movies
call the DLLs as `f4se.plugins.CleanroomUI` and
`f4se.plugins.CleanroomDialogue`.

## Building

```
python -m crui all --out dist/CleanroomUI
python -m crui check --game "<Fallout 4>/Data"
python -m pytest
```

`all` writes a mod folder ready to install. Each step also runs on its
own:

| Step | Builds | Needs |
|---|---|---|
| `plugin` | `CleanroomUI.esl` | Python |
| `translations` | `Interface/Translations/` | Python |
| `movies` | `Interface/*.swf` | Java and the Apache Flex SDK 4.16.1 with playerglobal 11.2. `python -m crui.flex <folder>` installs both; then set `CRUI_FLEX` to that folder. |
| `scripts` | `Scripts/*.pex` | Fallout 4's Creation Kit and F4SE's script sources. Set `CRUI_FALLOUT4` if the game isn't found. |
| `mcm` | `MCM/Config/CleanroomUI/` | Nothing |
| `dll` | `F4SE/Plugins/*.dll` | Visual Studio, xmake 3.1 and CommonLibF4 (`CRUI_COMMONLIBF4`). The patches in `dll/patches/` are applied first. |

Other commands:

- `python -m crui check` builds every movie twice, checks both builds
  match, and checks every font is one the game has.
- `python -m crui translations --test-language` writes stretched, accented
  English to show where a longer translation would be cut off.
- `python -m crui example` builds the example menu into `dist/ExampleMenu`.
- `python -m crui api` regenerates the reference in `docs/api.md`.

GitHub Actions builds the plugin, translations and movies and runs the
checks and tests on every push. Scripts and DLLs need the game and Visual
Studio, so build those locally. Built files never go in git.

## What's in this repo

| Folder | What's in it |
|---|---|
| `as3/crui/` | The ActionScript library |
| `menus/` | The movies: HUD host and editor, Pip-Boy overlay and action menu, level-up menu, dialogue menu |
| `papyrus/` | The Papyrus scripts |
| `dll/` | Both DLLs (`framework/`, `dialogue/`), their shared code (`base/`), and the CommonLibF4 patches |
| `translations/` | The text, one file per language for each DLL |
| `mcm/` | The MCM pages |
| `examples/` | The example menu for authors |
| `crui/` | The build |
| `docs/` | The API guide and design notes |
| `tests/` | The tests |

Cleanroom UI began inside the
[FO3/FNV to Fallout 4 converter](https://github.com/ThyMrMan/fo3fnv-in-fo4).
New Vegas's own rules, such as Damage Threshold and Hardcore, stay there.

## Roadmap

1. **Release:** the installer and a Nexus page.
2. **More game versions.** Played on Steam 1.11.240. Also built for
   old-gen 1.10.163 and next-gen 1.10.984 to 1.11.221, not yet played
   there. GOG needs an Address Library first.
3. **More testing beside other mods.**
4. **More HUD options:** fades, rotation, and separate places for parts
   in power armor.
5. **HUDFramework widgets in the HUD editor:** a guide for authors, a
   tool that moves your installed widget mods onto Cleanroom UI's HUD,
   and installer patches for the most popular ones.
6. **Other mods' own menus in the HUD editor,** such as Condition Boy and
   Equipment Condition Classic's bar.

## License

GPL-3.0, with the modding and linking exceptions in
[EXCEPTIONS](EXCEPTIONS). Menus and mods built on Cleanroom UI may use any
license.
