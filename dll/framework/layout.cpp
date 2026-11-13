// Our HUD host's saved layout and its edit menu (docs/swf_framework.md#hud-edit).
//
// Each widget on our host (not HUDFramework's), and each of the game's own HUD parts (named "part:" and its clip,
// placed by the host's PartLayout from where the game and DEF_HUD.xml put it), may have a place the player gave it:
// an offset from where it would be, in fractions of the screen, a scale, whether it is hidden, and a color it takes
// over its own. Places are kept by widget name in
// Documents\My Games\Fallout4\F4SE\CleanroomUI_Hud.txt, for every save, read once as the game loads. The host
// (CRUI_Hud.swf) reports each widget's unmoved origin and bounds (HudReport) and applies its place (HudWidgets'
// entries carry it). The edit menu (CRUI_HudEdit.swf, opened by CRUI_Hud.EditLayout) draws a box per widget where its
// place puts it (HudLayout), moves, scales, hides, colors and resets them (HudMove, HudScale, HudHide, HudTint,
// HudReset), and on Done saves the file
// and closes (HudEditDone). The boxes come from the reports and places alone, so they are right even while the HUD
// movie is paused under the menu.
#include "condition.h"

#include "f4se/GameMenus.h"
#include "f4se/ScaleformCallbacks.h"

#include <shlobj.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <unordered_map>

namespace crui {

namespace {

const char *EDIT_MENU = "CRUI_HudEdit", *FILE_NAME = "\\My Games\\Fallout4\\F4SE\\CleanroomUI_Hud.txt";
const char *PART = "part:";
const float MIN_SCALE = 0.25f, MAX_SCALE = 4.0f;

// Where the host found a widget before moving it, in fractions of the HUD's stage: its origin and its bounds.
struct Rect {
    double ox = 0.0, oy = 0.0, left = 0.0, top = 0.0, width = 0.0, height = 0.0;
};

std::map<std::string, HudPlace> g_places;        // by name, so the file is written in order; under g_lock
std::unordered_map<std::string, Rect> g_rects;  // under g_lock

std::string LayoutPath() {
    char docs[MAX_PATH];
    if (FAILED(SHGetFolderPathA(nullptr, CSIDL_MYDOCUMENTS, nullptr, SHGFP_TYPE_CURRENT, docs))) return "";
    return std::string(docs) + FILE_NAME;
}

// Write every place to the layout file (held under g_lock).
void Save() {
    std::ofstream out(LayoutPath());
    out << "# Cleanroom UI's HUD layout, set in its edit menu: widget<tab>x offset<tab>y offset (fractions of the "
           "screen)<tab>scale<tab>hidden (1) or shown (0)<tab>color (RRGGBB, - for its own)\n";
    for (const auto &[name, place] : g_places) {
        char color[8] = "-";
        if (place.color >= 0) snprintf(color, sizeof(color), "%06X", UInt32(place.color) & 0xFFFFFF);
        out << name << '\t' << place.dx << '\t' << place.dy << '\t' << place.scale << '\t' << (place.hidden ? 1 : 0)
            << '\t' << color << '\n';
    }
    _MESSAGE("HUD layout: %u places saved", UInt32(g_places.size()));
}

// The string argument `i` of `args`, or "".
std::string Arg(GFxFunctionHandler::Args *args, UInt32 i) {
    return args->numArgs > i && args->args[i].IsString() ? args->args[i].GetString() : "";
}

// HudReport(name, ox, oy, left, top, width, height): where the host found widget `name` before moving it.
class ReportNative : public GFxFunctionHandler {
public:
    void Invoke(Args *args) override {
        if (args->numArgs < 7) return;
        std::lock_guard<std::mutex> hold(g_lock);
        g_rects[Arg(args, 0)] = {Num(args->args[1]), Num(args->args[2]), Num(args->args[3]), Num(args->args[4]),
                                 Num(args->args[5]), Num(args->args[6])};
    }
};

// HudLayout(): our widgets and the HUD's parts the host has reported, where their places put them,
// [{name, left, top, width, height, hidden, color}] (the box in fractions of the screen).
class LayoutNative : public GFxFunctionHandler {
public:
    void Invoke(Args *args) override {
        GFxMovieRoot *movie = args->movie->movieRoot;
        std::lock_guard<std::mutex> hold(g_lock);
        movie->CreateArray(args->result);
        for (const auto &[name, rect] : g_rects) {
            if (!OurWidget(name) && name.rfind(PART, 0) != 0) continue;
            HudPlace place = PlaceOf(name);
            GFxValue entry, text, left(rect.ox + place.dx + (rect.left - rect.ox) * place.scale),
                top(rect.oy + place.dy + (rect.top - rect.oy) * place.scale), width(rect.width * place.scale),
                height(rect.height * place.scale), hidden(place.hidden), color(double(place.color));
            movie->CreateObject(&entry);
            movie->CreateString(&text, name.c_str());
            entry.SetMember("name", &text);
            entry.SetMember("left", &left);
            entry.SetMember("top", &top);
            entry.SetMember("width", &width);
            entry.SetMember("height", &height);
            entry.SetMember("hidden", &hidden);
            entry.SetMember("color", &color);
            args->result->PushBack(&entry);
        }
    }
};

// HudPlaceOf(name): the place the player gave `name`, {dx, dy, scale, hidden, color}, or null for none.
class PlaceOfNative : public GFxFunctionHandler {
public:
    void Invoke(Args *args) override {
        std::lock_guard<std::mutex> hold(g_lock);
        auto it = g_places.find(Arg(args, 0));
        if (it == g_places.end()) {
            args->result->SetNull();
            return;
        }
        MakePlace(args->movie->movieRoot, it->second, args->result);
    }
};

// HudHide(name): hide widget `name`, or show it again when hidden.
class HideNative : public GFxFunctionHandler {
public:
    void Invoke(Args *args) override {
        std::lock_guard<std::mutex> hold(g_lock);
        HudPlace &place = g_places[Arg(args, 0)];
        place.hidden = !place.hidden;
        HudChanged();
    }
};

// HudTint(name, color): give widget `name` a color over its own (0xRRGGBB), or its own again for -1.
class TintNative : public GFxFunctionHandler {
public:
    void Invoke(Args *args) override {
        if (args->numArgs < 2) return;
        double color = Num(args->args[1]);
        std::lock_guard<std::mutex> hold(g_lock);
        g_places[Arg(args, 0)].color = color < 0.0 ? -1 : SInt32(UInt32(color) & 0xFFFFFF);
        HudChanged();
    }
};

// HudMove(name, dx, dy): move widget `name` by dx, dy (fractions of the screen).
class MoveNative : public GFxFunctionHandler {
public:
    void Invoke(Args *args) override {
        if (args->numArgs < 3) return;
        std::lock_guard<std::mutex> hold(g_lock);
        HudPlace &place = g_places[Arg(args, 0)];
        place.dx += float(Num(args->args[1]));
        place.dy += float(Num(args->args[2]));
        HudChanged();
    }
};

// HudScale(name, factor): scale widget `name` by `factor`, within a quarter and four times its own size.
class ScaleNative : public GFxFunctionHandler {
public:
    void Invoke(Args *args) override {
        if (args->numArgs < 2) return;
        std::lock_guard<std::mutex> hold(g_lock);
        HudPlace &place = g_places[Arg(args, 0)];
        place.scale = std::clamp(place.scale * float(Num(args->args[1])), MIN_SCALE, MAX_SCALE);
        HudChanged();
    }
};

// HudReset(name): put widget `name` back where its mod puts it, shown, in its own color.
class ResetNative : public GFxFunctionHandler {
public:
    void Invoke(Args *args) override {
        std::lock_guard<std::mutex> hold(g_lock);
        g_places.erase(Arg(args, 0));
        HudChanged();
    }
};

// HudEditDone(): save the layout and close the edit menu.
class DoneNative : public GFxFunctionHandler {
public:
    void Invoke(Args *) override {
        {
            std::lock_guard<std::mutex> hold(g_lock);
            Save();
        }
        BSFixedString menu(EDIT_MENU);
        if (*g_uiMessageManager) CALL_MEMBER_FN(*g_uiMessageManager, SendUIMessage)(menu, kMessage_Close);
    }
};

}  // namespace

void MakePlace(GFxMovieRoot *movie, const HudPlace &place, GFxValue *out) {
    GFxValue dx(double(place.dx)), dy(double(place.dy)), scale(double(place.scale)), hidden(place.hidden),
        color(double(place.color));
    movie->CreateObject(out);
    out->SetMember("dx", &dx);
    out->SetMember("dy", &dy);
    out->SetMember("scale", &scale);
    out->SetMember("hidden", &hidden);
    out->SetMember("color", &color);
}

HudPlace PlaceOf(const std::string &name) {
    auto it = g_places.find(name);
    return it != g_places.end() ? it->second : HudPlace();
}

void LoadLayout() {
    std::ifstream in(LayoutPath());
    std::string line;
    std::lock_guard<std::mutex> hold(g_lock);
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream fields(line);
        std::string name;
        HudPlace place;
        if (!std::getline(fields, name, '\t') || !(fields >> place.dx >> place.dy >> place.scale)) continue;
        place.scale = std::clamp(place.scale, MIN_SCALE, MAX_SCALE);
        // Hidden and color came later (2026-11-13): a line without them is shown in its own color.
        int hidden = 0;
        std::string color;
        if (fields >> hidden) place.hidden = hidden != 0;
        if (fields >> color && color != "-") place.color = SInt32(std::strtoul(color.c_str(), nullptr, 16) & 0xFFFFFF);
        g_places[name] = place;
    }
    _MESSAGE("HUD layout: %u places", UInt32(g_places.size()));
}

void RegisterHudLayout(GFxMovieRoot *movie, GFxValue *plugin) {
    RegisterFunction<ReportNative>(plugin, movie, "HudReport");
    RegisterFunction<PlaceOfNative>(plugin, movie, "HudPlaceOf");
    RegisterFunction<LayoutNative>(plugin, movie, "HudLayout");
    RegisterFunction<MoveNative>(plugin, movie, "HudMove");
    RegisterFunction<ScaleNative>(plugin, movie, "HudScale");
    RegisterFunction<HideNative>(plugin, movie, "HudHide");
    RegisterFunction<TintNative>(plugin, movie, "HudTint");
    RegisterFunction<ResetNative>(plugin, movie, "HudReset");
    RegisterFunction<DoneNative>(plugin, movie, "HudEditDone");
}

}  // namespace crui
