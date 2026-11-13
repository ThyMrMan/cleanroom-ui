// CleanroomUI.dll: the item health shared by the Papyrus natives (plugin.cpp) and the Pip-Boy (pipboy.cpp).
#pragma once

#include "gfx.h"

#include "f4se/GameExtraData.h"
#include "f4se/GameFormComponents.h"
#include "f4se/GameForms.h"
#include "f4se/GameReferences.h"
#include "f4se/PapyrusVM.h"
#include "f4se/ScaleformCallbacks.h"
#include "f4se/ScaleformMovie.h"
#include "f4se/ScaleformValue.h"

#include <initializer_list>
#include <mutex>
#include <string>
#include <unordered_set>
#include <vector>

namespace crui {

// The framework's version (CRUI_Framework.GetVersion, the F4SE version query, F4SEPlugin_Version): raised whenever a
// native, event or rule changes, so a mod can ask for the one it was built against.
constexpr UInt32 VERSION = 3;

// The forms that wear (CRUI_Condition.Track): the Pip-Boy shows their condition.
extern std::mutex g_lock;
extern std::unordered_set<UInt32> g_tracked;

// The item highlighted in the Pip-Boy's inventory: its form ID (0 for none) and stack index.
struct Selection {
    UInt32 form = 0;
    SInt32 stack = -1;
};
extern Selection g_selection;

// The stack of `item` held by `holder`: stack `index` when it has one, else its equipped one, else its first;
// its index in `at` when given.
BGSInventoryItem::Stack *FindStack(TESObjectREFR *holder, TESForm *item, SInt32 index = -1, SInt32 *at = nullptr);

// A stack's health (0 to 1); -1 with none set yet.
float HealthOf(BGSInventoryItem::Stack *stack);

// Set the health of `item`'s stack `index` in `holder` (as FindStack picks it) through the engine, which splits
// one copy off a shared stack, so only that copy changes; false when it is not held.
bool WriteHealth(TESObjectREFR *holder, TESForm *item, SInt32 index, float value);

// A spare to repair an item with (repair.cpp): a stack of the item or of its repair list, its health, its count.
struct Spare {
    TESForm *form;
    SInt32 stack;
    float health;
    UInt32 count;
};

// Add `item`'s FNV repair list and Jury Rigging group (to those other mods gave it), and the Repair skill's actor
// value (CRUI_Condition.TrackRepair).
void TrackRepair(TESForm *item, BGSListForm *list, SInt32 group, ActorValueInfo *skill);

// Whether the player has Jury Rigging (CRUI_Condition.SetJuryRigging).
void SetJuryRigging(bool has);

// The holder's Repair skill.
float RepairSkill(Actor *holder);

// FNV's repaired condition (0 to 1) of an item at `ours` from a spare at `theirs`, at Repair `skill`.
float RepairResult(float ours, float theirs, float skill);

// The holder's spares for stack `stack` of `item` (unequipped stacks of it or of its repair list).
std::vector<Spare> Spares(Actor *holder, TESForm *item, SInt32 stack);

// The index in `spares` of the worst spare that still gives the best result; -1 with none.
SInt32 DefaultSpare(const std::vector<Spare> &spares, float ours, float skill);

// Repair stack `stack` of `item` from one copy of the spare's stack `spareStack` (-1: its worst stack): the new
// health, or -1 when there is no such spare.
float RepairWith(Actor *holder, TESForm *item, SInt32 stack, TESForm *spare, SInt32 spareStack);

// Remove `count` copies from stack `stack` of `form` in `list` (held for writing) with the engine's own
// remove-from-stack: the count removed.
UInt32 TakeFromStack(BGSInventoryList *list, TESForm *form, UInt32 stack, UInt32 count);

// A repair chosen in the Pip-Boy's repair list (pipboy.cpp): the item's stack, and the spare's; or, from a
// vendor (health >= 0), its price and the condition it reaches.
struct Repair {
    UInt32 item = 0;
    SInt32 stack = -1;
    UInt32 spare = 0;
    SInt32 spareStack = -1;
    UInt32 cost = 0;
    float health = -1.0f;
};

// ---- Vendor repair (vendor.cpp; docs/fork/fo4_condition.md#vendor-repair) ----

// A vendor's repair of an item: the condition it reaches and its price in caps (0: it can do nothing).
struct Offer {
    float health;
    UInt32 cost;
};

// FNV's vendor repair rules (CRUI_Condition.SetServiceRules): fRepairSkillBase and fRepairSkillMax, the cap in
// tenths at Repair 0 and 100, and fItemRepairCostMult.
void SetServiceRules(float skillBase, float skillMax, float costMult);

// The vendor whose repair services the Pip-Boy offers, with its Repair skill; None clears it.
void SetVendor(Actor *vendor);

// Whether a vendor offers repair services; its name and Repair skill when it does.
bool Vendor(std::string *name, float *skill);

// FNV's vendor repair (FalloutNV.exe 0x648230) of an item worth `value` at `health`, by a vendor at `skill`.
Offer VendorOffer(float health, UInt32 value, float skill);

// `item`'s value in caps (a weapon's or armor's own; else 0).
UInt32 ItemValue(TESForm *item);

// The caps `holder` carries.
UInt32 Caps(Actor *holder);

// Pay `cost` caps to set stack `stack` of `item` to `health`: the new health, -2 short of caps, -1 without it.
float BuyRepair(Actor *holder, TESForm *item, SInt32 stack, UInt32 cost, float health);

// The repair chosen in the Pip-Boy, cleared as it is taken; false with none.
bool TakePipboyRepair(Repair *out);

// ---- Flash menus ----

// Keep a CND line at `health` in item card `card`'s InfoObj, redrawing the card when it is added or changes.
void ShowLine(GFxMovieRoot *movie, GFxValue &card, float health, bool force);

// Hook the transfer and barter menus (transfer.cpp): each line's condition, shown on the item card.
void InstallTransfer();

// Listen for the transfer or barter menu `name` opening's frames (transfer.cpp), on the UI thread.
void HookTransferMenu(const char *name);

// Listen for the Pip-Boy opening (pipboy.cpp), with F4SE's task interface; once the game has loaded.
void InstallPipboy(void *taskInterface);

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
    SInt32 chosen = -1;
};

// The Pip-Boy's current page into `page` when it is the inventory page (it has an item card).
bool CurrentPage(GFxMovieRoot *movie, GFxValue *page);

// Turn the inventory page's modal on or off: its fade and its list's input.
void SetModal(GFxValue &page, bool on);

// Translation key `key` with `slot` for its {0}, as our movies' Text.show fills it (docs/fork/swf_framework.md#translations).
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
void CardLinesOf(UInt32 formID, std::vector<Row> *out);

// Forget every mod's lines (a save is loading or a new game starts).
void ForgetCardLines();

// Register CRUI_ItemCard's native SetLine.
void RegisterItemCard(VirtualMachine *vm);

// Send F4SE external event `event` to Papyrus from the Pip-Boy's movie; false when it could not be sent.
bool SendExternal(GFxMovieRoot *movie, const char *event);

// Make a hint's click handler in `out` (a GFxValue is never copied: its copy would release the handler twice).
using MakeClick = void (*)(GFxMovieRoot *movie, GFxValue *out);

// A button hint `label` on key `key` whose click is made by `click`, into `hint`; false when it cannot be made.
bool MakeHint(GFxMovieRoot *movie, const char *label, const char *key, MakeClick click, GFxValue *hint);

// The first stack index the game lists on Pip-Boy line `nodeID` of form `formID`, or -1.
SInt32 PipboyLineStack(UInt32 formID, UInt32 nodeID);

// Whether the repair list is open.
bool RepairListOpen();

// Give `plugin` (a movie's f4se.plugins.CleanroomUI) the Pip-Boy movie's natives PipboyCardLines and PipboyList.
void RegisterPipboyMenu(GFxMovieRoot *movie, GFxValue *plugin);

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
bool AmmoKey(GFxMovieRoot *movie, int key);

// The AMMO hint into `hint`; false when it cannot be made.
bool MakeAmmoHint(GFxMovieRoot *movie, GFxValue *hint);

// Register CRUI_PipboyAmmo's natives.
void RegisterAmmo(VirtualMachine *vm);

// ---- Pip-Boy tabs (pages.cpp; docs/fork/fo4_pipboy_tabs.md) ----

// One Pip-Boy frame for our STAT page tabs: give a newly loaded STAT page its tabs, and show the current one.
// `base` is the Pip-Boy menu instance's path.
void UpdatePages(GFxMovieRoot *movie, const std::string &base);

// Forget the STAT page given the tabs (the Pip-Boy opened again).
void ResetPages();

// Register CRUI_Pipboy's native SetTab.
void RegisterPages(VirtualMachine *vm);

// ---- The HUD host (hud.cpp; docs/swf_framework.md#hud) ----

// Listen for the HUD opening (F4SE's task interface `tasks`), to load our host into it once a mod adds a widget.
void InstallHud(void *tasks);

// Forget every mod's widgets, stats lines and messages (a save loads or a new game starts).
void ForgetHud();

// Give `plugin` (a movie's f4se.plugins.CleanroomUI) the host movie's natives HudWidgets, HudMessages and HudStats.
void RegisterHudMenu(GFxMovieRoot *movie, GFxValue *plugin);

// Register CRUI_Hud's natives.
void RegisterHud(VirtualMachine *vm);

// ---- Sex change (sex.cpp; docs/fork/fo4_pipeline.md#sex-change) ----

// Register CRUI_Sex's native ChangeSex, run through F4SE's task interface `tasks`.
void RegisterSex(VirtualMachine *vm, void *tasks);

}  // namespace crui
