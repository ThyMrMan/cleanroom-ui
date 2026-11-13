// HUD parts and widgets shown on conditions (docs/swf_framework.md#hud-conditions), step 1.
//
// A box (a widget on our host, or one of the game's HUD parts, "part:" and its clip) may have a rule: the
// conditions it shows under, comma separated. With none it is always shown. The rule is the player's (the place's
// `show`, from CleanroomUI_Hud.txt; "always" for none), else the preset's ([Hud] sShow in CleanroomUI.ini, MCM's
// copy over it: "Immersive" or none). The host asks HudState() each frame for the conditions that hold and the
// linger ([Hud] fShowLinger, seconds a box stays after its last condition ends), and hides a box whose rule holds
// for none of them (as the player's hiding does: Look.as, alpha for our widgets).
//
// The conditions of step 1:
// - combat: the player is in combat (Actor::IsInCombat, the virtual F4SE names; Papyrus's IsInCombat calls it).
// - drawn: the player has a weapon out (ActorState::IsWeaponDrawn).
// - vats: V.A.T.S. is open (VATSMenu).
// - peek: our show key ([Keys] sHudPeek) is held while the game's window is in front, read with GetAsyncKeyState
//   (keyboard only; a key also bound in game does both).
// The settings are read again each time the HUD movie loads (a game loads), so a change in MCM applies then.
#include "condition.h"

#include "f4se/GameMenus.h"
#include "f4se/GameReferences.h"
#include "f4se/ScaleformCallbacks.h"

#include <cctype>
#include <cstdlib>
#include <map>
#include <string>

namespace crui {

namespace {

const char *INI_FILES[] = {".\\Data\\F4SE\\Plugins\\CleanroomUI.ini", ".\\Data\\MCM\\Settings\\CleanroomUI.ini"};
const float DEFAULT_LINGER = 3.0f;

// The Immersive preset: the parts that only show while needed. HP and AP in fights and V.A.T.S.; the ammo count,
// the explosives count and the FNV preset's CND bar with a weapon out; the compass on the show key. Every one
// also on the show key. The rest are always shown (the crosshair also aims activation; the engine shows the
// enemy health and sneak meters itself).
const std::map<std::string, std::string> IMMERSIVE = {
    {"part:HPMeter_mc", "combat,vats,peek"},
    {"part:ActionPointMeter_mc", "combat,vats,peek"},
    {"part:AmmoCount_mc", "drawn,peek"},
    {"part:ExplosiveAmmoCount_mc", "drawn,peek"},
    {"part:CRUI_WeaponCondition_mc", "drawn,peek"},
    {"part:CompassWidget_mc", "peek"},
};

std::mutex g_settingsLock;
std::string g_preset;
float g_linger = DEFAULT_LINGER;

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

// HudState(): the conditions now, {combat, drawn, vats, peek} (each true or false), and linger (seconds).
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
        GFxValue combat(player != nullptr && player->IsInCombat()),
            drawn(player != nullptr && player->actorState.IsWeaponDrawn()),
            vats(*g_ui != nullptr && (*g_ui)->IsMenuOpen(vatsMenu)),
            peek(key != 0 && (GetAsyncKeyState(key) & 0x8000) != 0 && GameInFront()), seconds{double(linger)};
        args->movie->movieRoot->CreateObject(args->result);
        args->result->SetMember("combat", &combat);
        args->result->SetMember("drawn", &drawn);
        args->result->SetMember("vats", &vats);
        args->result->SetMember("peek", &peek);
        args->result->SetMember("linger", &seconds);
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
    if (g_preset != "immersive") return "";
    auto it = IMMERSIVE.find(name);
    return it != IMMERSIVE.end() ? it->second : "";
}

void RegisterHudShow(GFxMovieRoot *movie, GFxValue *plugin) {
    RegisterFunction<StateNative>(plugin, movie, "HudState");
}

}  // namespace crui
