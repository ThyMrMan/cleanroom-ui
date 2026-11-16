// HUD parts and widgets shown on conditions (docs/swf_framework.md#hud-conditions), steps 1 to 5.
//
// A box (a widget on our host, or one of the game's HUD parts, "part:" and its clip) may have a rule: the
// conditions it shows under, comma separated. With none it is always shown. The rule is the player's (the place's
// `show`, from CleanroomUI_Hud.txt; "always" for none), else its mod's (SetShowRule), else the preset's ([Hud] sShow
// in CleanroomUI.ini, MCM's copy over it: "Immersive" or none). The host asks HudState() each frame for the
// conditions that hold and the linger ([Hud] fShowLinger, seconds a box stays after its last condition ends), and
// hides a box whose rule holds for none of them (as the player's hiding does: Look.as, alpha for our widgets).
//
// The conditions of step 1:
// - combat: the player is in combat (Actor::IsInCombat; Papyrus's IsInCombat calls it).
// - drawn: the player has a weapon out (its weapon state drawn or sheathing).
// - vats: V.A.T.S. is open (VATSMenu).
// - peek: our show key ([Keys] sHudPeek) is held while the game's window is in front, read with GetAsyncKeyState
//   (keyboard only; a key also bound in game does both).
// - aiming (step 3): the player aims down the sights (the player camera in its iron sights state) or through a
//   scope (ScopeMenu open). Third person's over-the-shoulder aim isn't a camera state of its own and doesn't count.
// Step 3's sneaking is the host's: the game's sneak meter on screen (ShowRules.as); so is powerarmor, the game's
// HUD in its power armor mode (docs/swf_framework.md#hud-extras).
// Step 2's are worked out by the host (ShowRules.as) from what it reads: changed (the box's value or text changed),
// low (its value under a threshold) and moved (the location text or the quest updates changed). The values come
// from HudValues: the player's health and AP as fractions, rads, the equipped weapon and its condition.
// Step 5: other mods' conditions (CRUI_Hud.SetCondition, SetConditionGlobal), given in HudState under their own
// names, and a mod's default rule for a box (CRUI_Hud.SetShowRule), between the player's and the preset's; both
// forgotten as a save loads, so each mod gives its own again.
// The settings are read again each time the HUD movie loads (a game loads), so a change in MCM applies then.
#include "condition.h"

#include <cctype>
#include <cstdlib>
#include <map>
#include <string>

// The Windows calls `peek` needs (user32 and kernel32, linked by CommonLibF4), declared as REX declares its own.
extern "C" {
__declspec(dllimport) void *__stdcall GetForegroundWindow();
__declspec(dllimport) unsigned long __stdcall GetWindowThreadProcessId(void *window, unsigned long *process);
__declspec(dllimport) unsigned long __stdcall GetCurrentProcessId();
__declspec(dllimport) short __stdcall GetAsyncKeyState(int key);
}

namespace crui {

namespace {

const float DEFAULT_LINGER = 3.0f;
// Fallout4.esm's actor values the conditions watch.
const std::uint32_t HEALTH = 0x2D4, ACTION_POINTS = 0x2D5, RADS = 0x2E1;

// The Immersive preset: the parts that only show while needed. HP and AP in fights and V.A.T.S., as they change
// (a hit, healing, AP spent or coming back) and while low; the ammo count and the explosives count with a weapon
// out and as they change (ammo picked up), the ammo also while low; the FNV preset's CND bar with a weapon out and
// while low; the compass as the location or an objective changes and while sneaking. Every one also on the show
// key. The rest are always shown (the crosshair also aims activation; the engine shows the enemy health, sneak and
// rads meters itself).
const std::map<std::string, std::string> IMMERSIVE = {
    {"part:HPMeter_mc", "combat,vats,changed,low,peek"},
    {"part:ActionPointMeter_mc", "combat,vats,changed,low,peek"},
    {"part:AmmoCount_mc", "drawn,changed,low,peek"},
    {"part:ExplosiveAmmoCount_mc", "drawn,changed,peek"},
    {"part:CRUI_WeaponCondition_mc", "drawn,low,peek"},
    {"part:CompassWidget_mc", "moved,sneaking,peek"},
};

std::mutex g_settingsLock;
std::string g_preset;
float g_linger = DEFAULT_LINGER;

// The script our Papyrus natives are on, and the conditions of ours (a mod's can't take these names).
const char *SCRIPT = "CRUI_HudNative";
const char *BUILT_IN[] = {"combat", "drawn", "vats", "peek", "aiming", "sneaking", "powerarmor", "changed", "low",
                          "moved", "always", "linger"};

// ---- Other mods' conditions and default rules (Papyrus, CRUI_HudNative, called by CRUI_Hud) ----

// A condition a mod sets: on, or while its global is not 0; the words the edit menu's list shows for it.
struct ModCondition {
    bool on = false;
    RE::TESGlobal *global = nullptr;
    std::string label;
};
std::map<std::string, ModCondition> g_conditions;  // under g_settingsLock
std::map<std::string, std::string> g_defaults;     // a mod's rule for a box, under g_settingsLock

// `text` lowercased.
std::string Lower(std::string text) {
    for (char &c : text) c = char(std::tolower(static_cast<unsigned char>(c)));
    return text;
}

// Whether `name` can be a mod's condition: letters, digits, _ and . only (a rule splits on , and :), and none of
// ours.
bool ConditionName(const std::string &name) {
    bool letters = !name.empty() && std::ranges::all_of(name, [](unsigned char c) {
        return std::isalnum(c) || c == '_' || c == '.';
    });
    return letters && std::ranges::find(BUILT_IN, Lower(name)) == std::end(BUILT_IN);
}

// Condition `name`, made when new, or null (logged) for a name it can't take.
ModCondition *Condition(const std::string &name) {
    if (!ConditionName(name)) {
        REX::WARN("HUD rules: \"{}\" can't be a mod's condition (letters, digits, _ and . only; not ours)", name);
        return nullptr;
    }
    return &g_conditions[name];
}

// Papyrus: condition `name` holds (`on`) or not; `label` is how the edit menu lists it ("" its name; a $KEY is
// translated).
void SetCondition(std::monostate, std::string name, bool on, std::string label) {
    std::lock_guard hold(g_settingsLock);
    if (ModCondition *condition = Condition(name)) {
        condition->on = on;
        condition->global = nullptr;
        if (!label.empty()) condition->label = label;
    }
}

// Papyrus: condition `name` holds while `value` is not 0 (read each frame; None: never); `label` as SetCondition.
void SetConditionGlobal(std::monostate, std::string name, RE::TESGlobal *value, std::string label) {
    std::lock_guard hold(g_settingsLock);
    if (ModCondition *condition = Condition(name)) {
        condition->on = false;
        condition->global = value;
        if (!label.empty()) condition->label = label;
    }
}

// Papyrus: take condition `name` off (a rule naming it then counts it as holding, as any unknown one).
void RemoveCondition(std::monostate, std::string name) {
    std::lock_guard hold(g_settingsLock);
    g_conditions.erase(name);
}

// Papyrus: box `name`'s rule from its mod (a widget's name, or "part:" and a HUD part's clip), used while the player
// has set none; over the preset's. "" takes it off. Spaces are dropped.
void SetShowRule(std::monostate, std::string name, std::string rule) {
    std::erase_if(rule, [](unsigned char c) { return std::isspace(c); });
    {
        std::lock_guard hold(g_settingsLock);
        if (rule.empty()) g_defaults.erase(name);
        else g_defaults[name] = rule;
    }
    std::lock_guard hold(g_lock);
    HudChanged();
}

// HudConditions(): the mods' conditions for the edit menu's list, [{name, label}] by name.
class ConditionsNative : public FunctionHandler {
public:
    void Call(const Params &params) override {
        Movie *movie = params.movie;
        movie->CreateArray(params.retVal);
        std::lock_guard hold(g_settingsLock);
        for (const auto &[name, condition] : g_conditions) {
            Value entry;
            movie->CreateObject(&entry);
            entry.SetMember("name", String(movie, name));
            entry.SetMember("label", String(movie, condition.label.empty() ? name : condition.label));
            params.retVal->PushBack(entry);
        }
    }
};

// Whether the game's window is the one in front (so a key held elsewhere doesn't count).
bool GameInFront() {
    void *front = GetForegroundWindow();
    unsigned long process = 0;
    if (front) GetWindowThreadProcessId(front, &process);
    return process == GetCurrentProcessId();
}

// Whether the menu `name` is open.
bool MenuOpen(const char *name) {
    RE::UI *ui = RE::UI::GetSingleton();
    return ui && ui->GetMenuOpen(name);
}

// Whether the player aims down the sights (the camera's iron sights state) or through a scope.
bool Aiming() {
    RE::PlayerCamera *camera = RE::PlayerCamera::GetSingleton();
    bool sighted = camera && camera->currentState &&
                   camera->currentState == camera->cameraStates[RE::CameraStates::kIronSights];
    return sighted || MenuOpen("ScopeMenu");
}

// HudState(): the conditions now, {combat, drawn, vats, peek, aiming} (each true or false), the mods' conditions
// under their own names, and linger (seconds).
class StateNative : public FunctionHandler {
public:
    void Call(const Params &params) override {
        RE::PlayerCharacter *player = RE::PlayerCharacter::GetSingleton();
        int key = KeyCode(OurKey::HudPeek);
        params.movie->CreateObject(params.retVal);
        params.retVal->SetMember("combat", player && player->IsInCombat());
        params.retVal->SetMember("drawn", player && player->GetWeaponMagicDrawn());
        params.retVal->SetMember("vats", MenuOpen("VATSMenu"));
        params.retVal->SetMember("peek", key != 0 && (GetAsyncKeyState(key) & 0x8000) != 0 && GameInFront());
        params.retVal->SetMember("aiming", Aiming());
        std::lock_guard hold(g_settingsLock);
        for (const auto &[name, condition] : g_conditions)
            params.retVal->SetMember(name, condition.on || (condition.global && condition.global->GetValue() != 0.0f));
        params.retVal->SetMember("linger", double(g_linger));
    }
};

// The actor value with form ID `id` (Fallout4.esm's own), or null.
RE::ActorValueInfo *ValueInfo(std::uint32_t id) {
    RE::TESForm *form = FormOf(id);
    return form ? form->As<RE::ActorValueInfo>() : nullptr;
}

// The player's actor value `info` as a fraction of its maximum (0 to 1); -1 without it.
double Fraction(RE::PlayerCharacter *player, RE::ActorValueInfo *info) {
    if (!player || !info) return -1.0;
    float most = player->GetPermanentActorValue(*info);
    return most > 0.0f ? double(player->GetActorValue(*info) / most) : -1.0;
}

// HudValues(): the values the conditions changed and low watch, {health, ap (fractions of their maximum), rads
// (the value itself), condition (the equipped weapon's, as the CND bar shows it; -1 for none), weapon (its form ID;
// 0 for none)}; -1 for a value not found. The host asks a few times a second.
class ValuesNative : public FunctionHandler {
public:
    void Call(const Params &params) override {
        static RE::ActorValueInfo *health = ValueInfo(HEALTH), *ap = ValueInfo(ACTION_POINTS), *rads = ValueInfo(RADS);
        RE::PlayerCharacter *player = RE::PlayerCharacter::GetSingleton();
        RE::TESForm *weapon = player ? EquippedWeapon(player) : nullptr;
        params.movie->CreateObject(params.retVal);
        params.retVal->SetMember("health", Fraction(player, health));
        params.retVal->SetMember("ap", Fraction(player, ap));
        params.retVal->SetMember("rads", player && rads ? double(player->GetActorValue(*rads)) : -1.0);
        params.retVal->SetMember("condition", WeaponCondition(player, weapon));
        params.retVal->SetMember("weapon", double(weapon ? weapon->GetFormID() : 0));
    }
};

}  // namespace

void ReadShowSettings() {
    std::string preset = Lower(IniText("Hud", "sShow")), linger = IniText("Hud", "fShowLinger");
    // Our MCM page's switch, when sShow isn't set.
    if (preset.empty() && IniText("Hud", "bImmersive") == "1") preset = "immersive";
    float seconds = DEFAULT_LINGER;
    if (!linger.empty()) {
        char *end = nullptr;
        float read = std::strtof(linger.c_str(), &end);
        if (end && *end == '\0' && read >= 0.0f && read <= 60.0f) seconds = read;
        else REX::WARN("[Hud] fShowLinger={}: not 0 to 60 seconds, keeping {:.1f}", linger, DEFAULT_LINGER);
    }
    if (preset != "" && preset != "immersive" && preset != "none") {
        REX::WARN("[Hud] sShow={}: no such preset, none used", preset);
        preset = "";
    }
    std::lock_guard hold(g_settingsLock);
    g_preset = preset == "none" ? "" : preset;
    g_linger = seconds;
    REX::DEBUG("HUD show rules: preset \"{}\", linger {:.1f} s", g_preset, g_linger);
}

std::string ShowRule(const std::string &name, const HudPlace &place) {
    if (!place.show.empty()) return place.show == "always" ? "" : place.show;
    std::lock_guard hold(g_settingsLock);
    auto own = g_defaults.find(name);
    if (own != g_defaults.end()) return own->second == "always" ? "" : own->second;
    if (g_preset != "immersive") return "";
    auto it = IMMERSIVE.find(name);
    return it != IMMERSIVE.end() ? it->second : "";
}

void RegisterHudShow(Movie *movie, Value *plugin) {
    RegisterFunction<StateNative>(plugin, movie, "HudState");
    RegisterFunction<ValuesNative>(plugin, movie, "HudValues");
    RegisterFunction<ConditionsNative>(plugin, movie, "HudConditions");
}

void ForgetShow() {
    std::lock_guard hold(g_settingsLock);
    g_conditions.clear();
    g_defaults.clear();
}

void RegisterShow(RE::BSScript::IVirtualMachine *vm) {
    vm->BindNativeMethod(SCRIPT, "SetCondition", SetCondition);
    vm->BindNativeMethod(SCRIPT, "SetConditionGlobal", SetConditionGlobal);
    vm->BindNativeMethod(SCRIPT, "RemoveCondition", RemoveCondition);
    vm->BindNativeMethod(SCRIPT, "SetShowRule", SetShowRule);
}

}  // namespace crui
