// Our HUD host's saved layout and its edit menu (docs/swf_framework.md#hud-edit).
//
// Each widget on our host, and each of the game's own HUD parts (named "part:" and its clip,
// placed by the host's PartLayout from where the game and DEF_HUD.xml put it), may have a place the player gave it:
// an offset from where it would be, in fractions of the screen, a scale, whether it is hidden, and a color it takes
// over its own. By default places belong to each save: kept in its F4SE co-save, cleared for a new game and for a
// save without them ([Hud] bLayoutPerSave=0 shares one layout between every save, in the file below, as before
// 2026-11-16). Documents\My Games\Fallout4\F4SE\CleanroomUI_Hud.txt is then the backup: the edit menu writes the
// layout there (HudBackup) and puts it back from there (HudRestore). The host (CRUI_Hud.swf) reports each widget's unmoved origin and bounds
// (HudReport) and applies its place (HudWidgets' entries carry it). The edit menu (CRUI_HudEdit.swf, opened by
// CRUI_Hud.EditLayout) draws a box per widget where its place puts it (HudLayout), moves, scales, hides, colors and
// resets them (HudMove, HudScale, HudHide, HudTint, HudReset), and on Done closes, saving the file when layouts are
// shared (HudEditDone). The boxes come from the reports and places alone, so they are right even while the HUD movie is
// paused under the menu.
#include "condition.h"

#include <REX/W32/OLE32.h>
#include <REX/W32/SHELL32.h>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>

namespace crui {

namespace {

const char *EDIT_MENU = "CRUI_HudEdit", *FILE_NAME = "CleanroomUI_Hud.txt";
const char *PART = "part:";
const float MIN_SCALE = 0.25f, MAX_SCALE = 4.0f;

// Where the host found a widget before moving it, in fractions of the HUD's stage: its origin and its bounds.
struct Rect {
    double ox = 0.0, oy = 0.0, left = 0.0, top = 0.0, width = 0.0, height = 0.0;
};

std::map<std::string, HudPlace> g_places;        // by name, so the file is written in order; under g_lock
std::unordered_map<std::string, Rect> g_rects;  // under g_lock
// Whether places belong to each save ([Hud] bLayoutPerSave, default on), read once as the game loads.
bool g_perSave = true;
// The co-save record holding a save's places (its lines as the file writes them), and its version.
const std::uint32_t RECORD = 'HUDL', RECORD_VERSION = 1;
const char *HEADER =
    "# Cleanroom UI's HUD layout, set in its edit menu: widget<tab>x offset<tab>y offset (fractions of the "
    "screen)<tab>scale<tab>hidden (1) or shown (0)<tab>color (RRGGBB, - for its own)<tab>shown under "
    "(conditions, comma separated: combat, drawn, vats, peek; always for none; - for the preset's)\n";

// Documents\My Games\<the game's save folder>\F4SE\CleanroomUI_Hud.txt, where F4SE keeps our log; empty when
// Documents can't be found.
std::filesystem::path LayoutPath() {
    wchar_t *found = nullptr;
    auto result = REX::W32::SHGetKnownFolderPath(REX::W32::FOLDERID_Documents, REX::W32::KF_FLAG_DEFAULT, nullptr,
                                                 &found);
    std::unique_ptr<wchar_t[], decltype(&REX::W32::CoTaskMemFree)> documents(found, REX::W32::CoTaskMemFree);
    if (!documents || result != 0) return {};
    return std::filesystem::path(documents.get()) / "My Games" / F4SE::GetSaveFolderName() / "F4SE" / FILE_NAME;
}

// Every place as the layout file's lines, the header first (held under g_lock).
std::string Lines() {
    std::ostringstream out;
    out << HEADER;
    for (const auto &[name, place] : g_places) {
        std::string color = place.color >= 0 ? std::format("{:06X}", std::uint32_t(place.color) & 0xFFFFFF) : "-";
        out << name << '\t' << place.dx << '\t' << place.dy << '\t' << place.scale << '\t' << (place.hidden ? 1 : 0)
            << '\t' << color << '\t' << (place.show.empty() ? "-" : place.show) << '\n';
    }
    return out.str();
}

// Write every place to the layout file (held under g_lock); false when it can't be written.
bool Save() {
    std::ofstream out(LayoutPath());
    out << Lines();
    REX::INFO("HUD layout: {} places written to {}", g_places.size(), FILE_NAME);
    return bool(out);
}

// The string argument `i` of `params`, or "".
std::string Arg(const FunctionHandler::Params &params, std::uint32_t i) {
    return params.argCount > i && params.args[i].IsString() ? params.args[i].GetString() : "";
}

// HudReport(name, ox, oy, left, top, width, height): where the host found widget `name` before moving it.
class ReportNative : public FunctionHandler {
public:
    void Call(const Params &params) override {
        if (params.argCount < 7) return;
        const Value *a = params.args;
        std::lock_guard hold(g_lock);
        g_rects[Arg(params, 0)] = {Num(a[1]), Num(a[2]), Num(a[3]), Num(a[4]), Num(a[5]), Num(a[6])};
    }
};

// Widget `name`'s box where `place` puts it, as the edit menu draws it: {name, left, top, width, height, hidden,
// color, show, showOwn} (show its rule now, showOwn the player's: "" for the preset's, "always" for none).
Value Box(Movie *movie, const std::string &name, const Rect &rect, const HudPlace &place) {
    Value entry;
    movie->CreateObject(&entry);
    entry.SetMember("name", String(movie, name));
    entry.SetMember("left", rect.ox + place.dx + (rect.left - rect.ox) * place.scale);
    entry.SetMember("top", rect.oy + place.dy + (rect.top - rect.oy) * place.scale);
    entry.SetMember("width", rect.width * place.scale);
    entry.SetMember("height", rect.height * place.scale);
    entry.SetMember("hidden", place.hidden);
    entry.SetMember("color", double(place.color));
    entry.SetMember("show", String(movie, ShowRule(name, place)));
    entry.SetMember("showOwn", String(movie, place.show));
    return entry;
}

// Place `place` of widget `name` moved back so its box stays on the screen (whole, unless it is wider or taller than
// the screen: then its top-left corner stays on it). A box the host hasn't reported yet is left alone.
void KeepOnScreen(const std::string &name, HudPlace *place) {
    auto it = g_rects.find(name);
    if (it == g_rects.end()) return;
    const Rect &rect = it->second;
    double left = rect.ox + place->dx + (rect.left - rect.ox) * place->scale;
    double top = rect.oy + place->dy + (rect.top - rect.oy) * place->scale;
    double width = rect.width * place->scale, height = rect.height * place->scale;
    double toLeft = std::clamp(left, 0.0, std::max(0.0, 1.0 - width)) - left;
    double toTop = std::clamp(top, 0.0, std::max(0.0, 1.0 - height)) - top;
    place->dx += float(toLeft);
    place->dy += float(toTop);
}

// HudLayout(): our widgets and the HUD's parts the host has reported, where their places put them, [Box] (the box
// in fractions of the screen).
class LayoutNative : public FunctionHandler {
public:
    void Call(const Params &params) override {
        std::lock_guard hold(g_lock);
        params.movie->CreateArray(params.retVal);
        for (const auto &[name, rect] : g_rects) {
            if (!OurWidget(name) && !name.starts_with(PART)) continue;
            params.retVal->PushBack(Box(params.movie, name, rect, PlaceOf(name)));
        }
    }
};

// HudPlaceOf(name): the place the player gave `name`, {dx, dy, scale, hidden, color, show}, or null for none and no
// rule (a part with only the preset's rule gets the default place with it).
class PlaceOfNative : public FunctionHandler {
public:
    void Call(const Params &params) override {
        std::string name = Arg(params, 0);
        std::lock_guard hold(g_lock);
        auto it = g_places.find(name);
        HudPlace place = it != g_places.end() ? it->second : HudPlace();
        if (it == g_places.end() && ShowRule(name, place).empty()) *params.retVal = nullptr;
        else MakePlace(params.movie, name, place, params.retVal);
    }
};

// HudHide(name): hide widget `name`, or show it again when hidden.
class HideNative : public FunctionHandler {
public:
    void Call(const Params &params) override {
        std::lock_guard hold(g_lock);
        HudPlace &place = g_places[Arg(params, 0)];
        place.hidden = !place.hidden;
        HudChanged();
    }
};

// HudTint(name, color): give widget `name` a color over its own (0xRRGGBB), or its own again for -1.
class TintNative : public FunctionHandler {
public:
    void Call(const Params &params) override {
        if (params.argCount < 2) return;
        double color = Num(params.args[1]);
        std::lock_guard hold(g_lock);
        g_places[Arg(params, 0)].color = color < 0.0 ? -1 : std::int32_t(std::uint32_t(color) & 0xFFFFFF);
        HudChanged();
    }
};

// HudMove(name, dx, dy): move widget `name` by dx, dy (fractions of the screen).
class MoveNative : public FunctionHandler {
public:
    void Call(const Params &params) override {
        if (params.argCount < 3) return;
        std::lock_guard hold(g_lock);
        std::string name = Arg(params, 0);
        HudPlace &place = g_places[name];
        place.dx += float(Num(params.args[1]));
        place.dy += float(Num(params.args[2]));
        KeepOnScreen(name, &place);
        HudChanged();
    }
};

// HudScale(name, factor): scale widget `name` by `factor`, within a quarter and four times its own size.
class ScaleNative : public FunctionHandler {
public:
    void Call(const Params &params) override {
        if (params.argCount < 2) return;
        std::lock_guard hold(g_lock);
        std::string name = Arg(params, 0);
        HudPlace &place = g_places[name];
        place.scale = std::clamp(place.scale * float(Num(params.args[1])), MIN_SCALE, MAX_SCALE);
        KeepOnScreen(name, &place);
        HudChanged();
    }
};

// HudSetShow(name, rule): the conditions widget `name` shows under, comma separated ("" for the preset's rule,
// "always" for none); spaces are dropped.
class SetShowNative : public FunctionHandler {
public:
    void Call(const Params &params) override {
        std::string rule = Arg(params, 1);
        std::erase_if(rule, [](unsigned char c) { return std::isspace(c); });
        std::lock_guard hold(g_lock);
        g_places[Arg(params, 0)].show = rule;
        HudChanged();
    }
};

// HudReset(name): put widget `name` back where its mod puts it, shown, in its own color, under the preset's rule.
class ResetNative : public FunctionHandler {
public:
    void Call(const Params &params) override {
        std::lock_guard hold(g_lock);
        g_places.erase(Arg(params, 0));
        HudChanged();
    }
};

// HudEditDone(): close the edit menu; the layout goes with the next game save, or into the file when layouts are
// shared.
class DoneNative : public FunctionHandler {
public:
    void Call(const Params &) override {
        {
            std::lock_guard hold(g_lock);
            if (!g_perSave) Save();
        }
        if (RE::UIMessageQueue *queue = RE::UIMessageQueue::GetSingleton())
            queue->AddMessage(EDIT_MENU, RE::UI_MESSAGE_TYPE::kHide);
    }
};

// Layout file line `line` as a place into `place`; false for a comment, a blank line or one it can't read. Hidden,
// color and show came later (2026-11-13): a line without them is shown in its own color, under the preset's rule.
bool ReadPlace(const std::string &line, std::string *name, HudPlace *place) {
    if (line.empty() || line[0] == '#') return false;
    std::istringstream fields(line);
    if (!std::getline(fields, *name, '\t') || !(fields >> place->dx >> place->dy >> place->scale)) return false;
    place->scale = std::clamp(place->scale, MIN_SCALE, MAX_SCALE);
    int hidden = 0;
    std::string color, show;
    if (fields >> hidden) place->hidden = hidden != 0;
    if (fields >> color && color != "-")
        place->color = std::int32_t(std::strtoul(color.c_str(), nullptr, 16) & 0xFFFFFF);
    if (fields >> show && show != "-") place->show = show;
    return true;
}

// The places in `in` (the layout file's lines) in place of the ones held (held under g_lock); how many.
std::size_t ReadLines(std::istream &in) {
    g_places.clear();
    std::string line, name;
    while (std::getline(in, line)) {
        HudPlace place;
        if (ReadPlace(line, &name, &place)) g_places[name] = place;
    }
    return g_places.size();
}

// HudBackup(): the layout written to the layout file; how many places (-1 when it can't be written).
class BackupNative : public FunctionHandler {
public:
    void Call(const Params &params) override {
        std::lock_guard hold(g_lock);
        *params.retVal = Save() ? double(g_places.size()) : -1.0;
    }
};

// HudRestore(): the layout put back from the layout file; how many places (-1 when there is no file).
class RestoreNative : public FunctionHandler {
public:
    void Call(const Params &params) override {
        std::ifstream in(LayoutPath());
        std::lock_guard hold(g_lock);
        if (!in) {
            *params.retVal = -1.0;
            return;
        }
        std::size_t count = ReadLines(in);
        HudChanged();
        REX::INFO("HUD layout: {} places restored from {}", count, FILE_NAME);
        *params.retVal = double(count);
    }
};

// HudLayoutPerSave(): whether places belong to each save (else one layout for every save).
class PerSaveNative : public FunctionHandler {
public:
    void Call(const Params &params) override {
        *params.retVal = g_perSave;
    }
};

}  // namespace

void MakePlace(Movie *movie, const std::string &name, const HudPlace &place, Value *out) {
    movie->CreateObject(out);
    out->SetMember("dx", double(place.dx));
    out->SetMember("dy", double(place.dy));
    out->SetMember("scale", double(place.scale));
    out->SetMember("hidden", place.hidden);
    out->SetMember("color", double(place.color));
    out->SetMember("show", String(movie, ShowRule(name, place)));
}

HudPlace PlaceOf(const std::string &name) {
    auto it = g_places.find(name);
    return it != g_places.end() ? it->second : HudPlace();
}

void LoadLayout() {
    std::string setting = IniText("Hud", "bLayoutPerSave");
    g_perSave = setting.empty() || std::strtol(setting.c_str(), nullptr, 10) != 0;
    if (g_perSave) {
        REX::INFO("HUD layout: kept with each save ({} is the backup)", FILE_NAME);
        return;
    }
    std::ifstream in(LayoutPath());
    std::lock_guard hold(g_lock);
    REX::INFO("HUD layout: {} places, shared by every save", ReadLines(in));
}

void RevertLayout(const F4SE::SerializationInterface *) {
    std::lock_guard hold(g_lock);
    if (!g_perSave) return;
    g_places.clear();
    HudChanged();
}

void SaveLayoutRecord(const F4SE::SerializationInterface *save) {
    std::lock_guard hold(g_lock);
    if (!g_perSave) return;
    std::string lines = Lines();
    if (!save->WriteRecord(RECORD, RECORD_VERSION, lines.data(), std::uint32_t(lines.size())))
        REX::WARN("HUD layout: could not write it into the save");
}

void LoadLayoutRecord(const F4SE::SerializationInterface *load) {
    std::uint32_t type = 0, version = 0, length = 0;
    std::lock_guard hold(g_lock);
    while (load->GetNextRecordInfo(type, version, length)) {
        if (type != RECORD || version != RECORD_VERSION || !g_perSave) continue;
        std::string lines(length, '\0');
        lines.resize(load->ReadRecordData(lines.data(), length));
        std::istringstream in(lines);
        REX::INFO("HUD layout: {} places from the save", ReadLines(in));
        HudChanged();
    }
}

void RegisterHudLayout(Movie *movie, Value *plugin) {
    RegisterFunction<ReportNative>(plugin, movie, "HudReport");
    RegisterFunction<PlaceOfNative>(plugin, movie, "HudPlaceOf");
    RegisterFunction<LayoutNative>(plugin, movie, "HudLayout");
    RegisterFunction<MoveNative>(plugin, movie, "HudMove");
    RegisterFunction<ScaleNative>(plugin, movie, "HudScale");
    RegisterFunction<HideNative>(plugin, movie, "HudHide");
    RegisterFunction<TintNative>(plugin, movie, "HudTint");
    RegisterFunction<SetShowNative>(plugin, movie, "HudSetShow");
    RegisterFunction<ResetNative>(plugin, movie, "HudReset");
    RegisterFunction<DoneNative>(plugin, movie, "HudEditDone");
    RegisterFunction<BackupNative>(plugin, movie, "HudBackup");
    RegisterFunction<RestoreNative>(plugin, movie, "HudRestore");
    RegisterFunction<PerSaveNative>(plugin, movie, "HudLayoutPerSave");
}

}  // namespace crui
