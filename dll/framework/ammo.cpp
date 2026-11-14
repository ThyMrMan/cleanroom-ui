// Our ammo list in the Pip-Boy (docs/fork/fo4_condition.md#ammo-list): FNV's choice of ammo for the
// highlighted gun, drawn by our Pip-Boy movie from AmmoView the way the repair list is (pipboy.cpp). The ammo key
// (B, a setting: keys.cpp) or the AMMO hint opens it on a gun of an FNV ammo list: each round of the list the player
// carries, and the loaded one, with the count carried, the loaded one marked. Not V: V is the game's own Pip-Boy
// zoom, which a list closed by any other key left on (played 2026-11-12). Confirming sends the F4SE external event
// CRUI_AmmoSwitch; the mod's script takes the choice (CRUI_PipboyAmmo.ChosenGun, TakeChoice) and attaches that
// round's ammo mod. Keys (Flash key codes): Up or W, Down or S move; Enter or E loads; Esc, Tab or the ammo key
// cancels.
#include "condition.h"

#include <algorithm>
#include <string>
#include <vector>

namespace crui {

namespace {

const char *SCRIPT = "CRUI_PipboyAmmo", *EVENT = "CRUI_AmmoSwitch", *HINT = "$CRUI_AMMO";
enum : int { KEY_TAB = 9, KEY_ENTER = 13, KEY_ESC = 27, KEY_UP = 38, KEY_DOWN = 40, KEY_E = 69, KEY_S = 83,
             KEY_W = 87 };

// One FNV ammo list (a mod's arrays): the keyword its guns carry, its first round's index there, its rounds and
// the form IDs of their ammo mods.
struct Group {
    RE::TESForm *list = nullptr;
    std::int32_t start = 0;
    std::vector<RE::TESForm *> rounds;
    std::vector<std::uint32_t> mods;
};
std::vector<Group> g_groups;     // every mod's, under g_lock
std::uint32_t g_chosenGun = 0;   // the confirmed gun, its list and round (its index in that mod's Rounds), under g_lock
RE::TESForm *g_chosenList = nullptr;
std::int32_t g_chosenRound = -1;

// The open list: `shown` holds the indexes in `group` of the rounds listed (carried, or loaded), `counts` their
// counts; `chosen` and `loaded` are positions in `shown`.
struct AmmoList {
    bool open = false;
    std::uint32_t gun = 0;
    Group group;
    std::vector<std::int32_t> shown;
    std::vector<std::uint32_t> counts;
    std::int32_t chosen = 0, loaded = 0;
} g_ammo;  // UI thread only

// The index in g_groups of gun `form`'s list (a list keyword on its record), or -1 (held under g_lock).
std::int32_t GroupOf(RE::TESForm *form) {
    auto *gun = form ? form->As<RE::TESObjectWEAP>() : nullptr;
    for (std::int32_t g = 0; gun && g < std::int32_t(g_groups.size()); g++)
        if (g_groups[g].list && gun->HasKeywordID(g_groups[g].list->GetFormID())) return g;
    return -1;
}

// How many of `form` `holder` carries, over all its stacks.
std::uint32_t Carried(RE::TESObjectREFR *holder, RE::TESForm *form) {
    std::uint32_t total = 0;
    ForEachStack(holder, [&](RE::TESForm *item, Stack *s, std::int32_t) {
        if (form && item == form) total += s->GetCount();
    });
    return total;
}

// Which of `group`'s rounds gun stack `stack` holds (an ammo mod among its instance's mods); the first when none
// does, as the gun's default combination holds the first round's mod.
std::int32_t Loaded(Stack *stack, const Group &group) {
    auto *extra = stack && stack->extra ? stack->extra->GetByType<RE::BGSObjectInstanceExtra>() : nullptr;
    if (!extra || !extra->values) return 0;
    for (const RE::BGSMod::ObjectIndexData &mod : extra->GetIndexData()) {
        auto at = std::ranges::find(group.mods, mod.objectID);
        if (at != group.mods.end()) return std::int32_t(at - group.mods.begin());
    }
    return 0;
}

bool CanLoad() {
    return g_ammo.chosen != g_ammo.loaded && g_ammo.counts[g_ammo.chosen] > 0;
}

// The rounds of g_ammo.group the player carries, and the loaded one, into g_ammo's shown list.
void ListRounds(RE::PlayerCharacter *player, std::int32_t loaded) {
    g_ammo.shown.clear();
    g_ammo.counts.clear();
    for (std::int32_t i = 0; i < std::int32_t(g_ammo.group.rounds.size()); i++) {
        std::uint32_t count = Carried(player, g_ammo.group.rounds[i]);
        if (!count && i != loaded) continue;
        if (i == loaded) g_ammo.loaded = std::int32_t(g_ammo.shown.size());
        g_ammo.shown.push_back(i);
        g_ammo.counts.push_back(count);
    }
}

// Open the ammo list for the highlighted gun, on its loaded round; on a gun of one ammo type, or with two copies
// of it (Papyrus attaches a mod to the form, not to one copy), say so on the card.
void Open(Movie *movie) {
    Value page, list, entry, id, node;
    if (!CurrentPage(movie, &page) || !PipboyMovieThere() || !Get(page, "List_mc", &list) ||
        !Get(list, "selectedEntry", &entry) || !Get(entry, "formID", &id))
        return;
    auto formID = std::uint32_t(std::int64_t(Num(id)));
    RE::TESForm *gun = FormOf(formID);
    RE::PlayerCharacter *player = RE::PlayerCharacter::GetSingleton();
    std::int32_t g;
    {
        std::lock_guard hold(g_lock);
        g = GroupOf(gun);
        if (g >= 0) g_ammo.group = g_groups[g];
    }
    if (g < 0 || !player) {
        if (gun && gun->GetFormType() == RE::ENUM_FORM_ID::kWEAP) SayOnCard("$CRUI_ONE_TYPE", HINT);
        return;
    }
    if (Carried(player, gun) > 1) {
        SayOnCard("$CRUI_DROP_A_COPY", HINT);
        return;
    }
    Get(entry, "nodeID", &node);
    std::int32_t loaded = Loaded(FindStack(player, gun, PipboyLineStack(formID, std::uint32_t(Num(node)))),
                                 g_ammo.group);
    ListRounds(player, loaded);
    g_ammo.chosen = g_ammo.loaded;
    g_ammo.gun = formID;
    g_ammo.open = true;
    REX::DEBUG("Pip-Boy ammo list for {:08X}: {} of {} rounds carried or loaded, loaded {}", formID,
               g_ammo.shown.size(), g_ammo.group.rounds.size(), loaded);
    SetModal(page, true);
}

// Close the ammo list, loading the chosen round when `load`; the game redraws the item card.
void Close(Movie *movie, bool load) {
    Value page;
    if (load) {
        std::lock_guard hold(g_lock);
        g_chosenGun = g_ammo.gun;
        g_chosenList = g_ammo.group.list;
        g_chosenRound = g_ammo.group.start + g_ammo.shown[g_ammo.chosen];
    }
    g_ammo.open = false;
    if (CurrentPage(movie, &page)) {
        SetModal(page, false);
        page.Invoke("onListSelectionChange");
    }
    if (load)
        REX::DEBUG("Pip-Boy ammo: {:08X} round {}, {} {}", g_ammo.gun, g_ammo.group.start + g_ammo.shown[g_ammo.chosen],
                   EVENT, SendExternal(movie, EVENT) ? "sent" : "not sent");
}

// The AMMO hint's click: open the ammo list.
class HintClick : public FunctionHandler {
public:
    void Call(const Params &params) override {
        if (!g_ammo.open && !RepairListOpen()) AmmoKey(params.movie, KeyCode(OurKey::Ammo));
    }
};

// ---- Papyrus (CRUI_PipboyAmmo) ----

// Ammo list `g` of a mod's arrays (its Lists, Starts, Rounds and Mods): rounds starts[g] up to starts[g + 1].
Group GroupFrom(const std::vector<RE::TESForm *> &lists, const std::vector<std::int32_t> &starts,
                const std::vector<RE::TESForm *> &rounds, const std::vector<RE::TESForm *> &mods, std::size_t g) {
    Group group{lists[g], starts[g], {}, {}};
    std::int32_t end = std::min({starts[g + 1], std::int32_t(rounds.size()), std::int32_t(mods.size())});
    for (std::int32_t m = group.start; m >= 0 && m < end; m++) {
        group.rounds.push_back(rounds[m]);
        group.mods.push_back(mods[m] ? mods[m]->GetFormID() : 0);
    }
    return group;
}

// Papyrus: a mod's ammo lists (its Lists, Starts, Rounds and Mods arrays), added to other mods'; a list given again
// replaces its old rounds. The plugin forgets them each game start. Lists of one round are left out.
void SetLists(std::monostate, std::vector<RE::TESForm *> lists, std::vector<std::int32_t> starts,
              std::vector<RE::TESForm *> rounds, std::vector<RE::TESForm *> mods) {
    std::lock_guard hold(g_lock);
    for (std::size_t g = 0; g < lists.size() && g + 1 < starts.size(); g++) {
        Group group = GroupFrom(lists, starts, rounds, mods, g);
        if (!group.list || group.rounds.size() < 2) continue;
        auto at = std::ranges::find_if(g_groups, [&](const Group &other) { return other.list == group.list; });
        if (at != g_groups.end()) *at = group;
        else g_groups.push_back(group);
    }
    REX::INFO("ammo lists: {}", g_groups.size());
}

// Papyrus: the gun whose ammo was chosen in the Pip-Boy; None with no choice waiting.
RE::TESForm *ChosenGun(std::monostate) {
    std::lock_guard hold(g_lock);
    return g_chosenRound >= 0 ? FormOf(g_chosenGun) : nullptr;
}

// Papyrus: the list of the chosen round (a mod takes the choice only from its own lists); None with no choice.
RE::TESForm *ChosenList(std::monostate) {
    std::lock_guard hold(g_lock);
    return g_chosenRound >= 0 ? g_chosenList : nullptr;
}

// Papyrus: the chosen round's index in the Rounds array of ChosenList's mod, cleared as it is taken; -1 with none.
std::int32_t TakeChoice(std::monostate) {
    std::lock_guard hold(g_lock);
    return std::exchange(g_chosenRound, -1);
}

}  // namespace

bool AmmoListOpen() {
    return g_ammo.open;
}

bool AmmoInUse() {
    std::lock_guard hold(g_lock);
    return !g_groups.empty();
}

void ForgetAmmoLists() {
    std::lock_guard hold(g_lock);
    g_groups.clear();
    g_chosenRound = -1;
}

void ForgetAmmoList() {
    g_ammo.open = false;
}

bool AmmoKey(Movie *movie, int key) {
    if (!g_ammo.open) {
        if (key != KeyCode(OurKey::Ammo) || !AmmoInUse()) return false;
        Open(movie);
        return true;
    }
    std::int32_t last = std::int32_t(g_ammo.shown.size()) - 1;
    if (key == KEY_UP || key == KEY_W || key == KEY_DOWN || key == KEY_S) {
        std::int32_t step = key == KEY_UP || key == KEY_W ? -1 : 1;
        g_ammo.chosen = std::clamp(g_ammo.chosen + step, 0, std::max(last, 0));
    } else if (key == KEY_ENTER || key == KEY_E) {
        if (g_ammo.chosen == g_ammo.loaded || CanLoad()) Close(movie, CanLoad());
    } else if (key == KEY_ESC || key == KEY_TAB || key == KeyCode(OurKey::Ammo)) {
        Close(movie, false);
    }
    return true;
}

bool AmmoView(ListView *view) {
    if (!g_ammo.open) return false;
    view->heading = {"$CRUI_AMMO_TYPE", "$CRUI_CARRIED"};
    for (std::size_t i = 0; i < g_ammo.shown.size(); i++) {
        std::string text = NameOf(g_ammo.group.rounds[g_ammo.shown[i]]);
        if (text.empty()) text = "?";
        if (std::int32_t(i) == g_ammo.loaded) text = Fill("$CRUI_LOADED", text);
        view->rows.push_back({text, std::to_string(g_ammo.counts[i])});
    }
    view->chosen = g_ammo.chosen;
    view->keys = {CanLoad() ? "$CRUI_KEY_LOAD" : "", "$CRUI_KEY_CANCEL"};
    return true;
}

bool MakeAmmoHint(Movie *movie, Value *hint) {
    MakeClick click = [](Movie *m, Value *out) { CreateFunction<HintClick>(out, m); };
    return MakeHint(movie, HINT, KeyName(OurKey::Ammo).c_str(), click, hint);
}

void RegisterAmmo(RE::BSScript::IVirtualMachine *vm) {
    vm->BindNativeMethod(SCRIPT, "SetLists", SetLists);
    vm->BindNativeMethod(SCRIPT, "ChosenGun", ChosenGun);
    vm->BindNativeMethod(SCRIPT, "ChosenList", ChosenList);
    vm->BindNativeMethod(SCRIPT, "TakeChoice", TakeChoice);
}

}  // namespace crui
