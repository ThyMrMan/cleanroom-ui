// Our repair (docs/fork/fo4_condition.md#repair-list): FNV's repair from a spare, on one copy.
//
// A spare is an unequipped stack of the same item (another stack, or another copy of the item's own stack) or
// of an item in its FNV repair list (CRUI_Condition.TrackRepair). FNV's rule: the better of the two
// conditions, plus 5, plus 0.15 per Repair point, plus 0.05 of the worse, in percent, capped at 100. The
// default spare is the worst one that still gives the best result. Repairing removes one copy of the chosen
// spare stack with the engine's own remove-from-stack, then writes the item's stack (WriteHealth).
#include "condition.h"

#include <algorithm>
#include <unordered_map>
#include <unordered_set>

namespace crui {

namespace {

// Item form ID -> the repair lists and Jury Rigging groups mods gave it (under g_lock).
std::unordered_map<std::uint32_t, std::vector<RE::BGSListForm *>> g_repairLists;
std::unordered_map<std::uint32_t, std::unordered_set<std::int32_t>> g_juryGroups;
RE::ActorValueInfo *g_repairSkill = nullptr;
bool g_juryRigging = false;

float Whole(float health) {
    return health < 0.0f ? 1.0f : health;
}

// Whether `item` and `form` share a Jury Rigging group.
bool SameJuryGroup(RE::TESForm *item, RE::TESForm *form) {
    auto ours = g_juryGroups.find(item->GetFormID()), theirs = g_juryGroups.find(form->GetFormID());
    if (ours == g_juryGroups.end() || theirs == g_juryGroups.end()) return false;
    return std::ranges::any_of(ours->second, [&](std::int32_t group) { return theirs->second.contains(group); });
}

// Whether `form` can repair `item` (FalloutNV.exe 0x47BB50): the same item, one in any of its repair lists, or,
// with Jury Rigging, one sharing a Jury Rigging group (docs/fork/fo4_condition.md#jury-rigging).
bool CanRepairWith(RE::TESForm *item, RE::TESForm *form) {
    if (form == item || (g_juryRigging && SameJuryGroup(item, form))) return true;
    auto it = g_repairLists.find(item->GetFormID());
    if (it == g_repairLists.end()) return false;
    return std::ranges::any_of(it->second, [&](RE::BGSListForm *list) {
        return std::ranges::find(list->arrayOfForms, form) != list->arrayOfForms.end();
    });
}

}  // namespace

float RepairSkill(RE::Actor *holder) {
    std::lock_guard hold(g_lock);
    return holder && g_repairSkill ? holder->GetActorValue(*g_repairSkill) : 0.0f;
}

float RepairResult(float ours, float theirs, float skill) {
    float better = std::max(ours, theirs), worse = std::min(ours, theirs);
    return std::min(1.0f, better + 0.05f + 0.0015f * skill + 0.05f * worse);
}

std::vector<Spare> Spares(RE::Actor *holder, RE::TESForm *item, std::int32_t stack) {
    std::vector<Spare> out;
    if (!item) return out;
    std::lock_guard hold(g_lock);
    ForEachStack(holder, [&](RE::TESForm *form, Stack *s, std::int32_t n) {
        if (!form || !CanRepairWith(item, form)) return;
        bool itself = form == item && n == stack;
        if (s->IsEquipped() || (itself && s->GetCount() < 2)) return;
        out.push_back({form, n, Whole(HealthOf(s)), s->GetCount()});
    });
    return out;
}

std::int32_t DefaultSpare(const std::vector<Spare> &spares, float ours, float skill) {
    float best = -1.0f;
    for (const Spare &s : spares) best = std::max(best, RepairResult(ours, s.health, skill));
    std::int32_t chosen = -1;
    for (std::int32_t i = 0; i < std::int32_t(spares.size()); i++)
        if (RepairResult(ours, spares[i].health, skill) >= best - 0.005f &&
            (chosen < 0 || spares[i].health < spares[chosen].health))
            chosen = i;
    return chosen;
}

float RepairWith(RE::Actor *holder, RE::TESForm *item, std::int32_t stack, RE::TESForm *spare,
                 std::int32_t spareStack) {
    std::int32_t at = -1;
    Stack *target = FindStack(holder, item, stack, &at);
    if (!target || at < 0) return -1.0f;
    float ours = Whole(HealthOf(target)), skill = RepairSkill(holder);
    std::vector<Spare> spares = Spares(holder, item, at);
    const Spare *used = nullptr;
    if (!spare) {
        std::int32_t chosen = DefaultSpare(spares, ours, skill);
        used = chosen < 0 ? nullptr : &spares[chosen];
    }
    for (const Spare &s : spares)
        if (spare && s.form == spare && (spareStack < 0 ? !used || s.health < used->health : s.stack == spareStack))
            used = &s;
    if (!used) return -1.0f;
    float result = RepairResult(ours, used->health, skill);
    std::uint32_t removed;
    {
        RE::BSAutoWriteLock hold(holder->inventoryList->rwLock);
        removed = TakeFromStack(holder->inventoryList, used->form, std::uint32_t(used->stack), 1);
    }
    if (!removed) return -1.0f;
    if (used->form == item && used->stack < at && used->count == 1) at--;
    WriteHealth(holder, item, at, result);
    return result;
}

void TrackRepair(RE::TESForm *item, RE::BGSListForm *list, std::int32_t group, RE::ActorValueInfo *skill) {
    std::lock_guard hold(g_lock);
    if (item && list) {
        std::vector<RE::BGSListForm *> &lists = g_repairLists[item->GetFormID()];
        if (std::ranges::find(lists, list) == lists.end()) lists.push_back(list);
    }
    if (item && group) g_juryGroups[item->GetFormID()].insert(group);
    if (skill) g_repairSkill = skill;
}

void SetJuryRigging(bool has) {
    std::lock_guard hold(g_lock);
    g_juryRigging = has;
}

}  // namespace crui
