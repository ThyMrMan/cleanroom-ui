// Our own keys (docs/swf_framework.md#keys): the Pip-Boy's Repair and Ammo keys and the HUD edit menu's reset, hide
// and color keys, and the show key that brings back the HUD parts shown on conditions,
// which the game has no controls for, so its Controls page can't remap them (docs/swf_framework.md#remap).
//
// Each is a setting under [Keys]: a letter, a digit or F1 to F12. They are read from Data\F4SE\Plugins\CleanroomUI.ini,
// then from Data\MCM\Settings\CleanroomUI.ini over it (where an MCM page saves its values), as the plugin loads and
// again as the Pip-Boy or the edit menu opens, so a change in MCM applies the next time either opens. A key that
// can't be read, or that our lists already use, keeps its default, with a warning in the log. Codes are Flash key
// codes (Windows virtual keys): letters and digits are their capital ASCII codes, F1 is 112.
#include "condition.h"

#include <REX/W32/KERNEL32.h>

#include <atomic>
#include <cctype>
#include <cstring>
#include <string>
#include <utility>

namespace crui {

namespace {

const char *SECTION = "Keys";
// Our INI, then MCM's saved copy of it, which wins.
const char *INI_FILES[] = {".\\Data\\F4SE\\Plugins\\CleanroomUI.ini", ".\\Data\\MCM\\Settings\\CleanroomUI.ini"};
const int F1 = 112;

// One of our keys: its setting, its default, and the keys it may not take (the open lists' own keys: Enter, Esc
// and Tab can't be named anyway; W, S and E move and choose in the Pip-Boy's lists).
struct Setting {
    const char *name;
    const char *fallback;
    const char *taken;
    std::atomic<int> code;
};
Setting g_keys[] = {{"sRepair", "G", "WSE", 'G'}, {"sAmmo", "B", "WSE", 'B'}, {"sHudReset", "R", "", 'R'},
                    {"sHudHide", "H", "", 'H'},  {"sHudColor", "C", "", 'C'}, {"sHudPeek", "Y", "", 'Y'},
                    {"sHudShow", "V", "", 'V'}};

// `text`'s key code: a letter or a digit, or F1 to F12; 0 for anything else.
int Parse(const std::string &text) {
    std::string upper;
    for (char c : text)
        if (!std::isspace(static_cast<unsigned char>(c))) upper += char(std::toupper(static_cast<unsigned char>(c)));
    if (upper.size() == 1 && std::isalnum(static_cast<unsigned char>(upper[0]))) return upper[0];
    bool function = upper.size() >= 2 && upper.size() <= 3 && upper[0] == 'F' &&
                    std::isdigit(static_cast<unsigned char>(upper[1])) &&
                    (upper.size() == 2 || std::isdigit(static_cast<unsigned char>(upper[2])));
    int n = function ? std::stoi(upper.substr(1)) : 0;
    return n >= 1 && n <= 12 ? F1 + n - 1 : 0;
}

Setting &Of(OurKey key) {
    return g_keys[int(key)];
}

// HudKeys(): the edit menu's keys, {reset, resetName, hide, hideName, color, colorName, show, showName} (each a
// code and its name), read again as it opens.
class KeysNative : public FunctionHandler {
public:
    void Call(const Params &params) override {
        ReadKeys();
        params.movie->CreateObject(params.retVal);
        const std::pair<const char *, OurKey> keys[] = {
            {"reset", OurKey::HudReset}, {"hide", OurKey::HudHide}, {"color", OurKey::HudColor},
            {"show", OurKey::HudShow}};
        for (const auto &[member, key] : keys) {
            params.retVal->SetMember(member, std::uint32_t(KeyCode(key)));
            params.retVal->SetMember(std::string(member) + "Name", String(params.movie, KeyName(key)));
        }
    }
};

// Put `keys` back to their defaults when two of them share a key, warning `what`.
void Distinct(std::initializer_list<OurKey> keys, const char *what) {
    bool shared = false;
    for (auto a = keys.begin(); a != keys.end(); ++a)
        for (auto b = a + 1; b != keys.end(); ++b) shared = shared || Of(*a).code == Of(*b).code;
    if (!shared) return;
    REX::WARN("[{}] {} share a key, keeping their defaults", SECTION, what);
    for (OurKey key : keys) Of(key).code = Parse(Of(key).fallback);
}

}  // namespace

std::string IniText(const char *section, const char *name) {
    std::string text;
    char buffer[64];
    for (const char *file : INI_FILES)
        if (REX::W32::GetPrivateProfileStringA(section, name, "", buffer, sizeof(buffer), file) > 0) text = buffer;
    std::size_t first = text.find_first_not_of(" \t\r\n"), last = text.find_last_not_of(" \t\r\n");
    return first == std::string::npos ? "" : text.substr(first, last - first + 1);
}

void ReadKeys() {
    for (Setting &key : g_keys) {
        std::string text = IniText(SECTION, key.name);
        if (text.empty()) text = key.fallback;
        int code = Parse(text);
        if (code && code < 128 && std::strchr(key.taken, char(code))) {
            REX::WARN("[{}] {}={}: our lists use that key, keeping {}", SECTION, key.name, text, key.fallback);
            code = 0;
        } else if (!code) {
            REX::WARN("[{}] {}={}: not a letter, digit or F1-F12, keeping {}", SECTION, key.name, text, key.fallback);
        }
        key.code = code ? code : Parse(key.fallback);
    }
    // The Pip-Boy's two keys must differ, and so must the edit menu's four.
    Distinct({OurKey::Repair, OurKey::Ammo}, "sRepair and sAmmo");
    Distinct({OurKey::HudReset, OurKey::HudHide, OurKey::HudColor, OurKey::HudShow},
             "sHudReset, sHudHide, sHudColor and sHudShow");
    REX::DEBUG("keys: repair {}, ammo {}, HUD reset {}, hide {}, color {}, peek {}, show {}", KeyName(OurKey::Repair),
               KeyName(OurKey::Ammo), KeyName(OurKey::HudReset), KeyName(OurKey::HudHide), KeyName(OurKey::HudColor),
               KeyName(OurKey::HudPeek), KeyName(OurKey::HudShow));
}

int KeyCode(OurKey key) {
    return Of(key).code;
}

std::string KeyName(OurKey key) {
    int code = Of(key).code;
    return code >= F1 ? "F" + std::to_string(code - F1 + 1) : std::string(1, char(code));
}

void RegisterKeys(Movie *movie, Value *plugin) {
    RegisterFunction<KeysNative>(plugin, movie, "HudKeys");
}

}  // namespace crui
