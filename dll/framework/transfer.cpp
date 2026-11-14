// Our transfer and barter menus: a CND line on the item card for items that wear, each line at
// its own stack's condition (docs/fork/fo4_condition.md#transfer-menu).
//
// Each menu builds its Flash lines with a virtual function, slot 28 of ContainerMenu's and BarterMenu's vtables
// (Fallout4.exe 1.11.240: 0x103E690 and 0xA4B500; ContainerMenuBase leaves it pure). Its arguments: the menu,
// the owner's reference handle, the game's line record (an InventoryInterface handle, then the line's stack
// indices: count at +8, inline at +0x10 when the count is negative, else a pointer there) and the Flash line.
// Both slots are replaced: the original builds the line, then the line's item (BGSInventoryInterface's
// RequestInventoryItem) and first stack give it cruiCND, its condition. While a menu is open, a native enterFrame
// listener keeps a CND line at the highlighted line's cruiCND on the item card (whose InfoObj is that line's
// ItemCardInfoList).
#include "condition.h"

#include <string>

namespace crui {

namespace {

using LineFn = void (*)(void *, std::uint32_t *, std::uint8_t *, Value *);
const Ids CONTAINER_LINE_SLOT = Next(1098621, 0xE0);  // ContainerMenu's vtable, 1.11.240 RVA 0x25AC7A8
const Ids BARTER_LINE_SLOT = Next(910737, 0xE0);      // BarterMenu's vtable, 1.11.240 RVA 0x2534710
const std::size_t RECORD_COUNT = 0x8, RECORD_STACKS = 0x10;
LineFn g_containerLine = nullptr, g_barterLine = nullptr;
const char *STAMP = "cruiCND", *HOOKED = "cruiHooked";
const char *LISTS[] = {"ContainerList_mc", "PlayerInventory_mc.PlayerList_mc"};

// The health (0 to 1) of game line record `entry`'s first stack, when its item wears; -1 otherwise.
float LineHealth(std::uint8_t *entry) {
    RE::BGSInventoryInterface *inventory = entry ? RE::BGSInventoryInterface::GetSingleton() : nullptr;
    const RE::BGSInventoryItem *item =
        inventory ? inventory->RequestInventoryItem(*reinterpret_cast<std::uint32_t *>(entry)) : nullptr;
    if (!item || !item->object) return -1.0f;
    {
        std::lock_guard hold(g_lock);
        if (!g_tracked.contains(item->object->GetFormID())) return -1.0f;
    }
    auto count = Field<std::int32_t>(entry, RECORD_COUNT);
    if (!count) return -1.0f;
    std::uint16_t *stacks = count < 0 ? &Field<std::uint16_t>(entry, RECORD_STACKS)
                                      : Field<std::uint16_t *>(entry, RECORD_STACKS);
    Stack *stack = item->stackData.get();
    for (std::uint16_t n = 0; stack && n < stacks[0]; n++) stack = stack->nextStack.get();
    if (!stack) return -1.0f;
    float health = HealthOf(stack);
    return health < 0.0f ? 1.0f : health;
}

void Stamp(std::uint8_t *entry, Value *line) {
    float health = line && line->IsObject() ? LineHealth(entry) : -1.0f;
    if (health >= 0.0f) line->SetMember(STAMP, double(health));
}

void ContainerLine(void *menu, std::uint32_t *owner, std::uint8_t *entry, Value *line) {
    g_containerLine(menu, owner, entry, line);
    Stamp(entry, line);
}

void BarterLine(void *menu, std::uint32_t *owner, std::uint8_t *entry, Value *line) {
    g_barterLine(menu, owner, entry, line);
    Stamp(entry, line);
}

std::string g_base;  // the path of the open transfer menu's instance (UI thread only)

// One transfer menu frame: the highlighted line whose ItemCardInfoList the card shows gets its CND line.
void Update(Movie *movie) {
    Value card, info;
    if (!movie->GetVariable(&card, (g_base + ".ItemCard_mc").c_str()) || !Get(card, "InfoObj", &info) ||
        !info.IsArray())
        return;
    for (const char *path : LISTS) {
        Value entry, list, stamp;
        if (!movie->GetVariable(&entry, (g_base + "." + path + ".selectedEntry").c_str()) || !entry.IsObject() ||
            !Get(entry, "ItemCardInfoList", &list) || Identity(list) != Identity(info) || !Get(entry, STAMP, &stamp))
            continue;
        ShowLine(movie, card, float(Num(stamp)), false);
        return;
    }
}

class FrameHandler : public FunctionHandler {
public:
    void Call(const Params &params) override { Update(params.movie); }
};

// Replace the line builder in vtable slot `ids` with `hook`, keeping the original in `original`; false when the
// slot's address is unknown.
bool HookSlot(const Ids &ids, LineFn hook, LineFn *original) {
    std::uintptr_t slot = Address(ids);
    if (!slot) return false;
    *original = *reinterpret_cast<LineFn *>(slot);
    return REL::WriteSafe(slot, &hook, sizeof(hook));
}

}  // namespace

void InstallTransfer() {
    bool hooked = HookSlot(CONTAINER_LINE_SLOT, ContainerLine, &g_containerLine) &&
                  HookSlot(BARTER_LINE_SLOT, BarterLine, &g_barterLine);
    if (hooked) REX::INFO("hooked the transfer and barter menus' lines");
    else REX::WARN("transfer and barter menus: a line builder's address is unknown; no CND lines there");
}

void HookTransferMenu(const char *name) {
    Movie *movie = MenuMovie(name);
    if (!movie) return;
    Value code, root;
    // The transfer menu's root holds only FilterHolder_mc (played: its members were f4se and FilterHolder_mc).
    if (!FindMenu(movie, {"root.FilterHolder_mc.Menu_mc", "root.FilterHolder_mc", "root.Menu_mc", "root"},
                  "ItemCard_mc", &g_base, &root, &code)) {
        REX::WARN("{} opened, but no known path has its item card; root.FilterHolder_mc's members:", name);
        if (movie->GetVariable(&root, "root.FilterHolder_mc")) LogMembers(root);
        return;
    }
    if (code.HasMember(HOOKED)) return;
    Listen<FrameHandler>(movie, root, "enterFrame");
    code.SetMember(HOOKED, true);
    REX::DEBUG("hooked {} at {}", name, g_base);
}

}  // namespace crui
