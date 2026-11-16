# Cleanroom UI

Cleanroom UI is a menu framework for Fallout 4. It lets mods add their own
menus, HUD widgets, Pip-Boy lines and lists without shipping a copy of
Bethesda's interface files.

Every movie, script and DLL here was written from scratch. Nothing is
decompiled from the game or copied from another mod, so mods built on it
can share their menus freely and don't fight each other over the same
replaced file.

> **Status: private and pre-release.** Everything below is built and its
> tests pass, but most of it hasn't been played in game yet. See
> [Roadmap](#roadmap).

## What you install

Cleanroom UI comes as two separate downloads. Both need
[F4SE](https://f4se.silverlock.org/) and nothing else.

| Download | What it is | Who it's for |
|---|---|---|
| **Cleanroom UI** | `CleanroomUI.dll`, `CleanroomUI.esl`, our movies and scripts | Mod authors, and players of the mods built on it. It changes little on its own. |
| **Cleanroom Dialogue** | `CleanroomDialogue.dll` and its dialogue menu | Players who want a dialogue menu with every option listed. It works without the framework. |

## Features

### Your own menus

- **An ActionScript 3 library** (the `crui` package) for building menus
  that look and feel like the game's: `Theme` takes the player's HUD color,
  `Draw` and `Text` draw panels and labels in the game's fonts,
  `ScrollList` is a scrolling list, `Modal` a pop-up, `Images` shows game
  textures, and `Input` and `HintBar` handle mouse, keyboard and gamepad
  with the button hints along the bottom.
- **A bridge to the game.** `Bridge` connects a movie to Papyrus and to our
  DLL: it sends events to your scripts, reads your mod's INI settings
  (with MCM's saved copy taking priority), and knows whether our DLL is
  installed.
- **Your movie inside any game menu.** Drop an INI into
  `Data\F4SE\Plugins\CleanroomUI\Menus\` naming a menu and your movie, and
  the DLL loads your movie into that menu each time it opens. No replaced
  `.swf` is needed.
- **An example to copy.** [`examples/example_menu`](examples/example_menu)
  is a small, complete menu: the movie, a quest script that opens it and
  listens for its events, and an INI with a setting. Rename it and you
  have the start of your own. It's explained in
  [the API guide](docs/api.md#example).

### The HUD

Cleanroom UI loads its own HUD host into whichever HUD you use (vanilla,
DEF_HUD or any other). It doesn't replace `HUDMenu.swf`.

- **Widgets.** A mod puts its own movie on the HUD with
  `CRUI_Hud.AddWidget` and talks to it with `Send`. `AddStats` gives a
  ready-made line of labels and the player's actor values, in the HUD's
  font and color, with no movie of your own.
- **An edit menu** where players move, resize, hide and recolor every
  widget and every part of the game's own HUD, with the mouse, keyboard or
  gamepad. The layout is saved once and used in every save.
- **Layouts.** It reads DEF_HUD's saved layout (`DEF_HUD.xml`), so players
  keep theirs, and it has a preset that arranges the HUD like Fallout: New
  Vegas.
- **Show only when needed.** Each part can be set to appear only in
  combat, with a weapon drawn, in V.A.T.S., while aiming or sneaking, when
  its value changes or runs low, or when a key is pressed. Other mods can
  add conditions of their own.

### The Pip-Boy

These work under the vanilla Pip-Boy and alongside FallUI.

- **Item-card lines.** `CRUI_ItemCard.SetLine` adds a labeled line to an
  item's card in the inventory, for one item or every item with a keyword.
- **Item condition.** Weapons and armor that wear down show a CND line, in
  the Pip-Boy and in the container and barter menus.
- **Repair and ammo lists.** Pop-up lists over the item card: repair an
  item from a spare copy, or pick a weapon's ammo type.
- **Extra STAT tabs** (`CRUI_Pipboy.SetTab`) for a mod's own pages of
  values.
- **Gamepad.** The View button opens a small menu of the item actions in
  use (Repair, Ammo), since every other button is taken by the Pip-Boy.

### The level-up menu

A level-up menu of our own, opened in place of the game's. It covers skill
points, perks, traits and tag skills, lists every DLC's perks, and lets
other mods add their own skills (`AddSkill`). Its options are on an MCM
page, and a hotkey opens it.

### The dialogue menu (separate download)

`CleanroomDialogue.dll` replaces the four-button dialogue wheel with a list
that shows every option. Its options and answers come from text files in
`Data\F4SE\Plugins\CleanroomDialogue\`, one per plugin. It stays out of the
way when XDI is installed.

### For scripts

- **Item condition per stack**, repair and vendor repair, through
  `CRUI_Condition`.
- **`CRUI_Array`**, array helpers for scripts converted from xNVSE.
- **`CRUI_Sex.ChangeSex`**, which changes an actor's sex through the
  game's own code.

### Translations and gamepads

- **Every game language.** All our text is in translation files, written
  for every language the game ships. The build checks them for missing or
  unknown keys and for mismatched placeholders.
  `python -m crui translations --test-language` writes stretched, accented
  English to show where a longer translation would be cut off.
- **Gamepad support** in every menu we ship, through `Input` and
  `HintBar`.

## For mod authors

Start with **[the API guide](docs/api.md)**. It covers what your mod needs,
how to build your own menu from the example, and has a full reference of
every script function, every native a movie can call, and every library
class. The reference is generated from the sources, so it stays current.

Script names, movies and translation keys all start with **`CRUI_`**. Movies
reach the DLLs as `f4se.plugins.CleanroomUI` and
`f4se.plugins.CleanroomDialogue`.

## Building

```
python -m crui all --out dist/CleanroomUI
python -m crui check --game "<Fallout 4>/Data"
python -m pytest
```

`all` writes a mod folder ready to install. Each of its five steps also
runs on its own:

| Step | What it builds | What it needs |
|---|---|---|
| `plugin` | `CleanroomUI.esl` | Python |
| `translations` | `Interface/Translations/` | Python |
| `movies` | `Interface/*.swf` | Java and the Apache Flex SDK 4.16.1 with playerglobal 11.2. `python -m crui.flex <folder>` installs both; then set `CRUI_FLEX` to that folder. |
| `scripts` | `Scripts/*.pex` | Fallout 4's Creation Kit and F4SE's script sources. Set `CRUI_FALLOUT4` if the game isn't found. |
| `dll` | `F4SE/Plugins/*.dll` | Visual Studio, xmake 3.1 and CommonLibF4 (`CRUI_COMMONLIBF4`). Our patches to CommonLibF4 in `dll/patches/` are applied first. |

Other commands:

- `python -m crui check` builds every movie twice and checks it: the two
  builds must match, and every font it names must be one the game has
  (with `--game`, read from the game's own font list).
- `python -m crui example` builds the example menu into
  `dist/ExampleMenu`.
- `python -m crui api` regenerates the reference in `docs/api.md`.

Built files never go in git. GitHub Actions builds the plugin, translations
and movies, runs the movie check and the tests on every push
([build.yml](.github/workflows/build.yml)). Scripts and DLLs need the game
and Visual Studio, so they're built locally.

More detail on each tool: [toolchain](docs/swf_framework.md#toolchain),
[mxmlc setup](docs/swf_framework.md#mxmlc-setup),
[the DLL port](docs/spin_offs.md#port-status).

## What's in this repo

| Folder | What's in it |
|---|---|
| `as3/crui/` | The ActionScript library |
| `menus/` | Our own movies: the HUD host and its edit menu, the Pip-Boy overlay and action menu, the level-up menu, the dialogue menu |
| `papyrus/` | Our Papyrus scripts: the APIs for other mods and the level-up host |
| `dll/` | Both DLLs (`framework/` and `dialogue/`), the code they share (`base/`), and our patches to CommonLibF4 |
| `translations/` | Our text, one file per language for each download |
| `mcm/` | The level-up menu's MCM page templates |
| `examples/` | The example menu for authors (never part of our release) |
| `crui/` | The build (`python -m crui`) |
| `docs/` | The API guide and the design notes |
| `tests/` | Tests that keep the movies, scripts and DLLs in agreement, and check that builds are reproducible |

The project started inside the
[FO3/FNV to Fallout 4 converter](https://github.com/ThyMrMan/fo3fnv-in-fo4)
and was split out with its history. Anything specific to Fallout: New Vegas's
rules, such as Damage Threshold and Hardcore, stayed in the converter.

## Roadmap

The detailed list is in [the framework plan](docs/swf_framework.md#next).
In short:

1. **Play-test everything.** Almost everything built since 2026-11-12,
   including both DLLs on CommonLibF4, needs a check in game, then a check
   of Cleanroom UI alone on an unmodded Fallout 4.
2. **The converter on Cleanroom UI as a submodule.** It already runs on
   our DLLs from a pinned commit.
3. **More game versions.** Both DLLs support Steam 1.11.240, old-gen
   1.10.163 and next-gen 1.10.984 to 1.11.221. GOG waits for an Address
   Library database.
4. **What play-testing asks for**: the rest of the HUD options (fades,
   rotation, power armor), and the level-up menu's last open decision.
5. **Release**: release downloads and the Nexus page. The example menu,
   API guide and CI are done.
6. **Moving HUDFramework mods onto our HUD**, later: a guide for authors
   and a tool players run on their installed mods
   ([plan](docs/swf_framework.md#hud-migrate)). It uses nothing of
   HUDFramework's.

## License

GPL-3.0, with the modding and linking exceptions in
[EXCEPTIONS](EXCEPTIONS), so menus and mods built on Cleanroom UI may use
any license.
