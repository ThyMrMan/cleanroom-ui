// Our Pip-Boy: a CND line on the inventory item card for items that wear, a REPAIR (G) button
// hint, the AMMO (B) hint and its keys (ammo.cpp; G and B are settings, keys.cpp), and the highlighted item for
// CRUI_Condition.PipboySelection (docs/fork/fo4_condition.md#pipboy).
//
// When the Pip-Boy menu opens (a menu open event, then a UI task), its movie gets a native enterFrame listener.
// The menu's instance (root.Menu_mc) keeps its current page in CurrentPage. The inventory page
// (pipboy_invpage.swf) has List_mc (its selectedEntry: formID, nodeID, equipState) and ItemCard_mc, which
// draws the line objects in its InfoObj array ({text, value, showAsPercent}) when it is dirty. Every frame
// while the Pip-Boy is open this file finds the highlighted line's stack (the game's own StackID for the line,
// LineStack) and its health. Once per menu it loads our own movie into the menu instance (LoadOverlay), which
// asks PipboyCardLines for the card's lines each frame and keeps them in InfoObj, and draws the open repair or
// ammo list from PipboyList. This file keeps the lists' state and keys: G or the REPAIR hint opens the repair
// list (below); confirming it sends the F4SE external event CRUI_ConditionRepair to Papyrus.
#include "condition.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstring>
#include <string>

namespace crui {

namespace {

// The Pip-Boy's own keys there: Enter, X, R, Q, C, Tab, Z, and T (Grid View); G is free. No gamepad button is.
const char *MENU = "PipboyMenu", *LINE = "$CRUI_CND", *HINT = "$CRUI_REPAIR", *HOOKED = "cruiHooked";
const char *HINT_CLASS = "Shared.AS3.BSButtonHintData", *REPAIR_EVENT = "CRUI_ConditionRepair";
const std::uint32_t KEYBOARD_MOUSE = 0;  // PlatformChangeEvent.PLATFORM_PC_KB_MOUSE
std::atomic<bool> g_refresh{false};
std::string g_lastKey;             // The highlighted entry last resolved (UI thread only).
float g_health = 1.0f;             // Its health, when it wears.
bool g_wears = false;              // Whether it does.
const void *g_hintPage = nullptr;  // The inventory page given the REPAIR hint (identity only).
bool g_loggedEntry = false;
std::string g_base = "root";  // The path of the PipboyMenu instance (root, or root.Menu_mc).

// The game's Pip-Boy inventory lines (docs/fork/fo4_condition.md#pipboy-lines): PipboyDataManager's
// PipboyInventoryData (checked by its vtable) and that one's map of form ID -> its lines, at +0xF8 in it.
const Ids INVENTORY_DATA_VTABLE = Both(952887, 952887);  // 1.11.240 RVA 0x2544750
const std::size_t FORM_LINES = 0xF8;

// A node of a form's line list (circular, with a sentinel): its record starts with the line's PipboyObject.
struct LineNode {
    LineNode *next, *prev;
    RE::PipboyObject **record;
};

// One slot of the form ID map (the engine's hash set layout: free while `next` is null).
struct FormLines {
    std::uint32_t formID, pad;
    LineNode **list;
    FormLines *next;
};

// The StackID array's first stack index on line object `line`, or -1.
std::int32_t FirstStackID(RE::PipboyObject *line) {
    auto found = line->memberMap.find(RE::BSFixedString("StackID"));
    auto *stacks = found != line->memberMap.end() ? static_cast<RE::PipboyArray *>(found->second) : nullptr;
    if (!stacks || stacks->elements.empty() || !stacks->elements[0]) return -1;
    return std::int32_t(static_cast<RE::PipboyPrimitiveValue<std::uint32_t> *>(stacks->elements[0])->m_value);
}

// The first stack index the game lists on Pip-Boy line `nodeID` of form `formID` (its StackID), or -1.
std::int32_t LineStack(std::uint32_t formID, std::uint32_t nodeID) {
    RE::PipboyDataManager *manager = RE::PipboyDataManager::GetSingleton();
    std::uintptr_t vtable = Address(INVENTORY_DATA_VTABLE);
    if (!manager || !vtable || Field<std::uintptr_t>(&manager->inventoryData, 0) != vtable) return -1;
    auto *map = &Field<std::uint8_t>(&manager->inventoryData, FORM_LINES);
    auto size = Field<std::uint32_t>(map, 4);
    auto *slots = Field<FormLines *>(map, 0x20);
    for (std::uint32_t i = 0; slots && i < size; i++) {
        FormLines &slot = slots[i];
        if (!slot.next || slot.formID != formID || !slot.list || !*slot.list) continue;
        LineNode *head = *slot.list;
        for (LineNode *n = head->next; n && n != head; n = n->next) {
            RE::PipboyObject *line = n->record ? *n->record : nullptr;
            if (line && line->m_id == nodeID) return FirstStackID(line);
        }
    }
    return -1;
}

// The player's stack of form `formID` (one that wears) Pip-Boy line `nodeID` shows: the game's own stack index
// for the line (LineStack), else, logged once, the equipped stack, else the first. Sets g_selection and
// g_health; false when the form does not wear or is not held.
bool Resolve(std::uint32_t formID, std::uint32_t nodeID) {
    RE::PlayerCharacter *player = RE::PlayerCharacter::GetSingleton();
    std::int32_t stack = LineStack(formID, nodeID);
    static bool logged = false;
    if (stack < 0 && !logged)
        REX::WARN("no StackID for Pip-Boy line {} of {:08X}; using the equipped or first stack", nodeID, formID);
    logged = logged || stack < 0;
    std::lock_guard hold(g_lock);
    g_selection = Selection();
    if (!player || !g_tracked.contains(formID)) return false;
    Stack *chosen = FindStack(player, FormOf(formID), stack);
    if (!chosen) return false;
    g_selection.form = formID;
    g_selection.stack = stack;
    float health = HealthOf(chosen);
    g_health = health < 0.0f ? 1.0f : health;
    return true;
}

// Whether a mod tracks an item (CRUI_Condition.Track): only then do the REPAIR hint and G open a list.
bool RepairInUse() {
    std::lock_guard hold(g_lock);
    return !g_tracked.empty();
}

// Ask Papyrus to repair the highlighted item: the mod's script listens for the F4SE external event.
void SendRepair(Movie *movie, const char *why) {
    bool sent = SendExternal(movie, REPAIR_EVENT);
    REX::DEBUG("Pip-Boy repair ({}): {} {}", why, REPAIR_EVENT, sent ? "sent" : "not sent");
}

// ---- The repair list ----
// FNV's "repair with" menu, drawn by our Pip-Boy movie from RepairView while it is open: the Repair skill, the
// spares (name, CND) with the chosen one highlighted, and the item's CND before and after. The page's own modal
// (ModalFadeRect_mc, which its pop-ups use) blocks the page's actions and List_mc's input meanwhile.
// Keys (Flash key codes): Up or W, Down or S move; Enter or E repairs; Esc, Tab or G cancels.

struct RepairListState {
    bool open = false;
    std::vector<Spare> spares;
    std::int32_t chosen = -1;
    std::uint32_t item = 0;
    std::int32_t stack = -1;
    float health = 1.0f, skill = 0.0f;
    bool service = false;  // a vendor's offer (FNV's repair services) instead of the spares
    std::string vendor;
    Offer offer{};
    std::uint32_t caps = 0;
} g_list;  // UI thread only
Repair g_pending;
bool g_hasPending = false;          // under g_lock
Row g_say;                          // why nothing opened, on the card until the selection changes (UI thread only)
bool g_movieThere = false;          // our movie called PipboyCardLines in this Pip-Boy menu (UI thread only)
std::uint32_t g_selectedForm = 0;   // the highlighted entry's form (UI thread only)
enum : int { KEY_TAB = 9, KEY_ENTER = 13, KEY_ESC = 27, KEY_UP = 38, KEY_DOWN = 40, KEY_E = 69, KEY_S = 83,
             KEY_W = 87 };

std::string Percent(float health) {
    return std::to_string(int(std::floor(health * 100.0f + 0.5f))) + "%";
}

}  // namespace

bool CurrentPage(Movie *movie, Value *page) {
    return movie->GetVariable(page, (g_base + ".CurrentPage").c_str()) && page->IsObject() &&
           page->HasMember("ItemCard_mc");
}

namespace {

// The vendor's offer: its skill, the condition before and after, the price, the caps held.
void OfferView(ListView *view) {
    view->heading = {"$CRUI_REPAIR_SERVICES", g_list.vendor};
    view->notes = {{"$CRUI_VENDOR_REPAIR", std::to_string(int(g_list.skill))},
                   {LINE, Percent(g_list.health) + " > " + Percent(g_list.offer.health)},
                   {"$CRUI_COST", Fill("$CRUI_CAPS", std::to_string(g_list.offer.cost))},
                   {"$CRUI_YOUR_CAPS", std::to_string(g_list.caps)}};
    view->keys = {g_list.caps >= g_list.offer.cost ? "$CRUI_KEY_REPAIR" : "$CRUI_NOT_ENOUGH_CAPS", "$CRUI_KEY_CANCEL"};
}

// The open repair list, or the vendor's offer, as a view for our movie.
void RepairView(ListView *view) {
    if (g_list.service) {
        OfferView(view);
        return;
    }
    view->heading = {"$CRUI_REPAIR_SKILL", std::to_string(int(g_list.skill))};
    for (const Spare &spare : g_list.spares) {
        std::string text = NameOf(spare.form);
        if (text.empty()) text = "?";
        if (spare.count > 1) text += " (" + std::to_string(spare.count) + ")";
        view->rows.push_back({text, Percent(spare.health)});
    }
    view->chosen = g_list.chosen;
    float result = RepairResult(g_list.health, g_list.spares[g_list.chosen].health, g_list.skill);
    view->notes = {{LINE, Percent(g_list.health) + " > " + Percent(result)}};
    view->keys = {"$CRUI_KEY_REPAIR", "$CRUI_KEY_CANCEL"};
}

}  // namespace

// Turn `page`'s modal (its fade, which blocks the page's actions, and its list's input) on or off. Our movie draws
// the list above the page, fade included.
void SetModal(Value &page, bool on) {
    Value rect, list;
    if (Get(page, "ModalFadeRect_mc", &rect)) rect.SetMember("visible", on);
    if (Get(page, "List_mc", &list)) list.SetMember("disableInput", on);
}

void SayOnCard(const char *why, const char *label) {
    g_say = {label, why};
}

bool PipboyMovieThere() {
    static bool logged = false;
    if (!g_movieThere && !logged) REX::WARN("Pip-Boy: no list without our movie ({})", "CRUI_Pipboy.swf");
    logged = logged || !g_movieThere;
    return g_movieThere;
}

bool SendExternal(Movie *movie, const char *event) {
    Value f4se, name = String(movie, event);
    if (!movie->GetVariable(&f4se, "root.f4se") || !f4se.IsObject()) return false;
    return f4se.Invoke("SendExternalEvent", nullptr, &name, 1);
}

bool MakeHint(Movie *movie, const char *label, const char *key, MakeClick click, Value *hint) {
    Value args[6] = {String(movie, label), String(movie, key), String(movie, ""), String(movie, ""),
                     std::uint32_t(1), Value()};
    click(movie, &args[5]);
    movie->CreateObject(hint, HINT_CLASS, args, 6);
    if (!hint->IsObject()) REX::WARN("could not create the Pip-Boy {} hint ({})", label, HINT_CLASS);
    return hint->IsObject();
}

namespace {

// Open the vendor's offer for the highlighted item (stack chosen in g_list); on one the vendor cannot improve,
// say so on the card.
void OpenOffer(Value &page) {
    g_list.offer = VendorOffer(g_list.health, ItemValue(FormOf(g_list.item)), g_list.skill);
    g_list.caps = Caps(RE::PlayerCharacter::GetSingleton());
    REX::DEBUG("vendor repair offer for {:08X} stack {}: {:.3f} for {} caps (held {})", g_list.item, g_list.stack,
               g_list.offer.health, g_list.offer.cost, g_list.caps);
    if (!g_list.offer.cost) {
        SayOnCard(g_list.health >= 0.995f ? "$CRUI_NOT_NEEDED" : "$CRUI_BEYOND_VENDOR");
        return;
    }
    g_list.open = true;
    SetModal(page, true);
}

// Open the repair list for the highlighted item, on its default spare; on a whole item or with no spare, say so
// on the card. While a vendor offers repair services, open its offer instead.
void OpenList(Movie *movie) {
    Value page;
    if (!g_wears || !CurrentPage(movie, &page) || !PipboyMovieThere()) return;
    Selection selected;
    {
        std::lock_guard hold(g_lock);
        selected = g_selection;
    }
    RE::PlayerCharacter *player = RE::PlayerCharacter::GetSingleton();
    g_list.item = selected.form;
    g_list.stack = selected.stack;
    g_list.health = g_health;
    g_list.service = Vendor(&g_list.vendor, &g_list.skill);
    if (g_list.service) {
        OpenOffer(page);
        return;
    }
    g_list.spares = Spares(player, FormOf(selected.form), selected.stack);
    g_list.skill = RepairSkill(player);
    REX::DEBUG("Pip-Boy repair list for {:08X} stack {}: {} spares", selected.form, selected.stack,
               g_list.spares.size());
    bool whole = g_list.health >= 0.995f;
    if (whole || g_list.spares.empty()) {
        SayOnCard(whole ? "$CRUI_NOT_NEEDED" : "$CRUI_NO_SPARE");
        return;
    }
    g_list.chosen = DefaultSpare(g_list.spares, g_list.health, g_list.skill);
    g_list.open = true;
    SetModal(page, true);
}

// Close the repair list, repairing with the chosen spare when `repair`; the game redraws the item card.
void CloseList(Movie *movie, bool repair) {
    Value page;
    if (repair) {
        std::lock_guard hold(g_lock);
        const Spare *spare = g_list.service ? nullptr : &g_list.spares[g_list.chosen];
        g_pending = spare ? Repair{g_list.item, g_list.stack, spare->form->GetFormID(), spare->stack}
                          : Repair{g_list.item, g_list.stack, 0, -1, g_list.offer.cost, g_list.offer.health};
        g_hasPending = true;
    }
    g_list.open = false;
    if (CurrentPage(movie, &page)) {
        SetModal(page, false);
        page.Invoke("onListSelectionChange");
    }
    g_lastKey.clear();
    if (repair) SendRepair(movie, "list");
}

// The REPAIR hint's click: open the repair list.
class HintClick : public FunctionHandler {
public:
    void Call(const Params &params) override {
        if (!g_list.open && !AmmoListOpen()) OpenList(params.movie);
    }
};

// Whether key event `event` was typed into a text box (FallUI's list search), which our keys leave alone.
bool Typed(const Value &event) {
    Value target, type;
    return Get(event, "target", &target) && Get(target, "type", &type) && type.IsString() &&
           !std::strcmp(type.GetString(), "input");
}

// The open repair list's keys: move, repair, cancel.
void ListKey(Movie *movie, int key) {
    std::int32_t last = std::int32_t(g_list.spares.size()) - 1;
    bool vertical = key == KEY_UP || key == KEY_W || key == KEY_DOWN || key == KEY_S;
    if (vertical && !g_list.service) {
        std::int32_t step = key == KEY_UP || key == KEY_W ? -1 : 1;
        g_list.chosen = std::clamp(g_list.chosen + step, 0, std::max(last, 0));
    } else if ((key == KEY_ENTER || key == KEY_E) && (!g_list.service || g_list.caps >= g_list.offer.cost)) {
        CloseList(movie, true);
    } else if (key == KEY_ESC || key == KEY_TAB || key == KeyCode(OurKey::Repair)) {
        CloseList(movie, false);
    }
}

// The Pip-Boy's own key events: the repair key (G) opens the repair list on an item that wears, the ammo key (B)
// the ammo list (ammo.cpp); the open list's keys. Keys typed into a text box are left alone.
class KeyHandler : public FunctionHandler {
public:
    void Call(const Params &params) override {
        Value code;
        if (params.argCount < 1 || !Get(params.args[0], "keyCode", &code) || Typed(params.args[0])) return;
        int key = int(Num(code));
        if (g_list.open) ListKey(params.movie, key);
        else if (!AmmoKey(params.movie, key) && key == KeyCode(OurKey::Repair) && RepairInUse()) OpenList(params.movie);
    }
};

// Give inventory page `page` the REPAIR hint once a mod tracks an item and the AMMO hint once one gives ammo lists,
// once per page, on keyboard and mouse only.
void AddHint(Movie *movie, Value &root, Value &page) {
    if (Identity(page) == g_hintPage) return;
    bool repair = RepairInUse(), ammo = AmmoInUse();
    Value hints, bar, hint, ammoHint, platform;
    if ((!repair && !ammo) || !Get(page, "buttonHintDataV", &hints) || !Get(root, "ButtonHintBar_mc", &bar)) return;
    g_hintPage = Identity(page);
    if (Get(root, "uiPlatform", &platform) && Num(platform) != KEYBOARD_MOUSE) return;
    MakeClick click = [](Movie *m, Value *out) { CreateFunction<HintClick>(out, m); };
    if (repair && MakeHint(movie, HINT, KeyName(OurKey::Repair).c_str(), click, &hint))
        hints.Invoke("push", nullptr, &hint, 1);
    if (ammo && MakeAmmoHint(movie, &ammoHint)) hints.Invoke("push", nullptr, &ammoHint, 1);
    bar.Invoke("SetButtonHintData", nullptr, &hints, 1);
    REX::DEBUG("added the Pip-Boy hints (repair {}, ammo {})", repair, ammo);
}

// Set the equipped mark (equipState) of each of Pip-Boy `list`'s lines of form `formID` from its own stack (its
// StackID), redrawing the list when one changes: the game leaves the marks of two copies of one form as they were
// when the equipped copy changes (played 2026-10-23).
void FixEquipMarks(Value &list, std::uint32_t formID) {
    Value entries;
    RE::PlayerCharacter *player = RE::PlayerCharacter::GetSingleton();
    RE::TESForm *form = FormOf(formID);
    if (!player || !form || !Get(list, "entryList", &entries) || !entries.IsArray()) return;
    bool changed = false;
    for (std::uint32_t i = 0, n = entries.GetArraySize(); i < n; i++) {
        Value item, id, node, state;
        if (!entries.GetElement(i, &item) || !Get(item, "formID", &id) ||
            std::uint32_t(std::int64_t(Num(id))) != formID || !Get(item, "nodeID", &node))
            continue;
        std::int32_t at = LineStack(formID, std::uint32_t(Num(node)));
        Stack *stack = at < 0 ? nullptr : FindStack(player, form, at);
        if (!stack) continue;
        bool on = stack->IsEquipped();
        if (Get(item, "equipState", &state) && (Num(state) != 0.0) == on) continue;
        item.SetMember("equipState", std::uint32_t(on ? 1 : 0));
        changed = true;
    }
    if (changed) {
        list.Invoke("UpdateList");
        REX::DEBUG("fixed the Pip-Boy equipped marks of {:08X}", formID);
    }
}

// Off the inventory page: nothing is highlighted and no list is open.
void LeftInventory() {
    std::lock_guard hold(g_lock);
    g_selection = Selection();
    g_lastKey.clear();
    g_list.open = false;
    ForgetAmmoList();
}

// One Pip-Boy frame: on the inventory page, resolve the highlighted entry and keep its CND line.
void Update(Movie *movie) {
    Value root, page, card, list, entry, id, node, equip;
    UpdatePages(movie, g_base);
    if (!movie->GetVariable(&root, g_base.c_str()) || !CurrentPage(movie, &page)) {
        LeftInventory();
        return;
    }
    AddHint(movie, root, page);
    if (g_list.open || AmmoListOpen() || !Get(page, "ItemCard_mc", &card) || !Get(page, "List_mc", &list)) return;
    bool force = g_refresh.exchange(false);
    std::string key;
    std::uint32_t formID = 0;
    if (Get(list, "selectedEntry", &entry) && entry.IsObject() && Get(entry, "formID", &id)) {
        formID = std::uint32_t(std::int64_t(Num(id)));
        Get(entry, "nodeID", &node);
        Get(entry, "equipState", &equip);
        key = std::to_string(formID) + "|" + std::to_string(Num(node)) + "|" + std::to_string(Num(equip));
        if (!g_loggedEntry) LogMembers(entry);
        g_loggedEntry = true;
    }
    g_selectedForm = formID;
    if (key != g_lastKey || force) {
        if (key != g_lastKey) g_say = Row();
        g_lastKey = key;
        g_wears = !key.empty() && Resolve(formID, std::uint32_t(Num(node)));
        if (g_wears)
            REX::DEBUG("Pip-Boy line {} of {:08X}: stack {} at {:.3f}", std::uint32_t(Num(node)), formID,
                       g_selection.stack, g_health);
    }
    if (force && formID) FixEquipMarks(list, formID);
}

class FrameHandler : public FunctionHandler {
public:
    void Call(const Params &params) override { Update(params.movie); }
};

// A {text, value} object of `row`.
Value RowObject(Movie *movie, const Row &row) {
    Value out;
    movie->CreateObject(&out);
    out.SetMember("text", String(movie, row.text));
    out.SetMember("value", String(movie, row.value));
    return out;
}

// `rows`' objects pushed onto array `out`.
void PushRows(Movie *movie, const std::vector<Row> &rows, Value *out) {
    for (const Row &row : rows) out->PushBack(RowObject(movie, row));
}

// PipboyCardLines(): the lines for the highlighted item's card, [{text, value, showAsPercent}], for our Pip-Boy
// movie to keep there: the CND line of an item that wears, why a list did not open, then other mods' lines
// (card.cpp). Null while a list of ours is open or off the inventory page.
class CardLinesNative : public FunctionHandler {
public:
    void Call(const Params &params) override {
        Value page;
        g_movieThere = true;
        *params.retVal = nullptr;
        if (g_list.open || AmmoListOpen() || !CurrentPage(params.movie, &page)) return;
        params.movie->CreateArray(params.retVal);
        if (g_wears) {
            Value line;
            params.movie->CreateObject(&line);
            line.SetMember("text", String(params.movie, LINE));
            line.SetMember("value", std::floor(g_health * 100.0 + 0.5));
            line.SetMember("showAsPercent", true);
            params.retVal->PushBack(line);
        }
        std::vector<Row> rows;
        if (!g_say.text.empty()) rows.push_back(g_say);
        if (g_selectedForm) CardLinesOf(g_selectedForm, &rows);
        PushRows(params.movie, rows, params.retVal);
    }
};

// PipboyList(): the open repair or ammo list for our movie to draw, {heading, rows, chosen, notes, keys} (heading
// and keys {text, value}; rows and notes arrays of them; chosen the highlighted row, -1 for none); null when none.
class ListNative : public FunctionHandler {
public:
    void Call(const Params &params) override {
        Movie *movie = params.movie;
        ListView view;
        *params.retVal = nullptr;
        if (g_list.open) RepairView(&view);
        else if (!AmmoView(&view)) return;
        Value rows, notes;
        movie->CreateArray(&rows);
        PushRows(movie, view.rows, &rows);
        movie->CreateArray(&notes);
        PushRows(movie, view.notes, &notes);
        movie->CreateObject(params.retVal);
        params.retVal->SetMember("heading", RowObject(movie, view.heading));
        params.retVal->SetMember("rows", rows);
        params.retVal->SetMember("chosen", view.chosen);
        params.retVal->SetMember("notes", notes);
        params.retVal->SetMember("keys", RowObject(movie, view.keys));
    }
};

// Our own movie (Interface/CRUI_Pipboy.swf) loaded into the menu instance `menu` with a Loader, as F4SE's UI.Load
// does; it draws PipboyCardLines on the item card (docs/fork/swf_framework.md#pipboy).
const char *OVERLAY = "CRUI_Pipboy.swf";

void LoadOverlay(Movie *movie, Value &menu) {
    REX::DEBUG("Pip-Boy: {} {}", OVERLAY, LoadChild(movie, menu, OVERLAY) ? "loading" : "not loaded");
}

// A display object joined the menu: when it is a newly loaded STAT page, give it our tabs at once, before the
// menu's header reads its tab names (from the next frame on, a page loaded on one of ours showed none of them).
class AddedHandler : public FunctionHandler {
public:
    void Call(const Params &params) override {
        Value target;
        if (params.argCount > 0 && Get(params.args[0], "target", &target) && target.HasMember("SPECIALTab_mc"))
            UpdatePages(params.movie, g_base);
    }
};

// The open Pip-Boy menu's movie: listen for its frames, once per menu (a flag on its BGSCodeObj). On the UI thread.
void HookPipboy() {
    Movie *movie = MenuMovie(MENU);
    Value root, code;
    if (!movie) {
        REX::WARN("Pip-Boy opened, but its menu has no movie");
        return;
    }
    if (!FindMenu(movie, {"root", "root.Menu_mc"}, nullptr, &g_base, &root, &code)) {
        REX::WARN("Pip-Boy opened, but neither root nor root.Menu_mc has a BGSCodeObj; root's members:");
        if (movie->GetVariable(&root, "root")) LogMembers(root);
        return;
    }
    if (code.HasMember(HOOKED)) return;
    Listen<FrameHandler>(movie, root, "enterFrame");
    bool listening = Listen<KeyHandler>(movie, root, "keyUp");
    Listen<AddedHandler>(movie, root, "added");
    REX::DEBUG("Pip-Boy key listener {}", listening ? "added" : "not added");
    code.SetMember(HOOKED, true);
    g_movieThere = false;
    LoadOverlay(movie, root);
    g_hintPage = nullptr;
    ResetPages();
    g_list.open = false;
    ForgetAmmoList();
    g_lastKey.clear();
    Value f4se;
    REX::DEBUG("hooked the Pip-Boy menu at {} (root.f4se: {})", g_base,
               movie->GetVariable(&f4se, "root.f4se") && f4se.IsObject() ? "yes" : "no");
}

const char *TRANSFER_MENUS[] = {"ContainerMenu", "BarterMenu"};

// Menu open and close events: hook the Pip-Boy, or the transfer or barter menu (transfer.cpp), on the UI thread as
// it opens; the vendor's repair services end as the Pip-Boy closes.
class OpenSink : public RE::BSTEventSink<RE::MenuOpenCloseEvent> {
public:
    RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent &event,
                                          RE::BSTEventSource<RE::MenuOpenCloseEvent> *) override {
        const char *name = event.menuName.c_str();
        if (!event.opening && name && !std::strcmp(name, MENU)) SetVendor(nullptr);
        if (!event.opening || !name || !Tasks()) return RE::BSEventNotifyControl::kContinue;
        if (!std::strcmp(name, MENU)) {
            ReadKeys();
            Tasks()->AddUITask([] { HookPipboy(); });
        }
        for (const char *transfer : TRANSFER_MENUS)
            if (!std::strcmp(name, transfer)) Tasks()->AddUITask([transfer] { HookTransferMenu(transfer); });
        return RE::BSEventNotifyControl::kContinue;
    }
};
OpenSink g_openSink;

}  // namespace

void InstallPipboy() {
    RE::UI *ui = RE::UI::GetSingleton();
    if (ui) ui->GetEventSource<RE::MenuOpenCloseEvent>()->RegisterSink(&g_openSink);
    REX::INFO("Pip-Boy hook {}", ui ? "listening for the menu" : "could not listen for menus");
}

bool TakePipboyRepair(Repair *out) {
    std::lock_guard hold(g_lock);
    if (!g_hasPending) return false;
    *out = g_pending;
    g_hasPending = false;
    return true;
}

void RefreshPipboy() {
    g_refresh = true;
}

std::int32_t PipboyLineStack(std::uint32_t formID, std::uint32_t nodeID) {
    return LineStack(formID, nodeID);
}

bool RepairListOpen() {
    return g_list.open;
}

void RegisterPipboyMenu(Movie *movie, Value *plugin) {
    RegisterFunction<CardLinesNative>(plugin, movie, "PipboyCardLines");
    RegisterFunction<ListNative>(plugin, movie, "PipboyList");
}

void ShowLine(Movie *movie, Value &card, float health, bool force) {
    Value info, value;
    if (!Get(card, "InfoObj", &info) || !info.IsArray()) return;
    double percent = std::floor(health * 100.0 + 0.5);
    std::int32_t at = -1;
    for (std::uint32_t i = 0, n = info.GetArraySize(); i < n && at < 0; i++) {
        Value item, label;
        if (info.GetElement(i, &item) && Get(item, "text", &label) && label.IsString() &&
            !std::strcmp(label.GetString(), LINE))
            at = std::int32_t(i);
    }
    Value line;
    if (at >= 0 && info.GetElement(std::uint32_t(at), &line)) {
        if (Get(line, "value", &value) && Num(value) == percent && !force) return;
    } else {
        movie->CreateObject(&line);
        line.SetMember("text", String(movie, LINE));
        line.SetMember("showAsPercent", true);
        info.PushBack(line);
        static bool logged = false;
        if (!logged) REX::INFO("added the {} line to an item card", LINE);
        logged = true;
    }
    line.SetMember("value", percent);
    card.Invoke("onDataChange");
}

}  // namespace crui
