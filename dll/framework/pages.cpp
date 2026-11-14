// Our Pip-Boy tabs (docs/fork/fo4_pipboy_tabs.md): FNV's Skills, Reputation and Karma, as tabs
// of the Pip-Boy's STAT page after its own (STATUS, SPECIAL, PERKS).
//
// CRUI_Pipboy.SetTab gives a tab: its rows (a FormList of actor values on the player, or globals), the strings
// a row's value picks (none: the number shows) and the globals that show each row. The STAT page's tab names are
// the array its public TabNames getter returns, so a name pushed there is a tab the header shows and the menu
// switches to (the engine keeps the current tab number). Each tab is a copy of the page's own SPECIAL tab
// (SPECIALTab, in the menu's class domain once the page is loaded): its list (BSScrollingList of
// SPECIALListEntry rows: text, value, modifier, description) and description field move into a clip of ours on
// the page, at the SPECIAL tab's place; the copy itself never joins the stage, so it never takes SPECIAL's data.
// While its tab is current, a clip is shown, filled and given the keyboard focus.
#include "condition.h"

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cmath>
#include <string>
#include <vector>

namespace crui {

namespace {

struct Tab {
    std::string name;
    std::int32_t order;
    std::vector<std::uint32_t> rows, shown;
    std::vector<std::string> values;
};
std::vector<Tab> g_tabs;  // by order, under g_lock
std::atomic<bool> g_dirty{false};

// The STAT page given our tabs (identity only), the page's own tab count, and which of ours is shown (-1: none).
const void *g_page = nullptr;
std::uint32_t g_first = 0;
std::int32_t g_showing = -1;
std::string g_description;
const char *CLIP = "cruiTab", *SCRIPT = "CRUI_Pipboy";

std::string ClipName(std::size_t i) {
    return std::string(CLIP) + std::to_string(i);
}

// A form's value on the player (an actor value's, with its base in `base`) or a global's; false otherwise.
bool ValueOf(RE::TESForm *form, float *value, float *base) {
    RE::PlayerCharacter *player = RE::PlayerCharacter::GetSingleton();
    if (auto *av = form ? form->As<RE::ActorValueInfo>() : nullptr) {
        if (!player) return false;
        *value = player->GetActorValue(*av);
        *base = player->GetBaseActorValue(*av);
        return true;
    }
    if (auto *global = form ? form->As<RE::TESGlobal>() : nullptr) {
        *value = *base = global->GetValue();
        return true;
    }
    return false;
}

// A row's name: an actor value's own, else a global's editor ID.
std::string RowName(RE::TESForm *form) {
    if (auto *av = form->As<RE::ActorValueInfo>()) {
        const char *name = av->GetFullName();
        if (name && *name) return name;
    }
    auto *global = form->As<RE::TESGlobal>();
    const char *edid = global ? global->formEditorID.c_str() : nullptr;
    return edid ? edid : "";
}

std::string DescriptionOf(RE::TESForm *form) {
    auto *av = form->As<RE::ActorValueInfo>();
    if (!av) return "";
    RE::BSString text;
    av->GetDescription(text, av);
    const char *got = text.c_str();
    return got ? got : "";
}

// Tab `tab`'s list entries: a row per shown form, its name, its value (a string of the tab's, or the number and
// its modifier from its base) and description.
void Entries(Movie *movie, const Tab &tab, Value *out) {
    movie->CreateArray(out);
    for (std::size_t i = 0; i < tab.rows.size(); i++) {
        float value = 0.0f, base = 0.0f, shown = 1.0f, unused = 0.0f;
        RE::TESForm *form = FormOf(tab.rows[i]);
        if (!ValueOf(form, &value, &base)) continue;
        if (i < tab.shown.size() && (!ValueOf(FormOf(tab.shown[i]), &shown, &unused) || shown < 1.0f)) continue;
        auto index = std::int32_t(std::floor(value + 0.5f));
        bool named = !tab.values.empty() && index >= 0 && index < std::int32_t(tab.values.size());
        Value entry;
        movie->CreateObject(&entry);
        entry.SetMember("text", String(movie, RowName(form)));
        entry.SetMember("value", named ? String(movie, tab.values[index]) : Value(index));
        entry.SetMember("modifier", named ? 0 : std::int32_t(std::floor(value - base + 0.5f)));
        entry.SetMember("description", String(movie, DescriptionOf(form)));
        out->PushBack(entry);
    }
}

// A clip for tab `tab` at the SPECIAL tab's place: a SPECIAL tab copy's list and description; false when the copy
// could not be made.
bool MakeClip(Movie *movie, const Tab &tab, std::size_t i, const Value &special, Value *clip) {
    Value copy, list, description, x, y;
    movie->CreateObject(&copy, "SPECIALTab");
    movie->CreateObject(clip, "flash.display.MovieClip");
    if (!copy.IsObject() || !clip->IsObject() || !Get(copy, "List_mc", &list)) {
        REX::WARN("could not copy the Pip-Boy's SPECIAL tab for the {} tab", tab.name);
        return false;
    }
    list.SetMember("numListItems", std::uint32_t(std::max<std::size_t>(tab.rows.size(), 7)));
    clip->Invoke("addChild", nullptr, &list, 1);
    if (Get(copy, "Description_tf", &description)) clip->Invoke("addChild", nullptr, &description, 1);
    Get(special, "x", &x);
    Get(special, "y", &y);
    clip->SetMember("name", String(movie, ClipName(i)));
    clip->SetMember("x", x);
    clip->SetMember("y", y);
    clip->SetMember("visible", false);
    return true;
}

// Give STAT page `page` a clip per tab and the tabs' names.
bool Install(Movie *movie, Value &page, const std::vector<Tab> &tabs) {
    Value names, special;
    if (!Get(page, "TabNames", &names) || !names.IsArray() || !Get(page, "SPECIALTab_mc", &special)) return false;
    g_first = names.GetArraySize();
    for (std::size_t i = 0; i < tabs.size(); i++) {
        Value clip;
        if (!MakeClip(movie, tabs[i], i, special, &clip)) return false;
        page.Invoke("addChild", nullptr, &clip, 1);
        std::string upper = tabs[i].name;  // Papyrus strings keep the case first stored ("skills")
        std::ranges::transform(upper, upper.begin(), [](unsigned char c) { return char(std::toupper(c)); });
        names.PushBack(String(movie, upper));
    }
    REX::DEBUG("added {} Pip-Boy STAT tabs after its {}", tabs.size(), g_first);
    return true;
}

// Our tab `i`'s clip on `page` into `clip`; false when it is not there.
bool ClipOf(Movie *movie, Value &page, std::size_t i, Value *clip) {
    Value name = String(movie, ClipName(i));
    return page.Invoke("getChildByName", clip, &name, 1) && clip->IsObject();
}

// Fill tab `tab`'s list in `clip` and give it the keyboard focus.
void Fill(Movie *movie, const Tab &tab, Value &clip) {
    Value list, entries, stage, selected;
    if (!Get(clip, "List_mc", &list)) return;
    Entries(movie, tab, &entries);
    list.SetMember("entryList", entries);
    list.Invoke("InvalidateData");
    if (Get(list, "selectedIndex", &selected) && Num(selected) < 0) list.SetMember("selectedClipIndex", 0);
    if (Get(list, "stage", &stage) && stage.IsObject()) stage.SetMember("focus", list);
}

// Show the clip of our tab `shown` (-1: none) on `page`; filling and focusing its list as it shows or when
// the tabs change.
void Show(Movie *movie, Value &page, std::int32_t shown, const std::vector<Tab> &tabs, bool refill) {
    for (std::size_t i = 0; i < tabs.size(); i++) {
        Value clip;
        if (!ClipOf(movie, page, i, &clip)) continue;
        clip.SetMember("visible", std::int32_t(i) == shown);
        if (std::int32_t(i) == shown && (shown != g_showing || refill)) Fill(movie, tabs[i], clip);
    }
    g_showing = shown;
}

// Keep the shown tab's description field at its highlighted row's description.
void Describe(Movie *movie, Value &page) {
    Value clip, list, entry, text, field;
    if (!ClipOf(movie, page, std::size_t(g_showing), &clip) || !Get(clip, "List_mc", &list) ||
        !Get(list, "selectedEntry", &entry) || !Get(entry, "description", &text) || !text.IsString() ||
        g_description == text.GetString() || !Get(clip, "Description_tf", &field))
        return;
    g_description = text.GetString();
    field.SetMember("text", text);
}

// `list`'s FormIDs: its authored entries, then those scripts added (FormList.AddForm, a DLC's rows); none for None.
std::vector<std::uint32_t> ListIds(RE::BGSListForm *list) {
    std::vector<std::uint32_t> ids;
    if (!list) return ids;
    for (RE::TESForm *form : list->arrayOfForms) ids.push_back(form ? form->GetFormID() : 0);
    if (list->scriptAddedTempForms)
        for (std::uint32_t id : *list->scriptAddedTempForms) ids.push_back(id);
    return ids;
}

// Papyrus: give the Pip-Boy's STAT page tab `name` (placed by `order` among ours): `rows`' values on the player
// (actor values) or their own (globals), each picking from `values` (none: the number), shown while its global
// in `shown` (None: always) is at least 1. Rows a script added to the lists (a DLC's) follow the authored ones.
void SetTab(std::monostate, std::string name, std::int32_t order, RE::BGSListForm *rows,
            std::vector<std::string> values, RE::BGSListForm *shown) {
    Tab tab{name, order, ListIds(rows), ListIds(shown), values};
    std::lock_guard hold(g_lock);
    auto at = std::ranges::find_if(g_tabs, [&](const Tab &t) { return t.name == tab.name; });
    if (at != g_tabs.end()) *at = tab;
    else g_tabs.push_back(tab);
    std::ranges::stable_sort(g_tabs, {}, &Tab::order);
    g_dirty = true;
}

}  // namespace

void UpdatePages(Movie *movie, const std::string &base) {
    Value page, tab;
    if (!movie->GetVariable(&page, (base + ".CurrentPage").c_str()) || !page.IsObject() ||
        !page.HasMember("SPECIALTab_mc") || !movie->GetVariable(&tab, (base + ".DataObj.CurrentTab").c_str()))
        return;
    std::vector<Tab> tabs;
    {
        std::lock_guard hold(g_lock);
        tabs = g_tabs;
    }
    if (tabs.empty()) return;
    if (Identity(page) != g_page) {
        g_page = Identity(page);
        g_showing = -1;
        g_description.clear();
        if (!Install(movie, page, tabs)) return;
        Value menu;
        if (movie->GetVariable(&menu, base.c_str())) menu.Invoke("InvalidateData");
    }
    std::int32_t current = std::int32_t(Num(tab)) - std::int32_t(g_first);
    std::int32_t shown = current >= 0 && current < std::int32_t(tabs.size()) ? current : -1;
    Show(movie, page, shown, tabs, g_dirty.exchange(false));
    if (shown >= 0) Describe(movie, page);
}

void ResetPages() {
    g_page = nullptr;
}

void RegisterPages(RE::BSScript::IVirtualMachine *vm) {
    vm->BindNativeMethod(SCRIPT, "SetTab", SetTab);
}

}  // namespace crui
