// Our vendor repair (docs/fork/fo4_condition.md#vendor-repair): FNV's ShowRepairMenu, offered on
// the Pip-Boy's repair card while a vendor is set (pipboy.cpp).
//
// FalloutNV.exe's rule (0x648230, called by RepairServicesMenu 0x7B7920 per item): the vendor reaches
// fRepairSkillBase + (fRepairSkillMax - fRepairSkillBase) x Repair / 100 tenths of condition (FalloutNV.esm: 4
// and 10, so 40% plus 0.6% a point); the price is fItemRepairCostMult (2) x the item's value x the tenths gained
// / 10, nothing at 0.5 or less, else at least 1 cap. Paying removes the caps with the engine's remove-from-stack.
#include "condition.h"

#include <algorithm>
#include <cmath>

namespace crui {

namespace {

const std::uint32_t CAPS = 0x0000000F;  // Fallout4.esm's Caps001
float g_skillBase = 4.0f, g_skillMax = 10.0f, g_costMult = 2.0f;  // under g_lock
bool g_vendor = false;
std::string g_vendorName;
float g_vendorSkill = 0.0f;

}  // namespace

void SetServiceRules(float skillBase, float skillMax, float costMult) {
    std::lock_guard hold(g_lock);
    g_skillBase = skillBase, g_skillMax = skillMax, g_costMult = costMult;
}

void SetVendor(RE::Actor *vendor) {
    float skill = vendor ? RepairSkill(vendor) : 0.0f;
    std::string name = vendor ? NameOf(vendor->GetObjectReference()) : "";
    std::lock_guard hold(g_lock);
    if (!vendor && !g_vendor) return;
    g_vendor = vendor != nullptr;
    g_vendorName = name;
    g_vendorSkill = skill;
    REX::DEBUG("repair vendor {} (Repair {:.0f})", g_vendor ? g_vendorName : "cleared", skill);
}

bool Vendor(std::string *name, float *skill) {
    std::lock_guard hold(g_lock);
    *name = g_vendorName;
    *skill = g_vendorSkill;
    return g_vendor;
}

Offer VendorOffer(float health, std::uint32_t value, float skill) {
    std::lock_guard hold(g_lock);
    float cap = g_skillBase + (g_skillMax - g_skillBase) * skill / 100.0f;
    float cost = g_costMult * float(value) * (cap - health * 10.0f) / 10.0f;
    if (cost <= 0.5f) return {health, 0};
    return {std::min(1.0f, cap / 10.0f), std::max<std::uint32_t>(1, std::uint32_t(std::lround(cost)))};
}

std::uint32_t ItemValue(RE::TESForm *item) {
    if (auto *weapon = item ? item->As<RE::TESObjectWEAP>() : nullptr) return weapon->weaponData.value;
    if (auto *armor = item ? item->As<RE::TESObjectARMO>() : nullptr) return armor->armorData.value;
    return 0;
}

std::uint32_t Caps(RE::Actor *holder) {
    std::uint32_t total = 0;
    ForEachStack(holder, [&](RE::TESForm *form, Stack *s, std::int32_t) {
        if (form && form->GetFormID() == CAPS) total += s->GetCount();
    });
    return total;
}

float BuyRepair(RE::Actor *holder, RE::TESForm *item, std::int32_t stack, std::uint32_t cost, float health) {
    std::int32_t at = -1;
    if (!FindStack(holder, item, stack, &at) || at < 0) return -1.0f;
    if (Caps(holder) < cost) return -2.0f;
    RE::TESForm *caps = FormOf(CAPS);
    {
        RE::BSAutoWriteLock hold(holder->inventoryList->rwLock);
        for (std::uint32_t paid = 0, removed = 1; paid < cost && removed;) {
            removed = TakeFromStack(holder->inventoryList, caps, 0, cost - paid);
            paid += removed;
        }
    }
    WriteHealth(holder, item, at, health);
    return health;
}

}  // namespace crui
