// CleanroomUI.dll: the item health shared by the Papyrus natives (plugin.cpp) and the Pip-Boy (pipboy.cpp), and every
// file's entry points. Built on CommonLibF4 (base.h).
#pragma once

#include "base.h"

#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_set>
#include <vector>

namespace crui {

// The framework's version (CRUI_Framework.GetVersion, the version query, and the major part of F4SE's plugin version,
// which xmake.lua reads from here): raised whenever a native, event or rule changes, so a mod can ask for the one it
// was built against.
constexpr std::uint32_t VERSION = 17;

// The F4SE task interface our natives and menu hooks queue work on (plugin.cpp); null before the plugin loads.
const F4SE::TaskInterface *Tasks();

// The forms that wear (CRUI_Condition.Track): the Pip-Boy shows their condition.
extern std::mutex g_lock;
extern std::unordered_set<std::uint32_t> g_tracked;

// The item highlighted in the Pip-Boy's inventory: its form ID (0 for none) and stack index.
struct Selection {
    std::uint32_t form = 0;
    std::int32_t stack = -1;
};
extern Selection g_selection;

using Stack = RE::BGSInventoryItem::Stack;

// Visit each stack of every item in `holder`'s inventory under the list's read lock: `visit(item, stack, its index
// among the item's stacks)`.
template <class F>
void ForEachStack(RE::TESObjectREFR *holder, F visit) {
    RE::BGSInventoryList *list = holder ? holder->inventoryList : nullptr;
    if (!list) return;
    RE::BSAutoReadLock hold(list->rwLock);
    for (RE::BGSInventoryItem &entry : list->data) {
        std::int32_t n = 0;
        for (Stack *s = entry.stackData.get(); s; s = s->nextStack.get(), n++) visit(entry.object, s, n);
    }
}

// The stack of `item` held by `holder`: stack `index` when it has one, else its equipped one, else its first;
// its index in `at` when given.
Stack *FindStack(RE::TESObjectREFR *holder, RE::TESForm *item, std::int32_t index = -1, std::int32_t *at = nullptr);

// A stack's health (0 to 1); -1 with none set yet.
float HealthOf(const Stack *stack);

// The weapon `actor` has equipped, or null.
RE::TESForm *EquippedWeapon(RE::Actor *actor);

// Set the health of `item`'s stack `index` in `holder` (as FindStack picks it) through the engine, which splits
// one copy off a shared stack, so only that copy changes; false when it is not held.
bool WriteHealth(RE::TESObjectREFR *holder, RE::TESForm *item, std::int32_t index, float value);

// `form`'s name, "" for none.
std::string NameOf(const RE::TESForm *form);

// The form with ID `id`, or null.
inline RE::TESForm *FormOf(std::uint32_t id) {
    return id ? RE::TESForm::GetFormByID(id) : nullptr;
}

// A spare to repair an item with (repair.cpp): a stack of the item or of its repair list, its health, its count.
struct Spare {
    RE::TESForm *form;
    std::int32_t stack;
    float health;
    std::uint32_t count;
};

// Add `item`'s FNV repair list and Jury Rigging group (to those other mods gave it), and the Repair skill's actor
// value (CRUI_Condition.TrackRepair).
void TrackRepair(RE::TESForm *item, RE::BGSListForm *list, std::int32_t group, RE::ActorValueInfo *skill);

// Whether the player has Jury Rigging (CRUI_Condition.SetJuryRigging).
void SetJuryRigging(bool has);

// The holder's Repair skill.
float RepairSkill(RE::Actor *holder);

// FNV's repaired condition (0 to 1) of an item at `ours` from a spare at `theirs`, at Repair `skill`.
float RepairResult(float ours, float theirs, float skill);

// The holder's spares for stack `stack` of `item` (unequipped stacks of it or of its repair list).
std::vector<Spare> Spares(RE::Actor *holder, RE::TESForm *item, std::int32_t stack);

// The index in `spares` of the worst spare that still gives the best result; -1 with none.
std::int32_t DefaultSpare(const std::vector<Spare> &spares, float ours, float skill);

// Repair stack `stack` of `item` from one copy of the spare's stack `spareStack` (-1: its worst stack): the new
// health, or -1 when there is no such spare.
float RepairWith(RE::Actor *holder, RE::TESForm *item, std::int32_t stack, RE::TESForm *spare, std::int32_t spareStack);

// Remove `count` copies from stack `stack` of `form` in `list` (held for writing) with the engine's own
// remove-from-stack: the count removed.
std::uint32_t TakeFromStack(RE::BGSInventoryList *list, RE::TESForm *form, std::uint32_t stack, std::uint32_t count);

// A repair chosen in the Pip-Boy's repair list (pipboy.cpp): the item's stack, and the spare's; or, from a
// vendor (health >= 0), its price and the condition it reaches.
struct Repair {
    std::uint32_t item = 0;
    std::int32_t stack = -1;
    std::uint32_t spare = 0;
    std::int32_t spareStack = -1;
    std::uint32_t cost = 0;
    float health = -1.0f;
};

// ---- Vendor repair (vendor.cpp; docs/fork/fo4_condition.md#vendor-repair) ----

// A vendor's repair of an item: the condition it reaches and its price in caps (0: it can do nothing).
struct Offer {
    float health;
    std::uint32_t cost;
};

// FNV's vendor repair rules (CRUI_Condition.SetServiceRules): fRepairSkillBase and fRepairSkillMax, the cap in
// tenths at Repair 0 and 100, and fItemRepairCostMult.
void SetServiceRules(float skillBase, float skillMax, float costMult);

// The vendor whose repair services the Pip-Boy offers, with its Repair skill; None clears it.
void SetVendor(RE::Actor *vendor);

// Whether a vendor offers repair services; its name and Repair skill when it does.
bool Vendor(std::string *name, float *skill);

// FNV's vendor repair (FalloutNV.exe 0x648230) of an item worth `value` at `health`, by a vendor at `skill`.
Offer VendorOffer(float health, std::uint32_t value, float skill);

// `item`'s value in caps (a weapon's or armor's own; else 0).
std::uint32_t ItemValue(RE::TESForm *item);

// The caps `holder` carries.
std::uint32_t Caps(RE::Actor *holder);

// Pay `cost` caps to set stack `stack` of `item` to `health`: the new health, -2 short of caps, -1 without it.
float BuyRepair(RE::Actor *holder, RE::TESForm *item, std::int32_t stack, std::uint32_t cost, float health);

// The repair chosen in the Pip-Boy, cleared as it is taken; false with none.
bool TakePipboyRepair(Repair *out);

// ---- Flash menus ----

// Hook the transfer and barter menus (transfer.cpp): each line of an item that wears gets its condition.
void InstallTransfer();

// Load our transfer movie (CRUI_Transfer.swf, which shows that condition on the item card) into the transfer or
// barter menu `name` as it opens (transfer.cpp), on the UI thread.
void HookTransferMenu(const char *name);

// Listen for the Pip-Boy opening (pipboy.cpp); once the game has loaded.
void InstallPipboy();

// Ask the Pip-Boy to show the highlighted item's condition again (after a repair).
void RefreshPipboy();

// ---- Pip-Boy lists (pipboy.cpp), shared by the ammo list; our Pip-Boy movie draws them ----

// One line of a Pip-Boy list or item card: its text and value.
struct Row {
    std::string text, value;
};

// A list our Pip-Boy movie draws as a pop-up over the item card (docs/fork/swf_framework.md#pipboy): a heading,
// rows of which `chosen` is highlighted (-1: none), notes under them, and the keys.
struct ListView {
    Row heading, keys;
    std::vector<Row> rows, notes;
    std::int32_t chosen = -1;
};

// The Pip-Boy's current page into `page` when it is the inventory page (it has an item card).
bool CurrentPage(Movie *movie, Value *page);

// Turn the inventory page's modal on or off: its fade and its list's input.
void SetModal(Value &page, bool on);

// Translation key `key` with `slot` for its {0}, as our movies' Text.show fills it
// (docs/fork/swf_framework.md#translations).
inline std::string Fill(const char *key, const std::string &slot) {
    return std::string(key) + "\t" + slot;
}

// Show `label`: `why` (why nothing opened; translation keys) on the item card until the selection changes.
void SayOnCard(const char *why, const char *label = "$CRUI_REPAIR");

// Whether our Pip-Boy movie has called us since this Pip-Boy menu opened (it draws the lists); logs once when not.
bool PipboyMovieThere();

// The open ammo list as a view for the movie; false when it is closed.
bool AmmoView(ListView *view);

// ---- Other mods' item-card lines (card.cpp; docs/fork/swf_framework.md#card-lines) ----

// Append the lines mods set for item `formID` (for it, or a keyword it carries) to `out`.
void CardLinesOf(std::uint32_t formID, std::vector<Row> *out);

// Forget every mod's lines (a save is loading or a new game starts).
void ForgetCardLines();

// Register CRUI_ItemCard's native SetLine.
void RegisterItemCard(RE::BSScript::IVirtualMachine *vm);

// Send F4SE external event `event` to Papyrus from the Pip-Boy's movie; false when it could not be sent.
bool SendExternal(Movie *movie, const char *event);

// Make a hint's click handler in `out`.
using MakeClick = void (*)(Movie *movie, Value *out);

// A button hint `label` on key `key` whose click is made by `click`, into `hint`; false when it cannot be made.
bool MakeHint(Movie *movie, const char *label, const char *key, MakeClick click, Value *hint);

// The first stack index the game lists on Pip-Boy line `nodeID` of form `formID`, or -1.
std::int32_t PipboyLineStack(std::uint32_t formID, std::uint32_t nodeID);

// Whether the repair list is open.
bool RepairListOpen();

// Whether a mod tracks an item (CRUI_Condition.Track): only then do the REPAIR hint and its key open a list.
bool RepairInUse();

// Pip-Boy key `key` (a Flash key code) on the Pip-Boy's movie `movie`: the open list's keys, else the ammo or repair
// key opening its list. On the UI thread.
void PipboyKey(Movie *movie, int key);

// Give `plugin` (a movie's f4se.plugins.CleanroomUI) the Pip-Boy movie's natives PipboyCardLines and PipboyList.
void RegisterPipboyMenu(Movie *movie, Value *plugin);

// ---- The Pip-Boy on a pad (pad.cpp; docs/swf_framework.md#pipboy-pad) ----

// Listen for View on a pad, which opens the action menu over the Pip-Boy; once the game has loaded.
void InstallPad();

// Give `plugin` the action menu's natives PipboyActions, PipboyPress, PipboyListOpen and PipboyActionsDone.
void RegisterPad(Movie *movie, Value *plugin);

// ---- The ammo list (ammo.cpp; docs/fork/fo4_condition.md#ammo-list) ----

// Whether the ammo list is open.
bool AmmoListOpen();

// Whether a mod gave ammo lists (CRUI_PipboyAmmo.SetLists): only then do the AMMO hint and B open a list.
bool AmmoInUse();

// Forget every mod's ammo lists (a save loads or a new game starts; each mod gives its own again).
void ForgetAmmoLists();

// Forget an open ammo list (the page changed or the Pip-Boy opened again).
void ForgetAmmoList();

// A Pip-Boy key (Flash key code): B opens the ammo list, and the open list takes every key; false otherwise.
bool AmmoKey(Movie *movie, int key);

// The AMMO hint into `hint`; false when it cannot be made.
bool MakeAmmoHint(Movie *movie, Value *hint);

// Register CRUI_PipboyAmmo's natives.
void RegisterAmmo(RE::BSScript::IVirtualMachine *vm);

// ---- Pip-Boy tabs (pages.cpp; docs/fork/fo4_pipboy_tabs.md) ----

// One Pip-Boy frame for our STAT page tabs: give a newly loaded STAT page its tabs, and show the current one.
// `base` is the Pip-Boy menu instance's path.
void UpdatePages(Movie *movie, const std::string &base);

// Forget the STAT page given the tabs (the Pip-Boy opened again).
void ResetPages();

// Register CRUI_Pipboy's native SetTab.
void RegisterPages(RE::BSScript::IVirtualMachine *vm);

// ---- The HUD host (hud.cpp; docs/swf_framework.md#hud) ----

// Listen for the HUD opening, to load our host into it once a mod adds a widget.
void InstallHud();

// Forget every mod's widgets, stats lines and messages (a save loads or a new game starts).
void ForgetHud();

// Give `plugin` (a movie's f4se.plugins.CleanroomUI) the host movie's natives HudWidgets, HudMessages and HudStats.
void RegisterHudMenu(Movie *movie, Value *plugin);

// A widget's place on our host, set in the edit menu: its offset from where its mod puts it (fractions of the
// screen), its scale, whether the player hid it, a color it takes over its own (0xRRGGBB; -1: its own), and the
// conditions it shows under ("": the preset's; "always": none) (layout.cpp).
struct HudPlace {
    float dx = 0.0f, dy = 0.0f, scale = 1.0f;
    bool hidden = false;
    std::int32_t color = -1;
    std::string show;
};

// Widget `name`'s `place` as our movies take it, {dx, dy, scale, hidden, color, show (its rule now: ShowRule)},
// made into `out`.
void MakePlace(Movie *movie, const std::string &name, const HudPlace &place, Value *out);

// ---- Parts and widgets shown on conditions (show.cpp; docs/swf_framework.md#hud-conditions) ----

// Read [Hud] sShow (the preset) and fShowLinger again from the INI files.
void ReadShowSettings();

// The conditions widget `name` shows under, comma separated ("" always): the player's, else its mod's, else the
// preset's.
std::string ShowRule(const std::string &name, const HudPlace &place);

// Give `plugin` the host's HudState and HudValues, and the edit menu's HudConditions.
void RegisterHudShow(Movie *movie, Value *plugin);

// Forget every mod's conditions and default rules (a save loads or a new game starts).
void ForgetShow();

// Register CRUI_HudNative's SetCondition, SetConditionGlobal, RemoveCondition and SetShowRule.
void RegisterShow(RE::BSScript::IVirtualMachine *vm);

// Widget `name`'s place (the default for none). Held under g_lock.
HudPlace PlaceOf(const std::string &name);

// Whether widget `name` was added. Held under g_lock.
bool OurWidget(const std::string &name);

// A place changed: the host gets the widget list again. Held under g_lock.
void HudChanged();

// Read the saved layout (Documents\My Games\Fallout4\F4SE\CleanroomUI_Hud.txt).
void LoadLayout();

// F4SE's save callbacks for the layout when it belongs to each save ([Hud] bLayoutPerSave): cleared for a new game or
// a load, written into and read from the co-save.
void RevertLayout(const F4SE::SerializationInterface *intfc);
void SaveLayoutRecord(const F4SE::SerializationInterface *intfc);
void LoadLayoutRecord(const F4SE::SerializationInterface *intfc);

// Give `plugin` the host's HudReport and HudPlaceOf, and the edit menu's HudLayout, HudMove, HudScale, HudHide,
// HudTint, HudSetShow, HudReset and HudEditDone.
void RegisterHudLayout(Movie *movie, Value *plugin);

// Register CRUI_HudNative's natives.
void RegisterHud(RE::BSScript::IVirtualMachine *vm);

// ---- The game's HUD parts laid out from DEF_HUD.xml (parts.cpp; docs/swf_framework.md#hud-layout) ----

// The condition of `weapon` held by `player` as the HUD shows it (0 to 1; 1 while none is set yet); -1 for no
// weapon or one that doesn't wear (CRUI_Condition.Track).
double WeaponCondition(RE::Actor *player, RE::TESForm *weapon);

// Read our other HUD parts' settings again (the clock, the power armor meters) from the INI files.
void ReadPartSettings();

// Give `plugin` the host's HudPartLayout and HudExtras, and the FNV preset's HudWeaponCondition.
void RegisterHudParts(Movie *movie, Value *plugin);

// ---- Our own keys (keys.cpp; docs/swf_framework.md#keys) ----

// The keys the game has no controls for, set under [Keys] in CleanroomUI.ini.
enum class OurKey { Repair, Ammo, HudReset, HudHide, HudColor, HudPeek, HudShow, HudBackup, HudRestore };

// Read the keys again from the INI files.
void ReadKeys();

// Key `key`'s Flash key code, and its name as a hint shows it ("G", "F5").
int KeyCode(OurKey key);
std::string KeyName(OurKey key);

// Give `plugin` the edit menu's HudKeys.
void RegisterKeys(Movie *movie, Value *plugin);

// Setting `name` under [`section`] in CleanroomUI.ini, MCM's saved copy over it (base.h IniText), or "".
inline std::string IniText(const char *section, const char *name) {
    return IniText("CleanroomUI", section, name);
}

// ---- Other mods' movies in the game's menus (inject.cpp; docs/swf_framework.md#inject) ----

// Read every Data\F4SE\Plugins\CleanroomUI\Menus\*.ini, and listen for the menus they name opening; once the game
// has loaded.
void InstallInjections();

// ---- Sex change (sex.cpp; docs/fork/fo4_pipeline.md#sex-change) ----

// Register CRUI_Sex's native ChangeSex, run as a main-thread task.
void RegisterSex(RE::BSScript::IVirtualMachine *vm);

// ---- The game's perk points (perks.cpp; docs/level_up_plan.md#game-points) ----

// Register CRUI_LevelUpNative's SetGamePerkPoints, run as a main-thread task.
void RegisterPerks(RE::BSScript::IVirtualMachine *vm);

}  // namespace crui
