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

#include "f4se/ScaleformCallbacks.h"

#include <atomic>
#include <cctype>
#include <string>
#include <utility>

namespace crui {

namespace {

const char *SECTION = "Keys";
const char *FILES[] = {".\\Data\\F4SE\\Plugins\\CleanroomUI.ini", ".\\Data\\MCM\\Settings\\CleanroomUI.ini"};
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
int Parse(std::string text) {
    std::string upper;
    for (char c : text)
        if (!std::isspace(static_cast<unsigned char>(c))) upper += char(std::toupper(static_cast<unsigned char>(c)));
    if (upper.size() == 1 && std::isalnum(static_cast<unsigned char>(upper[0]))) return upper[0];
    if (upper.size() >= 2 && upper.size() <= 3 && upper[0] == 'F' && std::isdigit(static_cast<unsigned char>(upper[1])) &&
        (upper.size() == 2 || std::isdigit(static_cast<unsigned char>(upper[2])))) {
        int n = std::stoi(upper.substr(1));
        if (n >= 1 && n <= 12) return F1 + n - 1;
    }
    return 0;
}

// Setting `key`'s text in the INI files (the later file wins), or its default.
std::string Text(const Setting &key) {
    std::string text = key.fallback;
    char buffer[32];
    for (const char *file : FILES)
        if (GetPrivateProfileStringA(SECTION, key.name, "", buffer, sizeof(buffer), file) > 0) text = buffer;
    return text;
}

Setting &Of(OurKey key) {
    return g_keys[int(key)];
}

// HudKeys(): the edit menu's keys, {reset, resetName, hide, hideName, color, colorName, show, showName} (each a
// code and its name),
// read again as it opens.
class KeysNative : public GFxFunctionHandler {
public:
    void Invoke(Args *args) override {
        ReadKeys();
        GFxMovieRoot *movie = args->movie->movieRoot;
        movie->CreateObject(args->result);
        const std::pair<const char *, OurKey> keys[] = {
            {"reset", OurKey::HudReset}, {"hide", OurKey::HudHide}, {"color", OurKey::HudColor},
            {"show", OurKey::HudShow}};
        for (const auto &[member, key] : keys) {
            std::string name = KeyName(key);
            GFxValue code(UInt32(KeyCode(key))), text;
            movie->CreateString(&text, name.c_str());
            args->result->SetMember(member, &code);
            args->result->SetMember((std::string(member) + "Name").c_str(), &text);
        }
    }
};

}  // namespace

void ReadKeys() {
    for (Setting &key : g_keys) {
        std::string text = Text(key);
        int code = Parse(text);
        if (code && code < 128 && strchr(key.taken, char(code))) {
            _WARNING("[%s] %s=%s: our lists use that key, keeping %s", SECTION, key.name, text.c_str(), key.fallback);
            code = 0;
        } else if (!code) {
            _WARNING("[%s] %s=%s: not a letter, digit or F1-F12, keeping %s", SECTION, key.name, text.c_str(),
                     key.fallback);
        }
        key.code = code ? code : Parse(key.fallback);
    }
    // The Pip-Boy's two keys must differ: both go back to their defaults when they don't.
    if (Of(OurKey::Repair).code == Of(OurKey::Ammo).code) {
        _WARNING("[%s] sRepair and sAmmo are the same key, keeping G and B", SECTION);
        Of(OurKey::Repair).code = 'G';
        Of(OurKey::Ammo).code = 'B';
    }
    // So must the edit menu's four: all go back to their defaults when two are the same.
    const OurKey edit[] = {OurKey::HudReset, OurKey::HudHide, OurKey::HudColor, OurKey::HudShow};
    bool shared = false;
    for (int i = 0; i < 4; i++)
        for (int j = i + 1; j < 4; j++) shared = shared || Of(edit[i]).code == Of(edit[j]).code;
    if (shared) {
        _WARNING("[%s] sHudReset, sHudHide, sHudColor and sHudShow share a key, keeping R, H, C and V", SECTION);
        Of(OurKey::HudReset).code = 'R';
        Of(OurKey::HudHide).code = 'H';
        Of(OurKey::HudColor).code = 'C';
        Of(OurKey::HudShow).code = 'V';
    }
    _VMESSAGE("keys: repair %s, ammo %s, HUD reset %s, hide %s, color %s, show %s", KeyName(OurKey::Repair).c_str(),
              KeyName(OurKey::Ammo).c_str(), KeyName(OurKey::HudReset).c_str(), KeyName(OurKey::HudHide).c_str(),
              KeyName(OurKey::HudColor).c_str(), KeyName(OurKey::HudPeek).c_str());
}

int KeyCode(OurKey key) {
    return Of(key).code;
}

std::string KeyName(OurKey key) {
    int code = Of(key).code;
    return code >= F1 ? "F" + std::to_string(code - F1 + 1) : std::string(1, char(code));
}

void RegisterKeys(GFxMovieRoot *movie, GFxValue *plugin) {
    RegisterFunction<KeysNative>(plugin, movie, "HudKeys");
}

}  // namespace crui
