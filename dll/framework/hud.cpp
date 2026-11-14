// Our HUD host (docs/swf_framework.md#hud): mods' widgets on the HUD without replacing HUDMenu.swf.
//
// A mod adds a widget through CRUI_Hud: a movie of its own (any HUDFramework widget: it implements
// processMessage(command, params)) or our built-in stats line (labeled actor values with dim, warning and flashing
// steps). CRUI_Hud (Papyrus) picks the host and tells us (CRUI_HudNative): HUDFramework when it is installed, which
// then loads the widget movies, and our stats widget (Interface/CRUI_HudStats.swf) for the stats lines; else ours.
// Each time the HUD movie loads, this file loads our host movie (Interface/CRUI_Hud.swf) into whatever HUD movie is
// installed, with or without widgets of ours: it also lays out and places the game's own HUD parts (parts.cpp,
// layout.cpp), and the edit menu needs them measured before it opens, since the HUD may not run under it. Each
// frame the host and the stats widget ask for the widget list (HudWidgets, only when it changed), the host for the
// messages sent since (HudMessages), and both for each stats line's values (HudStats). Widgets, stats and messages
// are forgotten as a save loads or a new game starts; each mod adds its own on every load.
#include "condition.h"

#include <algorithm>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

namespace crui {

namespace {

const char *SCRIPT = "CRUI_HudNative", *MENU = "HUDMenu", *HOST = "CRUI_Hud.swf", *HOOKED = "cruiHud";

// A widget: its movie ("" for a stats line), the HUD part it sits under ("" for the HUD itself), its place from
// that part's top-left, and its host ("" ours; "HUDFramework").
struct Widget {
    std::string name, movie, anchor;
    float x = 0.0f, y = 0.0f;
    std::string host;
};

// A stats line: a label and actor value per entry, its steps (dim below, warning from, flashing from) and the global
// that shows it while not 0 (none: always).
struct Stats {
    std::vector<std::string> labels;
    std::vector<RE::ActorValueInfo *> values;
    std::vector<float> steps;
    RE::TESGlobal *shown = nullptr;
};

// A message for widget `name`'s processMessage(command, params).
struct Message {
    std::string name, command;
    std::vector<std::string> params;
};

std::vector<Widget> g_widgets;                   // under g_lock
std::unordered_map<std::string, Stats> g_stats;  // under g_lock
std::vector<Message> g_messages;                 // under g_lock
double g_version = 0.0;                          // raised as g_widgets changes, under g_lock

// The widget named `name`, or null (held under g_lock).
const Widget *Find(const std::string &name) {
    auto at = std::ranges::find(g_widgets, name, &Widget::name);
    return at != g_widgets.end() ? &*at : nullptr;
}

}  // namespace

bool OurWidget(const std::string &name) {
    const Widget *widget = Find(name);
    return widget && widget->host.empty();
}

void HudChanged() {
    g_version++;
}

namespace {

// Load our host into the HUD movie, once per HUD movie. On the UI thread.
void HookMovie() {
    ReadShowSettings();
    Movie *movie = MenuMovie(MENU);
    Value root;
    if (!movie) return;
    if (!movie->GetVariable(&root, "root") || !root.IsObject()) {
        REX::WARN("HUD opened, but its movie has no root: no widgets");
        return;
    }
    if (root.HasMember(HOOKED)) return;
    root.SetMember(HOOKED, true);
    REX::DEBUG("HUD: {} {}", HOST, LoadChild(movie, root, HOST) ? "loading" : "not loaded");
}

void Hook() {
    if (Tasks()) Tasks()->AddUITask([] { HookMovie(); });
}

// Put `widget` in the list (a name given again is replaced in place).
void Put(const Widget &widget) {
    {
        std::lock_guard hold(g_lock);
        auto at = std::ranges::find(g_widgets, widget.name, &Widget::name);
        if (at != g_widgets.end()) *at = widget;
        else g_widgets.push_back(widget);
        g_version++;
    }
    Hook();
}

// The HUD opened (each game load gives a new HUD movie): load our host into it.
class OpenSink : public RE::BSTEventSink<RE::MenuOpenCloseEvent> {
public:
    RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent &event,
                                          RE::BSTEventSource<RE::MenuOpenCloseEvent> *) override {
        const char *name = event.menuName.c_str();
        if (event.opening && name && !std::strcmp(name, MENU)) Hook();
        return RE::BSEventNotifyControl::kContinue;
    }
};
OpenSink g_openSink;

// ---- Papyrus (CRUI_HudNative, called by CRUI_Hud) ----

// Papyrus: show `movie` (a path under Interface, any HUDFramework widget) as widget `name` under HUD part `anchor`
// (a clip path from the HUD's root; "" for the HUD itself) at x, y from that part's top-left, on host `host` ("" ours;
// "HUDFramework", which CRUI_Hud has registered it with).
void AddWidget(std::monostate, std::string name, std::string movie, float x, float y, std::string anchor,
               std::string host) {
    Put({name, movie, anchor, x, y, host});
}

// Papyrus: show a stats line as widget `name`: each label with its actor value on the player, dim below steps[0],
// in the HUD's warning color from steps[1] and flashing from steps[2] (fewer steps: those stay off), while `shown`
// is not 0 (None: always); placed and hosted as AddWidget does a widget.
void AddStats(std::monostate, std::string name, std::vector<std::string> labels,
              std::vector<RE::ActorValueInfo *> values, std::vector<float> steps, RE::TESGlobal *shown, float x,
              float y, std::string anchor, std::string host) {
    {
        std::lock_guard hold(g_lock);
        g_stats[name] = {labels, values, steps, shown};
    }
    Put({name, "", anchor, x, y, host});
}

// Papyrus: widget `name`'s host ("" ours, "HUDFramework"); "none" for no such widget.
std::string HostOf(std::monostate, std::string name) {
    std::lock_guard hold(g_lock);
    const Widget *widget = Find(name);
    return widget ? widget->host : "none";
}

// Papyrus: widget `name`'s movie ("" for a stats line or no such widget).
std::string MovieOf(std::monostate, std::string name) {
    std::lock_guard hold(g_lock);
    const Widget *widget = Find(name);
    return widget ? widget->movie : "";
}

// Papyrus: take widget `name` off the HUD.
void RemoveWidget(std::monostate, std::string name) {
    std::lock_guard hold(g_lock);
    std::erase_if(g_widgets, [&](const Widget &w) { return w.name == name; });
    g_stats.erase(name);
    g_version++;
}

// Papyrus: call widget `name`'s processMessage(command, params) on its next frame (held until it has loaded).
void Send(std::monostate, std::string name, std::string command, std::vector<std::string> params) {
    std::lock_guard hold(g_lock);
    g_messages.push_back({name, command, params});
}

// ---- The host movie's natives (f4se.plugins.CleanroomUI) ----

// Widget `w` as the host takes it, {name, movie, anchor, x, y, host, place}.
Value WidgetObject(Movie *movie, const Widget &w) {
    Value entry, place;
    movie->CreateObject(&entry);
    entry.SetMember("name", String(movie, w.name));
    entry.SetMember("movie", String(movie, w.movie));
    entry.SetMember("anchor", String(movie, w.anchor));
    entry.SetMember("x", double(w.x));
    entry.SetMember("y", double(w.y));
    entry.SetMember("host", String(movie, w.host));
    MakePlace(movie, w.name, PlaceOf(w.name), &place);
    entry.SetMember("place", place);
    return entry;
}

// HudWidgets(version): {version, widgets: [{name, movie, anchor, x, y, host, place: {dx, dy, scale, hidden, color,
// show}}]}, or null while the list is still the one numbered `version` (a widget added or removed, or a place
// changed in the edit menu).
class WidgetsNative : public FunctionHandler {
public:
    void Call(const Params &params) override {
        Movie *movie = params.movie;
        std::lock_guard hold(g_lock);
        *params.retVal = nullptr;
        if (params.argCount > 0 && Num(params.args[0]) == g_version) return;
        Value widgets;
        movie->CreateArray(&widgets);
        for (const Widget &w : g_widgets) widgets.PushBack(WidgetObject(movie, w));
        movie->CreateObject(params.retVal);
        params.retVal->SetMember("version", g_version);
        params.retVal->SetMember("widgets", widgets);
    }
};

// HudMessages(): the messages sent since the last call, [{name, command, params}], in order.
class MessagesNative : public FunctionHandler {
public:
    void Call(const Params &params) override {
        Movie *movie = params.movie;
        std::vector<Message> messages;
        {
            std::lock_guard hold(g_lock);
            messages.swap(g_messages);
        }
        movie->CreateArray(params.retVal);
        for (const Message &m : messages) {
            Value entry, list;
            movie->CreateObject(&entry);
            movie->CreateArray(&list);
            for (const std::string &param : m.params) list.PushBack(String(movie, param));
            entry.SetMember("name", String(movie, m.name));
            entry.SetMember("command", String(movie, m.command));
            entry.SetMember("params", list);
            params.retVal->PushBack(entry);
        }
    }
};

// HudStats(name): stats line `name` now, {shown, rows: [{text, value}], steps}; null for no such line.
class StatsNative : public FunctionHandler {
public:
    void Call(const Params &params) override {
        Movie *movie = params.movie;
        *params.retVal = nullptr;
        if (params.argCount < 1 || !params.args[0].IsString()) return;
        Stats stats;
        {
            std::lock_guard hold(g_lock);
            auto it = g_stats.find(params.args[0].GetString());
            if (it == g_stats.end()) return;
            stats = it->second;
        }
        RE::PlayerCharacter *player = RE::PlayerCharacter::GetSingleton();
        Value rows, steps;
        movie->CreateArray(&rows);
        for (std::size_t i = 0; i < stats.labels.size() && i < stats.values.size(); i++) {
            Value row;
            movie->CreateObject(&row);
            row.SetMember("text", String(movie, stats.labels[i]));
            row.SetMember("value", stats.values[i] && player ? double(player->GetActorValue(*stats.values[i])) : 0.0);
            rows.PushBack(row);
        }
        movie->CreateArray(&steps);
        for (float step : stats.steps) steps.PushBack(double(step));
        movie->CreateObject(params.retVal);
        params.retVal->SetMember("shown", player && (!stats.shown || stats.shown->GetValue() != 0.0f));
        params.retVal->SetMember("rows", rows);
        params.retVal->SetMember("steps", steps);
    }
};

}  // namespace

void InstallHud() {
    RE::UI *ui = RE::UI::GetSingleton();
    if (ui) ui->GetEventSource<RE::MenuOpenCloseEvent>()->RegisterSink(&g_openSink);
    REX::INFO("HUD host {}", ui ? "listening for the HUD" : "could not listen for menus");
    LoadLayout();
    Hook();
}

void ForgetHud() {
    std::lock_guard hold(g_lock);
    g_widgets.clear();
    g_stats.clear();
    g_messages.clear();
    g_version++;
    ForgetShow();
}

void RegisterHudMenu(Movie *movie, Value *plugin) {
    RegisterFunction<WidgetsNative>(plugin, movie, "HudWidgets");
    RegisterFunction<MessagesNative>(plugin, movie, "HudMessages");
    RegisterFunction<StatsNative>(plugin, movie, "HudStats");
    RegisterHudLayout(movie, plugin);
    RegisterHudParts(movie, plugin);
    RegisterHudShow(movie, plugin);
}

void RegisterHud(RE::BSScript::IVirtualMachine *vm) {
    vm->BindNativeMethod(SCRIPT, "AddWidget", AddWidget);
    vm->BindNativeMethod(SCRIPT, "AddStats", AddStats);
    vm->BindNativeMethod(SCRIPT, "HostOf", HostOf);
    vm->BindNativeMethod(SCRIPT, "MovieOf", MovieOf);
    vm->BindNativeMethod(SCRIPT, "RemoveWidget", RemoveWidget);
    vm->BindNativeMethod(SCRIPT, "Send", Send);
    RegisterShow(vm);
}

}  // namespace crui
