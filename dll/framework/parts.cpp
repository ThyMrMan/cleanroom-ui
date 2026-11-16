// The game's own HUD parts laid out from DEF_HUD.xml (docs/swf_framework.md#hud-layout) or a preset ([Hud] sPreset
// in CleanroomUI.ini; docs/swf_framework.md#fnv-preset), with no HUDMenu.swf replaced: this file reads them, our
// host movie (CRUI_Hud.swf, PartLayout) applies them.
//
// DEF_HUD.xml (Data\Interface\DEF_CONF\, written by DEF_HUD's own editor) is <HUD> holding a few settings (SafeX,
// SafeY, globalscaleX, globalscaleY, ...) and one element per HUD part, named by its clip, holding x, y, scaleX,
// scaleY, rotation, color, color2, flags, parname (the HUD group the part sits in: one of the seven groups the game
// pins to the safe area, so one layout serves every screen shape) and defx, defy (where the game puts it). Only the
// format is read; nothing of DEF's movies or programs is used (docs/spin_offs.md#def-ui).
//
// The FNV preset's own widget, the weapon condition (CND) bar, reads the equipped weapon's condition here
// (HudWeaponCondition; docs/swf_framework.md#fnv-widgets).
//
// Our other HUD parts' settings and the game's hour (HudExtras; docs/swf_framework.md#hud-extras): the clock
// ([Hud] bClock, bClockRealTime, bClock24) and the game's meters kept in power armor ([Hud] bPowerArmorMeters), read
// again as the HUD loads and as the pause menu closes (ReadPartSettings), so MCM's changes apply then.
#include "condition.h"

#include <cstdlib>
#include <fstream>
#include <mutex>
#include <regex>
#include <sstream>
#include <string>

namespace crui {

namespace {

const char *FILE_PATH = ".\\Data\\Interface\\DEF_CONF\\DEF_HUD.xml";
// Fallout4.esm's GameHour global (0 to 24).
const std::uint32_t GAME_HOUR = 0x38;

// Our other parts' settings (ReadPartSettings), under g_extrasLock.
struct Extras {
    bool clock = false, clockRealTime = false, clock24 = false, powerArmorMeters = false;
};
std::mutex g_extrasLock;
Extras g_extras;

// File `path`'s text; "" when it can't be read.
std::string ReadText(const char *path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return "";
    std::ostringstream out;
    out << in.rdbuf();
    return out.str();
}

// The text between <HUD> and </HUD>, comments taken out; "" when there is none.
std::string Body(std::string text) {
    for (std::size_t at = text.find("<!--"); at != std::string::npos; at = text.find("<!--", at)) {
        std::size_t end = text.find("-->", at);
        text.erase(at, end == std::string::npos ? std::string::npos : end + 3 - at);
    }
    std::size_t open = text.find("<HUD>"), close = text.rfind("</HUD>");
    if (open == std::string::npos || close == std::string::npos || close < open) return "";
    return text.substr(open + 5, close - open - 5);
}

// Each <name>value</name> directly inside `text`, as (name, value) pairs in order.
std::vector<std::pair<std::string, std::string>> Children(const std::string &text) {
    static const std::regex ELEMENT(R"(<(\w+)>([\s\S]*?)</\1>)");
    std::vector<std::pair<std::string, std::string>> out;
    for (std::sregex_iterator it(text.begin(), text.end(), ELEMENT), end; it != end; ++it)
        out.emplace_back((*it)[1].str(), (*it)[2].str());
    return out;
}

// `value` trimmed.
std::string Trim(const std::string &value) {
    std::size_t first = value.find_first_not_of(" \t\r\n"), last = value.find_last_not_of(" \t\r\n");
    return first == std::string::npos ? "" : value.substr(first, last - first + 1);
}

// Set member `name` of `object` to `value`: a number when it reads as one in full (0x... as hex), else a string.
void SetValue(Movie *movie, Value *object, const std::string &name, const std::string &value) {
    std::string text = Trim(value);
    char *end = nullptr;
    bool hex = text.starts_with("0x") || text.starts_with("0X");
    double number = hex ? double(std::strtoul(text.c_str(), &end, 16)) : std::strtod(text.c_str(), &end);
    bool numeric = !text.empty() && end && *end == '\0';
    object->SetMember(name, numeric ? Value(number) : String(movie, text));
}

// HudPartLayout(): the preset and DEF_HUD.xml, as {preset, settings: {name: value}, parts: [{name, parname, x, y,
// defx, defy, scaleX, scaleY, rotation, color, color2, flags}]} (each element's fields under their own names);
// settings and parts are empty when there is no file or no <HUD> in it.
class PartLayoutNative : public FunctionHandler {
public:
    void Call(const Params &params) override {
        std::string body = Body(ReadText(FILE_PATH)), preset = IniText("Hud", "sPreset");
        // Our MCM page's switch, when sPreset isn't set.
        if (preset.empty() && IniText("Hud", "bFnvPreset") == "1") preset = "FNV";
        Movie *movie = params.movie;
        Value settings, parts;
        movie->CreateObject(params.retVal);
        params.retVal->SetMember("preset", String(movie, preset));
        movie->CreateObject(&settings);
        movie->CreateArray(&parts);
        for (const auto &[name, value] : Children(body)) {
            if (value.find('<') == std::string::npos) {
                SetValue(movie, &settings, name, value);
                continue;
            }
            Value part;
            movie->CreateObject(&part);
            part.SetMember("name", String(movie, name));
            for (const auto &[field, content] : Children(value)) SetValue(movie, &part, field, content);
            parts.PushBack(part);
        }
        params.retVal->SetMember("settings", settings);
        params.retVal->SetMember("parts", parts);
        REX::DEBUG("HUD layout: preset \"{}\", {} parts from {}", preset, parts.GetArraySize(), FILE_PATH);
    }
};

// HudWeaponCondition(): the condition of the player's equipped weapon, for the FNV preset's CND bar (0 to 1; 1 while
// none is set yet, as the Pip-Boy shows it); -1 with no weapon equipped or one that doesn't wear
// (CRUI_Condition.Track).
class WeaponConditionNative : public FunctionHandler {
public:
    void Call(const Params &params) override {
        RE::PlayerCharacter *player = RE::PlayerCharacter::GetSingleton();
        *params.retVal = WeaponCondition(player, EquippedWeapon(player));
    }
};

// HudExtras(): {clock (our clock shown), clockRealTime (with the real time under the game's), clock24 (24-hour),
// powerArmorMeters (the game's HP, AP and ammo kept in power armor), hour (the game's, 0 to 24; -1 not found)}.
// The host asks a few times a second.
class ExtrasNative : public FunctionHandler {
public:
    void Call(const Params &params) override {
        static RE::TESGlobal *hour = FormOf(GAME_HOUR) ? FormOf(GAME_HOUR)->As<RE::TESGlobal>() : nullptr;
        Extras now;
        {
            std::lock_guard hold(g_extrasLock);
            now = g_extras;
        }
        params.movie->CreateObject(params.retVal);
        params.retVal->SetMember("clock", now.clock);
        params.retVal->SetMember("clockRealTime", now.clockRealTime);
        params.retVal->SetMember("clock24", now.clock24);
        params.retVal->SetMember("powerArmorMeters", now.powerArmorMeters);
        params.retVal->SetMember("hour", hour ? double(hour->GetValue()) : -1.0);
    }
};

}  // namespace

void ReadPartSettings() {
    Extras read;
    read.clock = IniText("Hud", "bClock") == "1";
    read.clockRealTime = IniText("Hud", "bClockRealTime") == "1";
    read.clock24 = IniText("Hud", "bClock24") == "1";
    read.powerArmorMeters = IniText("Hud", "bPowerArmorMeters") == "1";
    std::lock_guard hold(g_extrasLock);
    g_extras = read;
    REX::DEBUG("HUD extras: clock {} (real time {}, 24-hour {}), power armor meters {}", read.clock,
               read.clockRealTime, read.clock24, read.powerArmorMeters);
}

double WeaponCondition(RE::Actor *player, RE::TESForm *weapon) {
    if (!weapon) return -1.0;
    bool tracked;
    {
        std::lock_guard hold(g_lock);
        tracked = g_tracked.contains(weapon->GetFormID());
    }
    if (!tracked) return -1.0;
    float set = HealthOf(FindStack(player, weapon));
    return set < 0.0f ? 1.0 : double(set);
}

void RegisterHudParts(Movie *movie, Value *plugin) {
    RegisterFunction<PartLayoutNative>(plugin, movie, "HudPartLayout");
    RegisterFunction<WeaponConditionNative>(plugin, movie, "HudWeaponCondition");
    RegisterFunction<ExtrasNative>(plugin, movie, "HudExtras");
}

}  // namespace crui
