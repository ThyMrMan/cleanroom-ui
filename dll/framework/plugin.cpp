// CleanroomUI.dll: Cleanroom UI's framework for Fallout 4 (docs/spin_offs.md#split): item condition, the
// Pip-Boy's item-card lines, lists and STAT tabs, and ChangeSex, each doing nothing until a mod asks for it.
//
// Condition is the engine's own item health (ExtraHealth, 0 to 1) on an inventory stack, which the save keeps.
// Fallout 4 gives Papyrus no way to read or write it on an inventory item, so this plugin adds natives to the
// script CRUI_Condition; the rules (wear, effects, repair) live in Papyrus and records. pipboy.cpp shows the
// condition on the Pip-Boy's item card and reports the highlighted item.
// Built on CommonLibF4 (xmake.lua); everything here waits for the runtime its layouts were read in (base.h
// CheckedRuntime).
#include "condition.h"

namespace crui {

std::mutex g_lock;
std::unordered_set<std::uint32_t> g_tracked;
Selection g_selection;

Stack *FindStack(RE::TESObjectREFR *holder, RE::TESForm *item, std::int32_t index, std::int32_t *at) {
    Stack *first = nullptr, *equipped = nullptr, *indexed = nullptr;
    std::int32_t equippedAt = -1;
    RE::BGSInventoryList *list = holder && item ? holder->inventoryList : nullptr;
    if (list) {
        RE::BSAutoReadLock hold(list->rwLock);
        auto entry = std::find_if(list->data.begin(), list->data.end(),
                                  [&](const RE::BGSInventoryItem &e) { return e.object == item; });
        std::int32_t n = 0;
        for (Stack *s = entry != list->data.end() ? entry->stackData.get() : nullptr; s; s = s->nextStack.get(), n++) {
            if (!first) first = s;
            if (n == index) indexed = s;
            if (!equipped && s->IsEquipped()) equipped = s, equippedAt = n;
        }
    }
    if (at) *at = indexed ? index : equipped ? equippedAt : first ? 0 : -1;
    return indexed ? indexed : equipped ? equipped : first;
}

float HealthOf(const Stack *stack) {
    auto *health = stack && stack->extra ? stack->extra->GetByType<RE::ExtraHealth>() : nullptr;
    return health ? health->health : -1.0f;
}

RE::TESForm *EquippedWeapon(RE::Actor *actor) {
    RE::TESForm *weapon = nullptr;
    ForEachStack(actor, [&](RE::TESForm *form, Stack *s, std::int32_t) {
        if (form && form->GetFormType() == RE::ENUM_FORM_ID::kWEAP && s->IsEquipped()) weapon = form;
    });
    return weapon;
}

std::string NameOf(const RE::TESForm *form) {
    return form ? std::string(RE::TESFullName::GetFullName(*form)) : "";
}

namespace {

// The engine's own stack writer (docs/fork/fo4_condition.md#stack-writer): TESObjectREFR::FindAndWriteStackData(
// object, compare, write, objectCompare, alwaysContinue) visits the stacks of the items objectCompare matches and
// writes the first the compare functor accepts. CheckStackIDFunctor accepts the stack its countdown reaches;
// SetHealthFunctor writes health, splitting one copy off a shared stack and moving the equipped flag to it (both
// flags set, as the game's own call at 1.11.240 RVA 0x98E750 sets them).
using FindAndWrite = void (*)(RE::TESObjectREFR *, RE::TESBoundObject *,
                              RE::BGSInventoryItem::StackDataCompareFunctor &,
                              RE::BGSInventoryItem::StackDataWriteFunctor &,
                              bool (*)(RE::TESBoundObject *, RE::TESBoundObject *), bool);
const Ids FIND_AND_WRITE_STACK_DATA{0, 2200937, 2200937};  // 1.11.240 RVA 0x502560

// The engine's remove-from-stack, BGSInventoryList::RemoveItem1 (object, stack index, count, more stacks follow):
// the count removed (CommonLibF4 declares it void). It sends the inventory event, marks the list's weight dirty and
// drops an emptied stack (1.11.240 RVA 0x34E020; docs/fork/fo4_condition.md#repair-list).
using RemoveFromStack = std::uint32_t (*)(RE::BGSInventoryList *, RE::TESBoundObject *, std::uint32_t, std::uint32_t,
                                          bool);

}  // namespace

bool WriteHealth(RE::TESObjectREFR *holder, RE::TESForm *item, std::int32_t index, float value) {
    std::int32_t at = -1;
    auto *object = item ? item->As<RE::TESBoundObject>() : nullptr;
    auto write = At<FindAndWrite>(FIND_AND_WRITE_STACK_DATA);
    if (!object || !write || !FindStack(holder, item, index, &at) || at < 0) return false;
    RE::BGSInventoryItem::CheckStackIDFunctor compare{std::uint32_t(at)};
    RE::BGSInventoryItem::SetHealthFunctor health{std::clamp(value, 0.0f, 1.0f)};
    health.shouldSplitStacks = true;
    health.transferEquippedToSplitStack = true;
    write(holder, object, compare, health, RE::BGSInventoryList::StandardObjectCompareCallbackFn, false);
    return true;
}

std::uint32_t TakeFromStack(RE::BGSInventoryList *list, RE::TESForm *form, std::uint32_t stack, std::uint32_t count) {
    static REL::Relocation<RemoveFromStack> remove{RE::ID::BGSInventoryList::RemoveItem1};
    auto *object = form ? form->As<RE::TESBoundObject>() : nullptr;
    return object ? remove(list, object, stack, count, false) : 0;
}

}  // namespace crui

namespace {

using namespace crui;

const char *NAME = "CleanroomUI", *SCRIPT = "CRUI_Condition", *FRAMEWORK_SCRIPT = "CRUI_Framework";
// The F4SE message type another plugin sends us (to "CleanroomUI") to ask our version; the answer comes back to it
// with the same type and VERSION as a std::uint32_t.
const std::uint32_t VERSION_QUERY = 'CRUV';

// Another plugin's message: answer a version query.
void OnPluginMessage(F4SE::MessagingInterface::Message *msg) {
    if (msg->type != VERSION_QUERY || !msg->sender) return;
    std::uint32_t version = VERSION;
    REX::INFO("version {} asked by {}", version, msg->sender);
    F4SE::GetMessagingInterface()->Dispatch(VERSION_QUERY, &version, sizeof(version), msg->sender);
}

// F4SE messages: once every plugin has loaded, listen for other plugins' version queries; hook the Pip-Boy once the
// game has loaded (the UI exists then); forget mods' item-card lines and ammo lists as a save loads or a new game
// starts.
void OnMessage(F4SE::MessagingInterface::Message *msg) {
    using Messaging = F4SE::MessagingInterface;
    if (msg->type == Messaging::kPostLoad && !F4SE::GetMessagingInterface()->RegisterListener(OnPluginMessage, {}))
        REX::WARN("no version queries: could not listen for other plugins");
    if (msg->type == Messaging::kPreLoadGame || msg->type == Messaging::kNewGame) {
        ForgetCardLines();
        ForgetAmmoLists();
        ForgetHud();
    }
    if (msg->type == Messaging::kGameLoaded) {
        InstallPipboy();
        InstallHud();
    }
}

// F4SE's Scaleform callback: give a movie's f4se.plugins.CleanroomUI the Pip-Boy and HUD movies' natives and the
// shared ones.
bool RegisterMovie(Movie *movie, Value *plugin) {
    if (!movie || !plugin) return false;
    RegisterSharedNatives(movie, plugin);
    RegisterPipboyMenu(movie, plugin);
    RegisterHudMenu(movie, plugin);
    RegisterKeys(movie, plugin);
    return true;
}

// Papyrus: the health of `item` in `holder` (0 to 1); -1 when it is not held or has none set yet.
float GetHealth(std::monostate, RE::TESObjectREFR *holder, RE::TESForm *item) {
    return HealthOf(FindStack(holder, item));
}

// Papyrus: set `item`'s health in `holder`, adding it when it has none; false when it is not held.
bool SetHealth(std::monostate, RE::TESObjectREFR *holder, RE::TESForm *item, float value) {
    return WriteHealth(holder, item, -1, value);
}

// Papyrus: GetHealth on stack `stack` of `item` (-1: its equipped, else first; a stack it lacks: the same).
float GetStackHealth(std::monostate, RE::TESObjectREFR *holder, RE::TESForm *item, std::int32_t stack) {
    return HealthOf(FindStack(holder, item, stack));
}

// Papyrus: SetHealth on stack `stack` of `item`, as GetStackHealth picks it.
bool SetStackHealth(std::monostate, RE::TESObjectREFR *holder, RE::TESForm *item, std::int32_t stack, float value) {
    return WriteHealth(holder, item, stack, value);
}

// Papyrus: the health of the weapon `actor` has equipped (0 to 1); -1 with none or none set.
float GetEquippedWeaponHealth(std::monostate, RE::Actor *actor) {
    RE::TESForm *weapon = EquippedWeapon(actor);
    return weapon ? HealthOf(FindStack(actor, weapon)) : -1.0f;
}

// Papyrus: how many of `item` share its stack in `holder` (they share one condition); 0 when not held.
std::uint32_t StackCount(std::monostate, RE::TESObjectREFR *holder, RE::TESForm *item) {
    Stack *stack = FindStack(holder, item);
    return stack ? stack->GetCount() : 0;
}

// Papyrus: `items` wear, so the Pip-Boy shows their condition; adds to the forms already given.
void Track(std::monostate, std::vector<RE::TESForm *> items) {
    std::lock_guard hold(g_lock);
    for (RE::TESForm *form : items)
        if (form) g_tracked.insert(form->GetFormID());
}

// Papyrus: the item highlighted in the Pip-Boy's inventory when it wears; None otherwise.
RE::TESForm *PipboySelection(std::monostate) {
    std::lock_guard hold(g_lock);
    return FormOf(g_selection.form);
}

// Papyrus: the stack index of PipboySelection's item (-1 with none).
std::int32_t PipboyStack(std::monostate) {
    std::lock_guard hold(g_lock);
    return g_selection.form ? g_selection.stack : -1;
}

// Papyrus: show the highlighted item's condition again on the Pip-Boy's item card.
void PipboyRefresh(std::monostate) {
    RefreshPipboy();
}

// Papyrus: the framework's version (VERSION).
std::uint32_t GetVersion(std::monostate) {
    return VERSION;
}

// Papyrus: FNV repair lists (`lists`: weapons' first, then armors', parallel to the two), the Jury Rigging groups
// (parallel to `lists`) and the Repair skill, added to what other mods gave.
void TrackRepairLists(std::monostate, std::vector<RE::TESForm *> weapons, std::vector<RE::TESForm *> armors,
                      std::vector<RE::TESForm *> lists, std::vector<std::int32_t> groups, RE::ActorValueInfo *skill) {
    std::vector<RE::TESForm *> items = weapons;
    items.insert(items.end(), armors.begin(), armors.end());
    for (std::size_t i = 0; i < items.size() && i < lists.size(); i++) {
        RE::BGSListForm *list = lists[i] ? lists[i]->As<RE::BGSListForm>() : nullptr;
        TrackRepair(items[i], list, i < groups.size() ? groups[i] : 0, skill);
    }
    TrackRepair(nullptr, nullptr, 0, skill);
}

// Papyrus: whether the player has Jury Rigging.
void SetJuryRiggingNative(std::monostate, bool has) {
    SetJuryRigging(has);
}

// Papyrus: repair stack `stack` of `item` from one copy of `spare` (its stack `spareStack`; -1: its worst):
// the new health, or -1 with no such spare.
float RepairWithNative(std::monostate, RE::Actor *holder, RE::TESForm *item, std::int32_t stack, RE::TESForm *spare,
                       std::int32_t spareStack) {
    return RepairWith(holder, item, stack, spare, spareStack);
}

// Papyrus: how many spare stacks `holder` carries for stack `stack` of `item`.
std::uint32_t SpareCount(std::monostate, RE::Actor *holder, RE::TESForm *item, std::int32_t stack) {
    std::int32_t at = -1;
    return FindStack(holder, item, stack, &at) ? std::uint32_t(Spares(holder, item, at).size()) : 0;
}

// Papyrus: carry out the repair chosen in the Pip-Boy's repair list or vendor offer: the new health, -1 with none
// chosen, -2 short of the vendor's price.
float PipboyRepair(std::monostate, RE::Actor *holder) {
    Repair chosen;
    if (!TakePipboyRepair(&chosen)) return -1.0f;
    if (chosen.health >= 0.0f) return BuyRepair(holder, FormOf(chosen.item), chosen.stack, chosen.cost, chosen.health);
    return RepairWith(holder, FormOf(chosen.item), chosen.stack, FormOf(chosen.spare), chosen.spareStack);
}

// Papyrus: offer `vendor`'s repair services on the Pip-Boy's repair card until it closes (None: stop).
void SetVendorNative(std::monostate, RE::Actor *vendor) {
    SetVendor(vendor);
}

// Papyrus: FNV's vendor repair GMSTs (fRepairSkillBase, fRepairSkillMax, fItemRepairCostMult).
void SetServiceRulesNative(std::monostate, float skillBase, float skillMax, float costMult) {
    SetServiceRules(skillBase, skillMax, costMult);
}

bool Register(RE::BSScript::IVirtualMachine *vm) {
    RegisterPages(vm);
    RegisterAmmo(vm);
    RegisterItemCard(vm);
    RegisterSex(vm);
    RegisterPerks(vm);
    RegisterHud(vm);
    vm->BindNativeMethod(SCRIPT, "GetHealth", GetHealth);
    vm->BindNativeMethod(SCRIPT, "SetHealth", SetHealth);
    vm->BindNativeMethod(SCRIPT, "GetStackHealth", GetStackHealth);
    vm->BindNativeMethod(SCRIPT, "SetStackHealth", SetStackHealth);
    vm->BindNativeMethod(SCRIPT, "GetEquippedWeaponHealth", GetEquippedWeaponHealth);
    vm->BindNativeMethod(SCRIPT, "StackCount", StackCount);
    vm->BindNativeMethod(SCRIPT, "Track", Track);
    vm->BindNativeMethod(SCRIPT, "PipboySelection", PipboySelection);
    vm->BindNativeMethod(SCRIPT, "PipboyStack", PipboyStack);
    vm->BindNativeMethod(SCRIPT, "PipboyRefresh", PipboyRefresh);
    vm->BindNativeMethod(SCRIPT, "TrackRepair", TrackRepairLists);
    vm->BindNativeMethod(SCRIPT, "SetJuryRigging", SetJuryRiggingNative);
    vm->BindNativeMethod(SCRIPT, "RepairWith", RepairWithNative);
    vm->BindNativeMethod(SCRIPT, "SpareCount", SpareCount);
    vm->BindNativeMethod(SCRIPT, "SetVendor", SetVendorNative);
    vm->BindNativeMethod(SCRIPT, "SetServiceRules", SetServiceRulesNative);
    vm->BindNativeMethod(SCRIPT, "PipboyRepair", PipboyRepair);
    vm->BindNativeMethod(FRAMEWORK_SCRIPT, "GetVersion", GetVersion);
    REX::INFO("registered {} and {} natives", SCRIPT, FRAMEWORK_SCRIPT);
    return true;
}

}  // namespace

namespace crui {

const F4SE::TaskInterface *Tasks() {
    return F4SE::GetTaskInterface();
}

}  // namespace crui

// Old-gen F4SE asks this before loading a plugin (next-gen reads F4SEPlugin_Version instead).
F4SE_PLUGIN_QUERY(const F4SE::QueryInterface *a_f4se, F4SE::PluginInfo *a_info) {
    if (const auto *data = F4SE::PluginVersionData::GetSingleton()) {
        a_info->infoVersion = F4SE::PluginInfo::kVersion;
        a_info->name = data->GetPluginName().data();
        a_info->version = data->GetPluginVersion().pack();
    }
    return a_f4se->RuntimeVersion() >= REL::Version(F4SE::RUNTIME_1_10_163);
}

F4SE_PLUGIN_LOAD(const F4SE::LoadInterface *a_f4se) {
    F4SE::Init(a_f4se, {.logLevel = LogLevel(NAME), .logName = NAME});
    REX::INFO("{} {} loaded on {}", NAME, VERSION, RuntimeName());
    if (!CheckedRuntime()) {
        REX::WARN("{}: not checked on this game version yet; every native and menu hook stays off", NAME);
        return true;
    }
    ReadKeys();
    const F4SE::ScaleformInterface *scaleform = F4SE::GetScaleformInterface();
    if (!scaleform || !scaleform->Register(NAME, RegisterMovie)) REX::WARN("no movie natives");
    InstallTransfer();
    const F4SE::MessagingInterface *messaging = F4SE::GetMessagingInterface();
    if (!messaging || !Tasks() || !messaging->RegisterListener(OnMessage))
        REX::WARN("no Pip-Boy or HUD (messaging or task interface missing)");
    const F4SE::PapyrusInterface *papyrus = F4SE::GetPapyrusInterface();
    if (!papyrus || !papyrus->Register(Register)) REX::ERROR("no Papyrus natives");
    return true;
}
