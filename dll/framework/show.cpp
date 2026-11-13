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
// - combat: the player is in combat (Actor::IsInCombat, the virtual F4SE names; Papyrus's IsInCombat calls it).
// - drawn: the player has a weapon out (ActorState::IsWeaponDrawn).
// - vats: V.A.T.S. is open (VATSMenu).
// - peek: our show key ([Keys] sHudPeek) is held while the game's window is in front, read with GetAsyncKeyState
//   (keyboard only; a key also bound in game does both).
// - aiming (step 3): the player aims down the sights (the player camera in its iron sights state, F4SE's
//   PlayerCamera) or through a scope (ScopeMenu open). Third person's over-the-shoulder aim isn't a camera state of
//   its own and doesn't count.
// Step 3's sneaking is the host's: the game's sneak meter on screen (ShowRules.as).
// Step 2's are worked out by the host (ShowRules.as) from what it reads: changed (the box's value or text changed),
// low (its value under a threshold) and moved (the location text or the quest updates changed). The values come
// from HudValues: the player's health and AP as fractions, rads, the equipped weapon and its condition.
// Step 5: other mods' conditions (CRUI_Hud.SetCondition, SetConditionGlobal), given in HudState under their own
// names, and a mod's default rule for a box (CRUI_Hud.SetShowRule), between the player's and the preset's; both
// forgotten as a save loads, so each mod gives its own again.
// The settings are read again each time the HUD movie loads (a game loads), so a change in MCM applies then.
#include "condition.h"

#include "f4se/GameCamera.h"
#include "f4se/GameMenus.h"
#include "f4se/GameReferences.h"
#include "f4se/PapyrusArgs.h"
#include "f4se/PapyrusNativeFunctions.h"
#include "f4se/ScaleformCallbacks.h"

#include <cctype>
#include <cstdlib>
#include <map>
#include <string>

namespace crui {

namespace {

const char *INI_FILES[] = {".\\Data\\F4SE\\Plugins\\CleanroomUI.ini", ".\\Data\\MCM\\Settings\\CleanroomUI.ini"};
const float DEFAULT_LINGER = 3.0f;
// Fallout4.esm's actor values the conditions watch.
const UInt32 HEALTH = 0x2D4, ACTION_POINTS = 0x2D5, RADS = 0x2E1;

// The Immersive preset: the parts that only show while needed. HP and AP in fights and V.A.T.S., as they change
// (a hit, healing, AP spent or coming back) and while low; the ammo count and the explosives count with a weapon
// out and as they change (ammo picked up), the ammo also while low; the FNV preset's CND bar with a weapon out and
// while low; the compass as the location or an objective changes and while sneaking. Every one also on the show
// key. The rest are
// always shown (the crosshair also aims activation; the engine shows the enemy health, sneak and rads meters
// itself).
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
const char *BUILT_IN[] = {"combat", "drawn", "vats", "peek", "aiming", "sneaking", "changed", "low", "moved",
                          "always", "linger"};

// ---- Other mods' conditions and default rules (Papyrus, CRUI_HudNative, called by CRUI_Hud) ----

// A condition a mod sets: on, or while its global is not 0; the words the edit menu's list shows for it.
struct ModCondition {
    bool on = false;
    TESGlobal *global = nullptr;
    std::string label;
};
std::map<std::string, ModCondition> g_conditions;  // under g_settingsLock
std::map<std::string, std::string> g_defaults;     // a mod's rule for a box, under g_settingsLock

std::string Text(BSFixedString &text) {
    return text.c_str() ? text.c_str() : "";
}

// Whether `name` can be a mod's condition: letters, digits, _ and . only (a rule splits on , and :), and none of
// ours.
bool ConditionName(const std::string &name) {
    if (name.empty()) return false;
    for (char c : name)
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_' && c != '.') return false;
    std::string lower;
    for (char c : name) lower += char(std::tolower(static_cast<unsigned char>(c)));
    for (const char *ours : BUILT_IN)
        if (lower == ours) return false;
    return true;
}

// Condition `name`, made when new, or null (logged) for a name it can't take.
ModCondition *Condition(const std::string &name) {
    if (!ConditionName(name)) {
        _WARNING("HUD rules: \"%s\" can't be a mod's condition (letters, digits, _ and . only; not one of ours)",
                 name.c_str());
        return nullptr;
    }
    return &g_conditions[name];
}

// Papyrus: condition `name` holds (`on`) or not; `label` is how the edit menu lists it ("" its name; a $KEY is
// translated).
void SetCondition(StaticFunctionTag *, BSFixedString name, bool on, BSFixedString label) {
    std::lock_guard<std::mutex> hold(g_settingsLock);
    if (ModCondition *condition = Condition(Text(name))) {
        condition->on = on;
        condition->global = nullptr;
        if (!Text(label).empty()) condition->label = Text(label);
    }
}

// Papyrus: condition `name` holds while `value` is not 0 (read each frame; None: never); `label` as SetCondition.
void SetConditionGlobal(StaticFunctionTag *, BSFixedString name, TESGlobal *value, BSFixedString label) {
    std::lock_guard<std::mutex> hold(g_settingsLock);
    if (ModCondition *condition = Condition(Text(name))) {
        condition->on = false;
        condition->global = value;
        if (!Text(label).empty()) condition->label = Text(label);
    }
}

// Papyrus: take condition `name` off (a rule naming it then counts it as holding, as any unknown one).
void RemoveCondition(StaticFunctionTag *, BSFixedString name) {
    std::lock_guard<std::mutex> hold(g_settingsLock);
    g_conditions.erase(Text(name));
}

// Papyrus: box `name`'s rule from its mod (a widget's name, or "part:" and a HUD part's clip), used while the player
// has set none; over the preset's. "" takes it off. Spaces are dropped.
void SetShowRule(StaticFunctionTag *, BSFixedString name, BSFixedString rule) {
    std::string kept;
    for (char c : Text(rule))
        if (!std::isspace(static_cast<unsigned char>(c))) kept += c;
    {
        std::lock_guard<std::mutex> hold(g_settingsLock);
        if (kept.empty()) g_defaults.erase(Text(name));
        else g_defaults[Text(name)] = kept;
    }
    std::lock_guard<std::mutex> hold(g_lock);
    HudChanged();
}

// HudConditions(): the mods' conditions for the edit menu's list, [{name, label}] by name.
class ConditionsNative : public GFxFunctionHandler {
public:
    void Invoke(Args *args) override {
        GFxMovieRoot *movie = args->movie->movieRoot;
        movie->CreateArray(args->result);
        std::lock_guard<std::mutex> hold(g_settingsLock);
        for (const auto &[name, condition] : g_conditions) {
            GFxValue entry, text, label;
            movie->CreateObject(&entry);
            movie->CreateString(&text, name.c_str());
            movie->CreateString(&label, (condition.label.empty() ? name : condition.label).c_str());
            entry.SetMember("name", &text);
            entry.SetMember("label", &label);
            args->result->PushBack(&entry);
        }
    }
};

// Setting `name` under [Hud] in the INI files (the later file wins), or "".
std::string Setting(const char *name) {
    std::string text;
    char buffer[64];
    for (const char *file : INI_FILES)
        if (GetPrivateProfileStringA("Hud", name, "", buffer, sizeof(buffer), file) > 0) text = buffer;
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.back()))) text.pop_back();
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.front()))) text.erase(0, 1);
    return text;
}

// Whether the game's window is the one in front (so a key held elsewhere doesn't count).
bool GameInFront() {
    HWND front = GetForegroundWindow();
    DWORD process = 0;
    if (front) GetWindowThreadProcessId(front, &process);
    return process == GetCurrentProcessId();
}

// HudState(): the conditions now, {combat, drawn, vats, peek, aiming} (each true or false), and linger (seconds).
class StateNative : public GFxFunctionHandler {
public:
    void Invoke(Args *args) override {
        PlayerCharacter *player = *g_player;
        BSFixedString vatsMenu("VATSMenu");
        int key = KeyCode(OurKey::HudPeek);
        float linger;
        {
            std::lock_guard<std::mutex> hold(g_settingsLock);
            linger = g_linger;
        }
        PlayerCamera *camera = *g_playerCamera;
        BSFixedString scopeMenu("ScopeMenu");
        bool sighted = camera && camera->cameraState &&
                       camera->GetCameraStateId(camera->cameraState) == PlayerCamera::kCameraState_IronSights;
        GFxValue aiming(sighted || (*g_ui != nullptr && (*g_ui)->IsMenuOpen(scopeMenu)));
        GFxValue combat(player != nullptr && player->IsInCombat()),
            drawn(player != nullptr && player->actorState.IsWeaponDrawn()),
            vats(*g_ui != nullptr && (*g_ui)->IsMenuOpen(vatsMenu)),
            peek(key != 0 && (GetAsyncKeyState(key) & 0x8000) != 0 && GameInFront()), seconds{double(linger)};
        args->movie->movieRoot->CreateObject(args->result);
        args->result->SetMember("combat", &combat);
        args->result->SetMember("drawn", &drawn);
        args->result->SetMember("vats", &vats);
        args->result->SetMember("peek", &peek);
        args->result->SetMember("aiming", &aiming);
        // The mods' conditions, under their own names.
        std::lock_guard<std::mutex> hold(g_settingsLock);
        for (const auto &[name, condition] : g_conditions) {
            GFxValue holds(condition.on || (condition.global != nullptr && condition.global->value != 0.0f));
            args->result->SetMember(name.c_str(), &holds);
        }
        args->result->SetMember("linger", &seconds);
    }
};

// The actor value with form ID `id` (Fallout4.esm's own), or null.
ActorValueInfo *ValueInfo(UInt32 id) {
    TESForm *form = LookupFormByID(id);
    return form && form->formType == kFormType_AVIF ? static_cast<ActorValueInfo *>(form) : nullptr;
}

// The player's actor value `info` as a fraction of its maximum (0 to 1); -1 without it.
double Fraction(PlayerCharacter *player, ActorValueInfo *info) {
    if (!player || !info) return -1.0;
    float most = player->actorValueOwner.GetMaximum(info);
    return most > 0.0f ? double(player->actorValueOwner.GetValue(info) / most) : -1.0;
}

// HudValues(): the values the conditions changed and low watch, {health, ap (fractions of their maximum), rads
// (the value itself), condition (the equipped weapon's, as the CND bar shows it; -1 for none), weapon (its form ID;
// 0 for none)}; -1 for a value not found. The host asks a few times a second.
class ValuesNative : public GFxFunctionHandler {
public:
    void Invoke(Args *args) override {
        static ActorValueInfo *health = ValueInfo(HEALTH), *ap = ValueInfo(ACTION_POINTS), *rads = ValueInfo(RADS);
        PlayerCharacter *player = *g_player;
        TESForm *weapon = player ? EquippedWeapon(player) : nullptr;
        GFxValue h{Fraction(player, health)}, a{Fraction(player, ap)},
            r{player && rads ? double(player->actorValueOwner.GetValue(rads)) : -1.0},
            c{WeaponCondition(player, weapon)}, w{double(weapon ? weapon->formID : 0)};
        args->movie->movieRoot->CreateObject(args->result);
        args->result->SetMember("health", &h);
        args->result->SetMember("ap", &a);
        args->result->SetMember("rads", &r);
        args->result->SetMember("condition", &c);
        args->result->SetMember("weapon", &w);
    }
};

}  // namespace

void ReadShowSettings() {
    std::string preset = Setting("sShow"), linger = Setting("fShowLinger");
    for (char &c : preset) c = char(std::tolower(static_cast<unsigned char>(c)));
    float seconds = DEFAULT_LINGER;
    if (!linger.empty()) {
        char *end = nullptr;
        float read = std::strtof(linger.c_str(), &end);
        if (end && *end == '\0' && read >= 0.0f && read <= 60.0f) seconds = read;
        else _WARNING("[Hud] fShowLinger=%s: not 0 to 60 seconds, keeping %.1f", linger.c_str(), DEFAULT_LINGER);
    }
    if (preset != "" && preset != "immersive" && preset != "none") {
        _WARNING("[Hud] sShow=%s: no such preset, none used", preset.c_str());
        preset = "";
    }
    std::lock_guard<std::mutex> hold(g_settingsLock);
    g_preset = preset == "none" ? "" : preset;
    g_linger = seconds;
    _VMESSAGE("HUD show rules: preset \"%s\", linger %.1f s", g_preset.c_str(), g_linger);
}

std::string ShowRule(const std::string &name, const HudPlace &place) {
    if (!place.show.empty()) return place.show == "always" ? "" : place.show;
    std::lock_guard<std::mutex> hold(g_settingsLock);
    auto own = g_defaults.find(name);
    if (own != g_defaults.end()) return own->second == "always" ? "" : own->second;
    if (g_preset != "immersive") return "";
    auto it = IMMERSIVE.find(name);
    return it != IMMERSIVE.end() ? it->second : "";
}

void RegisterHudShow(GFxMovieRoot *movie, GFxValue *plugin) {
    RegisterFunction<StateNative>(plugin, movie, "HudState");
    RegisterFunction<ValuesNative>(plugin, movie, "HudValues");
    RegisterFunction<ConditionsNative>(plugin, movie, "HudConditions");
}

void ForgetShow() {
    std::lock_guard<std::mutex> hold(g_settingsLock);
    g_conditions.clear();
    g_defaults.clear();
}

void RegisterShow(VirtualMachine *vm) {
    vm->RegisterFunction(new NativeFunction3<StaticFunctionTag, void, BSFixedString, bool, BSFixedString>(
        "SetCondition", SCRIPT, SetCondition, vm));
    vm->RegisterFunction(new NativeFunction3<StaticFunctionTag, void, BSFixedString, TESGlobal *, BSFixedString>(
        "SetConditionGlobal", SCRIPT, SetConditionGlobal, vm));
    vm->RegisterFunction(new NativeFunction1<StaticFunctionTag, void, BSFixedString>(
        "RemoveCondition", SCRIPT, RemoveCondition, vm));
    vm->RegisterFunction(new NativeFunction2<StaticFunctionTag, void, BSFixedString, BSFixedString>(
        "SetShowRule", SCRIPT, SetShowRule, vm));
}

}  // namespace crui
