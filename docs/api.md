# Cleanroom UI for mod authors

What your mod can use, and how. The [reference](#reference) at the end is
written from the sources by `python -m crui api`, so it always matches them;
everything above it is by hand.

## What your mod needs

- **F4SE**, and Cleanroom UI installed: `CleanroomUI.esl`, `CleanroomUI.dll`
  (and `CleanroomDialogue.dll` for the dialogue menu), the scripts and the
  movies. Never ship any of them in your mod; require Cleanroom UI instead.
- **Your scripts compile against ours**: put Cleanroom UI's `papyrus/` on
  the compiler's import path. Ship only your own `.pex`.
- **A master only if you use its forms.** Most of the API is scripts and
  natives; `CleanroomUI.esl` is needed as a master only for its shared forms
  (the level-up holder's).
- **Which version is installed:** `CRUI_Framework.GetVersion()` in Papyrus;
  from another F4SE plugin, send the message `'CRUV'` to `"CleanroomUI"` and
  the answer comes back with the version as a `std::uint32_t`.

## <a id="example"></a>Your own menu: the example

`examples/example_menu/` is a whole menu to copy: a panel in the player's
HUD color, a title from its INI, a list of three items and button hints,
worked by mouse, keyboard or a pad.

| File | What it is |
|---|---|
| `as3/ExampleMenu.as` | The movie: the library's `Bridge`, `Theme`, `Text`, `Draw`, `ScrollList`, `Input` and `HintBar` |
| `papyrus/ExampleMenu.psc` | A quest script: registers the menu with F4SE, opens it (`ExampleMenu.Open()`, or `cgf "ExampleMenu.Open"` in the console), and hears its picks |
| `ExampleMenu.ini` | Its settings, read in the movie with `bridge.settings("ExampleMenu")` |

`python -m crui example` builds it into `dist/ExampleMenu` (movie, script
and INI); attach the script to a quest of your plugin that starts with the
game. To make it yours, copy the folder and rename `ExampleMenu`
everywhere.

To build a movie of your own without our build, compile it with the Apache
Flex SDK's `mxmlc` and add our library to the source path:
`-source-path+=<Cleanroom UI>/as3`
([our settings](swf_framework.md#mxmlc-setup); `python -m crui.flex <folder>`
installs the SDK for you). Use only the game's font
names (`$MAIN_Font`, `$MAIN_Font_Bold`), never one of your own, so every
language has its letters.

## What else your mod can do

- **Put a movie into a game menu** without a script: list it in
  `Data\F4SE\Plugins\CleanroomUI\Menus\<your mod>.ini`
  ([how](swf_framework.md#inject)). Your movie finds F4SE's object with
  `bridge.attach(stage)`.
- **Read your settings in a movie:** `bridge.settings("<your mod>")` gives
  `Data\F4SE\Plugins\<your mod>.ini` with MCM's saved copy over it
  ([how](swf_framework.md#settings)).
- **Items that wear:** `CRUI_Condition` reads and writes an inventory
  stack's condition and shows it on the Pip-Boy, container and barter item
  cards; repair lists and vendor repairs ([plan](swf_framework.md#pipboy)).
- **Lines on the Pip-Boy's item card:** `CRUI_ItemCard.SetLine`.
- **Ammo lists:** `CRUI_PipboyAmmo.SetLists`, with the Pip-Boy's ammo list.
- **STAT tabs on the Pip-Boy:** `CRUI_Pipboy.SetTab`.
- **HUD widgets:** `CRUI_Hud.AddWidget` (your movie, which implements
  `processMessage(command, params)`) or `AddStats` (a line of actor values),
  placed by the player in our edit menu ([HUD](swf_framework.md#hud)).
- **The level-up menu:** a host quest of your own on `CRUI_LevelUp`, your
  skills added with `AddSkill`, and an MCM page for its options written by
  `crui.mcm` ([level-up](level_up_plan.md#holder)).
- **Words in your movies** are your own keys in your own
  `Interface\Translations\<your plugin>_<language>.txt`; ours are `$CRUI_`.

## Our settings

`Data\F4SE\Plugins\CleanroomUI.ini`, with MCM's copy
(`Data\MCM\Settings\CleanroomUI.ini`) over it:
- `[Keys]`: the Pip-Boy's repair and ammo keys and the HUD edit menu's
  keys ([keys](swf_framework.md#keys));
- `[Hud]`: `sPreset`, `sShow`, `fShowLinger` ([HUD](swf_framework.md#hud-conditions));
- `[Log] iLevel`: 0 errors only, 1 warnings, 2 messages (the default), 3
  everything; the same in `CleanroomDialogue.ini` for that DLL.

## <a id="reference"></a>Reference

<!-- api:start -->

## Papyrus

### CRUI_Array

xNVSE arrays for converted scripts. An array is a Var[] whose element 0 is its kind: 1 Array (a list, keys 0..n-1, values from element 1), 2 Map (numeric keys) or 3 StringMap; a map stores key, value pairs from element 1, kept in key order as xNVSE iterates them. None is xNVSE's null array. See: docs/fork/nvse_script_language.md#arrays

- `Var[] Function Construct(String asKind) Global`: ar_Construct: a new empty "Array", "Map" or "StringMap".
- `Var[] Function FromForms(Form[] akForms) Global`: An Array holding akForms, for a native that returns a Papyrus array where xNVSE returned one of its own.
- `Var[] Function Box(Var akValue) Global`: The & operator: a one-element Array.
- `Var[] Function Copy(Var[] akArray) Global`: ar_Copy: a new array with the same kind and elements (nested arrays shared).
- `Int Function Kind(Var[] akArray) Global`: 1 Array, 2 Map, 3 StringMap; 0 for the null array.
- `Int Function Size(Var[] akArray) Global`: ar_Size: how many elements; -1 for the null array, as xNVSE.
- `Int Function Slot(Var[] akArray, Var akKey) Global`: The element of akArray holding akKey's value, or -1 when it has no such key.
- `Bool Function SameKey(Int aiKind, Var akA, Var akB) Global`: Do two map keys name the same element? Numeric keys compare as numbers, string keys ignoring case.
- `Bool Function SameValue(Var akA, Var akB) Global`: xNVSE's == on two elements: forms by identity, strings ignoring case, everything else as numbers.
- `Bool Function KeyBefore(Int aiKind, Var akA, Var akB) Global`: Does map key akA sort before akB?
- `Var Function Get(Var[] akArray, Var akKey) Global`: arr[key]: the element's value, None when the key is absent.
- `Bool Function HasKey(Var[] akArray, Var akKey) Global`: ar_HasKey.
- `Var Function Find(Var akValue, Var[] akArray) Global`: ar_Find: the key of the first element equal to akValue; ar_BadNumericIndex (-99999), or "" in a StringMap.
- `Var Function KeyAt(Var[] akArray, Int aiIndex) Global`: The aiIndex-th key in iteration order.
- `Var Function ValueAt(Var[] akArray, Int aiIndex) Global`: The aiIndex-th value in iteration order.
- `Var[] Function Iterator(Var[] akArray, Int aiIndex) Global`: The StringMap a ForEach loop gives its iterator: "key" and "value" of the aiIndex-th element.
- `Var Function Unbox(Var[] akArray) Global`: The * operator: a StringMap's "value", else the first element.
- `Bool Function Set(Var[] akArray, Var akKey, Var akValue) Global`: arr[key] := value. An Array grows only by writing one past its end, as xNVSE; a map inserts in key order.
- `Bool Function Append(Var[] akArray, Var akValue) Global`: ar_Append: add to the end of an Array.
- `Int Function Erase(Var[] akArray, Var akKey) Global`: ar_Erase: remove one key; how many elements went.
- `Function Clear(Var[] akArray) Global`: Empty an array, keeping its kind.

### CRUI_Condition

FO3/FNV item condition: the engine's own item health (0 to 1) on an inventory item, which the save keeps. The natives come from CleanroomUI.dll; the rules live in F3NV_ConditionPlayer.

- `Float Function GetHealth(ObjectReference akHolder, Form akItem) Global Native`: The health of akItem in akHolder (its equipped copy first); -1 when not held or none set yet.
- `Bool Function SetHealth(ObjectReference akHolder, Form akItem, Float afHealth) Global Native`: Set akItem's health in akHolder (0 to 1), adding it when it has none; False when not held.
- `Float Function GetEquippedWeaponHealth(Actor akActor) Global Native`: The health of akActor's equipped weapon; -1 with none or none set yet.
- `Int Function StackCount(ObjectReference akHolder, Form akItem) Global Native`: How many of akItem share the stack its health sits on (they share one condition).
- `Float Function GetStackHealth(ObjectReference akHolder, Form akItem, Int aiStack) Global Native`: GetHealth on stack aiStack of akItem (-1, or a stack it lacks: its equipped copy, else its first).
- `Bool Function SetStackHealth(ObjectReference akHolder, Form akItem, Int aiStack, Float afHealth) Global Native`: SetHealth on stack aiStack of akItem, chosen as GetStackHealth chooses it.
- `Function Track(Form[] akItems) Global Native`: akItems wear: the Pip-Boy shows their condition (CND) on the item card. Adds to the items other mods track; the REPAIR hint and G key appear only once some mod tracks an item.
- `Form Function PipboySelection() Global Native`: The item highlighted in the Pip-Boy's inventory when it wears; None otherwise.
- `Int Function PipboyStack() Global Native`: The stack index of PipboySelection's item; -1 with none.
- `Function PipboyRefresh() Global Native`: Show the highlighted item's condition again on the Pip-Boy's item card (after a repair).
- `Function TrackRepair(Form[] akWeapons, Form[] akArmors, Form[] akRepairLists, Int[] aiJuryGroups, ActorValue akRepair) Global Native`: The FNV repair lists (akRepairLists: akWeapons' first, then akArmors'; a FormList or None each), the Jury Rigging groups (parallel; 0 for none) and the Repair skill, for repair and the Pip-Boy's repair list. They add to what other mods gave: an item repairs from any list it was given, and with Jury Rigging from any item sharing one of its groups (group numbers are shared by every mod). The last skill given is used.
- `Function SetJuryRigging(Bool abHas) Global Native`: Whether the player has Jury Rigging: a spare of the item's Jury Rigging group repairs it too.
- `Float Function RepairWith(Actor akHolder, Form akItem, Int aiStack, Form akSpare, Int aiSpareStack) Global Native`: Repair stack aiStack of akItem (-1: its equipped, else first) from one copy of akSpare's stack aiSpareStack (-1: its worst; akSpare None: the default spare, the worst that still gives the best result), by FNV's rule, using that copy up: the new health (0 to 1), or -1 with no such spare.
- `Int Function SpareCount(Actor akHolder, Form akItem, Int aiStack) Global Native`: How many spare stacks akHolder carries for stack aiStack of akItem (the same item, its repair list, and its Jury Rigging group while the player has the perk).
- `Float Function PipboyRepair(Actor akHolder) Global Native`: Carry out the repair chosen in the Pip-Boy's repair list or vendor offer (on PipboySelection's item), paying the vendor's price: the new health, -1 with none chosen, -2 short of caps.
- `Function SetVendor(Actor akVendor) Global Native`: Offer akVendor's repair services (FNV's ShowRepairMenu) on the Pip-Boy's repair card (G) until the Pip-Boy closes: the condition its Repair skill reaches, for caps. None stops.
- `Function SetServiceRules(Float afSkillBase, Float afSkillMax, Float afCostMult) Global Native`: FNV's vendor repair GMSTs: fRepairSkillBase and fRepairSkillMax (the condition reached in tenths at Repair 0 and 100) and fItemRepairCostMult.

### CRUI_Framework

Cleanroom UI's framework itself, from CleanroomUI.dll. See docs/spin_offs.md#framework-rules

- `Int Function GetVersion() Global Native`: The framework's version, raised whenever a native, event or rule changes: check it is at least the one you built against. 0 without CleanroomUI.dll (the call fails).

### CRUI_Hud

Widgets on the HUD without replacing HUDMenu.swf: CleanroomUI.dll loads our host movie into whatever HUD movie is installed, and the player moves, scales and hides each widget in our edit menu (EditLayout). A widget is forgotten as a save loads or a new game starts: add yours on every game load. See docs/swf_framework.md#hud

- `Function AddWidget(String asName, String asMovie, Float afX = 0.0, Float afY = 0.0, String asAnchor = "") Global`: Show movie asMovie (a path under Interface; a widget movie, which implements processMessage(command, params)) as widget asName, under HUD part asAnchor (clip names from the HUD's root, dotted, e.g. "LeftMeters_mc", so it moves and fades with that part; "" for the HUD itself) at afX, afY from that part's top-left. A name given again moves or replaces its widget.
- `Function AddStats(String asName, String[] asLabels, ActorValue[] akValues, Float[] afSteps, GlobalVariable akShown = None, Float afX = 0.0, Float afY = 0.0, String asAnchor = "") Global`: Show a line of stats as widget asName, placed as AddWidget places one: each label (a $KEY is translated) with its actor value on the player, left to right. A value below afSteps[0] is dim, from afSteps[1] it takes the HUD's warning color, from afSteps[2] it flashes; give fewer steps to leave the rest off. Shown while akShown is not 0 (None: always).
- `Function RemoveWidget(String asName) Global`: Take widget asName off the HUD.
- `Function Send(String asName, String asCommand, String[] asParams) Global`: Call widget asName's processMessage(asCommand, asParams) on its next frame; held until the widget has loaded.
- `Function SetCondition(String asName, Bool abOn, String asLabel = "") Global`: A condition of your own for the HUD's show rules (docs/swf_framework.md#hud-conditions): a box whose rule names asName (letters, digits, _ and . only; not one of ours, such as combat) shows while it holds. asLabel is how the edit menu's list shows it (a $KEY is translated; "" keeps the last one given, else asName). Forgotten as a save loads: set yours on every game load. A rule naming a condition nobody set counts it as holding, so a box never hides for good when your mod is gone.
- `Function SetConditionGlobal(String asName, GlobalVariable akValue, String asLabel = "") Global`: Condition asName holds while akValue is not 0, read each frame with no Papyrus call: for a state your mod keeps in a global anyway. asLabel as SetCondition.
- `Function RemoveCondition(String asName) Global`: Take condition asName off.
- `Function SetShowRule(String asName, String asRule) Global`: When box asName shows, unless the player set its rule in the edit menu: asRule is its conditions, comma separated (e.g. "combat,mymod.alarm,peek"; "always" for always; low:0.3 for a threshold), over the preset's ([Hud] sShow). asName is your widget's name, or "part:" and one of the HUD's parts (e.g. "part:CompassWidget_mc"). "" takes yours off. Forgotten as a save loads: set it on every game load.
- `Function EditLayout() Global`: Open the edit menu for the widgets on our host and the game's own HUD parts: drag a box to move its widget or part, turn the mouse wheel over it to scale it, R (a setting in CleanroomUI.ini) puts it back where its mod or the game puts it, Esc or Tab closes. The layout is kept with each save (K backs it up to Documents\My Games\Fallout4\F4SE\CleanroomUI_Hud.txt, L restores it; [Hud] bLayoutPerSave=0 shares that file between every save). Our MCM page has a button and a hotkey for it; from the console: cgf "CRUI_Hud.EditLayout".

### CRUI_HudNative

CleanroomUI.dll's HUD widget list, for CRUI_Hud: call CRUI_Hud, not these. See docs/swf_framework.md#hud

- `Function AddWidget(String asName, String asMovie, Float afX, Float afY, String asAnchor) Global Native`: Widget asName: movie asMovie, under HUD part asAnchor at afX, afY.
- `Function AddStats(String asName, String[] asLabels, ActorValue[] akValues, Float[] afSteps, GlobalVariable akShown, Float afX, Float afY, String asAnchor) Global Native`: Stats line asName; see CRUI_Hud.AddStats.
- `Function RemoveWidget(String asName) Global Native`: Take widget asName off the list.
- `Function Send(String asName, String asCommand, String[] asParams) Global Native`: Hand widget asName processMessage(asCommand, asParams) on its next frame.
- `Function SetCondition(String asName, Bool abOn, String asLabel) Global Native`: Condition asName for the HUD's show rules holds (abOn) or not; see CRUI_Hud.SetCondition.
- `Function SetConditionGlobal(String asName, GlobalVariable akValue, String asLabel) Global Native`: Condition asName holds while akValue is not 0; see CRUI_Hud.SetConditionGlobal.
- `Function RemoveCondition(String asName) Global Native`: Take condition asName off.
- `Function SetShowRule(String asName, String asRule) Global Native`: Box asName's rule from its mod; see CRUI_Hud.SetShowRule.

### CRUI_ItemCard

Lines of your own on the Pip-Boy's inventory item card, from CleanroomUI.dll (dll/framework/card.cpp), drawn by our Pip-Boy movie under any Pip-Boy that keeps the vanilla item card. Lines are forgotten when a save loads or a new game starts: set them again on each game load (OnPlayerLoadGame). See docs/swf_framework.md#card-lines

- `Function SetLine(Form akItem, String asLabel, String asValue) Global Native`: Show asLabel with asValue on akItem's item card (a Keyword: on the card of every item carrying it), replacing that item's line of the same label. An empty asValue removes the line.

### CRUI_LevelUp

Fallout 4 only. A level-up in place of the game's: the perk and skill points to spend (saved with the game as this script's variables), taking a perk and raising a skill, the perk list, each perk's rank and eligibility, and our level-up menu (Interface/CRUI_LevelUp.swf, registered with F4SE), also the tag skill and trait menus. Its partner CRUI_LevelUpHolder (Holder) keeps the tagged skills, is where Get() finds it, and decides what opening each menu does. What the menu shows is written as tables (the skills' here, each plugin's perks and traits on its CRUI_LevelUpTable), so opening it asks the engine only for the ranks and traits held and the player's values. TagsChosen is sent once the tag menu's picks are set. See docs/level_up_plan.md#prkf

- `CRUI_LevelUp Function Get() Global`: The running game's level-up, for other mods: CRUI_LevelUp.Get().AddPerkPoints(1). Cleanroom UI's shared holder (CRUI_LevelUpHolder.Shared) holds it, set as its owner starts and each time our menu opens; None without that plugin or before it is set. See docs/level_up_plan.md#plugin
- `CRUI_LevelUpHolder Function Holder()`: Events, else the holder on this quest.
- `Function Announce()`: Be the level-up Get() finds: held by this host's holder and by Cleanroom UI's shared one.
- `Bool Function AddPerk(Perk akPerk)`: List perk akPerk (a first rank) in the level-up menu after the build's, kept by the save; False when already added. Its name, description, image, level and ranks are read from the perk as the menu opens, and whether it can be taken is the game's own test of its conditions. A perk a plugin's table lists already is shown once.
- `Bool Function RemovePerk(Perk akPerk)`: Take perk akPerk, added by AddPerk, out of the level-up menu; False when it was not added.
- `Bool Function AddSkill(ActorValue akSkill, String asImage = "")`: List skill akSkill after the build's (Skills) in the level-up and tag menus, kept by the save; False when already added. Its name and description are read from it as the menu opens; asImage is its image, as SkillImages' are. The tag menu tags the first 62 skills only. A skill Skills lists already is shown once.
- `Bool Function RemoveSkill(ActorValue akSkill)`: Take skill akSkill, added by AddSkill, out of the menus; False when it was not added.
- `Function OpenLevelUpMenu()`: Open the level-up menu (skills, perks, taken) once no menu is open, as the holder's OpenLevelUp decides (the FO3/FNV conversion gives the levels gained since the last time their points first). MenuClosed is sent when it closes.
- `Function OpenTagMenu(Int aiTags)`: Open the tag skill menu for aiTags skills tagged in all (FNV: 3 at Doc Mitchell's; the tags held plus one for Tag!). Tags chosen before stay; TagsChosen is sent once the picks are set.
- `Function OpenTraitMenu(Int aiTraits = 0)`: Open the trait menu for up to aiTraits traits (0: the holder's own count).
- `Int Function AddPerkPoints(Int aiPoints)`: Adds aiPoints perk points (negative takes some away, never below 0); the points left.
- `Int Function AddSkillPoints(Int aiPoints)`: Adds aiPoints skill points (negative takes some away, never below 0); the points left.
- `Function ShowGamePoints()`: The game's own perk points, which the Pip-Boy's LEVEL UP notice shows (the game adds one each level), set to what is left to spend here: the perk points, else 1 while skill points are left, else none. Needs CleanroomUI.dll. See docs/level_up_plan.md#game-points
- `Bool Function Tag(ActorValue akSkill)`: Tags akSkill (Holder) and sends SkillTagged; False when it already was. A tag bonus is the holder's owner's to give.
- `Bool Function Untag(ActorValue akSkill)`: Untags akSkill (Holder) and sends SkillUntagged; False when it was not tagged.
- `Int Function RaiseSkill(ActorValue akSkill, Int aiPoints)`: Spends up to aiPoints skill points raising akSkill's base, no higher than SkillCap; the points spent.
- `Perk Function NextRank(Perk akPerk)`: The rank of akPerk (a first rank) the player would take next; None when they hold every rank.
- `Int Function Rank(Perk akPerk)`: How many of akPerk's (a first rank) ranks the player holds.
- `Bool Function Eligible(Perk akRank)`: Whether the player meets akRank's level and conditions (the engine's own test of the converted requirements).
- `Bool Function CanTake(Perk akPerk)`: Whether the player can take akPerk's (a first rank) next rank now: a rank left, a perk point, and eligible.
- `Bool Function TakePerk(Perk akPerk)`: Takes akPerk's (a first rank) next rank for a perk point and sends PerkTaken with that rank; False when it can't.
- `Perk[] Function PerkList(Int aiShow)`: The level-up list's perks (first ranks) with a rank left: aiShow 0 all, 1 those the player is eligible for, 2 those they are not. Perk points are not counted.
- `Function Show(Int aiMode, Int aiCount, Int aiBonus)`: Our menu in mode aiMode (Open) as soon as no menu is open: its data gathered first (Prepare), so the gathering overlaps the wait, without the settling a message box needs. See docs/level_up_plan.md#open-delay
- `Function Open(Int aiMode = 0, Int aiCount = 0, Int aiBonus = 0)`: Open our menu in mode aiMode (LevelUpMode, TagsMode for aiCount tags in all with aiBonus each, TraitsMode for up to aiCount traits): its data gathered first (Prepare) unless Prepare just did for this open; Send hands it over.
- `Float Function Since(Float afStart)`: Real seconds since afStart.
- `Function Prepare(Int aiMode, Int aiCount, Int aiBonus)`: Gather what our menu shows in mode aiMode (Built) ahead of opening it, registered first. Show (or a holder's own opener) calls it while the Pip-Boy closes, so the gathering overlaps that wait. See docs/level_up_plan.md#open-delay
- `Function Register()`: Be the level-up Get() finds; once per load (F4SE's registrations do not last past one), register the menu with F4SE and its events: each of those calls costs a frame. See docs/level_up_plan.md#open-delay
- `Function Send()`: Hand the open menu what Prepare gathered (root1.Open), once; gathered now when nothing waits.
- `Var[] Function Built()`: What the menu shows, in Character.as's order: every plugin's perk, trait and requirement tables joined (Collect, Merged), the player's points, level, sex and cap, the ranks held and values (asked in parallel, Gather), the tagged skills, the mode and the traits held, whether to show every perk at first, and each table's perk and row counts (the menu moves each table's row owners and perk targets by them).
- `String Function GroupCounts(Int[] aiCounts)`: aiCounts, one per shown table, then the added perks' count when there are any (one each, as perks or rows), SEP between: the groups the menu moves indexes by.
- `Function Collect()`: Into shown: this quest's table (the game's), then every other plugin's registered in Tables; and each one's perk, trait and row counts, of what the mode shows: perks and rows in a level-up, traits in the trait menu, neither in the tag menu (the rest count 0, so Send hands over and asks about none of them).
- `Function DescribeSkills()`: Into moreSkills and moreParts (names, descriptions, images): the skills AddSkill listed that Skills does not, each read from the skill; listedSkills is Skills' count.
- `String Function SkillTexts(String asTable, Int aiWhich)`: asTable (SkillNames, SkillDescriptions or SkillImages, one entry per Skills entry, or "" for none) with the added skills' text aiWhich (moreParts) after Skills' entries.
- `Int Function SkillCount()`: How many skills the menu shows: Skills', then the added ones (DescribeSkills).
- `ActorValue Function SkillAt(Int aiIndex)`: The menu's skill aiIndex: Skills' entry, else the added one (moreSkills).
- `Function DescribeAdded()`: Into extra and extraParts (CRUI_LevelUpTable.Text's order): the perks AddPerk listed that no shown table lists, each read from the perk, with one requirement row of kind 4, the game's own test of its next rank: the row's index, after every table's rows, goes in unlisted when the test passes. Owners count from 0, as a table's do.
- `Function AddPart(Int aiWhich, String asText, Int aiIndex)`: asText, the added perk aiIndex's, onto extraParts[aiWhich] (a tab inside it would split the entry).
- `Bool Function Tabled(Perk akPerk)`: Whether a shown table lists akPerk.
- `String Function Merged(Int aiWhich, Int[] aiCounts)`: Text aiWhich (CRUI_LevelUpTable.Text) of each shown table with entries (aiCounts, per table), then the added perks', SEP between.
- `Function Gather(Int aiSpecials, Int aiSkills)`: Into heldParts, valueParts, tagParts and traitParts: the ranks held of each listed perk, the player's base value of each Specials and Skills entry and its tag (0 none, 1 a suggestion the tag menu may change, 2 tagged), then whether the player has each trait; asked by Chunks stacks at once (ScanChunk), so the frame each native call costs overlaps. In a level-up, one more stack asks for the perks the requirement rows test that no table lists (ScanUnlisted). Gives up after 10 seconds.
- `Function ScanChunk(Int aiChunk, Int aiFirst, Int aiEnd, Int aiSpecials, Int aiSkills)`: Gather's items aiFirst to aiEnd into its parts aiChunk: listed perks' ranks held, values and their tags (Specials, then Skills), traits held; the perks and traits asked of their tables (Over).
- `Function ScanUnlisted()`: Gather's extra stack: onto unlisted, each table's rows testing a perk it does not list that the player has, moved to where the table's rows start (CRUI_LevelUpTable.UnlistedHeld).
- `String Function Over(Actor akPlayer, Bool abTraits, Int aiFrom, Int aiTo)`: The ranks akPlayer holds of the listed perks aiFrom to aiTo, counted across the shown tables, or whether they have each of those traits (abTraits); SEP between.
- `Int Function TagState(ActorValue akValue)`: 0 untagged, 1 tagged as SetPlayerTagSkill's suggestion (the tag menu may change it), 2 tagged.
- `String Function JoinedParts(String[] asParts)`: asParts' non-empty texts, SEP between.
- `Perk Function ListedPerk(Int aiIndex)`: The first rank of listed perk aiIndex, counted across the shown tables; None outside them.
- `Function OnMenuTake(Int aiIndex)`: The menu's pick of listed perk aiIndex: take its next rank (TakePerk, the engine's eligibility test), then tell the menu whether it was taken, the ranks now held and the perk points left (root1.Taken).
- `Function OnMenuRaise(Int aiSkill, Int aiPoints)`: The menu's aiPoints into its skill aiSkill (SkillAt): raise it (RaiseSkill), then tell the menu the points spent, the skill's base and the skill points left (root1.Raised).
- `Function OnMenuTags(Int aiMask, Int aiMore)`: The tag menu's picks, bit i of aiMask for the menu's skill i (SkillAt), bit i of aiMore for skill 31 + i: each tagged or untagged, the suggestions done with, TagsChosen sent (the rules give the bonus), then the menu closed.
- `Function OnMenuTraits(Int aiMask, Int aiShown)`: The trait menu's picks, bit i for trait i of the shown tables', of the first aiShown: each table adds the picked ones the player lacks and removes the others they have (CRUI_LevelUpTable.SetTraits); then the menu closed.
- `Var[] Function FormArgs(Form akForm)`: A custom event's arguments: akForm alone.

### CRUI_LevelUpHolder

The level-up host's partner: where CRUI_LevelUp.Get() finds the running host, the player's tagged skills, and what opening each menu does. Every function here works as written; a mod with rules of its own extends this script and overrides the functions it needs (the FO3/FNV conversion's F3NV_CharacterEvents keeps its tags and opens the menus through its character rules). The host reads it from its Events property, else from a holder on its own quest. See docs/level_up_plan.md#holder

- `CRUI_LevelUpHolder Function Shared() Global`: The holder quest of Cleanroom UI's own plugin (CleanroomUI.esl 0x800, crui/plugin.py), which every host announces itself to; None when the plugin is not loaded.
- `CRUI_LevelUp Function Running()`: The running host Get() hands to other mods; None before it registers.
- `Function Hold(CRUI_LevelUp akHost)`: Remember akHost as the running host.
- `Bool Function IsSuggested(ActorValue akSkill)`: Whether akSkill is tagged only as a suggestion, so the tag menu shows it picked but lets the player change it.
- `Bool Function Tag(ActorValue akSkill)`: Tag akSkill; False when it already was.
- `Bool Function Untag(ActorValue akSkill)`: Untag akSkill; False when it was not tagged.
- `Function SuggestTag(ActorValue akSkill)`: Tag akSkill ahead of the next tag menu as a suggestion the player may change there.
- `Function OpenLevelUp()`: The level-up menu (skills, perks, taken) once no menu is open, with the points the player has.
- `Function OpenTags(Int aiCount)`: The tag skill menu for aiCount skills tagged in all, no bonus shown.
- `Function OpenTraits(Int aiCount)`: The trait menu for up to aiCount traits (0: two, as FO3 and FNV allow).

### CRUI_LevelUpNative

The level-up's natives, from CleanroomUI.dll (framework version 17). See docs/level_up_plan.md#game-points

- `Function SetGamePerkPoints(Int aiPoints) Global Native`: Set the game's own perk points, which the Pip-Boy's LEVEL UP notice shows, to aiPoints (0 to 255), on the game's next frame. Game.AddPerkPoints can only add.

### CRUI_LevelUpTable

Fallout 4 only. One plugin's part of our level-up menu, as its build wrote it: its own listed perks and traits (names, descriptions and images SEP between, levels, rank chains) and its perks' requirement rows. The game's is on the character rules quest beside CRUI_LevelUp; a DLC's on a start-game quest of its own, which adds itself to Tables as it starts, so plugins stack in any load order. CRUI_LevelUp joins every table each time the menu opens. See docs/level_up_plan.md#tables

- `Function Register()`: Be in Tables.
- `String Function Text(Int aiWhich)`: Table aiWhich as the menu reads it, SEP between, as the build wrote it: 0 to 4 the perks' names, descriptions, images, levels and rank counts; 5 to 7 the traits' names, descriptions and images; 8 to 14 the rows' owners, kinds, targets, comparisons, values, OR flags and labels.
- `String Function RanksHeld(Actor akPlayer, Int aiFrom, Int aiTo)`: The ranks akPlayer holds of this table's listed perks aiFrom to aiTo (walking RankForms), SEP between.
- `String Function TraitsHeld(Actor akPlayer, Int aiFrom, Int aiTo)`: 1 or 0 for whether akPlayer has each of this table's traits aiFrom to aiTo, SEP between.
- `String Function UnlistedHeld(Actor akPlayer, Int aiFirstRow, String asOut)`: asOut with, SEP between, the index of each row testing a perk this table does not list that akPlayer has, moved to aiFirstRow, where this table's rows start among every table's.
- `Bool Function Lists(Perk akPerk)`: Whether akPerk is a rank of one of this table's listed perks.
- `Perk Function FirstRank(Int aiPerk)`: The first rank of this table's listed perk aiPerk.
- `Function SetTraits(Actor akPlayer, Int aiMask, Int aiFirst, Int aiShown)`: The trait menu's picks for this table's traits, bit aiFirst + i for trait i, of the first aiShown of every table's: each akPlayer has and isn't picked removed, each picked one they lack added. A trait past the mask is left.
- `String Function Joined(String asText, String asEntry, Int aiIndex) Global`: asText with asEntry added, a tab before it unless it is entry 0.

### CRUI_Pipboy

FO3/FNV's Pip-Boy tabs on Fallout 4's STAT page, from CleanroomUI.dll. See docs/fork/fo4_pipboy_tabs.md

- `Function SetTab(String asName, Int aiOrder, FormList akRows, String[] asValues, FormList akShown) Global Native`: Give the STAT page tab asName (placed by aiOrder among these tabs, after the page's own): a row per akRows form, an actor value on the player or a global, by its name; its value picks from asValues (none: the number shows); a row shows while its global in akShown (None: always) is at least 1. The plugin forgets tabs each game start.

### CRUI_PipboyActions

The Pip-Boy's action menu on a pad (Interface/CRUI_PipboyActions.swf): CleanroomUI.dll calls Open as View is pressed in the Pip-Boy while a mod uses Repair or Ammo. See docs/swf_framework.md#pipboy-pad

- `Function Open() Global`: Open the action menu over the Pip-Boy, registering it first: no cursor, the game paused, the pad's menu controls, above the Pip-Boy (depth 10), in the HUD's colors.

### CRUI_PipboyAmmo

The Pip-Boy's ammo list, from CleanroomUI.dll (dll/framework/ammo.cpp). See docs/fork/fo4_condition.md#ammo-list

- `Function SetLists(Form[] akLists, Int[] aiStarts, Form[] akRounds, Form[] akMods) Global Native`: Give the plugin your ammo lists: akLists the keywords your guns carry, aiStarts each list's first round in akRounds (one more entry: the end), akMods each round's ammo mod. They add to other mods' lists; a list given again replaces its rounds. The plugin forgets them each game start, so call this on every game load. The AMMO hint and B key appear only once some mod has given lists.
- `Form Function ChosenList() Global Native`: The list (keyword) of the round chosen in the Pip-Boy; take the choice only when it is one of yours. None with no choice waiting.
- `Form Function ChosenGun() Global Native`: The gun whose ammo was chosen in the Pip-Boy (it sends CRUI_AmmoSwitch as the choice is confirmed); None with no choice waiting.
- `Int Function TakeChoice() Global Native`: The chosen round's index in your akRounds (the mod whose list ChosenList is), cleared as it is taken; -1 with none.

### CRUI_Sex

FO3/FNV SexChange: the game's own SexChange command, run by CleanroomUI.dll, as FO4 Papyrus has no sex setter.

- `Function ChangeSex(Actor akActor) Global Native`: Toggle akActor's sex (body, head, 3D) on the game's next frame, as the console's sexchange does.

## Movie natives

- `TextureExists` (base/base.cpp): TextureExists(path): whether the game has Textures\<path> (the path f4se.MountImage takes): a loose file or one in a general archive, opened as the game opens a resource, or a name in a BA2's table (Archived). MountImage itself reports true for a missing file. See docs/swf_framework.md#images
- `HudColor` (base/base.cpp): HudColor([prefix]): the player's HUD color (iHUDColor, or the INI colors `prefix`, e.g. iHUDColorWarning) as 0xRRGGBB, read on every call, for menus of ours that draw their text and lines in it.
- `Log` (base/base.cpp): Log(text): a line from one of our movies in the DLL's log, at the verbose level.
- `Settings` (base/base.cpp): Settings(mod): mod `mod`'s INI files (IniFiles) as {section: {setting: value}}, MCM's copy over its own, each value typed by its name's prefix (Typed); null for a name that is not PlainName. Read on every call, so an MCM change shows the next time a movie asks. See docs/swf_framework.md#settings
- `HudWidgets` (framework/hud.cpp): HudWidgets(version): {version, widgets: [{name, movie, anchor, x, y, place: {dx, dy, scale, hidden, color, show}}]}, or null while the list is still the one numbered `version` (a widget added or removed, or a place changed in the edit menu).
- `HudMessages` (framework/hud.cpp): HudMessages(): the messages sent since the last call, [{name, command, params}], in order.
- `HudStats` (framework/hud.cpp): HudStats(name): stats line `name` now, {shown, rows: [{text, value}], steps}; null for no such line.
- `HudKeys` (framework/keys.cpp): HudKeys(): the edit menu's keys, {reset, resetName, hide, hideName, color, colorName, show, showName, backup, backupName, restore, restoreName} (each a code and its name), read again as it opens.
- `HudReport` (framework/layout.cpp): HudReport(name, ox, oy, left, top, width, height): where the host found widget `name` before moving it.
- `HudPlaceOf` (framework/layout.cpp): HudPlaceOf(name): the place the player gave `name`, {dx, dy, scale, hidden, color, show}, or null for none and no rule (a part with only the preset's rule gets the default place with it).
- `HudLayout` (framework/layout.cpp): HudLayout(): our widgets and the HUD's parts the host has reported, where their places put them, [Box] (the box in fractions of the screen).
- `HudMove` (framework/layout.cpp): HudMove(name, dx, dy): move widget `name` by dx, dy (fractions of the screen).
- `HudScale` (framework/layout.cpp): HudScale(name, factor): scale widget `name` by `factor`, within a quarter and four times its own size.
- `HudHide` (framework/layout.cpp): HudHide(name): hide widget `name`, or show it again when hidden.
- `HudTint` (framework/layout.cpp): HudTint(name, color): give widget `name` a color over its own (0xRRGGBB), or its own again for -1.
- `HudSetShow` (framework/layout.cpp): HudSetShow(name, rule): the conditions widget `name` shows under, comma separated ("" for the preset's rule, "always" for none); spaces are dropped.
- `HudReset` (framework/layout.cpp): HudReset(name): put widget `name` back where its mod puts it, shown, in its own color, under the preset's rule.
- `HudEditDone` (framework/layout.cpp): HudEditDone(): close the edit menu; the layout goes with the next game save, or into the file when layouts are shared.
- `HudBackup` (framework/layout.cpp): HudBackup(): the layout written to the layout file; how many places (-1 when it can't be written).
- `HudRestore` (framework/layout.cpp): HudRestore(): the layout put back from the layout file; how many places (-1 when there is no file).
- `HudLayoutPerSave` (framework/layout.cpp): HudLayoutPerSave(): whether places belong to each save (else one layout for every save).
- `PipboyActions` (framework/pad.cpp): PipboyActions(): the actions in use, [{text, key}]: Repair while a mod tracks an item, Ammo while one gives ammo lists.
- `PipboyPress` (framework/pad.cpp): PipboyPress(key): Pip-Boy key `key` (a Flash key code) on the open Pip-Boy, as if typed there; whether a list is open after it.
- `PipboyListOpen` (framework/pad.cpp): PipboyListOpen(): whether the repair or ammo list is open.
- `PipboyActionsDone` (framework/pad.cpp): PipboyActionsDone(): close the action menu.
- `HudPartLayout` (framework/parts.cpp): HudPartLayout(): the preset and DEF_HUD.xml, as {preset, settings: {name: value}, parts: [{name, parname, x, y, defx, defy, scaleX, scaleY, rotation, color, color2, flags}]} (each element's fields under their own names); settings and parts are empty when there is no file or no <HUD> in it.
- `HudWeaponCondition` (framework/parts.cpp): HudWeaponCondition(): the condition of the player's equipped weapon, for the FNV preset's CND bar (0 to 1; 1 while none is set yet, as the Pip-Boy shows it); -1 with no weapon equipped or one that doesn't wear (CRUI_Condition.Track).
- `PipboyCardLines` (framework/pipboy.cpp): PipboyCardLines(): the lines for the highlighted item's card, [{text, value, showAsPercent}], for our Pip-Boy movie to keep there: the CND line of an item that wears, why a list did not open, then other mods' lines (card.cpp). Null while a list of ours is open or off the inventory page.
- `PipboyList` (framework/pipboy.cpp): PipboyList(): the open repair or ammo list for our movie to draw, {heading, rows, chosen, notes, keys} (heading and keys {text, value}; rows and notes arrays of them; chosen the highlighted row, -1 for none); null when none.
- `HudState` (framework/show.cpp): HudState(): the conditions now, {combat, drawn, vats, peek, aiming} (each true or false), the mods' conditions under their own names, and linger (seconds).
- `HudValues` (framework/show.cpp): HudValues(): the values the conditions changed and low watch, {health, ap (fractions of their maximum), rads (the value itself), condition (the equipped weapon's, as the CND bar shows it; -1 for none), weapon (its form ID; 0 for none)}; -1 for a value not found. The host asks a few times a second.
- `HudConditions` (framework/show.cpp): HudConditions(): the mods' conditions for the edit menu's list, [{name, label}] by name.

## The AS3 library (`crui`)

### Bridge

The movie's link to the game: F4SE's root object (handed over in onF4SEObjCreated) and our DLL's natives. See docs/fork/swf_framework.md#design

- `plain(items:Array) : Array`: F4SE hands each item of a Papyrus Var array over wrapped as {__var__: value}: the plain values.
- `natives(plugin:String) : Object`: The natives a plugin registered on every movie (f4se.plugins.<name>); null without F4SE or the plugin.
- `shared() : Object`: The natives every DLL of ours gives each movie (TextureExists, HudColor): CleanroomUI.dll's, else CleanroomDialogue.dll's; null with neither.
- `attach(stage:Stage) : void`: For a movie loaded into a menu (ours or another mod's: docs/swf_framework.md#inject): F4SE's object, which is on the menu movie's own root; null without F4SE. Call it once the movie is on the stage.
- `settings(mod:String) : Object`: A mod's INI files as {section: {setting: value}} (Data/F4SE/Plugins/<mod>.ini, MCM's copy over it), each value typed by its prefix (b, i, u, f; else text); null without our DLL. See docs/swf_framework.md#settings
- `send(event:String, ... args) : void`: Send an F4SE external event (Papyrus RegisterForExternalEvent); nothing before F4SE is there.
- `mount(menu:String, path:String, name:String) : Boolean`: Mount a DDS under Textures (path without that folder) as img://<name>; false without F4SE. MountImage returns true even for a missing file, and images drop when the menu closes.

### Draw

Buttons and panels outlined in Theme.color.

- `button(parent:Sprite, text:String, x:Number, y:Number, handler:Function, width:Number = 200) : Sprite`: A clickable box with centered text; `handler` gets the click.
- `outline(g:Graphics, x:Number, y:Number, wide:Number, high:Number, thick:Number, alpha:Number, color:Number = -1) : void`: An outline `thick` wide round the `wide` by `high` box at (`x`, `y`) in `color` (below 0: Theme.color) at `alpha`, as four strips that do not overlap (so a translucent outline has no darker corners).
- `box(g:Graphics, width:Number, height:Number, alpha:Number) : void`: A `width` by `height` box at the origin of `g`, outlined in Theme.color and filled black at `alpha`.
- `panel(parent:Sprite, x:Number, y:Number, width:Number, height:Number, alpha:Number) : Sprite`: An outlined box filled black at `alpha`.

### HintBar

A row of button hints, each an action's button and a label: on a keyboard the key's name ("ENTER) SPIN"), on a gamepad the button's glyph from the game's controller font, Xbox or PlayStation as Input says. It redraws when the device changes; clicking a hint fires its action. See docs/fork/swf_framework.md#input

- `show(hints:Array) : void`: Show `hints`: [actions, label] each, actions one action or an array of them shown together ([LEFT, RIGHT]), the label text or a $KEY.
- `buttons(actions:Array) : String`: The button `actions` show on the current device: key names joined by "/" and ")", or glyphs.

### Host

What every menu does once it is on the stage. See docs/fork/swf_framework.md#facts

- `focus(menu:InteractiveObject) : void`: Give `menu` the keyboard focus without Flash's yellow focus rectangle round the screen (FO4's own menus turn it off the same way).

### Images

Textures shown in a menu through f4se.MountImage. Any form of a texture's path is put the way MountImage takes it (under Textures, without that folder: played 2026-11-08); a missing file is caught first by our DLL's TextureExists, since MountImage reports true for one; and an image shows in a Loader, scaled to a square. See docs/fork/swf_framework.md#images

- `texturePath(path:String) : String`: `path` as MountImage takes it: forward slashes, no leading Data/ or Textures/ (any case), spaces kept.
- `exists(path:String) : Boolean`: Whether the game has the texture at `path`: our DLL's answer, or true without it (none, or an older one).
- `mount(path:String, name:String) : String`: Mount the texture at `path` as `name`: its img:// URL, or "" when it is missing or F4SE did not mount it.
- `show(parent:Sprite, url:String, x:Number, y:Number, size:Number = 0, done:Function = null) : Loader`: A Loader at (x, y) in `parent` showing `url`, scaled to `size` square once loaded (0: as it is), then `done(loader, loaded)` (null for none). For an empty url the Loader stays empty.

### Input

One map from keys and the game's controls (ProcessUserEvent) to actions, so a menu only ever sees actions and works the same on keyboard and gamepad; and which device is in use, for HintBar. Moves (up, down, left, right, prev, next) fire on the press, the rest on the release. A press that arrives as both a key and a control fires once, and nothing fires just after the menu opens (the key that opened it). See docs/fork/swf_framework.md#input

- `mapKey(code:uint, action:String, name:String) : void`: Key `code` does `action`; `name` is how a hint shows it (the first key mapped to an action names it).
- `mapControl(name:String, action:String, gamepad:Boolean = false) : void`: The game's control `name` does `action`; with `gamepad` only when a gamepad is in use (a control the keyboard sends for other keys, as the dialogue's MultiActivate on the arrows).
- `open() : void`: The menu has (re)opened: ignore what arrives in the next moment.
- `setPlatform(platform:uint) : void`: The game's SetPlatform: from now on the device is the game's word.
- `keyName(action:String) : String`: How a hint shows `action`'s key on a keyboard ("" for none).
- `watch(watcher:Function) : void`: `watcher()` runs each time the device changes.
- `control(name:String, down:Boolean) : Boolean`: The menu's ProcessUserEvent(name, down): true when the control is ours and the menu used it.
- `press(action:String) : Boolean`: A click on something showing `action` (a hint): fires it as the release would.

### Modal

A pop-up over a menu: a panel outlined in Theme.color and filled black round its content, with an optional fade over the screen behind it. Back closes it; its other actions go to `handler` (the content's). Place it (x, y) before opening it: the fade is drawn from there. See docs/fork/swf_framework.md#modal

- `open(body:DisplayObject, wide:Number, high:Number) : void`: Show `body` in a `wide` by `high` panel with its top left at this pop-up's origin, in place of what it showed before.
- `close() : void`: Hide it, and tell onClose.
- `action(action:String, digit:int) : Boolean`: One of Input's actions while open: Back closes, the rest go to `handler`; false when closed.

### ScrollList

The selectable, scrolling list: a window of rows over the items, a marker on the selected one, and scroll marks when there are more. Items are {text, value, dim}: the text on the left, an optional value on the right, `dim` drawn fainter. It takes Input's actions (up and down move round, accept and a digit pick) and the mouse (over selects, a click picks); `onPick(index)` gets the pick. A row is a real child only while it shows: a hidden row would still count in a parent's bounds (the dialogue's shaded backdrop is sized by them). See docs/fork/swf_framework.md#scroll-list

- `show(list:Array, selected:int) : void`: Show `list` with item `selected` selected (kept in range), and draw.
- `top() : int`: The first item shown.
- `listHeight() : Number`: The list's height as last drawn, padding included.
- `move(by:int) : void`: Move the selection `by` items, round the ends, and draw.
- `action(action:String, digit:int) : Boolean`: One of Input's actions: true when the list used it.
- `draw() : void`: Lay out the window round the selection: each shown row's text, alpha and place, the marker, the marks.
- `pick(index:int) : void`: `index`'s item to onPick, when there is one.

### Text

Text in FO4's own font, by name (the game maps it per language; no font library is imported).

- `field(size:int, align:String = null, font:String = FONT, leading:Object = null) : TextField`: A non-selectable field in the game font and Theme.color; `leading` the extra space between lines.
- `show(field:TextField, text:String) : TextField`: `text` in `field` (a template "$KEY<tab>slot0<tab>slot1" filled first), then field()'s format put back as its default and over all of the text. See docs/fork/swf_framework.md#spikes
- `fill(text:String) : String`: `text` with a template's slots filled: "$KEY<tab>a<tab>b" is $KEY's translation with {0} as a and {1} as b. Anything else is returned as it is, for the game to translate as it is shown. See docs/fork/swf_framework.md#translations
- `translate(key:String) : String`: The game's translation of `key` ("$KEY"), read back from a field it was put in; the key itself when no file has it.
- `label(parent:Sprite, x:Number, y:Number, size:int) : TextField`: A field that sizes to its text, added to `parent` at (x, y).

### Theme

The color everything we draw takes: the player's HUD color, read from our DLL's HudColor native on each open (white without the DLL). The menu colors itself; no engine tint. See docs/fork/swf_framework.md#spikes (2b)

- `fromHud(dll:Object) : uint`: Read the HUD color again from `dll` (our natives, or null); the color now in use.

<!-- api:end -->
