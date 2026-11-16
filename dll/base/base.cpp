// What every DLL of ours shares (base.h), on CommonLibF4.
#include "base.h"

#include <REX/W32/KERNEL32.h>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <unordered_set>

namespace crui {

namespace {

// The Steam runtimes every layout and call site here was read in: 1.11.240, old-gen 1.10.163 and next-gen 1.10.984
// to 1.11.221, where each one used is the same (docs/spin_offs.md#og-layouts, #ng-layouts).
const REL::Version CHECKED[] = {{1, 11, 240, 0}, {1, 10, 163, 0}, {1, 10, 984, 0}, {1, 11, 137, 0},
                                {1, 11, 159, 0}, {1, 11, 169, 0}, {1, 11, 191, 0}, {1, 11, 221, 0}};

// `path` lowercased with backslashes, as a BA2's name table holds it.
std::string Key(std::string path) {
    std::replace(path.begin(), path.end(), '/', '\\');
    std::transform(path.begin(), path.end(), path.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return path;
}

// Every name in archive `file`'s name table (BTDX: magic, version, type, file count, name table offset; then per
// file a 16-bit length and the name), into `names`.
void ReadNames(const std::string &file, std::unordered_set<std::string> *names) {
    std::ifstream in(file, std::ios::binary);
    char magic[4] = {}, type[4] = {};
    std::uint32_t version = 0, count = 0;
    std::uint64_t table = 0;
    in.read(magic, 4).read(reinterpret_cast<char *>(&version), 4).read(type, 4);
    in.read(reinterpret_cast<char *>(&count), 4).read(reinterpret_cast<char *>(&table), 8);
    if (!in || std::memcmp(magic, "BTDX", 4) || !table) return;
    in.seekg(std::streamoff(table));
    std::string name;
    for (std::uint32_t i = 0; i < count && in; i++) {
        std::uint16_t length = 0;
        in.read(reinterpret_cast<char *>(&length), 2);
        name.resize(length);
        if (length) in.read(name.data(), length);
        if (in) names->insert(Key(name));
    }
}

// Whether a BA2 in Data lists `path`, the archives' name tables read once. The game's resource stream cannot open a
// texture (DX10) archive's files, so TextureExists asks the tables (played 2026-11-12: every icon in
// FalloutNV - Textures.ba2 reported missing).
bool Archived(const std::string &path) {
    static std::unordered_set<std::string> names;
    static std::once_flag read;
    std::call_once(read, [] {
        std::error_code error;
        for (const auto &entry : std::filesystem::directory_iterator("Data", error))
            if (Key(entry.path().extension().string()) == ".ba2") ReadNames(entry.path().string(), &names);
        REX::INFO("TextureExists: {} names in the archives", names.size());
    });
    return names.contains(Key(path));
}

// TextureExists(path): whether the game has Textures\<path> (the path f4se.MountImage takes): a loose file or one in
// a general archive, opened as the game opens a resource, or a name in a BA2's table (Archived). MountImage itself
// reports true for a missing file. See docs/swf_framework.md#images
class TextureExistsNative : public FunctionHandler {
public:
    void Call(const Params &params) override {
        bool found = false;
        if (params.argCount > 0 && params.args[0].IsString()) {
            std::string path = Key(std::string("Textures\\") + params.args[0].GetString());
            RE::BSResourceNiBinaryStream stream(path.c_str());
            found = static_cast<bool>(stream) || Archived(path);
        }
        *params.retVal = found;
    }
};

// HudColor([prefix]): the player's HUD color (iHUDColor, or the INI colors `prefix`, e.g. iHUDColorWarning) as
// 0xRRGGBB, read on every call, for menus of ours that draw their text and lines in it.
class HudColorNative : public FunctionHandler {
public:
    void Call(const Params &params) override {
        bool named = params.argCount > 0 && params.args[0].IsString();
        *params.retVal = HudColor(named ? params.args[0].GetString() : "iHUDColor");
    }
};

// Log(text): a line from one of our movies in the DLL's log, at the verbose level.
class LogNative : public FunctionHandler {
public:
    void Call(const Params &params) override {
        if (params.argCount > 0 && params.args[0].IsString()) REX::DEBUG("{}", params.args[0].GetString());
    }
};

// INI text `text` as a Flash value by setting `name`'s prefix, as Bethesda's INIs name them: b a bool (not 0),
// i an int, u a uint, f a number, anything else the text.
Value Typed(Movie *movie, const std::string &name, const std::string &text) {
    char prefix = name.empty() ? 0 : char(std::tolower(static_cast<unsigned char>(name[0])));
    switch (prefix) {
        case 'b': return Value(std::atoi(text.c_str()) != 0 || Key(text) == "true");
        case 'i': return Value(std::int32_t(std::strtol(text.c_str(), nullptr, 0)));
        case 'u': return Value(std::uint32_t(std::strtoul(text.c_str(), nullptr, 0)));
        case 'f': return Value(std::atof(text.c_str()));
        default: return String(movie, text);
    }
}

// Whether `mod` names an INI file and no path: letters, digits, spaces, `_`, `-` and `.`, and no "..".
bool PlainName(const std::string &mod) {
    return !mod.empty() && mod.find("..") == std::string::npos && std::ranges::all_of(mod, [](unsigned char c) {
        return std::isalnum(c) || c == ' ' || c == '_' || c == '-' || c == '.';
    });
}

// Settings(mod): mod `mod`'s INI files (IniFiles) as {section: {setting: value}}, MCM's copy over its own, each value
// typed by its name's prefix (Typed); null for a name that is not PlainName. Read on every call, so an MCM change
// shows the next time a movie asks. See docs/swf_framework.md#settings
class SettingsNative : public FunctionHandler {
public:
    void Call(const Params &params) override {
        std::string mod = params.argCount > 0 && params.args[0].IsString() ? params.args[0].GetString() : "";
        *params.retVal = nullptr;
        if (!PlainName(mod)) return;
        params.movie->CreateObject(params.retVal);
        std::vector<std::string> sections;
        for (const std::string &file : IniFiles(mod))
            for (std::string &section : IniNames(file, nullptr))
                if (!std::ranges::contains(sections, section)) sections.push_back(std::move(section));
        for (const std::string &section : sections) {
            Value values;
            params.movie->CreateObject(&values);
            for (const std::string &file : IniFiles(mod))
                for (const std::string &name : IniNames(file, section.c_str()))
                    values.SetMember(name.c_str(), Typed(params.movie, name, IniText(mod, section.c_str(), name.c_str())));
            params.retVal->SetMember(section.c_str(), values);
        }
    }
};

}  // namespace

std::uintptr_t Address(const Ids &ids) {
    auto runtime = REX::FModule::GetRuntimeIndex();
    std::uint64_t id = runtime == REX::FModule::Runtime::kOG ? ids.og
                       : runtime == REX::FModule::Runtime::kNG ? ids.ng
                                                               : ids.ae;
    auto *db = REL::IDDB::GetSingleton();
    if (!id || !db->has(id)) return 0;
    std::uintptr_t offset = runtime == REX::FModule::Runtime::kOG ? ids.ogOffset : ids.offset;
    return REX::FModule::GetExecutingModule().GetBaseAddress() + db->offset(id) + offset;
}

bool OldGen() {
    return REX::FModule::GetRuntimeIndex() == REX::FModule::Runtime::kOG;
}

std::string RuntimeName() {
    auto runtime = REX::FModule::GetRuntimeIndex();
    const char *name = runtime == REX::FModule::Runtime::kOG ? "old-gen"
                       : runtime == REX::FModule::Runtime::kNG ? "next-gen" : "1.11";
    return std::format("{} ({})", REX::FModule::GetExecutingModule().GetFileVersion(), name);
}

bool CheckedRuntime() {
    return std::ranges::contains(CHECKED, REX::FModule::GetExecutingModule().GetFileVersion()) &&
           REX::W32::GetModuleHandleW(L"steam_api64.dll");
}

REX::ELogLevel LogLevel(const char *name) {
    std::string ini = std::string(".\\Data\\F4SE\\Plugins\\") + name + ".ini";
    int level = std::clamp(int(REX::W32::GetPrivateProfileIntA("Log", "iLevel", 2, ini.c_str())), 0, 3);
    const REX::ELogLevel levels[] = {REX::ELogLevel::Error, REX::ELogLevel::Warning, REX::ELogLevel::Info,
                                     REX::ELogLevel::Debug};
    return levels[level];
}

double Num(const Value &v) {
    switch (v.GetType()) {
        case Value::ValueType::kInt: return v.GetInt();
        case Value::ValueType::kUInt: return v.GetUInt();
        case Value::ValueType::kNumber: return v.GetNumber();
        case Value::ValueType::kBoolean: return v.GetBoolean() ? 1.0 : 0.0;
        default: return 0.0;
    }
}

Value String(Movie *movie, const char *text) {
    Value out;
    movie->asMovieRoot->CreateString(&out, text);
    return out;
}

bool Get(const Value &obj, const char *name, Value *out) {
    return obj.IsObject() && obj.GetMember(name, out);
}

const void *Identity(const Value &v) {
    // The object pointer is the union at +0x10 of CommonLibF4's Value (its own layout, asserted 0x20 there), which
    // keeps it private.
    static_assert(sizeof(Value) == 0x20);
    return v.IsObject() ? *reinterpret_cast<void *const *>(reinterpret_cast<const std::uint8_t *>(&v) + 0x10)
                        : nullptr;
}

void LogMembers(const Value &obj) {
    if (!obj.IsObject()) return;
    obj.VisitMembers([](const char *member, const Value &value) {
        REX::INFO("  member {} (type {})", member, std::to_underlying(value.GetType()));
    });
}

Movie *MenuMovie(const char *name) {
    RE::UI *ui = RE::UI::GetSingleton();
    Scaleform::Ptr<RE::IMenu> menu = ui ? ui->GetMenu(name) : nullptr;
    return menu && menu->uiMovie ? menu->uiMovie.get() : nullptr;
}

bool FindMenu(Movie *movie, std::initializer_list<const char *> bases, const char *needs, std::string *base,
              Value *root, Value *code) {
    for (const char *path : bases) {
        Value part;
        std::string at(path);
        if (movie->GetVariable(code, (at + ".BGSCodeObj").c_str()) && code->IsObject() &&
            (!needs || (movie->GetVariable(&part, (at + "." + needs).c_str()) && part.IsObject())) &&
            movie->GetVariable(root, path)) {
            *base = at;
            return true;
        }
    }
    base->clear();
    return false;
}

bool LoadChild(Movie *movie, Value &parent, const char *path) {
    Value loader, request, url = String(movie, path);
    movie->CreateObject(&loader, "flash.display.Loader");
    movie->CreateObject(&request, "flash.net.URLRequest", &url, 1);
    Value args[2] = {request, nullptr};
    return loader.IsObject() && parent.Invoke("addChild", nullptr, &loader, 1) &&
           loader.Invoke("load", nullptr, args, 2);
}

std::vector<std::string> IniNames(const std::string &file, const char *section) {
    std::vector<char> buffer(32767);
    auto size = REX::W32::GetPrivateProfileStringA(section, nullptr, "", buffer.data(),
                                                   std::uint32_t(buffer.size()), file.c_str());
    std::vector<std::string> names;
    for (std::size_t at = 0; at < size && buffer[at];) {
        names.emplace_back(buffer.data() + at);
        at += names.back().size() + 1;
    }
    return names;
}

std::vector<std::string> IniFiles(const std::string &mod) {
    return {".\\Data\\F4SE\\Plugins\\" + mod + ".ini", ".\\Data\\MCM\\Settings\\" + mod + ".ini"};
}

std::string IniText(const std::string &mod, const char *section, const char *name) {
    std::string text;
    char buffer[1024];
    for (const std::string &file : IniFiles(mod))
        if (REX::W32::GetPrivateProfileStringA(section, name, "", buffer, sizeof(buffer), file.c_str()) > 0)
            text = buffer;
    std::size_t first = text.find_first_not_of(" \t\r\n"), last = text.find_last_not_of(" \t\r\n");
    return first == std::string::npos ? "" : text.substr(first, last - first + 1);
}

std::uint32_t HudColor(const char *prefix) {
    std::uint32_t rgb = 0;
    for (const char *channel : {"R", "G", "B"}) {
        RE::Setting *setting = RE::GetINISetting(std::string(prefix) + channel + ":Interface");
        rgb = (rgb << 8) | (setting ? (std::uint32_t(setting->GetInt()) & 0xFF) : 0xFF);
    }
    return rgb;
}

void RegisterSharedNatives(Movie *movie, Value *plugin) {
    RegisterFunction<TextureExistsNative>(plugin, movie, "TextureExists");
    RegisterFunction<HudColorNative>(plugin, movie, "HudColor");
    RegisterFunction<LogNative>(plugin, movie, "Log");
    RegisterFunction<SettingsNative>(plugin, movie, "Settings");
}

bool CallsTarget(std::uintptr_t site, std::uintptr_t target) {
    if (!site || !target) return false;
    auto *call = reinterpret_cast<const std::uint8_t *>(site);
    std::int32_t rel;
    std::memcpy(&rel, call + 1, sizeof(rel));
    return call[0] == 0xE8 && site + 5 + rel == target;
}

}  // namespace crui
