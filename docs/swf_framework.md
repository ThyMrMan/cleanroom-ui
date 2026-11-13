# Our own Flash menu framework

A plan for one shared Scaleform (Flash) toolkit, containing nothing of
Bethesda's, used by three things: the
[dialogue menu](dialogue_menu_mod.md), the [casino](fo4_casino_plan.md)
menus and a Pip-Boy framework. Part of the [spin-off plan](spin_offs.md).
Status lives in the [roadmap](ROADMAP.md).
**V** = read in source or code, **P** = played in game, **S** = from search
results only.

**Names:** written for the FO3/FNV converter, so this plan says `f3ui`, `F3NV_LevelUp*`, `F3NV_Array`
and `$F3NV_LU_*`. In Cleanroom UI those are `crui`, `CRUI_LevelUp*`, `CRUI_Array` and `$CRUI_*`
(renamed 2026-11-12); the DLL is now `CleanroomUI.dll` (natives on `CRUI_Condition` and the rest,
`f4se.plugins.CleanroomUI`, `CRUI_Pipboy.swf`) and `CleanroomDialogue.dll`
([split](spin_offs.md#split-status)). Our translation files are
`CleanroomUI_<language>.txt` and `CleanroomDialogue_<language>.txt`, from
`translations/<release>/<language>.txt` (`crui/translations.py`).

## <a id="why"></a>Why

Each of our three menus solved the same problems on its own: finding the
F4SE object, mapping keys, making text in the game font, drawing panels,
lists and pop-ups, talking to the DLL. The Pip-Boy work draws everything
from C++ through Scaleform calls, which works but fights the game every
frame. One toolkit gives us one tested copy of each piece. Released on its
own, it gives other FO4 mod authors something they don't have: an open,
licensed way to build menus without starting from decompiled Bethesda files.

## <a id="have"></a>What we have

| Menu | How it's made | State |
|---|---|---|
| Casino (`fo4_import/casino/`) | Our own AS3 on the `f3ui` library, compiled by `mxmlc` (since 2026-11-11, [spike 1](#spikes)). Opened as an F4SE custom menu from Papyrus (`F3NV_Casino.psc`) | FFDec build played (slots in the HUD color). `mxmlc` build played: slots and roulette work |
| Dialogue menu (`fo4_import/dialogue_menu/`) | Our own AS3 only (since 2026-11-11, [spike 3](#spikes)). Since 2026-11-12 on `f3ui` (`Bridge`, `Text`), compiled by `mxmlc` like the casino's. Nothing read from the game | The patched-movie version played; our own movie (FFDec build) played 2026-11-12, looks the same. `mxmlc` build not played |
| Pip-Boy, container, barter, HUD (`tes_runtime/fo4_condition/`) | The Pip-Boy's CND line, repair and ammo lists, and other mods' item-card lines are drawn by our injected movie from the DLL's data ([framework](#pipboy)). The rest is drawn by the DLL into the open vanilla menus from C++: REPAIR and AMMO hints, STAT tabs, the container and barter CND line, HUD needs | CND, repair and tabs played from C++; the movie's versions built, not played. Ammo list and HUD needs built, not played |
| Level-up (`fo4_import/level_up/`, `fo4_import/spike/level_up*.py`, `F3NV_LevelUp.psc`) | Our own menu on f3ui ([level-up](#level-up)), opened by the character rules in place of `LevelUpMenu`, with skill, perk and taken pages, and as the tag-skill and trait menus, every DLC's perks and traits included; PRKF.dll's points, tags, perk list and requirement rows rebuilt in Papyrus | Built 2026-11-12, none played |

### <a id="where"></a>Where each piece stands (2026-11-12)

| Piece | Done | Not done |
|---|---|---|
| **Toolchain** | `mxmlc` builds, byte-stable, every movie we ship; FFDec route deleted | The [movie check](#testing) in preflight (sources only, no Bethesda tags, mapped fonts) |
| **AS3 library** ([design](#design)) | `Bridge`, `Theme`, `Host`, `Text` (with `Text.show` for `$KEY`s and slots), `Draw`; `Input` and `HintBar` ([input](#input)), `ScrollList` ([scroll list](#scroll-list)) and `Modal` ([modal](#modal)), with the dialogue list and the Pip-Boy's list panel on them; `Images` ([images](#images)) with the casino on it (all built, not played) | Every planned class is built; next, playing them |
| **Own menu** | Casino (Papyrus `UI.RegisterCustomMenu`), HUD color by the menu itself (spike 2b, played) | Registering from the DLL: not needed for color; only if a later menu needs it |
| **Replace a vanilla movie** | Dialogue menu on our own movie (spike 3, played); its `mxmlc` build. Level-up: our own registered menu opened in place of `LevelUpMenu` from Papyrus, no vanilla file shipped ([open](fo4_level_up_plan.md#open), built) | Both played; if the moment of `LevelUpMenu` first shows, F3NV.dll swapping its constructor |
| **Inject into a vanilla menu** | Pip-Boy (spike 4, played under vanilla); CND, lists and item-card API built | Any of it played; FallUI played; container and barter (still C++) |
| **HUD widget** | `CRUI_Hud`: HUDFramework hosts the widgets when installed, else our host movie loaded into any HUD by `CleanroomUI.dll`; widget movies with HUDFramework's `processMessage`; the built-in stats line ([host](#hud-built)); our host's saved places and edit menu ([edit](#hud-edit)); DEF_HUD.xml layouts of the game's parts ([layouts](#hud-layout)), moved and scaled in the edit menu too; the [FNV preset](#fnv-preset) with its [CND bar](#fnv-widgets); [hiding and recoloring](#hud-look) in the edit menu, with a [gamepad](#hud-edit) too; parts shown on [conditions](#hud-conditions) (steps 1 to 3: combat, weapon drawn, V.A.T.S., a show key, a value changing or low, the location moving, aiming, sneaking; the Immersive preset; each box's rule set in the edit menu; other mods' conditions and rules); built, not played | Played; fades if play asks, `flags`; the converter's Hardcore needs on it (still C++) |
| **DLL side** | GFx helpers (`gfx.cpp`), Pip-Boy injection (`LoadOverlay`), natives the movies call (`HudColor`, `Log`, `PipboyCardLines`, `PipboyList`, `TextureExists`) | Injection by menu URL for other menus, settings INI to the movie, GFx log routing |
| **Translations** ([translations](#translations)) | Files named after `FalloutHelpers.esl`, every language with English fallback, our fixed words as `$F3NV_` keys, the key check in preflight (spike 5, played) | The keys played; Papyrus key check; slot and per-language counts; a test language |
| **Controller** ([controller](#controller)) | The casino and dialogue menu on `Input` and `HintBar` (built, not played) | Playing them with a pad; the Pip-Boy "more" button; roulette's board on a pad |
| **Release** ([release](#release)) | Nothing | The library repo, the Pip-Boy framework on Nexus |

Shared already:
- `fo4_import/base/as3/f3ui/`: the AS3 library's first five classes,
  `Bridge`, `Theme`, `Host`, `Text` and `Draw` ([design](#design)), used by
  the casino, the dialogue menu and the Pip-Boy movie.
- `fo4_import/base/mxmlc.py`: the `mxmlc` build with the library on the
  source path, dated tags dropped, cached by the hash of the sources, the
  library and the options. Every movie we ship is built this way.
- The FFDec build route (a skeleton movie, then FFDec's script import:
  `swf_skeleton.py`, `ffdec.py`) was deleted on 2026-11-12, when the
  dialogue menu moved to `mxmlc`. FFDec stays a tool for reading vanilla
  movies.
- `asset_convert/ui/swf.py`: our SWF reader and writer.

Both builds are byte-stable (`tests/test_fo4_casino.py`,
`tests/test_fo4_dialogue_menu.py`).

## <a id="facts"></a>What we know about FO4's Flash

### From our own play-tests (P)

- `$MAIN_Font` and `$MAIN_Font_Bold` work by name with `embedFonts`; no font
  library needs importing. The game maps the names per language.
- `f4se.MountImage(menu, path, name)` shows a DDS as `img://name`. The path
  is under `Textures` without that prefix. It returns true even for a
  missing file, and images drop when the menu closes.
- F4SE's default custom-menu flags (`0x801849D`) made the casino draw only
  in a box around the cursor. Dropping Custom Rendering (`0x8000`) and
  Assign Cursor To Renderer (`0x10000`) fixed that **and lost the HUD
  color**. A changed registration needs a game restart.
- HUD color: the dialogue menu's holders registered through
  `BGSCodeObj.registerObjects` are tinted by the engine. Anything drawn
  white shows in the HUD color. Inside a tinted holder alpha only darkens,
  so no translucent fills: the backdrop is the engine's shaded background.
- `UI.Invoke` wraps array elements as `{__var__: v}` (casino `Ui.plain`
  unwraps). An AS3 `int` reaches Papyrus as Int.
- `ProcessUserEvent` delivers `Accept`, `Cancel`, `Left`, `Right`; the
  stage's `KEY_DOWN` works too. The mouse wheel comes as `ZoomIn`/`ZoomOut`.
- Pip-Boy: the menu object is `root.Menu_mc`; pages load into the menu's
  class domain; the game refills the item card's `InfoObj` on every
  selection change; Papyrus `OnKeyDown` never fires while it's open; no
  gamepad button is free; a STAT tab must exist before the header reads the
  names. Details in [fo4_condition.md](fo4_condition.md#pipboy) and
  [fo4_pipboy_tabs.md](fo4_pipboy_tabs.md).
- Making a TextField in the dialogue movie's constructor crashed the game.
  Build display objects on the first update call instead.
- **Focus rectangle:** a movie that gives its root the keyboard focus
  (`stage.focus = this`) gets Flash's yellow focus frame round the whole
  screen. Set `stage.stageFocusRect = false` and `focusRect = false`, as
  FO4's own `Shared.IMenu` does (casino, `fd59ed6f`, built, not played).
- **The HUD stays drawn behind a custom menu.** No menu flag hides it, and
  changing the casino's flags stopped the menu opening (spike 2a). The
  casino hides it with F4SE `UI.Set("HUDMenu", "root1.visible", False)` on
  open, shows it on close, and shows it again on every load so a crash
  can't leave it hidden (`F3NV_Casino.psc` `ShowHud`, `fd59ed6f`, built,
  not played). This also hides HUD widgets (HUDFramework, ours) while the
  menu is up, which is what we want. In the library this moves into the DLL
  side of `Host`, so every menu gets it.

### From F4SE, CommonLibF4 and other mods (V)

- **F4SE's root object:** every movie gets `root.f4se` (`SendExternalEvent`,
  `CallFunctionNoWait`, `GetDirectoryListing`, `MountImage`, `AllowTextInput`,
  `version`, `plugins.<name>`). It calls `onF4SEObjCreated(f4se)` on root and
  its direct children, then dispatches `F4SE::Initialized`. It can be missing
  on early frames: poll for it.
- **HUD color for custom menus:** extended flag 1 (InheritColors) applies the
  HUD color to the menu's filter holder, **and only with Custom Rendering on**
  does it reach the shader targets. That explains the casino's lost color.
  A clip that defines `onApplyColorChange(r, g, b, mult)` can override it.
  CommonLibF4's `BSGFxShaderFXTarget` has `SetToHUDColor` and
  `EnableShadedBackground`, which a DLL can call on any clip.
- **Menu flags and depths:** the full lists are in CommonLibF4
  (`UI_MENU_FLAGS.hpp`, 28 flags; `UI_DEPTH_PRIORITY`, 21 levels).
  `AllowSaving` (`0x800`) is in F4SE's default; without it the save button
  is grayed out.
- **Translations:** `Interface\Translations\<name>_<language>.txt`, UTF-16 LE
  with BOM, `$KEY<tab>text` lines; `$KEY` strings in text fields are swapped
  by the game.
- **Supported AS3** (Autodesk's table): Loader, Bitmap, BitmapData, Vector,
  XML, Error and `getDefinitionByName` work. Not supported: convolution and
  shader filters, `drawTriangles`, shader fills, gradient masks, most of
  `flash.net`. Blur, glow and drop shadow work on shapes and text with limits.
  Our own "no try/catch, no uint" rule has no recorded crash behind it; test
  it once, then keep or drop it.
- **`scaleform.gfx.*`** (Extensions, TextFieldEx, FocusManager…) are filled
  in by the game at runtime. Autodesk ships them as source, so stub classes
  with the same names compile; untested in FO4.
- **Injecting instead of replacing.** PipboyTabs and Extra Item Info (both
  unlicensed, read only) and F4-PIP-OS (MIT) add to the Pip-Boy without
  shipping `PipboyMenu.swf`: the DLL spots the movie by
  `root.loaderInfo.url`, loads its own small movie into `root.Menu_mc` with a
  `Loader`, and adds a native API object on root. F4SE's `UI.Load` does the
  same from Papyrus.
- **Pip-Boy movie layout:** `PipboyMenu.swf` is a shell that loads
  `Pipboy_<Stats|Inv|Data|Map|Radio>Page.swf` by name into one class domain
  (from F4-PIP-OS's notes). FallUI Inventory, DEF_INV, Vault Girl and others
  each replace the shell or pages, so only one wins. FallUI keeps the vanilla
  classes, which is why injected mods still work under it.
- **Holotape programs** run an SWF from `Data\Programs` on the Pip-Boy
  screen (826×700, no cursor) (S).

### <a id="hudframework"></a>HUDFramework (V)

The HUD's framework: 2.1M unique downloads, 362 mods using it, updated
2026-09-04, works on next-gen. Source 1.0f and its Nexus page are in
`references/HUDFramework Source Code 1.0f/` (no license file: read only).

- **Ships a patched `HUDMenu.swf`:** Bethesda's HUD movie with its own
  movie injected by an auto-patcher (RABCDASM). It needs no F4SE, so it runs
  on consoles. The cost: any other `HUDMenu.swf` replacer must be
  auto-patched, and the shipped file is Bethesda's movie.
- **Papyrus to Flash without F4SE:** commands are written as
  `| widget | command | body |` strings, joined with `^`, throttled, and
  sent as `Debug.Notification` text that the patched HUD intercepts
  (`HUDFramework.psc` `SendCommandRaw`).
- **Widgets** are SWFs loaded with a `Loader` into the HUD's own class
  domain, parented under the compass bar so they hide and fade with the HUD.
  A widget implements one method, `processMessage(command, params)`.
- **Framework duties it handles well:** register before load; reload every
  widget on each game load (the HUD movie starts over); clear registrations
  whose owner script is gone (an uninstalled mod); saved position, scale and
  opacity per widget, with a drag-to-edit mode; a power armor layout; an AS3
  expression evaluator so Papyrus can change any HUD element (`Eval`).

## <a id="toolchain"></a>Toolchain

Two working routes. Both produce a movie made only of our code.

| | FFDec over a skeleton (ours today) | Apache Flex `mxmlc` |
|---|---|---|
| Proven | Casino, played | fo4-silhouette (GPL-3), F4SEMenuFramework, BetterWeaponScrapping (MIT) |
| Needs | Java 8, FFDec (GPL-3, a tool, not shipped) | Java, Flex SDK 4.16.1 (Apache-2.0), Adobe's `playerglobal.swc` (Adobe license: can't go in our repo; build-time only) |
| Byte-stable | Yes, tested | No: writes a build date (tags 41 and 77). Fixable by stripping them after the build |
| Errors | Lenient: `var x:int = ;` compiles | A real compiler: type errors, missing members |
| New classes | Each needs a skeleton entry (the skeleton does it per `.as` file) | Normal |

**Decision: build with `mxmlc`, strip tags 41 and 77, and keep FFDec only
for reading vanilla movies.** A menu framework that other people build on
needs a real compiler; a lenient one hides bugs until play. Proof first
(spike 1): the casino spike built by `mxmlc` behaves the same in game and
builds byte-identical twice after stripping.

### <a id="mxmlc-setup"></a>`mxmlc` setup (done 2026-11-11, outside the repo)

- **Where:** `C:\Coding\tes4skyrim\tools_ext\FlexSDK` (beside FFDec).
  Downloads kept in `tools_ext\downloads\`.
- **Flex SDK 4.16.1:** `apache-flex-sdk-4.16.1-bin.zip` (72,396,756 bytes)
  from `archive.apache.org/dist/flex/4.16.1/binaries/`; MD5 matches
  Apache's published `.md5` (`8841c64bd5e32f8575eba86e2574873a`).
- **`playerglobal.swc` 11.2:** Adobe's download address now redirects to
  its Flash end-of-life page. Taken from the Internet Archive's copy of
  Adobe's own file
  (`web.archive.org/web/20160611061401id_/http://fpdownload.macromedia.com/get/flashplayer/updaters/11/playerglobal11_2.swc`,
  339,135 bytes). Its SHA-1 (base32 `3QHT2FXRFVNCFA2JX2XHADPEYP3LUCBZ`)
  matches the archive's record for both captures, 2014 and 2016. Installed
  as `frameworks\libs\player\11.2\playerglobal.swc`; `{playerglobalHome}`
  in `frameworks\flex-config.xml` replaced with `libs/player`.
- **Command** (run from the SDK folder; in PowerShell quote every
  argument):
  `java -Xmx1024m -jar lib\mxmlc.jar +flexlib=frameworks -target-player=11.2 -swf-version=15 -static-link-runtime-shared-libraries=true -debug=false -default-size 1280 720 -default-frame-rate 30 -source-path=<src> -output=<out.swf> <src>\<Main>.as`.
  Without `-default-size` and `-default-frame-rate` the movie gets a
  500×375 stage at 24 fps; our skeleton uses 1280×720 at 30.
- **Checked offline:**
  - A small test movie built twice: 696 and 698 bytes, different. With
    tags 41 and 77 removed (a scratch script that re-packs the movie with
    zlib level 9), both builds are **byte-identical**. Tag order: 69, 77,
    65, 9, 41, 43, 82, 76, 1, 0. No Flex framework code is linked in.
  - The casino's AS3 (`fo4_import/casino/as3`) compiles cleanly: 6,070
    bytes, no errors or warnings.
- **In the build** (`fo4_import/base/mxmlc.py`, 2026-11-11): the command
  above with `-default-background-color=0x000000` (the skeleton's black),
  `+flexlib` given as an absolute path so it runs from any folder, and
  `-source-path+=` for the library. Any `Warning:` or `Error:` in the
  output fails the build. Tags 41 and 77 are dropped, FileAttributes'
  HasMetadata bit (`0x10`) cleared, and the movie written **uncompressed**
  (`FWS`). Compressing with zlib is not byte-stable across Python builds:
  Python 3.14 on Windows ships zlib-ng (`zlib.ZLIB_RUNTIME_VERSION`
  `1.3.1.zlib-ng`), Python 3.13 ships stock zlib 1.3.1, and at level 9 the
  same casino movie came out at 11,323 and 11,305 bytes, with every tag
  identical. Uncompressed, the bytes depend only on the sources. (Since
  2026-11-12 the FO4 build has a [pinned zlib](fo4_pipeline.md#pinned-zlib)
  that would make a compressed movie stable too; the movies stay
  uncompressed, the format that has been played.) The SDK is found at `F3NV_FLEX`, else `tools_ext\FlexSDK` beside
  the repo (in a worktree, set `F3NV_FLEX`).
- **The casino moved to `mxmlc` and the library in one change** (spike 1).
  The dialogue menu stayed on FFDec until its own movie was played, so
  only one thing changed per play-test.
- **The dialogue menu moved to `mxmlc` and the library on 2026-11-12**
  (built, not played). Its AS3 compiled cleanly under `mxmlc` unchanged.
  Then: rows made by `Text.field` (bold 18, leading -2, white: the engine
  tints them) and filled through `Text.show`, F3NV.dll's natives through
  `Bridge`. Behavior is unchanged. Movie 9,585 bytes, the same on Python
  3.13 and 3.14. **Play check:** dialogue opens and looks as before; the
  options read in the HUD color, pick and leave as before.

**Ship built movies, don't build at install.** Today the converter runs
FFDec on the player's machine. With our own movies there is nothing of the
player's to compile against, so the release carries the `.swf` files and
players need no Java. This needs a rule change: the pre-push hook blocks
every `.swf` (it exists to keep Bethesda art out). Options:
- **Release assets only** (recommended): built movies go into the release
  package by CI, never into git. The hook stays as it is.
- An allowed folder (`interface_build/`) the hook exempts, with a check that
  each movie in it was built from our source.

## <a id="design"></a>Design

### The AS3 library

One package (name to pick; `f3ui` below), GPL-3.0 with the modding
exception ([license](spin_offs.md#license)), no Bethesda classes:

| Module | Does | Replaces |
|---|---|---|
| `Bridge` | Finds `root.f4se` (event or poll), our plugin's natives, `send`/`call`/`log`, unwraps `{__var__}` | Each menu's own copy |
| `Input` | One map from keys and `ProcessUserEvent` names to actions (up, down, accept, back, page, scroll); ignores the opening key for a moment; platform and gamepad from `SetPlatform`/`uiPlatform`, followed as it changes; text input on and off ([controller support](#controller)) | Two separate key tables |
| `Text` | Text fields and labels in `$MAIN_Font*`, sizes, wrap, `$KEY` translation, HTML-safe text | `Ui.as` and the dialogue menu's own |
| `Draw` | Panels, frames, dividers, arrows, white-for-HUD-color drawing, shaded backdrop request | `Ui.panel`, the dialogue `frame` |
| `ScrollList` | The selectable, scrolling list: rows, markers, icons, disabled and said rows, wheel, numbers 1–9 | The dialogue list, and the C++ repair and ammo lists |
| `HintBar` | Button hints that show keys or gamepad glyphs (`$Controller_Buttons` font) | Bethesda's `BSButtonHintBar` |
| `Modal` | A pop-up over a menu: fade, focus, back closes it | The Pip-Boy repair pop-up in C++ |
| `Images` | `MountImage` with the path rule built in; asks the DLL whether a file exists first | Casino's own |
| `Theme` | The HUD color from the DLL's `HudColor()` native on each open ([spike 2b](#spikes), played), colors over it, sizes, spacing, read from settings the DLL passes in | Hardcoded values |
| `Host` | Safe-rect; menu open/close reset (F4SE keeps a movie alive between openings); focus rectangle off; HUD hidden while open and restored on close and on load ([casino fixes](#facts)) | Each menu's own |

Each menu is then a thin document class on top: `CasinoMenu`,
`DialogueMenu`, and the Pip-Boy overlay.

### <a id="scroll-list"></a>`ScrollList` (built 2026-11-12, not played)

`fo4_import/base/as3/f3ui/ScrollList.as`: the selectable, scrolling list.
Items are `{text, value, dim}`. It shows a window of rows over them, marks
the selected one, and adds scroll marks when there are more. It takes
`Input`'s actions (up and down move round the ends, accept and a digit
pick) and the mouse (over selects, a click picks), and calls `onPick` and,
after each draw, `onDraw`. Options set the look:
- rows shown, width, font, size and line spacing;
- wrapping rows (each as tall as its text) or fixed-height lines with a
  value column on the right;
- the marker: a translucent bar or an outline;
- the scroll marks' column (or none), upper case, and the alphas of the
  selected, other and dim rows;
- centered scrolling (the selection stays mid-window) or minimal (the
  window moves only as far as it must);
- `layerOf(item)`: a row and its marker drawn in another layer.

A row is a child only while it shows, since a hidden row still counts in a
parent's bounds (the dialogue's backdrop is sized by them). Fields are
remade when `Theme.color` or the width changes, since a field keeps the
color it was made with.

**Its users:**
- **The dialogue list** (`DialogueMenu.as`): wrapping bold rows, the
  selected one outlined, said ones dim, scroll marks in the box's left
  column, every number as before. Failed skill checks go in the failed
  layer through `layerOf`. The menu keeps the box's outline
  (`Draw.outline`), its place, the backdrop and the overlay, redone in
  `onDraw` because scrolling changes the box's height.
- **The Pip-Boy's repair and ammo lists** (`ListPanel.as`): fixed 30-pixel
  lines with values, the translucent bar, the window centered on the
  highlighted row, no input (F3NV.dll owns the keys).

**`removeChildren()` is gone:** the old `ListPanel` used it, but it is
Flash Player 11's and Scaleform may lack it. Our AS3 now empties a
container a child at a time, and `tests/test_fo4_f3ui_input.py` keeps it
out.

### <a id="modal"></a>`Modal` (built 2026-11-12, not played)

`Modal.as`: a pop-up over a menu. It is a panel outlined in `Theme.color`
and filled black (`Draw.box`, shared with `Draw.panel`) round its content,
with an optional fade over the screen behind it. Back closes it
(`onClose`), and its other actions go to the content's `handler`. The
Pip-Boy's list panel is its first user: the heading, a `ScrollList`, the
notes and the keys, in the same size and place as before. Its fade and
Back aren't used there, since the DLL owns the keys.

**Play check:**
- **Dialogue menu:** the same as before. Rows wrap, the selected one is
  outlined, said ones are dim, the marks show past five options, failed
  checks are tinted, the mouse and number keys pick, and the backdrop fits.
- **Pip-Boy:** the repair and ammo lists draw as before.

### <a id="images"></a>`Images` (built 2026-11-12, not played)

`Images.as`: textures shown in a menu through `f4se.MountImage`.
- **The path rule:** `texturePath` puts any form of a texture's path the
  way MountImage takes it: forward slashes, with no leading `Data/` or
  `Textures/` (any case) and spaces kept. MountImage takes the path under
  Textures without that folder (played 2026-11-08).
- **Missing files:** MountImage reports true for a file that isn't there
  (the game's loader hands back a stand-in), so `mount` first asks
  F3NV.dll's new `TextureExists(path)` native. It is in `gfx.cpp` and
  registered on every movie with the other F3NV natives. It opens
  `Textures\<path>` through the game's file system, loose files or an
  archive, with `BSResourceNiBinaryStream`, the same call F4SE makes for
  our translation files (played in spike 5). A missing texture is logged
  and `mount` returns "". Without F3NV.dll, or with one older than this, it
  mounts as before.
- **Texture archives (P, 2026-11-12):** that stream can't open a file in a
  texture (DX10) BA2: the level-up menu logged every skill and perk icon
  as missing, all of them in `FalloutNV - Textures.ba2`. So
  `TextureExists` also reads the name table of every `.ba2` in Data once
  (the BTDX header's file count and name table offset, as
  `fo4_import/base/ba2.py` reads them) and answers from it. An archive in
  Data that the game doesn't load still counts (built, not played).
- **Showing:** `show(parent, url, x, y, size, done)` gives a Loader scaled
  to `size` square once loaded, then `done(loader, loaded)`. A failed load
  is logged. An empty url leaves the Loader empty, so a menu's layout
  needs no special case.

**The casino on it:** `MountAll` returns the mounted images' URLs ("" for
a missing one), the slot symbols show through `Images.show`, and the spike
screen labels a missing texture and still logs each image's size. Next
user: the level-up menu's perk and skill images.

**Checked offline:** `gfx.cpp` and `dialogue.cpp` compile with the DLL's
flags; the DLL itself is the main session's build. **Play check:** the
casino's spike screen (`cqf FNV_Casino Spike`) with the new F3NV.dll:
every image shows, and none is labeled missing. A path changed by hand to
one that doesn't exist shows as missing, with `missing texture` in the
log, instead of an empty mount.

### Four ways a movie meets the game

| Mode | Used by | How |
|---|---|---|
| **Own menu** | Casino, settings screens | Registered by our DLL through CommonLibF4 (`GameMenuBase`), not Papyrus, so flags, depth and HUD color are set in one place and need no restart |
| **Replace a vanilla movie** | Dialogue menu | Our movie at `Interface\DialogueMenu.swf`, implementing the engine's calls ([contract](dialogue_menu_mod.md#swf)). Only where the engine owns a menu we must take over completely |
| **Inject into a vanilla menu** | Pip-Boy, container, barter | The DLL loads our small movie into the open menu with a `Loader` and gives it a native API object. No vanilla file is shipped, so DEF_UI, FallUI and the rest keep working |
| **HUD widget** | HUD elements (our Hardcore needs) | Hosted by HUDFramework when installed, else injected by our DLL ([how](#hud)) |
| **Holotape program** | Possibly casino games, later | An SWF in `Data\Programs` played on the Pip-Boy screen; the same library, with a native bridge. Research only |

### The DLL side

One shared C++ module, used by every DLL built on the
[shared base](spin_offs.md#runtime):
- menu registration (flags, depth, HUD color, shaded background);
- injection: watch for a menu's movie by URL, load our movie into it, add
  our API object;
- native function registration (one helper, not one class per function);
- GFx helpers: **done in F3NV.dll 2026-11-12**, `gfx.cpp` (`Num`, `Get`,
  `LogMembers`, `FindMenu` for the menu-instance path probe, and the
  `Listen<Handler>` template for native event listeners), used by
  `pipboy.cpp`, `transfer.cpp` and `hardcore.cpp`;
- settings: read the mod's INI and hand it to the movie as one object;
- `bEnableGFXLog` routed into our own log when debugging.

## <a id="pipboy"></a>The Pip-Boy framework

### What moves from C++ into an injected movie

The game refills the item card on each selection, so a line has to be put
back after every refill. An injected movie can instead listen to the item
card's own redraw and add its lines in the same frame, and draw lists and
pop-ups with `ScrollList` and `Modal` instead of faking them as item-card
lines. Key handling moves into the movie, which already gets keys when
Papyrus doesn't.

Phases:
1. **The CND line** (built 2026-11-12, not played). F3NV.dll still resolves
   the highlighted stack and its health each frame, but no longer touches
   the card. Its native `PipboyCardLines()` gives the lines to show
   (`[{text, value, showAsPercent}]`, the CND line of an item that wears;
   null while a list of ours draws the card), and `F3NV_Pipboy.swf` keeps
   them in `CurrentPage.ItemCard_mc.InfoObj`, matched by text, calling
   `onDataChange` only when it adds a line or a value changes. The spike-4
   test line is gone.
   - **FallUI Inventory (2.2.1), read 2026-11-12, not played.** It ships
     its own `PipboyMenu.swf` and `Pipboy_InvPage.swf` (read with FFDec,
     unlicensed, read only). Everything we use is still there:
     `CurrentPage`, `ItemCard_mc`, `InfoObj` (FallUI sometimes assigns a new
     array, which the movie re-reads each frame), `onDataChange`,
     `ModalFadeRect_mc`, `List_mc.disableInput`. Its card draws plain
     `{text, value}` lines; `ItemCardFwCore.preprocessData` only copies the
     array and runs FallUI's own add-on hooks. Three differences:
     - **Color:** FallUI ships `M8rDisablePipboyEffects` (no game screen
       tint) and colors its art from `M8r.Mods.config.stdColor` (green by
       default). The list panel takes that color from the Pip-Boy's classes,
       as FallUI's own add-on API does (`PipboyApi.primaryColor`), else
       white.
     - **Typing:** its list search (F) is a text box, and our `keyUp`
       listener would hear G and V typed into it. Keys whose target is an
       input text field are now ignored.
     - FallUI has its own item-card add-on framework (SWFs in
       `Interface\UI-Framework\ItemCard`). Ours stays independent so it
       works under vanilla too; under FallUI both sets of lines show.
2. **The repair and ammo lists** (built 2026-11-12, not played). The DLL
   keeps each list's state, its keys (the native `keyUp` listener, which
   already works) and the repair, but no longer writes the list over the
   card's `InfoObj` or lifts the card. Its native `PipboyList()` gives the
   open list as `{heading, rows, chosen, notes, keys}` (`RepairView`,
   `OfferView`, `AmmoView`), and the movie's `ListPanel` draws it as a
   pop-up over the item card: a black panel outlined in white, the heading,
   eight rows round the highlighted one (a translucent bar), the notes and
   the keys. It is white, like the Pip-Boy's own art, because the game tints
   the whole screen. While a list is open the movie moves our `Loader` to
   the top of the menu, over the page and its fade. "Not needed", "No spare",
   "One type only" and "Drop a copy first" are card lines from
   `PipboyCardLines`, kept until the selection changes. The DLL opens a list
   only after the movie has called it in this Pip-Boy opening, so a missing
   movie never leaves an invisible list holding the keys (logged once).
   - The REPAIR and AMMO hints stay in C++ (`MakeHint`), which works in play.
3. **The item-card line API for other mods** (built 2026-11-12, not
   played): [below](#card-lines).

### <a id="card-lines"></a>Item-card lines for other mods

`F3NV_ItemCard.SetLine(Form akItem, String asLabel, String asValue)`
(Papyrus, global native; F3NV.dll `card.cpp`) shows a line on the inventory
item card of `akItem`, or of every item carrying it when it is a Keyword. A
second call with the same item and label replaces the line; an empty value
removes it. Our movie draws these lines after F3NV's own, under any Pip-Boy
that keeps the vanilla item card.
- Lines live in F3NV.dll's memory and are forgotten when a save loads or a
  new game starts (F4SE's `PreLoadGame` and `NewGame` messages), so a mod
  sets them again on each game load (`OnPlayerLoadGame`), as HUD widgets
  re-register. Nothing goes in the save, so an uninstalled mod leaves
  nothing behind.
- Values are text, shown as given. A label or value may be a mod's own
  `$KEY` from its own translation file; the card translates it as it does
  ours ([keys](#keys)).
- Test from the console:
  `cgf "F3NV_ItemCard.SetLine" <item form ID> "TEST" "HELLO"`.
- Not yet: lines from an INI, numbers with `showAsPercent`, and per-copy
  values (a line belongs to a form, not to one stack).

### What it offers other mods

From the Pip-Boy survey (S), ranked by what nothing provides today:

1. **Item-card fields.** Any mod registers a line (label, value, a filter by
   form type or keyword), from Papyrus or an INI. Our CND line becomes its
   first user. Nothing like it exists without FallUI.
2. **Pages and tabs with real content.** Not only lists of values
   (PipboyTabs does that) but a mod's own movie: buttons, lists, actions.
   Our STAT tabs move onto it.
3. **Item actions:** a key and hint on the item card that runs a mod's
   action (our Repair and Ammo are the first two), and on a gamepad one
   button that lists them all ([controller support](#controller)).
4. **Map layers and markers**, drawn over whichever map page won. Later.
5. **Radio page extensions.** Later; our radio work may want it.

### Living with other Pip-Boy mods

- Ship no `PipboyMenu.swf` or page movie, ever.
- Find the parts we touch by what they are (the item card's `InfoObj`, the
  STAT page's tab list), not by vanilla paths, and log once when a shell
  hides them. Test under vanilla, FallUI Inventory and DEF_INV.
- **PipboyTabs:** ours duplicated its Skills tab. Detect it and skip our
  version of a tab it already shows. Reading its `Data\PBT\*.ini` format
  for compatibility is possible later; its code is unlicensed, so read only.
- Power armor and VR use other Pip-Boy screens: check power armor; leave VR
  out of the first release.

### What we take from HUDFramework's design

The Pip-Boy framework is HUDFramework's job for a different menu, so it
should do the same housekeeping ([HUDFramework](#hudframework)):
- registrations kept in the save and re-applied each time the Pip-Boy
  movie loads;
- registrations from uninstalled mods cleared on load;
- one message channel with batching, and one method a mod's movie
  implements;
- power armor's Pip-Boy handled as its own layout.

It differs on purpose in one place: it injects through F4SE instead of
shipping a patched Bethesda movie. That keeps it clear of every Pip-Boy
replacer and of Bethesda's files, at the price of PC only.

## <a id="hud"></a>The HUD: work with HUDFramework, never require it

HUDFramework already solves HUD widgets, is installed by millions, and
keeps up with game versions. A second HUD framework would split authors for
little gain. But it has no license, so we can't ship, fork or fix it, and
our mods must not break when it's missing or changes.

**One widget, two hosts.** Our HUD pieces (first: Hardcore's H2O / FOD / SLP,
today drawn into `LeftMeters_mc` by `hardcore.cpp`) are one movie built with
our library. It works under either host, chosen once per game session:

| HUDFramework | Host | What the player gets |
|---|---|---|
| Installed | HUDFramework loads our movie as a widget; our script registers it | Its saved position, scale and opacity and its drag-to-edit mode, alongside their other widgets |
| Not installed | Our DLL injects the same movie into whatever `HUDMenu.swf` is loaded (vanilla, DEF_HUD, anything), as with the Pip-Boy | The widget at its default place, settings from our INI |

How, without depending on it:
- **No compile dependency.** Our Papyrus finds its quest with
  `Game.GetFormFromFile` on `HUDFramework.esm`, then calls it through
  `CastAs("HUDFramework")` and `CallFunction` (vanilla Papyrus), so our
  scripts compile and run without its `.psc` or `.esm`.
- **The movie speaks both ways:** it implements `processMessage(command,
  params)` for HUDFramework, and takes the same commands from our DLL's
  native calls. Values come from our DLL either way, not through
  HUDFramework's notification channel, so we add no on-screen message
  traffic and no throttle.
- **Detect, don't assume:** the patched HUD sets a `HUDFramework` member on
  the HUD movie. Our DLL checks for it before injecting, so a widget is never
  loaded twice.

Playing nicely:
- Ship no `HUDMenu.swf`, and never patch theirs.
- Add nothing to the HUD's root under their names; namespace ours.
- Follow their power armor switch when hosted; handle it ourselves when not.
- Test with vanilla, HUDFramework, and HUDFramework's DEF_UI patch.

### What we could add to it

Only with the author's agreement, offered as contributions, since we can't
release changes to unlicensed code:
- **The F4SE fast path it already reserves.** `HUDFramework.psc` calls
  `HUDFrameworkNative.SendMessageRaw` but forces it off
  (`StartSession`: "TODO - Unlock this once native functions are
  implemented"), so every message on PC goes through notification text and
  a throttle. A small DLL with that native, built on our shared base, would
  make PC messages instant. Their script must switch it on, so it needs them.
- **A license.** Asking costs nothing; with one, we could depend on it fully.

The Nexus author is registrator2000, very likely reg2k (XDI, MCM). If so,
one conversation covers this, the [`XDI.esm` stand-in](dialogue_menu_mod.md#decisions)
and MCM. Neanka (XDI co-author, DEF_UI, PipboyTabs) is the other person to
ask.

### <a id="hud-built"></a>The host (built 2026-11-13, not played)

`CRUI_Hud` (Papyrus) picks each widget's host; `CleanroomUI.dll`
(`dll/framework/hud.cpp`, natives on `CRUI_HudNative`, framework version 4)
keeps the list; `menus/hud/` holds our host movie `CRUI_Hud.swf` and the
stats widget `CRUI_HudStats.swf`.
- **`AddWidget(owner, name, movie, x, y, anchor)`** shows a widget movie
  (a path under `Interface`, as HUDFramework's widgets are).
- **`Send(name, command, params)`** calls the widget's
  `processMessage(command, params)`, HUDFramework's widget method, held
  until the widget has loaded.
- **`AddStats(owner, name, labels, values, steps, shown, x, y, anchor)`**
  is a built-in widget: a line of labels (a `$KEY` is translated) with the
  player's actor values, in the HUD's bold font and color. Below `steps[0]`
  a value is dim, from `steps[1]` it takes the HUD's warning color
  (`iHUDColorWarning`), from `steps[2]` it flashes; it shows while the
  `shown` global is not 0. FNV Hardcore's H2O / FOD / SLP is one call:
  labels `H2O`, `FOD`, `SLP`, its three needs, steps 400, 600, 800, the
  Hardcore global, anchored on `LeftMeters_mc` 30 pixels above it.
- **`RemoveWidget(name)`**; a name given again moves or replaces its
  widget. Every widget, stats line and message is forgotten as a save loads
  or a new game starts, so each mod adds its own on every load.

**With HUDFramework installed** (`HUDFramework.esm` or `.esp`), it hosts:
- `CRUI_Hud` reaches it only through vanilla calls
  (`Utility.CallGlobalFunction("HUDFramework", "GetInstance")`, then
  `CallFunction`), so nothing of ours needs its scripts or plugin.
- A widget movie is registered with it (`RegisterWidget`, loaded now and on
  every game load) **only when it is not registered yet**: registering
  again puts back our `x`, `y` over the place the player gave it with
  HUDFramework's editor. `x`, `y` are its first place in HUDFramework's
  own coordinates; the HUD-part anchor doesn't apply there. `owner` is its
  owner: HUDFramework drops a widget whose owner is gone and calls the
  owner's `HUD_WidgetLoaded(widgetID)` when it loads (a missing function
  only logs a Papyrus error).
- `Send` goes through its string channel (`SendMessageString`, the params
  joined with `" | "`, which it splits back into `params`). On PC that is
  about once a second, a newer message for the same command replacing an
  older one, since its own fast path is still switched off.
- Every stats line is drawn by our one stats widget, registered with it,
  one line under another: the player moves all of them together.
- `RemoveWidget` unloads and unregisters a widget there too.
- Our host movie still loads, with no widget of its own: it places the
  game's own HUD parts ([edit](#hud-edit), [layouts](#hud-layout)).

Without it, our host: `CleanroomUI.dll` loads `CRUI_Hud.swf` into whatever
`HUDMenu.swf` is installed (vanilla, DEF_HUD's) with a `Loader`, as the
Pip-Boy overlay does, each time the HUD movie loads (until 2026-11-13 only
once a widget of ours was added; the HUD's own parts need it now). A widget there joins its HUD part (`anchor`: clip names
from the HUD's root, dotted, e.g. `LeftMeters_mc`), so it moves, hides and
fades with it, at `x`, `y` from that part's top-left as measured before the
widget joined; `""` puts it on the HUD itself. Messages reach it on the
host's next frame, with no notification text or throttle. Each frame the
host asks `HudWidgets(version)` (null while the list is unchanged),
`HudMessages()` and `HudStats(name)`; the shared natives gained
`HudColor(prefix)` and `Log(text)` (the framework DLL had no `Log` since the
split, so the Pip-Boy movie's lines were lost).

Not built yet: the power armor HUD on our host ([saved places and the
edit menu](#hud-edit) and the [layout module](#hud-layout) are built), and
the converter's Hardcore needs on
`AddStats` in place of `hardcore.cpp`. **Play check:** a test widget and a
stats line under vanilla, DEF_HUD and HUDFramework (registered once, moved
in its editor, still there after a save loads); a message reaches the
widget under both hosts; on our host a widget anchored on a part hides with
it in menus and power armor.

### <a id="hud-edit"></a>Saved places and the edit menu (built 2026-11-12, not played)

Our host keeps a place per widget of ours (players without HUDFramework)
and, since 2026-11-13, per part of the game's own HUD
(`dll/framework/layout.cpp`, framework version 5; the parts 8):
- **A place** is an offset from where the widget's mod puts it, in
  fractions of the screen, and a scale (a quarter to four times). Places are
  kept by widget name in `Documents\My Games\Fallout4\F4SE\CleanroomUI_Hud.txt`
  (`name<tab>x offset<tab>y offset<tab>scale`), so one layout serves every
  save. The DLL reads it once as the game loads and writes it when the edit
  menu is done.
- **The host** places each widget where its mod puts it, reports that origin
  and bounds (`HudReport`), then moves and scales it by its place (each
  `HudWidgets` entry carries `place`). It places and reports every widget
  again every 15 frames, for one whose size changed.
- **The edit menu** (`Interface/CRUI_HudEdit.swf`) opens with
  `CRUI_Hud.EditLayout()`, from a mod's MCM button or the console
  (`cgf "CRUI_Hud.EditLayout"`). It is a custom menu with the level-up
  menu's flags (pauses, cursor). Over the game dimmed it draws a box with the
  name of each widget on our host where its place puts it, computed from the
  host's reports and the places alone (`HudLayout`), so the boxes are right
  even while the HUD is paused under the menu. Drag a box to move its
  widget (`HudMove`), turn the wheel over it to scale it (`HudScale`), R (a [setting](#keys)) puts
  it back (`HudReset`); Esc or Tab saves and closes (`HudEditDone`, which
  closes the menu through the game's UI messages). Its words are
  `$CRUI_HUD_EDIT_HINT` and `$CRUI_HUD_EDIT_NONE`.
- **The game's own HUD parts** are in it too (the 27 DEF_HUD lays out, named
  by their clips, saved as `part:HPMeter_mc` and so on). The host's
  `PartLayout` takes a part's base from where the game put it, with
  DEF_HUD.xml's changes over it ([layouts](#hud-layout)); it reports that
  base (`HudReport`) and moves and scales the part by its place
  (`HudPlaceOf`), again every 15 frames and as a place changes. A part the
  game moved itself since takes that as its new base, so the player's
  offset follows it. R puts a part back to the game's (or DEF's) place.
  For this the host now loads into every HUD, with or without widgets: the
  edit menu needs the parts measured before it opens.
- Widgets HUDFramework hosts are not in it: its own editor places them.

Hiding and recoloring are built ([look](#hud-look)), and parts shown on
[conditions](#hud-conditions).

**With a gamepad** (built 2026-11-13, not played): one box is picked
instead of the one under the cursor, drawn with a thicker outline.

| Pad | Does |
|---|---|
| LB, RB | Pick the previous, next box (top to bottom, then left to right) |
| D-pad, held | Move it, from 1 to 12 px a frame over 1.5 s |
| LT, RT | Scale it down, up (as a wheel notch) |
| X | Hide or show it |
| Y | Its next color |
| A | Its rule list ([when shown](#hud-conditions)), which the D-pad, A and B then work |
| View (Select) | Reset it |
| B | Save and close |

The menu takes FO4's controls (`ProcessUserEvent`, `SetPlatform`, as the
level-up menu does) through `Input`. FO4's menu-mode control map sends
some keyboard keys as the same controls (R is X, T is Y, C is LB, Q is
RB, V is View), so the pad's buttons count only while a pad is in use
(the game's word through `SetPlatform`, else `Input`'s guess); the
keyboard keeps its own keys, over the box under the cursor. The hint
shows the pad's (`$CRUI_HUD_EDIT_PAD_HINT`) while a pad is in use.

Not built yet: hiding a part's pieces, rotating in the menu (DEF_HUD.xml's
still applies), opacity, a key to open it without an MCM. **Play check:** whether the HUD's widgets show behind the
menu (the boxes don't depend on it); whether the HUD's and the menu's
stages line up, since the boxes map the host's fractions of its stage onto
the menu's; a moved and scaled widget stays put after a save loads and a
restart; with a pad, every row of the table above, whether the menu gets
the D-pad's releases (else a box keeps moving: then a fixed step per
press is next) and the triggers as controls, whether the left stick sends
the D-pad's controls too, and that the keyboard's R, C and V don't also
act as the pad's buttons; a moved HUD part follows the game when it moves the part (power
armor, the enemy health meter, the crosshair in third person), and a part
that is hidden (no enemy, no explosives) still gets a box to place it.

### <a id="hud-layout"></a>HUD layouts (what DEF_HUD did; reading built 2026-11-13, not played)

DEF_HUD lets players move, rotate, resize, recolor and hide every part of
the HUD, but it does so by replacing `HUDMenu.swf` (one copy per screen
shape), so it fights every other HUD mod and needs a merged copy for
HUDFramework ([DEF_UI](spin_offs.md#def-ui)). Our host does the same
with no replaced movie (`dll/framework/parts.cpp`,
`menus/hud/as3/PartLayout.as`, framework version 7):
- **Reads `Data\Interface\DEF_CONF\DEF_HUD.xml`**, where DEF_HUD's editor
  saves it, so players' layouts carry over. The DLL parses it
  (`HudPartLayout`: the settings, and per part its fields under DEF's
  names); our host loads with every HUD.
- **The format** (read from DEF_UI 1.6.0's default files, 27 parts): per
  part, named by its clip, `x`, `y`, `scaleX`, `scaleY`, `rotation`,
  `color`, `color2`, `flags`, `parname` and `defx`, `defy` (where the game
  puts it); and `SafeX`, `SafeY`, `globalscaleX`, `globalscaleY`, `trunc`.
  Every `parname` is one of the seven groups the vanilla `HUDMenu.swf`
  pins to the safe area's corners and middles (`LockToSafeRect` in its
  `onSetSafeRect`), and every part is a clip of that name in it. So the
  positions are already relative to the safe area, and DEF's five screen
  shapes ship the same file: **one layout serves every screen shape**.
- **What is set:** a part's position when it differs from `defx`, `defy`;
  a scale other than 1; a rotation other than the part's own; a color other
  than white, as a tint multiplied over what is drawn. The values are
  DEF's own, absolute in the group, so with DEF_HUD's own `HUDMenu.swf`
  installed both set the same values and nothing needs to stand down.
  Everything the file leaves at its default stays the game's.
- **Kept applied:** again every 15 frames, for parts the game moved or
  made again (power armor, HUD mode switches); a value already right is
  left alone.
- **Not applied yet, logged once:** `flags` (the readme: hiding pieces of
  a part, such as the HP meter's tag; which bit is which piece is only in
  DEF's movies, so it is to be worked out from the game's parts in play),
  `color2`, `SafeX`/`SafeY` (the game's own safe area is kept),
  `globalscaleX`/`globalscaleY`.
- **Edited in game** in the [edit menu](#hud-edit): moves and scales a
  part from where the game and the file put it, saved in our own layout
  file; DEF_HUD.xml itself is never written.
- **Hidden and recolored in game** too ([look](#hud-look)); the
  [FNV preset](#fnv-preset) is built. Not built yet: parts shown on
  [conditions](#hud-conditions).

**Play check:** with DEF's default file, nothing moves; a moved, scaled
and rotated part (one per group) sits where DEF's own movie puts it, at
16:9 and 21:9; it stays put through power armor and a save load; whether
a color survives the engine's HUD tint; `ExplosiveIndicatorBase_mc`
(rotation 180 in DEF's standalone default, 0 in its HUDFramework one):
which is the game's own.

Together with the widget host this covers both of the HUD's frameworks
in one injected movie, with no `HUDMenu.swf` shipped by anyone.

### <a id="fnv-preset"></a>The FNV layout preset (built 2026-11-13, not played)

`[Hud] sPreset=FNV` in `CleanroomUI.ini` (or MCM's saved copy) arranges
the game's own HUD parts as Fallout: New Vegas arranges its HUD
(`menus/hud/as3/FnvPreset.as`, framework version 9). Read from FNV's
`menus/main/hud_main_menu.xml` (in `Fallout - Misc.bsa`, for reference
only): it names the parts (`HitPoints`, `ActionPoints`, `EnemyHealth`,
`SneakMeter`, `ReticleCenter`, `QuestReminder`, `XPMeter`,
`RadiationMeter`, `HardcoreMode`, `crippled_limb_indicator`, ...) but
sets no positions; FNV's engine places them from the safe area. So the
preset is rules, worked out each time from each part's own bounds where
the game put it, right at any screen shape and HUD size:
- **Bottom left:** the compass under the HP bar, left edges together, at
  the HP bar's bottom; the HP bar, the rads meter and the location text go
  up above it (FNV's HP bracket holds the compass).
- **Bottom right:** the ammo count under the AP bar, right edges together;
  the AP bar goes up above it (FNV: AP, then weapon condition and ammo).
- **Bottom center:** the enemy health meter just above the critical meter.
- **Center:** the sneak meter just above the screen's middle, over the
  crosshair.
- No part leaves its group (the engine may reach a part by its path);
  each moves within it. A part that draws nothing is left where it is.
- It runs after each part's base and before the player's places
  (`PartLayout`: lay, arrange, then report and place), so the edit menu
  moves parts from the preset's positions and R puts them back there. With
  a preset, `DEF_HUD.xml` is left aside (logged).

FNV's parts that FO4's HUD has no part for: [its own
widgets](#fnv-widgets) (the weapon condition bar built). **Play check:** each rule at 16:9 and 21:9, in and out of
combat and sneaking; the ammo count, right-aligned, as its digits change;
the HP bar and compass with the rads meter showing; power armor's own HUD
is a different movie and is not touched.

### <a id="fnv-widgets"></a>The FNV preset's own widgets (built 2026-11-13, not played)

What FNV's HUD shows that FO4's has no part for, checked against where
FNV's engine places each (the FNV decomp's `HUDMainMenu` setup, read only):
- **Weapon condition (CND): built.** FNV draws a small CND meter on the
  ammo row of its AP block, at the block's left, the ammo count right-aligned
  beside it. `ConditionBar.as` draws "CND" (`$CRUI_CND`) and a meter in the
  HUD's bold font and color with our own code. It joins the group
  `RightMeters_mc`, so it hides with it, and takes the AP bar's alpha. The
  preset puts it on the ammo count's row at the AP bar's left edge (just
  under the AP bar without an ammo count). It is a part of the layout like
  the game's (`FnvPreset.WIDGETS`): in the edit menu as
  `CRUI_WeaponCondition`, moved, scaled and reset there. The value comes
  from `HudWeaponCondition` (`parts.cpp`), read every 6 frames: the
  equipped weapon's condition, 1 while none is set yet (as the Pip-Boy
  shows it), and hidden with no weapon or one that doesn't wear
  (`CRUI_Condition.Track`). Framework version 10.
- **Crippled limbs: nothing to build.** FNV places its limb figure at the
  top right, 200 px in from the safe area's right edge and 100 down; FO4's
  own condition Vault Boy (`VaultBoyCondition_mc`) already sits there and
  shows the same thing, so the preset leaves it.
- **Hardcore needs: the converter's.** FNV puts H2O / FOD / SLP at the HP
  block's top, 150 px in. The converter's [stats line](#hud-built),
  anchored 30 px above `LeftMeters_mc`, already sits there; with the
  preset the group's top is measured after the HP bar moves up, so it
  stays above it. Moving it onto `AddStats` is still converter work.
- **The critical meter: kept.** FNV has none, but the converter keeps
  FO4's V.A.T.S. critical meter alongside FNV's random criticals, so the
  preset doesn't hide it.

Not built: FNV's CND arrows (shown as the condition changes), the ammo
type label. **Play check:** the bar's row and size against the ammo count
at 16:9 and 21:9 and at another HUD scale; it hides in menus, dialogue
and power armor with the right meters; a melee weapon (no ammo count)
still shows it; the value after a repair, a swap and with an untracked
weapon (hidden).

### <a id="hud-look"></a>Hiding and recoloring in game (built 2026-11-13, not played)

Every box in the [edit menu](#hud-edit), our widgets and the game's own
parts alike, can be hidden and recolored (`menus/hud/as3/Look.as`,
`dll/framework/layout.cpp`, framework version 11):
- **Keys** ([settings](#keys)): H (`sHudHide`) hides the one under the
  cursor or shows it again; C (`sHudColor`) steps it through its own color,
  white, FNV's amber (`FFB642`), FO3's and FO4's green (`1AFF80`), blue
  and red; R (`sHudReset`) puts it back where it was, shown, in its own
  color. A hidden one keeps a dim box marked HIDDEN
  (`$CRUI_HUD_EDIT_HIDDEN`), so it can be shown again; a colored one's box
  takes its color.
- **Saved** with its place: two more columns in `CleanroomUI_Hud.txt`,
  hidden (`1` or `0`) and the color (`RRGGBB`, `-` for its own); a line
  from before them loads shown, in its own color. The natives:
  `HudHide(name)`, `HudTint(name, color)`; `HudPlaceOf`, `HudLayout` and
  `HudWidgets`' places carry `hidden` and `color`.
- **The game's parts** are shown, hidden and faded by the engine itself
  (it sets their `visible` and `alpha` from C++; `HUDMenu.as` sets
  neither), so we set neither: we'd fight it, and lose what it last
  wanted. A hidden part gets a mask covering a tenth of a pixel at its own
  top-left (inside it, so its bounds and the edit menu's box don't change),
  which the game never sets; taking it off shows the part as the game has
  it then. A color goes on as the part's color transform's multipliers
  (its alpha and offsets stay the game's), put back each frame whenever
  they aren't ours, so a write from the game is undone a frame later; the
  game's own are kept from its last write and put back when the color
  comes off. The player's color goes over DEF_HUD.xml's, which now goes
  on the same way.
- **Our widgets**: nothing else sets their alpha, so hidden is alpha 0. A
  stats line draws in the color (in place of the HUD color; the warning
  color stays); a widget movie takes it as its multipliers, over what it
  draws.

**Play check:** a hidden part stays hidden through HUD mode changes,
power armor, menus and a save load, and shows as the game has it when
shown again (an enemy health meter with no enemy stays hidden); whether
Scaleform hides a part under a mask that small (if it shows the part
whole, a mask the part's size placed off it is next); whether the engine's
HUD color goes on the parts' own transforms (our color then replaces it)
or over the whole movie (ours then multiplies with it: green times amber
is darker); a colored part keeps its color as the HUD color changes in
the settings.

### <a id="hud-conditions"></a>Parts shown on conditions (plan 2026-11-13; built, not played)

The most used HUD option after moving parts is hiding them while they
aren't needed: health and ammo out of fights, the compass until a key is
pressed. Immersive HUD and its like do this (ideas only: none of their
code or files is read). Our version, on the [look](#hud-look) above:

**A rule per box.** Each part or widget is shown always (today), or only
while one of its conditions holds, plus a linger (default 3 s) after the
last one ends, so it doesn't flicker. Conditions, read by the DLL once a
frame into one native (`HudState()`: a set of names that hold now):

| Condition | What it means | How we'd read it |
|---|---|---|
| `combat` | The player is in combat | `Actor::IsInCombat` (F4SE names the virtual) |
| `drawn` | A weapon is out | `ActorState::IsWeaponDrawn` (F4SE) |
| `aiming` | Aiming down the sights or the scope | Built: the player camera in its iron sights state, or `ScopeMenu` open |
| `sneaking` | Sneaking | Built: the game's sneak meter on screen (the engine shows it only while sneaking) |
| `vats` | V.A.T.S. is open | `UI::IsMenuOpen("VATSMenu")` |
| `peek` | Our show key is held (and the linger after) | `GetAsyncKeyState` on `[Keys] sHudPeek` while the game's window is in front (built); a `BSInputEventUser` would add a pad button |
| `changed` | The part's own value changed (HP, AP, rads, ammo, condition) | Built: the actor values (`HudValues`) for the meters, else the part's text |
| `low` | The part's value is under a threshold (HP under 50%, the magazine under a quarter) | Built: the same values, `low:F` for a threshold |
| `moved` | The location or an objective changed (for the compass) | Built: the location name's and the quest updates' text |
| a mod's own | Any condition a mod sets | Built: `CRUI_Hud.SetCondition(name, on)` from Papyrus, or `SetConditionGlobal` with a global |

**Showing and hiding** are the mask from the look (instant, never fights
the engine). A fade would go on the alpha the engine also sets, so it
would follow the colors' rule (the engine's last value kept and our
fraction put over it each frame); first only on parts whose alpha the
engine leaves alone (the meters, the compass), and only if the instant
switch looks wrong in play.

**Where rules live.** In the place, as one more column of condition names
(`combat,drawn,peek`; empty: always shown), so one layout file holds
everything the edit menu sets. A preset gives every part a rule at once:
`[Hud] sShow=Immersive` (or MCM's copy; step 1's is below): HP and AP on `combat`, `changed`,
`low`, `vats`, `peek`; ammo and the CND bar on `drawn`, `changed`, `peek`;
the crosshair on `drawn`, `aiming`; the compass on `peek`, `moved`,
`sneaking`; the rest always. A rule set in the edit menu wins over the
preset's.

**Edited in the edit menu**: a key (`sHudShow`, default V) over a box opens
a `ScrollList` of the conditions to tick; the box shows its rule after its
name (step 4, below).

**Step 1, built 2026-11-13, not played** (`dll/framework/show.cpp`,
`menus/hud/as3/ShowRules.as`, framework version 12):
- **The conditions:** `combat` (`Actor::IsInCombat`), `drawn`
  (`ActorState::IsWeaponDrawn`), `vats` (`VATSMenu` open) and `peek` (the
  show key, `[Keys] sHudPeek`, default Y, held while the game's window is
  in front). `HudState()` gives them to the host once a frame, with the
  linger.
- **The show key** is read with `GetAsyncKeyState` in that native, not a
  `BSInputEventUser` as first planned: no hook, and the HUD only asks
  during play. Keyboard only. Held, it shows; let go, it stays for the
  linger, so a tap shows the parts for a few seconds and holding keeps
  them up.
- **Rules:** the layout file's seventh column (`combat,drawn,peek`;
  `always` for none; `-` for the preset's). With none set, the preset's:
  ```ini
  [Hud]
  sShow=Immersive    ; or None (the default): every part always shown
  fShowLinger=3.0    ; seconds a part stays after its last condition ends (0 to 60)
  ```
  read again each time the HUD movie loads (as a game loads), so a change
  in MCM applies then. `Immersive`: HP and AP on `combat,vats,peek`; the
  ammo count, the explosives count and the FNV preset's CND bar on
  `drawn,peek`; the compass on `peek`. Every other part is always shown
  (the crosshair also aims activation; the engine shows the enemy health
  and sneak meters itself).
- **Applied** each frame by the host: a part whose rule holds for none of
  its conditions (and past its linger) is hidden by the look's mask, as the
  player's hiding is; a widget of ours by alpha 0. A condition the DLL
  doesn't know counts as holding (logged once), so a typo never hides a
  part for good; without the DLL every part shows.
- **In the edit menu** each box shows its rule in brackets after its name;
  V edits it (step 4).

**Play check (step 1):** with `sShow=Immersive`, HP and AP fade in as a
fight starts and out three seconds after it ends, and show in V.A.T.S.;
the ammo count and CND bar with a weapon out; the compass only while Y is
held and three seconds after; Y doesn't count while typing in the console
or with the game in the background; whether `IsInCombat` is safe to call
from the HUD's thread each frame (it is the engine's own combat check).

**Step 2, built 2026-11-13, not played** (`ShowRules.as`; `HudValues`
in `show.cpp`; framework version 13). Worked out by the host:
- **`changed`:** the box's value moved since the last reading. The
  meters' values come from `HudValues()`, read every 6 frames: the
  player's health and AP as fractions of their maximum (actor values
  `0x2D4`, `0x2D5`), rads (`0x2E1`), the equipped weapon's condition (as
  the CND bar shows it) and the weapon's form ID. A part with no value
  (the ammo count, the explosives count, any other) counts its text: the
  text fields under it, read each frame. A change shows the part for the
  linger, so a hit, a heal, AP spent or coming back, or ammo picked up
  brings it up for three seconds.
- **`low`** (`low:F` sets the threshold F, 0 to 1): HP, AP and the CND
  bar under half; the ammo count's magazine (the first number it shows)
  at or under a quarter of the fullest it has been with this weapon (the
  game's magazine size isn't read; a weapon change starts again).
- **`moved`:** the location's name (`LocationText_mc`) or the quest
  updates (`QuestUpdates_mc`) show new text; text being cleared doesn't
  count.
- **Our widgets** have no value, so `changed` and `low` never hold for
  them.
- **The preset** now: HP and AP on `combat,vats,changed,low,peek`; the
  ammo count on `drawn,changed,low,peek`; the explosives count on
  `drawn,changed,peek`; the CND bar on `drawn,low,peek`; the compass on
  `moved,peek`.

**Play check (step 2):** HP shows as it drops or heals and stays under
half; AP shows as it drains and while it comes back; the ammo count shows
on picking up ammo with the weapon away, and stays when the magazine is
nearly empty; the compass shows on entering a new location and on a new
objective, not as the name fades; whether the HUD reads the ammo count's
text as "magazine, then reserve" (its text fields' order isn't named in
the game's files).

**Step 3, built 2026-11-13, not played** (framework version 14):
- **`aiming`** (the DLL's, in `HudState`): the player camera in its iron
  sights state (F4SE's `PlayerCamera`, `kCameraState_IronSights`), or the
  scope menu (`ScopeMenu`) open. Third person's over-the-shoulder aim
  isn't a camera state of its own, so it doesn't count.
- **`sneaking`** (the host's): the game's sneak meter (`StealthMeter_mc`)
  is on screen, that is it and every clip above it visible and not faded
  out. The engine shows that meter only while the player sneaks (it calls
  `SetStealthMeter` then), so no engine state is read; F4SE names no
  sneaking flag, and the actor state's movement bits would need the game's
  code read first. Our own hiding is a mask, which leaves `visible` and
  `alpha` alone, so a player who hides the meter still has `sneaking`.
- **The preset:** the compass on `moved,sneaking,peek`.

**Play check (step 3):** the compass shows while crouched and goes three
seconds after standing; whether the sneak meter's clip really goes
invisible out of sneak (if the engine only empties it, `sneaking` holds
all the time and the compass never hides: then the meter's text is the
next thing to watch); `aiming` in iron sights and through a scope, and
not in third person.

**Step 4, built 2026-11-13, not played** (`menus/hud/as3/RuleList.as`,
`HudSetShow` in `layout.cpp`, framework version 15):
- **V** (`[Keys] sHudShow`) over a box in the edit menu opens a list in
  a pop-up in the middle of the screen: *As the preset sets*, *Always
  shown*, then each condition marked ON or not, then *Done*. Arrows or the
  mouse move, Enter or a click picks, Esc or Tab closes it (not the
  menu); while it is open the menu's own keys, dragging and the wheel
  leave the boxes alone.
- **Picking** the preset's or always sets that; picking a condition
  switches it in the box's own rule, starting from the rule in force (the
  preset's, when the box had none of its own), so a player tweaks the
  preset's rule instead of starting over; switching the last one off
  leaves the box always shown. A threshold on `low` (`low:0.3`, set in the
  file) stays while `low` stays on.
- Each pick goes to the DLL at once (`HudSetShow(name, rule)`: `""` the
  preset's, `always` none); Esc on the menu saves it with the rest of the
  layout. `HudLayout` now also gives each box's own rule (`showOwn`) next
  to the one in force (`show`).
- The linger stays one setting for every box (`fShowLinger`).

**Play check (step 4):** V opens the list over the box under the cursor;
the list's keys and clicks don't move or close anything else; Esc closes
the list first, then the menu; a rule set there survives a restart; the
preset's choice follows `sShow` after a change in MCM.

**Step 5, built 2026-11-13, not played** (`show.cpp`, `CRUI_Hud.psc`,
framework version 16). Other mods' conditions and rules, from Papyrus:
```papyrus
CRUI_Hud.SetCondition("mymod.alarm", True, "$MYMOD_ALARM")   ; holds, or not
CRUI_Hud.SetConditionGlobal("mymod.alarm", MyAlarmGlobal)    ; holds while the global is not 0
CRUI_Hud.RemoveCondition("mymod.alarm")
CRUI_Hud.SetShowRule("MyWidget", "combat,mymod.alarm,peek")  ; a box's rule from its mod
CRUI_Hud.SetShowRule("part:CompassWidget_mc", "mymod.alarm,peek")
```
- **A condition** has a name of letters, digits, `_` and `.` (a rule
  splits on `,` and `:`); one of ours (`combat`, ...) is refused with a
  warning. `HudState` gives it to the host beside ours, so a rule names it
  as any other. A global is read each frame by the DLL, with no Papyrus
  call, for a state a mod keeps in a global anyway.
- **The edit menu's list** shows the mods' conditions after ours
  (`HudConditions`), under their labels (a `$KEY` is translated).
- **A mod's rule** for a box (its own widget, or one of the HUD's parts)
  goes between the player's and the preset's: the player's in the edit
  menu still wins. The list's first row now reads *As its mod or the
  preset sets*.
- **Forgotten as a save loads**, as widgets are: each mod sets its own on
  every game load. A rule naming a condition nobody set counts it as
  holding, so a box never stays hidden when its mod is gone.
- **Fades**: not built. The plan keeps them until play shows the instant
  switch looks wrong.

**Play check (step 5):** a test script's condition shows and hides a
part; a global flipped in the console (`set MyAlarmGlobal to 1`) does
the same; the condition is listed in the edit menu; a mod's rule shows
in the box's brackets and the player's choice wins over it; both are
gone after loading a save made without the mod.

**Order of work**, each its own play test:
1. **Built.** The DLL's state (`combat`, `drawn`, `vats`) and the peek key
   (read with `GetAsyncKeyState`, held then the linger), the rule column
   and the `Immersive` preset, instant show and hide.
2. **Built.** `changed` and `low` from the values the host reads; `moved`
   for the compass.
3. **Built.** `aiming` and `sneaking`.
4. **Built.** The rule list in the edit menu.
5. **Built.** `CRUI_Hud.SetCondition` and `SetShowRule` for other mods;
   fades only if play asks for them.

**Open questions:** whether a part the engine hides on its own (the enemy
health meter, the sneak meter) needs any rule at all, or is left out of
the preset; whether the peek key is press-for-a-while or hold by default
(Immersive HUD offers both); whether HUDFramework's widgets (placed by its
editor, not ours) get rules through its own settings or ours.

## <a id="level-up"></a>Our own level-up menu (built 2026-11-12, not played)

The menu's features and order: [fo4_level_up_plan.md](fo4_level_up_plan.md).
This section is the framework side.

FNV's skills, perks, traits and tags went through LevelUpMenuEx (Neanka:
its own `LevelUpMenu.swf`, `PRKF.dll`, MCM config; unlicensed, one
maintainer). Our own menu on f3ui replaces it, follows our HUD color and
translations, and since 2026-11-12 LevelUpMenuEx is no longer required
([dropped](fo4_level_up_plan.md#dropped)).

**The art needs no authoring.** FNV's and FO3's level-up screens are a
list, a description and a Vault Boy picture, and every picture is named by
the source records: each PERK's and skill or SPECIAL AVIF's `ICON`.
Census of the exports (2026-11-12): FalloutNV.esm 156 distinct images (135
perk, 21 skill and SPECIAL), Fallout3.esm 91 (70 and 21), every one in the
export. They are DXT3 or DXT5, 256x256.

**Built 2026-11-12 (not played):** `fo4_import/spike/level_up_icons.py`
stages every one of them in each build, copied as it is to its FNV path
under our texture folder (`statics.copy_texture`, as the casino's
textures), so `f4se.MountImage` can show them. A DLC build stages only the
images its masters' builds did not. Counts: `level-up icons staged`,
`level-up icon missing from the export`.

**The image spike (built 2026-11-12, not played):** the casino's spike
screen (`cqf FNV_Casino Spike`) shows four images side by side: the DXT1
card face as the control, then `skills_barter.dds` (DXT3),
`perk_action_girl.dds` (DXT5) and `perk_action boy.dds` (DXT3, a space in
the name). The list is `records.SPIKE_LEVEL_UP`, passed in the bank
quest's `SpikeImages` property. Read the `F3NV_CasinoSpikeImage` lines in
`Papyrus.0.log`: "shown 256x256" for each means MountImage takes all
three. "Mounted" alone proves nothing with an older F3NV.dll, since a
missing file still mounts ([casino phase 0](fo4_casino_plan.md#phase-0)).
With the new DLL, a missing file is labeled missing ([images](#images)).

**The menu (built 2026-11-12, not played):** `fo4_import/level_up/`, our
own movie registered from Papyrus like the casino's, with SKILLS, PERKS
and TAKEN pages on `ScrollList`, `Modal`, `Input`, `HintBar` and `Images`
([menu](fo4_level_up_plan.md#menu)). What it shows comes from tables the
build writes onto `F3NV_LevelUp`, since each Papyrus call to a Form or
Perk native costs a frame. Images come from the records: each perk's in
its own PERK `ICON` and the table, a skill's in the table
([images](fo4_level_up_plan.md#images)).

**Replacing a vanilla menu (built, not played):** the character rules open
ours whenever `LevelUpMenu` opens, after closing it and the Pip-Boy
([open](fo4_level_up_plan.md#open)). No DLL and no vanilla file; the
framework's DLL route (swapping a menu's constructor in F4SE's menu table)
stays in reserve for when the moment of the vanilla screen shows.

**Tag skills and traits (built, not played):** the same movie in another
mode, one page each and no Back; picks come back to Papyrus as a bitmask
in one external event ([tags](fo4_level_up_plan.md#tags)).

**Every plugin's tables (built, not played):** each plugin's perks and
traits are its own quest's table, joined by Papyrus as the menu opens, so
the movie sees one list ([tables](fo4_level_up_plan.md#tables)).

Still to do:
- **Play the image spike** above, and the menu.
- Playing the menu, then the options left (level cap, perk sound, the
  off-by-default features). LevelUpMenuEx is no longer required
  ([dropped](fo4_level_up_plan.md#dropped)).

<a id="level-up-play"></a>**Play check:** level up (`player.setlevel` one
higher, or earn it), then open the level-up screen from the Pip-Boy:
- the Pip-Boy closes and our menu opens (a moment of the old screen first
  is expected); `Papyrus.0.log` has `F3NV_LevelUp: sent in Ns` (how long
  the open took) and an `open:` line with the counts (FNV 91 perks, 13
  skills, 105 rows);
- SKILLS: set points with Right, take back with Left, the list shows
  "45 > 50", X spends them and the skill rises; Back with points set asks;
- PERKS: only perks you can take, X shows all (others dimmed), each with
  its image, requirements ("Science 50 (you have 45)") and description;
  Accept asks, then takes it, and it moves to TAKEN;
- a male character sees Lady Killer as available at its level;
- leave with points unspent, reopen: they are still there.

Tags and traits, on a new game:
- after Doc Mitchell's questionnaire, the TAG SKILLS page opens with its
  three suggestions ticked; untag one and tag another, X before three are
  tagged says "Tag 1 more", X with three closes it; each tagged skill is
  15 higher, an untagged suggestion is not;
- Back does nothing on either page;
- the TRAITS page lists FNV's traits with images; choose two, a third says
  to drop one first; X closes it and the traits are on the player
  (`player.hasperk`);
- the Tag! perk at level 16 reopens TAG SKILLS for one more: the three
  tags stay locked, the fourth gets the bonus;
- re-entering SPECIAL at the Vit-o-matic keeps each tag's bonus once;
- a save made with LevelUpMenuEx, loaded with it still installed, keeps its
  banked points and tags once the menu opens; then, with LevelUpMenuEx
  removed, a level-up opens our menu and `Papyrus.0.log` has no `PRKF`
  errors.

With the DLCs loaded (FNV: Dead Money, Honest Hearts, Old World Blues,
Lonesome Road, Gun Runners' Arsenal):
- the `sent in` log line says `6 tables`, and PERKS with X (show all)
  lists 122 perks, the DLCs' among them;
- a DLC perk's requirements show (a game perk it needs is met or unmet as
  you hold it), and taking one takes that perk;
- TRAITS lists Old World Blues' six after the game's ten, and choosing one
  adds it.

Options, with MCM (Mod Configuration Menu, `F3NV: Fallout New Vegas`):
- set levels per perk to 1, level up: a perk point; set skill points per
  level to 20: the next level-up gives 20 + Intelligence x 0.5;
- set the skill cap to 150: SKILLS lets a skill past 100;
- turn on show every perk: PERKS opens with every perk, dimmed ones too;
- bind the hotkey and press it outside menus: our menu opens (if nothing
  happens, MCM's `CallFunction` didn't find `F3NV_CharacterRules` on the
  quest).

The API, from the console (`cqf` runs a quest function; the rules quest's
EditorID is `FNV_CharacterRules`):
- `cqf FNV_CharacterRules "F3NV_LevelUp.OpenTagMenu" 4`: the tag menu
  for a fourth tag, the three held locked;
- `cqf FNV_CharacterRules "F3NV_LevelUp.AddPerk" <a Fallout 4 perk's
  FormID>`, then open the menu: the perk is last on PERKS, its image if it
  has one, "Its own requirements" met or unmet as the game's test says;
- `Papyrus.0.log` after a level-up: no errors from `MenuClosed` or
  `SkillRaised`.

## <a id="casino"></a>The casino on the framework

- Move `Ui.as` and the bridge into the library; the games keep their own
  classes. **Done 2026-11-11** ([spike 1](#spikes)).
- The HUD color: settled by spike 2b (the menu colors itself from the
  DLL's `HudColor()`; played). The menu stays registered from Papyrus;
  registering from the DLL was for the color and isn't needed.
- Keep the Python reference models and in-game self-tests; they test game
  rules, which the library doesn't change.

## <a id="translations"></a>Translations

Every release should work in every language the game ships, and a
translator should be able to translate it by adding files, never by editing
a plugin or a movie.

### Where each kind of text comes from

| Text | Example | Comes from | Who translates |
|---|---|---|---|
| **Game data** | Dialogue options and lines, item, skill and actor value names, quest names | The game's own records, already in the player's language (strings files) | Nobody: we read it through the engine, never copy it into our files |
| **Our fixed words** | `[Leave]`, hint labels, tab names, settings labels, "CND" | `$KEY` strings in our movies, swapped by the game from `Interface\Translations\<Mod>_<language>.txt` | Translators, one file per language |
| **Text we build** | `[Speech 45/50]`, "Repair with: Laser Pistol (62%)" | A template under a `$KEY` with named slots (`[{skill} {have}/{need}]`), filled by our `Text` module; the slots take game data | Translators translate the template; word order is theirs |
| **Other mods' text** | A tab or item-card line another mod registers | That mod passes either game text or its own `$KEY` | That mod's translators, in that mod's file |
| **Settings pages** | MCM page names and help | `$KEY`s in MCM's config, the same translation files | Translators, same file |
| **Plugin records** | Messages, keywords' names | Keep player-visible words out of our plugins; where a record must hold text, use a `$KEY` | Same file |
| **Converted FNV text** | Casino labels from FNV's settings | The player's FNV install, in its own language. The converter must write it under the player's FO4 `sLanguage`, not always `_en` as the [casino design](fo4_casino_plan.md#design-text) says, or a German game misses it | Nobody: it follows the source game |

Papyrus has no translation of its own, so scripts pass `$KEY` strings, never
English sentences, to anything shown on screen.

### Files

- `Interface\Translations\<plugin>_<language>.txt`: UTF-16 LE with BOM, one
  `$KEY<tab>text` per line (F4SE `Translation.cpp`, V). `<language>` is the
  game's `sLanguage` (`en`, `de`, `fr`, `ru`, …). **`<plugin>` must be an
  active plugin's name:** F4SE reads `plugins.txt` and, for each `*` line
  ending `.esm`, `.esp` or `.esl`, loads that plugin's file through the
  game's file system (loose or BA2), once, when the translator is made (F4SE
  0.7.9 `ImportTranslationFiles`). A file named after anything else, such as
  a menu, is never read. Lines start with `$`, split at the last tab, and
  stop at 511 characters; no BOM means the file is skipped.
- **Ours are named after `FalloutHelpers.esl`**, which every converted
  FO3/FNV plugin masters: `FalloutHelpers_<language>.txt` holds every
  `$F3UI_` and menu key (`fo4_import/base/translations.py`).
- In the repo: one **English source** file per release in UTF-8, so changes
  show in diffs (`fo4_import/base/translation_files/en.txt`). The build
  writes the UTF-16 file for every game language, English filling any gap
  (`translations.install`, in the masterless build; [fallback](#translations)).
- Translations arrive as one file per language and ship in the main
  download, or as separate Nexus translation uploads that hold only that
  file.

### Fallback

Spike 5 settled it: a key no file has shows as the raw `$KEY`, and F4SE
loads only `FalloutHelpers_<sLanguage>.txt`. **Built 2026-11-12, instead of
the DLL table once planned:** `translations.install` writes a file for every
one of FO4's 11 languages (`cn de en es esmx fr it ja pl ptbr ru`, the
game's own `Interface\translate_<language>.txt` set), each the English table
with that language's own source laid over it. A German game with no German
translation shows English, and a partial translation shows English for the
keys it lacks. A translator's mod replaces our file for its language by
load order. No DLL work is needed.

### <a id="keys"></a>Our keys (moved 2026-11-12, built, not played)

Every fixed word our menus show is a key in `en.txt`, UPPER_SNAKE with our
prefix (`$F3NV_REPAIR`). Upper case because the dialogue menu, and maybe the
item card, uppercase their text before the game translates it. The English
text is what showed before, word for word.
- **Movies:** the dialogue menu's `[LEAVE]` (`$F3NV_LEAVE`) and its hints
  (`$F3NV_HINT_SELECT`, `$F3NV_HINT_LEAVE`). `$Controller_Buttons` is a
  font name, not a key, and the check skips it.
- **F3NV.dll** (`pipboy.cpp`, `ammo.cpp`): the CND line (Pip-Boy, container
  and barter), the REPAIR and AMMO hints and card labels, the repair list,
  vendor offer and ammo list headings, notes and keys, and the "Not
  needed" / "No spare" / "Beyond the vendor's skill" / "One type only" /
  "Drop a copy first" notes. The game translates them where they are shown:
  the button hint bar, the item card (vanilla's own lines are keys such as
  `$wt`), and our list panel.
- **Built text** goes as `$KEY<tab>slot0<tab>slot1` (C++ `Fill`); f3ui's
  `Text.show` translates the key (read back through a scratch field, as
  spike 5 read translations back) and puts each slot in for `{0}`, `{1}`:
  `$F3NV_CAPS` is `{0} caps`, `$F3NV_LOADED` is `{0} (loaded)`. Item-card
  lines (drawn by the game's card) never use slots.
- **Left as they are:** the casino's words, which are FNV's own settings in
  the player's language; the Hardcore HUD's `H2O`/`FOD`/`SLP` and the
  `[Speech 25]` tags, FNV's own formats drawn from C++; the casino's
  developer-only spike screen (Ping/Close); and the `[Leave]` topic the
  converter writes into plugins (`conversation.py`), which is record text.
- **Untested:** whether a key read back through an off-stage field
  translates (the slot templates rely on it), and whether the vanilla and
  FallUI item cards and the hint bar translate our keys as they do the
  game's.

### Fonts

- Use only `$MAIN_Font`, `$MAIN_Font_Bold`, `$Terminal_Font` and the other
  names `FontConfig.txt` maps. Each language's font library supplies the
  letters, so Cyrillic, Polish and Chinese, Japanese and Korean work without
  us shipping fonts.
- Never embed a font of our own: it would lack other languages' letters.
- Controller glyphs come from `$Controller_Buttons`, which is the same in
  every language.

### Layout

- No fixed text widths. German runs about a third longer than English, and
  CJK text breaks lines differently. `Text` measures the text and wraps, or
  shrinks within limits, and lists scroll horizontally only as a last resort.
- A **test language**: a tool writes a fake translation with every string
  lengthened and accented (`[Lëävë~~~~]`), installed as a language file. One
  look in game shows any text that is cut off or never translated.
- Right-to-left scripts: the game doesn't support them; out of scope.

### Checks

**Built 2026-11-12:** `translations.check()` reads every `"$KEY"` string
(or a template's `"$KEY\t`) in our movies' AS3 (each `as3/` folder under
`fo4_import/`) and F3NV.dll's C++ (`tes_runtime/fo4_condition/*.cpp`),
`//` comment lines and the font names `$MAIN_Font`/`$MAIN_Font_Bold` left
out, and finds:
- keys the English file lacks (error: the game shows them raw);
- English keys no movie uses (review);
- keys another language has that English lacks (error).

Preflight's `menus` audit reports them for every FO4 build
(`menus.translation_audit`), and `test_our_movies_use_only_keys_the_english_file_has`
holds the repo to none. `install` writes nothing while `en.txt` has no
keys. The masterless build installs the files beside the Pip-Boy movie
(`build_finish`), no longer only with the casino.

Still planned:
- keys in Papyrus sources too, once those pass `$KEY`s to a menu;
- each language file has the same `{slots}` as English;
- a count of untranslated keys per language for the release notes.

### Compatibility

- XDI ships translations (for example Ukrainian), and players will expect
  ours too. Our dialogue menu shows the game's own text for options and
  lines, so only our few fixed words need translating.
- Key names carry the mod's prefix (`$F3UI_Leave`), so two mods' files
  never fight over a key.
- Never ship a copy of the game's own `Interface\Translate_<lang>.txt`.
  DEF_UI does, and must redo it after every game update; our own files
  can't go stale that way.

## <a id="controller"></a>Controller support

Every menu must be fully usable with a gamepad, the same as with keyboard
and mouse: every action reachable, every hint showing the right button. A
feature that works only on keyboard is not finished.

### Where we stand

| Menu | Gamepad today | Gap |
|---|---|---|
| Dialogue menu | On `Input` and `HintBar` (built 2026-11-12): D-pad (`QuickkeyUp/Down`) moves, A picks, B leaves, hints under the list | Never played with a pad |
| Casino | On `Input` and `HintBar` (built 2026-11-12): A spins, the D-pad changes the bet, Y bets the most on slots, B leaves; the gamepad cursor flag is set | Never played with a pad; roulette's board is mouse-only |
| Pip-Boy repair and ammo | **None.** Keyboard only (G, V); hints hidden on a pad, because every gamepad button is taken in the Pip-Boy ([condition](fo4_condition.md#pipboy)) | A way in on a pad |
| STAT tabs | The game's own tab switching | Check with a pad |

### <a id="input"></a>`Input` and `HintBar` (built 2026-11-12, not played)

**What the game gives (V, read 2026-11-12, nothing copied):**
- **Menu-mode controls**, from the control map in `Fallout4.exe` (the game
  keeps it there, not in an archive): `Accept` (Enter, A), `Cancel` (Tab,
  B), `Up`/`Down`/`Left`/`Right` (arrows, D-pad), `XButton` (R, X),
  `YButton` (T, Y), `LShoulder` (C, LB), `RShoulder` (Q, RB),
  `LTrigger`/`RTrigger` (the triggers only), `Select` (V, Back), `L3` (Z),
  `R3` (X). The dialogue has its own block: the arrows and number keys send
  `MultiActivateA`-`Y`, and the D-pad sends `Quickkey*`.
- **Platforms** (`SetPlatform`): 0 keyboard and mouse, 1 a PC gamepad, 2
  Xbox, 3 PlayStation. A PC gamepad shows Xbox buttons.
- **Button glyphs:** `$Controller_Buttons` (the game's `fontconfig.txt` maps
  it to `fonts_console.swf`) draws each button as one letter. Xbox: A `A`,
  B `B`, X `C`, Y `D`, LB `G`, RB `L`, the D-pad `T`/`U`/`V`/`W` (left,
  right, down, up). PlayStation: cross `a`, circle `d`, square `c`, triangle
  `b`, L1 `g`, R1 `m`, the D-pad `t`/`u`/`v`/`w`. Read from the game's own
  button hint class (decompiled from DEF_UI's copy into the scratchpad,
  read only).

**`f3ui.Input`** (`fo4_import/base/as3/f3ui/Input.as`): one map from keys
and the game's controls to actions: `up`, `down`, `left`, `right`, `accept`,
`back`, `prev`, `next`, `x`, `y` and `digit` (the keys 1 to 9).
- Moves fire on the press (so a held key repeats); accept, back and the
  rest fire on the release, as both menus did before.
- A press that arrives as both a key and a control fires once (the same
  action from another source within 150 ms is dropped). The wheel
  (`ZoomIn`/`ZoomOut`) moves once a step, whether it comes as a press, a
  release or both.
- Nothing fires for 250 ms after `open()`, so the key that opened a menu
  doesn't act in it.
- Keys typed into an input text field are ignored (FallUI's search).
- **The device:** the game's `SetPlatform` wins once the game has sent it.
  Until then a key means the keyboard, and a control with no key within
  150 ms of it means a gamepad. Whether the game sends `SetPlatform` to an
  F4SE custom menu, whether a pad also sends key events, and whether
  Scaleform sends frame events to the off-screen object that times the
  check, are untested: each change of device is logged
  (`input device <n>`).
- A menu adds its own: `mapKey(code, action, name)`,
  `mapControl(name, action, gamepadOnly)`.

**`f3ui.HintBar`** (`HintBar.as`): a row of `[actions, label]` hints. On a
keyboard each shows the key's name (`ENTER) SPIN`, as our Pip-Boy hints),
on a gamepad the button's glyph; actions shown together share one hint
(`LEFT/RIGHT) -/+`, or two glyphs). It redraws when the device changes,
and clicking a hint fires its action. Labels may be `$KEY`s.

**The menus on it:**
- **Casino:** its own key table is gone. Back closes; the games take
  accept (spin), left and right (the bet) and, on slots, `y` (bet max, so a
  pad can reach every button). The hint bar runs along the bottom with the
  games' own words (FNV's settings: Spin, Bet Max, Exit, Increase and
  Decrease Bet). Roulette's board is still mouse-only.
- **Dialogue menu:** W, S and E, the `Quickkey` D-pad, `Activate`, and on
  a gamepad only `MultiActivateA`/`B` are its additions. The hints
  (`$F3NV_HINT_SELECT`, `$F3NV_HINT_LEAVE`) sit under the box, inside the
  tinted holder, so they take the HUD color and the shaded backdrop.
- `tests/test_fo4_f3ui_input.py` holds the glyph table and the control
  names to the game's, and both menus to taking keys only through `Input`.

**Play check:** each menu once on keyboard and once with a pad, mouse and
keyboard untouched: the hints change between key names and glyphs when the
device changes, every action works, nothing fires twice (one bet step per
press, one pick), and the log shows `input device` lines.

### Rules for the library

- **Actions, not keys.** `Input` maps keys, mouse and `ProcessUserEvent`
  control names to one set of actions (up, down, accept, back, page left
  and right, more). Menus only ever see actions, so no menu can forget the
  pad.
- **Follow the device as it changes.** The game reports the platform
  (`SetPlatform` / `uiPlatform`), and players switch between mouse and pad
  mid-menu. Hints, cursor and focus follow at once.
- **Hints from the action.** `HintBar` draws each action's current button:
  key names on keyboard, glyphs from the `$Controller_Buttons` font on a pad
  (Xbox or PlayStation layout as the game reports). A hint for an action
  the device can't do is a bug.
- **Focus always visible.** On a pad there is no pointer, so one row or
  button is always highlighted, and lists scroll to it.
- **No mouse-only actions:** nothing that needs hover, drag or a right click.
- **Remapping:** use the game's control names, so a player's remapped
  controls carry over; our own keyboard keys are settings.
- **Steam Deck** counts as a gamepad: test it, or at least a pad on PC
  with mouse and keyboard unplugged.

### <a id="remap"></a>The game's Controls page (researched 2026-11-12)

Whether our own actions (the Pip-Boy's Repair and Ammo keys, today fixed
at G and B) could be listed in Settings → Controls. Read from the game,
for reference only: `mainmenu.swf` decompiled, the 1.11.240 exe's strings,
the player's saved remaps, and F4SE 0.7.9's `GameInput.h`.
- **The page is drawn by the engine's list.** `MainMenu.as` asks
  `BGSCodeObj.RequestInputMappings(entryList)`, sorts on `sortIndex`, and
  shows each entry's `text` as `"$" + text`: the label is a translation
  key named after the control, so our own `translate_*` lines could name
  ours. Picking a row calls `StartRemapMode(text, list)`, and the engine
  answers `onFinishRemapMode` ("That button is reserved" on a clash).
  `ResetControlsToDefaults` is the engine's too. Nothing in the movie
  decides which controls are listed.
- **The default map is built into the exe**, a tab-separated table:
  control name, keyboard key, mouse button, gamepad button, then whether
  each of the three can be remapped, then the control's group flags. The
  exe names no loose `ControlMap.txt`, so unlike Skyrim a mod can't add a
  control with a text file.
- **The player's remaps** are `ControlMap_Custom.txt` (INI
  `sControlsRemapFile`) under `My Games\Fallout4`: binary pairs of a button
  and a number, apparently each control's place in the built-in table,
  not its name.
- **At run time** the map is `InputManager` (F4SE `g_inputMgr`): one context
  per input mode, each with keyboard, mouse and gamepad arrays of
  `{name, buttonID, sortIndex, two flags}`. F4SE reads it
  (`GetMappedKey(name, device, context)`; Papyrus `Input.GetMappedKey`),
  so a DLL can read whatever key a control is bound to now.

**What that allows.** Appending our controls to the gameplay context's
arrays as the game loads, before the Controls page or the saved remaps
are read, would list them on the page with our labels; our DLL would then
read their keys through `GetMappedKey` instead of fixed ones. Unverified
and with real risks, so not planned yet:
- which entries `RequestInputMappings` lists (likely the remappable flag
  among the two unknown flags) and whether it walks only the gameplay
  context;
- whether the saved remaps survive: they look like positions in the table,
  so ours must go after every built-in entry, and a remap of ours saved
  while our DLL is installed points past the table's end once it's removed
  (ignored or a crash: test before shipping);
- a control in the gameplay context is also live in gameplay: a key bound
  to it fires our control's user event there, which nothing handles, but
  the page also stops the player binding it to anything else;
- another mod doing the same would need the same order every launch.

**Built instead, first** ([keys](#keys)): our keys as settings in
`CleanroomUI.ini`, which an MCM page can write. MCM's own
hotkeys don't fit: they run scripts as global shortcuts and don't fire
while the Pip-Boy or another menu is open, where our keys are used. Our
menus' moves, Accept and Back already follow the game's controls through
`Input`'s `mapControl`, so a player's remaps carry over there today; the
fixed keyboard keys `Input` also maps are fallbacks.

### <a id="keys"></a>Our key settings (built 2026-11-13, not played)

The keys the game has no controls for are settings (`dll/framework/keys.cpp`,
framework version 6), under `[Keys]` in `Data\F4SE\Plugins\CleanroomUI.ini`:
```ini
[Keys]
; A letter, a digit or F1 to F12.
sRepair=G     ; Pip-Boy: open (and close) the repair list
sAmmo=B       ; Pip-Boy: open (and close) the ammo list
sHudReset=R   ; HUD edit menu: put the widget under the cursor back
sHudHide=H    ; HUD edit menu: hide the widget under the cursor, or show it
sHudColor=C   ; HUD edit menu: give the widget under the cursor the next color
sHudPeek=Y    ; in play: show the HUD parts shown on conditions (held, then the linger)
sHudShow=V    ; HUD edit menu: set when the widget under the cursor is shown
```
- `Data\MCM\Settings\CleanroomUI.ini` is read after it and wins: that is
  where an MCM page saves a setting with id `sRepair:Keys` and so on, so a
  mod's MCM page can offer them with no code of ours.
- They are read as the plugin loads and again each time the Pip-Boy or the
  edit menu opens, so a change applies the next time either opens.
- The Pip-Boy's REPAIR and AMMO hints and the edit menu's hint
  (`$CRUI_HUD_EDIT_HINT`: `{0}` reset, `{1}` hide, `{2}` color, `{3}`
  when shown) show the keys set.
- A key that isn't a letter, digit or F1-F12 keeps its default, as do
  `sRepair` and `sAmmo` set to W, S or E (the lists move and choose with
  them) or to the same key, and two of the edit menu's four set to the
  same key (all four go back to R, H, C and V); each logs a warning.
- Not checked: the game's own Pip-Boy keys. A key the Pip-Boy page also
  uses does both (V zooms the Pip-Boy, which is why the ammo key is B).

### The Pip-Boy problem

Item actions (our Repair and Ammo, and every action other mods register
through the framework) can't each have a button on a pad. Design:
- **One "more" button opens an action list** for the highlighted item:
  every registered action that applies, in a `ScrollList` pop-up. One
  button for all mods, so actions never fight over buttons.
- **Which button** is a research item: candidates are a long press on a
  button the page already uses, a stick click, or a shoulder-plus-button
  combination. Read what the inventory page leaves free on a pad, and check
  what FallUI and DEF_INV already use, before choosing.
- On keyboard, actions keep their own keys (G, V) as well as the list.

### Testing

- Each spike and each release is played once with a controller, start to
  finish, with mouse and keyboard untouched.
- The menus' self-test logs record which device drove each action, so a
  report shows whether the pad path ran.
- VR's motion controllers come later, with the [VR build](spin_offs.md#platforms).

## <a id="testing"></a>Testing

- No headless Flash player runs our movies. Keep logic in plain classes
  that Python reference models can mirror (as the casino does), and keep
  the movies thin.
- A **movie check** in preflight: every class we ship comes from our
  source; no tag imports assets from a Bethesda movie; font names are ones
  `FontConfig.txt` maps; the build is byte-stable.
- One in-game test scene per mode: own menu, replaced movie, injected
  overlay, each with a self-test log line.

## <a id="spikes"></a>Spikes, in order

Each spike is one change, played in game.

1. **`mxmlc` build:** the casino spike built by `mxmlc` (tags stripped)
   behaves the same and builds byte-identical twice.
   - **Played 2026-11-11: passed.** Slots and roulette both work on the
     `mxmlc`/`f3ui` movie (user: "both work fine with the custom menu
     work"). The movie played was zlib-compressed; it is now written
     uncompressed ([why](#mxmlc-setup)), the same format as the dialogue
     menu's own movie, which also loads in game.
   - **Built 2026-11-11.** The casino now compiles with `mxmlc`
     on the library's first five classes:
     - `Bridge`: the f4se object, `send`, `natives`, `mount`, `plain`;
     - `Theme`: `color`, `fromHud`;
     - `Host`: `focus` (focus rectangle off);
     - `Text`: `field`, `label`;
     - `Draw`: `button`, `panel`.

     The casino's own `Ui.as` is gone; its screens and document class call
     the library. Nothing else in the casino changed: same calls, keys,
     drawing and colors.
   - **Checked offline:** strict compile, no warnings. Byte-identical
     twice, from any folder or worktree. Tags: 69, 65, 9, 43, 82, 76, 1, 0. FFDec
     decompiles only our 11 classes (6 casino, 5 `f3ui`). A type error
     FFDec let through (`var a:int = "one"`) now fails the build
     (`tests/test_fo4_casino.py`).
2. **HUD color in our own menu.** One change per play-test; each needs a
   game restart, because F4SE keeps a menu's first registration.
   - **2a:** flags `0x800849D` (Custom Rendering back on, Assign Cursor To
     Renderer still off), extended `3` (Inherit Colors + gamepad cursor).
     Why it may work: F4SE pushes the HUD color to a custom menu's shader
     targets only with Custom Rendering on, and Assign Cursor To Renderer is
     the flag that hands the cursor to the custom renderer, the likelier
     cause of the box. The menu needs drawing in white for the tint to show,
     as in the dialogue menu. Only `F3NV_Casino.psc` changes, so it can be
     tested as a one-script patch on the built plugin.
   - **2a result (played 2026-11-11): failed.** With `0x800849D` / `3` the
     slot machine menu didn't open at all (the box didn't come back; the
     menu never showed). The original `.pex` was restored. Per the plan, no
     further flag tries.
   - **2b: works. The way our own menus get the HUD color.** The menu
     colors itself; no engine tint, no flag change (menu flags stay
     `0x800049D` / `2`).
     - **How** (commit `df3f9972`, in F3NV.dll from `2d3fc70c`): a
       `HudColor()` native on `f4se.plugins.F3NV` (every movie already gets
       F3NV.dll's natives) returns the player's `iHUDColorR/G/B`, read
       fresh on each call through the same `HudColor` the Hardcore HUD
       uses. The casino sets `Ui.color` from it each time a screen opens,
       and every outline and text that was white uses `Ui.color`. Only what
       we draw takes the color: reel symbols, card faces and roulette
       pockets share containers with the text, so a color transform on a
       whole layer would tint them too.
     - **Played 2026-11-11** (new F3NV.dll + casino movie, old casino
       script; slots only, roulette waits for the full build): slots open
       and play, drawn in the HUD color ("Slots work in the HUD color
       now"). The log shows `F3NV_Casino: input hud color f774f7` on both
       openings, the same color the Hardcore HUD read at load (`F774F7`
       in F3NV.log).
     - **Screenshot (The Tops, slots):** reel symbols and payout icons keep
       their own colors; text, button outlines and borders are in the HUD
       color. The buttons' 10% fill shows as a see-through tint: with the
       menu coloring itself, translucent fills work, unlike inside an
       engine-tinted holder (the dialogue menu's finding).
     - **Not checked yet:** the menu following a HUD color changed in
       Settings (both openings had the same color). The fallback path (no
       F3NV.dll, white) wasn't tried.
     - **Seen in the same play, fixed by the casino session** (`fd59ed6f`,
       built, not played; [casino notes](fo4_casino_plan.md#hud-hidden)):
       the game's HUD showed through behind the menu, and Flash drew its
       yellow focus rectangle round the whole screen. Both fixes belong in
       the `Host` module ([what we know](#facts)).
     - **Limit:** a color changed while the menu is open shows on the next
       opening. Fine for the casino: its flags include Disable Pause Menu
       (`0x80`), so the settings can't be opened while it is up.
   - **2c: not needed.** Real engine shader targets from the DLL
     (CommonLibF4's `BSGFxShaderFXTarget`) stay unbuilt unless a menu ever
     needs the engine's live tint.
3. **Replaced dialogue movie:** five empty holders and a text list as
   `DialogueMenu.swf`; does the engine tint them?
   - **Played 2026-11-11: works.** The engine tints holders made in code:
     the list showed in the HUD color ("Hud color matches the text box").
     Opening, picking, leaving and the talk going on all worked "nearly as
     well" as the patched menu, without its styling. One bug: `[Leave]`
     showed twice. The converted topic list already ends in its own
     `[Leave]` option (`conversation.py`), and the spike added one more;
     the shipping menu adds its own only when F3NV.dll has no options.
     Fixed in the spike source, not replayed. So the dialogue menu can drop
     the patched Bethesda movie: the XDI replacement starts from our own
     movie.
   - **Played 2026-11-12: works.** The shipping dialogue menu on its own
     movie plays fine and looks the same as before (user).
   - **Done 2026-11-11:** the shipping dialogue menu moved
     onto its own movie ([dialogue menu plan](fo4_dialogue_menu_plan.md#own-movie)).
     Spike 3 also settles two open facts: `try/catch` ran in game (the
     spike's placement used it), and `scaleform.gfx.Extensions` needs no
     stub to compile against when looked up by name.
   - **Built 2026-11-11.** Two AS3 classes, no Bethesda code or
     art: a root (`DialogueMenuRoot`) that holds the menu at `root.Menu_mc`,
     as the game's own movie does (its `MainTimeline` has only `Menu_mc`),
     and `f3ui.dialogue.DialogueMenu`, which answers the game's calls
     (`onCodeObjCreate` → `registerObjects` of five code-made holders,
     `SetButtonText`, `Show/HideButtonHelp`, `Enable/DisableMenu`,
     `ProcessUserEvent`, `SetSafeRect`, `SetPlatform`) and its own base
     class's job (focus rect off, safe-area placement). One white list in the
     Question holder: F3NV.dll's options, else the game's four buttons, plus
     `[Leave]`. `PlaySpeechChallengeAnim` has no animation and reports done
     on the next frame. No shaded background or failed-check tint (one
     variable). `mxmlc` output 3,440 bytes stripped, byte-identical twice;
     no font tags (`$MAIN_Font_Bold` by name).
4. **Pip-Boy injection:** a `Loader` movie in `root.Menu_mc` that adds one
   item-card line, under vanilla and under FallUI Inventory.
   - **Played 2026-11-12: works** (vanilla Pip-Boy). The inventory cards
     show `F3UI  SPIKE 4` (user). F3NV.log: `Pip-Boy: F3NV_Pipboy.swf
     loading`, `loaded into [object PipboyMenu] (F3NV.dll found)`, `added
     our line to an item card (1 lines)`, then `(9 lines)` on a weapon's
     card. The second opening loaded it again: the game makes a new
     Pip-Boy menu each time, and our movie comes with it. **Not yet tried
     under FallUI Inventory.**
   - **Built 2026-11-12.** Two pieces, no Pip-Boy movie of the
     game's shipped or read:
     - **F3NV.dll** (`pipboy.cpp` `LoadOverlay`): in the hook that already
       runs once per Pip-Boy menu (the `fnvConditionHooked` flag), it makes
       a `flash.display.Loader`, adds it to the menu instance
       (`root.Menu_mc`) and loads `F3NV_Pipboy.swf` (a path under
       `Data\Interface`, as HUDFramework's widgets load), the same calls as
       F4SE's `UI.Load`. Logs `Pip-Boy: F3NV_Pipboy.swf loading`.
     - **The movie** (`fo4_import/pipboy_overlay/as3/F3NV_Pipboy.as`,
       `mxmlc` with `f3ui`, 2,325 bytes): it finds the menu as its Loader's
       parent and F4SE's object on the Pip-Boy movie's root. Each frame it
       puts a line `F3UI  SPIKE 4` back on the shown page's
       `ItemCard_mc.InfoObj` when the game's refill dropped it, then calls
       `onDataChange()`, as `pipboy.cpp` does for CND today. It draws
       nothing and takes no input. It logs through F3NV.dll's `Log`
       native, so its lines read `dialogue menu: Pip-Boy movie: …` in
       F3NV.log (that native's prefix).
     - The masterless build installs `Interface\F3NV_Pipboy.swf` (count
       `Pip-Boy movie installed`).
   - **Checked offline:** `pipboy.cpp` compiles with the DLL's flags, no
     warnings; the movie builds the same on Python 3.13 and 3.14; the name
     the DLL loads is the name the build installs
     (`tests/test_fo4_pipboy_overlay.py`).
   - **Still to play:** the same under FallUI Inventory. The spike's test
     line is gone; the framework's CND line is what to look for now.
5. **Translation files:** the spike 1 movie shows one `$KEY` from our own
   `_en` file, then the same with `sLanguage` set to a language we have no
   file for. Records what the game shows, which settles the
   [fallback](#translations) design. Can ride along with spike 1.
   - **Played 2026-11-12: the translation works; one drawing bug.**
     - f4se.log: `Reading translations from
       Interface\Translations\FalloutHelpers_en.txt...`. The log: the
       present key reads back `Translated from FalloutHelpers_en.txt`, the
       absent one reads back `$F3UI_TranslationAbsent`. **So the game shows
       a key no file has as the raw `$KEY`**, which settles the
       [fallback](#translations): `Text` must never show an untranslated
       key itself.
     - **Bug:** both lines drew one small mark per letter (10, 4 and 21
       marks on the left: the words of the translation; 23 on the right:
       the raw key). The buttons beside them, made by the same
       `Text.field` (same font name, embedded fonts on), drew fine, and so
       do roulette's size 11 and 14 labels, so neither the font name nor
       the size is the cause. The only difference: these two texts start
       with `$` and go through the game's translator. Likely cause: the
       translator's result replaces the text without the field's default
       format, so it falls back to a font that isn't embedded, and the
       game draws a mark for each missing letter.
     - **First fix, played 2026-11-12: failed** (looks the same).
       `Text.show` reapplied `setTextFormat(defaultTextFormat)` after the
       assignment. Log, both keys: `font Times New Roman then Times New
       Roman`, so after the `$` assignment even the field's default format
       read as Times New Roman (Flash's default font, which the game
       doesn't embed).
     - **Second try, played 2026-11-12: works.** All three lines draw as
       text. **Cause, confirmed by the log:** assigning a string that starts
       with `$` to a code-made field resets both its text format and its
       `defaultTextFormat` to Times New Roman, whether or not the key is
       translated; plain text keeps `$MAIN_Font`. So f3ui puts text in a
       field only through `Text.show`, never `field.text = "$…"`:

       | Line | After a plain assignment (its default) | After `Text.show` |
       |---|---|---|
       | Key in our file | Times New Roman (Times New Roman) | `$MAIN_Font` |
       | Key in no file | Times New Roman (Times New Roman) | `$MAIN_Font` |
       | Plain text | `$MAIN_Font` (`$MAIN_Font`) | `$MAIN_Font` |
     - **What was built for it:** `Text.field` keeps its
       own copy of each field's format; `Text.show` assigns the text, then
       puts that copy back as the default and over all of the text.
       `Draw.button` uses it too. A third, plain line (no `$`) is the
       control. Each line logs `font: default <before>, plain <after a
       plain assignment> (default <its default then>), shown <after
       Text.show>`. Movie 19,014 bytes.
     - **Spike done.** The three test lines and `$F3UI_TranslationCheck`
       were removed 2026-11-12; the [check](#translations) now keeps keys
       and the English file in step.
   - **How it was built:** the file is
     `Interface\Translations\FalloutHelpers_en.txt`, since F4SE loads only
     files named after an active plugin ([files](#translations)). There was
     no `sLanguage` change: one key in our file and one in none showed both
     cases in one play.
6. **Level-up images** (built 2026-11-12, not played): the casino's spike
   screen shows DXT3, DXT5 and a name with a space beside the DXT1 card
   ([level-up](#level-up)).

### <a id="next"></a>Next, in order

Played first, since everything below builds on them (the morning test of
2026-11-13): the Pip-Boy CND line, lists and item-card API, under vanilla
and FallUI; the translated keys; the dialogue menu's `mxmlc` build; the
level-up image spike; the casino and dialogue menu on `Input` and
`HintBar`, with a pad ([play check](#input)); the dialogue list and the
Pip-Boy lists on `ScrollList` and `Modal` ([play check](#modal)); the
casino's images through `Images`, with the new F3NV.dll
([play check](#images)); the level-up menu ([play check](#level-up-play)).

Then:
1. **`Input` and `HintBar`** in the library, with the casino and the
   dialogue menu moved onto them, played once with a pad
   ([input](#input)). **Built 2026-11-12; the pad play is left.**
2. **`ScrollList` and `Modal`**, taking over the Pip-Boy's `ListPanel` and
   the dialogue list. **Built 2026-11-12, not played.**
3. **Replacing a vanilla menu by name** (the level-up menu in place of
   `LevelUpMenu`), then the level-up menu itself
   ([order](fo4_level_up_plan.md#order)). **Built 2026-11-12 (opening,
   skills, perks, taken, tags, traits).**
4. **The HUD widget**: Hardcore's needs as our movie, hosted by
   HUDFramework or injected by the DLL ([HUD](#hud)).
5. **The container and barter CND line** through an injected movie, as the
   Pip-Boy's, retiring that C++.
6. The [movie check](#testing) in preflight, then the
   [release](#release) pieces.

## <a id="release"></a>Release

- **The library** as its own GPL-3.0 repo (with the modding exception, so
  menus built on it can use any license) with an example menu and docs: for
  FO4 authors who want a menu without decompiling Bethesda's files.
- **The Pip-Boy framework** as a DLL + movie on Nexus, with the Papyrus API
  and INI format documented.
- The dialogue menu and the converter use both.

## <a id="decisions"></a>Open decisions

- Where built movies live: release assets only, or an exempt folder in git.
- Library and Papyrus prefix names.
- Whether the Pip-Boy framework reads PipboyTabs' INI files.
- Whether to contact HUDFramework's author (license, the F4SE fast path),
  together with the XDI question.
