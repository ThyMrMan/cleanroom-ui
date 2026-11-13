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
#include "condition.h"

#include "f4se/ScaleformCallbacks.h"

#include <cstdlib>
#include <fstream>
#include <regex>
#include <sstream>
#include <string>

namespace crui {

namespace {

const char *FILE_PATH = ".\\Data\\Interface\\DEF_CONF\\DEF_HUD.xml";
// Our INI, then MCM's saved copy of it (as keys.cpp reads them).
const char *INI_FILES[] = {".\\Data\\F4SE\\Plugins\\CleanroomUI.ini", ".\\Data\\MCM\\Settings\\CleanroomUI.ini"};

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
    for (size_t at = text.find("<!--"); at != std::string::npos; at = text.find("<!--", at)) {
        size_t end = text.find("-->", at);
        text.erase(at, end == std::string::npos ? std::string::npos : end + 3 - at);
    }
    size_t open = text.find("<HUD>"), close = text.rfind("</HUD>");
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
    size_t first = value.find_first_not_of(" \t\r\n"), last = value.find_last_not_of(" \t\r\n");
    return first == std::string::npos ? "" : value.substr(first, last - first + 1);
}

// Set member `name` of `object` to `value`: a number when it reads as one in full (0x... as hex), else a string.
void SetValue(GFxMovieRoot *movie, GFxValue *object, const std::string &name, const std::string &value) {
    std::string text = Trim(value);
    char *end = nullptr;
    double number = text.rfind("0x", 0) == 0 || text.rfind("0X", 0) == 0 ? double(std::strtoul(text.c_str(), &end, 16))
                                                                         : std::strtod(text.c_str(), &end);
    GFxValue member;
    if (!text.empty() && end && *end == '\0') member = GFxValue(number);
    else movie->CreateString(&member, text.c_str());
    object->SetMember(name.c_str(), &member);
}

// The HUD preset set in the INI files ([Hud] sPreset; MCM's saved copy wins), "" for none.
std::string Preset() {
    std::string preset;
    char buffer[64];
    for (const char *file : INI_FILES)
        if (GetPrivateProfileStringA("Hud", "sPreset", "", buffer, sizeof(buffer), file) > 0) preset = buffer;
    return Trim(preset);
}

// HudPartLayout(): the preset and DEF_HUD.xml, as {preset, settings: {name: value}, parts: [{name, parname, x, y,
// defx, defy, scaleX, scaleY, rotation, color, color2, flags}]} (each element's fields under their own names);
// settings and parts are empty when there is no file or no <HUD> in it.
class PartLayoutNative : public GFxFunctionHandler {
public:
    void Invoke(Args *args) override {
        std::string body = Body(ReadText(FILE_PATH)), preset = Preset();
        GFxMovieRoot *movie = args->movie->movieRoot;
        GFxValue settings, parts, presetText;
        movie->CreateObject(args->result);
        movie->CreateString(&presetText, preset.c_str());
        args->result->SetMember("preset", &presetText);
        movie->CreateObject(&settings);
        movie->CreateArray(&parts);
        for (const auto &[name, value] : Children(body)) {
            if (value.find('<') == std::string::npos) {
                SetValue(movie, &settings, name, value);
                continue;
            }
            GFxValue part, text;
            movie->CreateObject(&part);
            movie->CreateString(&text, name.c_str());
            part.SetMember("name", &text);
            for (const auto &[field, content] : Children(value)) SetValue(movie, &part, field, content);
            parts.PushBack(&part);
        }
        args->result->SetMember("settings", &settings);
        args->result->SetMember("parts", &parts);
        _VMESSAGE("HUD layout: preset \"%s\", %u parts from %s", preset.c_str(), parts.GetArraySize(), FILE_PATH);
    }
};

// HudWeaponCondition(): the condition of the player's equipped weapon, for the FNV preset's CND bar (0 to 1; 1 while
// none is set yet, as the Pip-Boy shows it); -1 with no weapon equipped or one that doesn't wear
// (CRUI_Condition.Track).
class WeaponConditionNative : public GFxFunctionHandler {
public:
    void Invoke(Args *args) override {
        Actor *player = *g_player;
        args->result->SetNumber(WeaponCondition(player, EquippedWeapon(player)));
    }
};

}  // namespace

double WeaponCondition(Actor *player, TESForm *weapon) {
    if (!weapon) return -1.0;
    bool tracked;
    {
        std::lock_guard<std::mutex> hold(g_lock);
        tracked = g_tracked.count(weapon->formID) != 0;
    }
    if (!tracked) return -1.0;
    float set = HealthOf(FindStack(player, weapon));
    return set < 0.0f ? 1.0 : double(set);
}

void RegisterHudParts(GFxMovieRoot *movie, GFxValue *plugin) {
    RegisterFunction<PartLayoutNative>(plugin, movie, "HudPartLayout");
    RegisterFunction<WeaponConditionNative>(plugin, movie, "HudWeaponCondition");
}

}  // namespace crui
