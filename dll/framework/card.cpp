// Other mods' lines on the Pip-Boy's inventory item card (docs/fork/swf_framework.md#card-lines): set from Papyrus
// (CRUI_ItemCard.SetLine) for one item, or for every item carrying a keyword, and drawn by our Pip-Boy movie after
// our own lines (pipboy.cpp PipboyCardLines). They are kept until a save loads or a new game starts, so a mod
// sets them again on each game load.
#include "condition.h"

#include <algorithm>
#include <string>
#include <vector>

namespace crui {

namespace {

const char *SCRIPT = "CRUI_ItemCard";

// A mod's line: the item's form ID (or a keyword's, for every item carrying it) and the line.
struct CardLine {
    std::uint32_t form;
    bool keyword;
    Row row;
};
std::vector<CardLine> g_cardLines;  // under g_lock

// Papyrus: show `label` with `value` on the item card of `item` (a Keyword: of every item carrying it), replacing
// that item's line of the same label; an empty value removes it.
void SetLine(std::monostate, RE::TESForm *item, std::string label, std::string value) {
    if (!item || label.empty()) return;
    std::lock_guard hold(g_lock);
    std::erase_if(g_cardLines, [&](const CardLine &l) { return l.form == item->GetFormID() && l.row.text == label; });
    if (!value.empty())
        g_cardLines.push_back({item->GetFormID(), item->GetFormType() == RE::ENUM_FORM_ID::kKYWD, {label, value}});
}

// Whether `form` carries the keyword of form ID `keyword`.
bool Carries(RE::TESForm *form, std::uint32_t keyword) {
    auto *keys = form ? form->As<RE::BGSKeywordForm>() : nullptr;
    return keys && keys->HasKeywordID(keyword);
}

}  // namespace

void CardLinesOf(std::uint32_t formID, std::vector<Row> *out) {
    RE::TESForm *form = FormOf(formID);
    std::lock_guard hold(g_lock);
    for (const CardLine &line : g_cardLines)
        if (line.keyword ? Carries(form, line.form) : line.form == formID) out->push_back(line.row);
}

void ForgetCardLines() {
    std::lock_guard hold(g_lock);
    g_cardLines.clear();
}

void RegisterItemCard(RE::BSScript::IVirtualMachine *vm) {
    vm->BindNativeMethod(SCRIPT, "SetLine", SetLine);
}

}  // namespace crui
