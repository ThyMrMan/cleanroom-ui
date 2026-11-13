// Our HUD host (docs/swf_framework.md#hud): mods' widgets on the HUD without replacing HUDMenu.swf.
//
// A mod adds a widget through CRUI_Hud: a movie of its own (any HUDFramework widget: it implements
// processMessage(command, params)) or our built-in stats line (labeled actor values with dim, warning and flashing
// steps). CRUI_Hud (Papyrus) picks the host and tells us (CRUI_HudNative): HUDFramework when it is installed, which
// then loads the widget movies, and our stats widget (Interface/CRUI_HudStats.swf) for the stats lines; else ours.
// Once a widget of ours is added, and again each time the HUD movie loads, this file loads our host movie
// (Interface/CRUI_Hud.swf) into whatever HUD movie is installed; with a DEF_HUD.xml layout it loads it too, to lay
// out the game's own HUD parts (parts.cpp). Each frame the host and the stats widget ask for the
// widget list (HudWidgets, only when it changed), the host for the messages sent since (HudMessages), and both for
// each stats line's values (HudStats). With no widget of ours and no layout, our host is not loaded. Widgets, stats and messages
// are forgotten as a save loads or a new game starts; each mod adds its own on every load.
#include "condition.h"

#include "f4se/GameMenus.h"
#include "f4se/GameThreads.h"
#include "f4se/PapyrusArgs.h"
#include "f4se/PapyrusNativeFunctions.h"
#include "f4se/PluginAPI.h"
#include "f4se/ScaleformCallbacks.h"

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
    std::vector<ActorValueInfo *> values;
    std::vector<float> steps;
    TESGlobal *shown = nullptr;
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
F4SETaskInterface *g_tasks = nullptr;

// `array`'s strings.
std::vector<std::string> Strings(VMArray<BSFixedString> &array) {
    std::vector<std::string> out;
    for (UInt32 i = 0; i < array.Length(); i++) {
        BSFixedString text;
        array.Get(&text, i);
        out.push_back(text.c_str() ? text.c_str() : "");
    }
    return out;
}

// Whether a widget is ours to host (none of HUDFramework's).
bool Ours() {
    std::lock_guard<std::mutex> hold(g_lock);
    return std::any_of(g_widgets.begin(), g_widgets.end(), [](const Widget &w) { return w.host.empty(); });
}

// The widget named `name`, or null (held under g_lock).
const Widget *Find(const std::string &name) {
    auto at = std::find_if(g_widgets.begin(), g_widgets.end(), [&](const Widget &w) { return w.name == name; });
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

// Load our host into the HUD movie, once per HUD movie and only once a widget of ours has been added or there is a
// layout for the HUD's parts.
class HookTask : public ITaskDelegate {
public:
    void Run() override {
        if (!Ours() && !HasPartLayout()) return;
        BSFixedString name(MENU);
        IMenu *menu = *g_ui ? (*g_ui)->GetMenu(name) : nullptr;
        GFxValue root, flag(true);
        if (!menu || !menu->movie || !menu->movie->movieRoot) return;
        GFxMovieRoot *movie = menu->movie->movieRoot;
        if (!movie->GetVariable(&root, "root") || !root.IsObject()) {
            _WARNING("HUD opened, but its movie has no root: no widgets");
            return;
        }
        if (root.HasMember(HOOKED)) return;
        root.SetMember(HOOKED, &flag);
        _VMESSAGE("HUD: %s %s", HOST, LoadChild(movie, root, HOST) ? "loading" : "not loaded");
    }
};

void Hook() {
    if (g_tasks) g_tasks->AddUITask(new HookTask);
}

// Put `widget` in the list (a name given again is replaced in place).
void Put(const Widget &widget) {
    {
        std::lock_guard<std::mutex> hold(g_lock);
        auto at = std::find_if(g_widgets.begin(), g_widgets.end(), [&](const Widget &w) { return w.name == widget.name; });
        if (at != g_widgets.end()) *at = widget;
        else g_widgets.push_back(widget);
        g_version++;
    }
    Hook();
}

// The HUD opened (each game load gives a new HUD movie): load our host into it.
class OpenSink : public BSTEventSink<MenuOpenCloseEvent> {
public:
    EventResult ReceiveEvent(MenuOpenCloseEvent *evn, void *) override {
        if (evn && evn->isOpen && !strcmp(evn->menuName.c_str(), MENU)) Hook();
        return kEvent_Continue;
    }
};
OpenSink g_openSink;

// ---- Papyrus (CRUI_HudNative, called by CRUI_Hud) ----

std::string Text(BSFixedString &text) {
    return text.c_str() ? text.c_str() : "";
}

// Papyrus: show `movie` (a path under Interface, any HUDFramework widget) as widget `name` under HUD part `anchor`
// (a clip path from the HUD's root; "" for the HUD itself) at x, y from that part's top-left, on host `host` ("" ours;
// "HUDFramework", which CRUI_Hud has registered it with).
void AddWidget(StaticFunctionTag *, BSFixedString name, BSFixedString movie, float x, float y, BSFixedString anchor,
               BSFixedString host) {
    Put({Text(name), Text(movie), Text(anchor), x, y, Text(host)});
}

// Papyrus: show a stats line as widget `name`: each label with its actor value on the player, dim below steps[0],
// in the HUD's warning color from steps[1] and flashing from steps[2] (fewer steps: those stay off), while `shown`
// is not 0 (None: always); placed and hosted as AddWidget does a widget.
void AddStats(StaticFunctionTag *, BSFixedString name, VMArray<BSFixedString> labels, VMArray<ActorValueInfo *> values,
              VMArray<float> steps, TESGlobal *shown, float x, float y, BSFixedString anchor, BSFixedString host) {
    Stats stats;
    stats.labels = Strings(labels);
    for (UInt32 i = 0; i < values.Length(); i++) {
        ActorValueInfo *value = nullptr;
        values.Get(&value, i);
        stats.values.push_back(value);
    }
    for (UInt32 i = 0; i < steps.Length(); i++) {
        float step = 0.0f;
        steps.Get(&step, i);
        stats.steps.push_back(step);
    }
    stats.shown = shown;
    {
        std::lock_guard<std::mutex> hold(g_lock);
        g_stats[Text(name)] = stats;
    }
    Put({Text(name), "", Text(anchor), x, y, Text(host)});
}

// Papyrus: widget `name`'s host ("" ours, "HUDFramework"); "none" for no such widget.
BSFixedString HostOf(StaticFunctionTag *, BSFixedString name) {
    std::lock_guard<std::mutex> hold(g_lock);
    const Widget *widget = Find(Text(name));
    return BSFixedString(widget ? widget->host.c_str() : "none");
}

// Papyrus: widget `name`'s movie ("" for a stats line or no such widget).
BSFixedString MovieOf(StaticFunctionTag *, BSFixedString name) {
    std::lock_guard<std::mutex> hold(g_lock);
    const Widget *widget = Find(Text(name));
    return BSFixedString(widget ? widget->movie.c_str() : "");
}

// Papyrus: take widget `name` off the HUD.
void RemoveWidget(StaticFunctionTag *, BSFixedString name) {
    std::lock_guard<std::mutex> hold(g_lock);
    std::string key = Text(name);
    g_widgets.erase(std::remove_if(g_widgets.begin(), g_widgets.end(), [&](const Widget &w) { return w.name == key; }),
                    g_widgets.end());
    g_stats.erase(key);
    g_version++;
}

// Papyrus: call widget `name`'s processMessage(command, params) on its next frame (held until it has loaded).
void Send(StaticFunctionTag *, BSFixedString name, BSFixedString command, VMArray<BSFixedString> params) {
    std::lock_guard<std::mutex> hold(g_lock);
    g_messages.push_back({Text(name), Text(command), Strings(params)});
}

// ---- The host movie's natives (f4se.plugins.CleanroomUI) ----

GFxValue Str(GFxMovieRoot *movie, const std::string &text) {
    GFxValue out;
    movie->CreateString(&out, text.c_str());
    return out;
}

// HudWidgets(version): {version, widgets: [{name, movie, anchor, x, y, host, place: {dx, dy, scale}}]}, or null while
// the list is still the one numbered `version` (a widget added or removed, or a place changed in the edit menu).
class WidgetsNative : public GFxFunctionHandler {
public:
    void Invoke(Args *args) override {
        GFxMovieRoot *movie = args->movie->movieRoot;
        std::lock_guard<std::mutex> hold(g_lock);
        args->result->SetNull();
        if (args->numArgs > 0 && Num(args->args[0]) == g_version) return;
        GFxValue widgets, version(g_version);
        movie->CreateArray(&widgets);
        for (const Widget &w : g_widgets) {
            GFxValue entry, name = Str(movie, w.name), path = Str(movie, w.movie), anchor = Str(movie, w.anchor),
                host = Str(movie, w.host);
            GFxValue x(double(w.x)), y(double(w.y));
            movie->CreateObject(&entry);
            entry.SetMember("name", &name);
            entry.SetMember("movie", &path);
            entry.SetMember("anchor", &anchor);
            entry.SetMember("x", &x);
            entry.SetMember("y", &y);
            entry.SetMember("host", &host);
            HudPlace at = PlaceOf(w.name);
            GFxValue place, dx(double(at.dx)), dy(double(at.dy)), scale(double(at.scale));
            movie->CreateObject(&place);
            place.SetMember("dx", &dx);
            place.SetMember("dy", &dy);
            place.SetMember("scale", &scale);
            entry.SetMember("place", &place);
            widgets.PushBack(&entry);
        }
        movie->CreateObject(args->result);
        args->result->SetMember("version", &version);
        args->result->SetMember("widgets", &widgets);
    }
};

// HudMessages(): the messages sent since the last call, [{name, command, params}], in order.
class MessagesNative : public GFxFunctionHandler {
public:
    void Invoke(Args *args) override {
        GFxMovieRoot *movie = args->movie->movieRoot;
        std::vector<Message> messages;
        {
            std::lock_guard<std::mutex> hold(g_lock);
            messages.swap(g_messages);
        }
        movie->CreateArray(args->result);
        for (const Message &m : messages) {
            GFxValue entry, params, name = Str(movie, m.name), command = Str(movie, m.command);
            movie->CreateObject(&entry);
            movie->CreateArray(&params);
            for (const std::string &param : m.params) {
                GFxValue text = Str(movie, param);
                params.PushBack(&text);
            }
            entry.SetMember("name", &name);
            entry.SetMember("command", &command);
            entry.SetMember("params", &params);
            args->result->PushBack(&entry);
        }
    }
};

// HudStats(name): stats line `name` now, {shown, rows: [{text, value}], steps}; null for no such line.
class StatsNative : public GFxFunctionHandler {
public:
    void Invoke(Args *args) override {
        GFxMovieRoot *movie = args->movie->movieRoot;
        args->result->SetNull();
        if (args->numArgs < 1 || !args->args[0].IsString()) return;
        Stats stats;
        {
            std::lock_guard<std::mutex> hold(g_lock);
            auto it = g_stats.find(args->args[0].GetString());
            if (it == g_stats.end()) return;
            stats = it->second;
        }
        Actor *player = *g_player;
        GFxValue rows, steps, shown(player && (!stats.shown || stats.shown->value != 0.0f));
        movie->CreateArray(&rows);
        for (size_t i = 0; i < stats.labels.size() && i < stats.values.size(); i++) {
            GFxValue row, text = Str(movie, stats.labels[i]);
            GFxValue value(stats.values[i] && player ? double(player->actorValueOwner.GetValue(stats.values[i])) : 0.0);
            movie->CreateObject(&row);
            row.SetMember("text", &text);
            row.SetMember("value", &value);
            rows.PushBack(&row);
        }
        movie->CreateArray(&steps);
        for (float step : stats.steps) {
            GFxValue at{double(step)};
            steps.PushBack(&at);
        }
        movie->CreateObject(args->result);
        args->result->SetMember("shown", &shown);
        args->result->SetMember("rows", &rows);
        args->result->SetMember("steps", &steps);
    }
};

}  // namespace

void InstallHud(void *tasks) {
    g_tasks = static_cast<F4SETaskInterface *>(tasks);
    bool added = *g_ui && (*g_ui)->menuOpenCloseEventSource.AddEventSink(&g_openSink);
    _MESSAGE("HUD host %s", added ? "listening for the HUD" : "could not listen for menus");
    LoadLayout();
    Hook();
}

void ForgetHud() {
    std::lock_guard<std::mutex> hold(g_lock);
    g_widgets.clear();
    g_stats.clear();
    g_messages.clear();
    g_version++;
}

void RegisterHudMenu(GFxMovieRoot *movie, GFxValue *plugin) {
    RegisterFunction<WidgetsNative>(plugin, movie, "HudWidgets");
    RegisterFunction<MessagesNative>(plugin, movie, "HudMessages");
    RegisterFunction<StatsNative>(plugin, movie, "HudStats");
    RegisterHudLayout(movie, plugin);
    RegisterHudParts(movie, plugin);
}

void RegisterHud(VirtualMachine *vm) {
    vm->RegisterFunction(new NativeFunction6<StaticFunctionTag, void, BSFixedString, BSFixedString, float, float,
                                             BSFixedString, BSFixedString>("AddWidget", SCRIPT, AddWidget, vm));
    vm->RegisterFunction(new NativeFunction9<StaticFunctionTag, void, BSFixedString, VMArray<BSFixedString>,
                                             VMArray<ActorValueInfo *>, VMArray<float>, TESGlobal *, float, float,
                                             BSFixedString, BSFixedString>("AddStats", SCRIPT, AddStats, vm));
    vm->RegisterFunction(new NativeFunction1<StaticFunctionTag, BSFixedString, BSFixedString>("HostOf", SCRIPT,
                                                                                              HostOf, vm));
    vm->RegisterFunction(new NativeFunction1<StaticFunctionTag, BSFixedString, BSFixedString>("MovieOf", SCRIPT,
                                                                                              MovieOf, vm));
    vm->RegisterFunction(new NativeFunction1<StaticFunctionTag, void, BSFixedString>("RemoveWidget", SCRIPT,
                                                                                     RemoveWidget, vm));
    vm->RegisterFunction(new NativeFunction3<StaticFunctionTag, void, BSFixedString, BSFixedString,
                                             VMArray<BSFixedString>>("Send", SCRIPT, Send, vm));
}

}  // namespace crui
