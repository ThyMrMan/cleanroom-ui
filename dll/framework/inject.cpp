// Other mods' movies in the game's menus (docs/swf_framework.md#inject), with no vanilla movie shipped and no script.
//
// Each Data\F4SE\Plugins\CleanroomUI\Menus\*.ini is one mod's list. A section names a menu, by its menu name
// ([ContainerMenu]) or by its movie's file name ([ContainerMenu.swf], any menu showing that movie, whatever it is
// registered as); under it, sMovie is the movie to load (a path under Interface) and sParent the clip to load it into
// (root when not given). As such a menu opens, the movie is loaded into it once, with a Loader as F4SE's UI.Load does;
// it reaches F4SE's object, and our natives, through stage.getChildAt(0).f4se. The files are read once, as the game
// loads.
#include "condition.h"

#include <REX/W32/KERNEL32.h>
#include <Scaleform/G/GFx_MovieDef.h>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <mutex>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace crui {

namespace {

const char *FOLDER = "Data\\F4SE\\Plugins\\CleanroomUI\\Menus";

// One movie to load: into which menu (`menu`, or the menu showing movie file `file`), from where, and into what.
struct Injection {
    std::string menu, file, movie, parent, source;
};

std::vector<Injection> g_injections;
// Which injections the open menus already have (menu name, index into g_injections), forgotten as a menu closes.
std::set<std::pair<std::string, std::size_t>> g_loaded;
std::mutex g_injectLock;

std::string Lower(std::string text) {
    std::ranges::transform(text, text.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return text;
}

// The file name of movie URL `url` ("Interface/ContainerMenu.swf"), lowercased.
std::string FileOf(const char *url) {
    std::string path = url ? Lower(url) : "";
    std::size_t slash = path.find_last_of("/\\");
    return slash == std::string::npos ? path : path.substr(slash + 1);
}

// Setting `name` under [`section`] of INI `file`, or "".
std::string Setting(const std::string &file, const std::string &section, const char *name) {
    char buffer[512];
    auto size = REX::W32::GetPrivateProfileStringA(section.c_str(), name, "", buffer, sizeof(buffer), file.c_str());
    return std::string(buffer, size);
}

// The injections of list `file`: one per section with an sMovie, into g_injections.
void ReadList(const std::filesystem::path &file) {
    std::string path = file.string();
    for (const std::string &section : IniNames(path, nullptr)) {
        std::string movie = Setting(path, section, "sMovie");
        if (movie.empty()) {
            REX::WARN("{} [{}]: no sMovie, skipped", file.filename().string(), section);
            continue;
        }
        std::string parent = Setting(path, section, "sParent");
        bool byFile = Lower(section).ends_with(".swf");
        g_injections.push_back({byFile ? "" : section, byFile ? Lower(section) : "", movie,
                                parent.empty() ? "root" : parent, file.filename().string()});
    }
}

// Whether injection `entry` goes into the menu `name`, whose movie is `movie`.
bool Targets(const Injection &entry, const char *name, Movie *movie) {
    if (!entry.menu.empty()) return Lower(entry.menu) == Lower(name);
    Scaleform::GFx::MovieDef *def = movie->GetMovieDef();
    return def && FileOf(def->GetFileURL()) == entry.file;
}

// Load every injection menu `name` takes and doesn't have yet. On the UI thread.
void Inject(const std::string &name) {
    Movie *movie = MenuMovie(name.c_str());
    if (!movie) return;
    for (std::size_t i = 0; i < g_injections.size(); i++) {
        const Injection &entry = g_injections[i];
        if (!Targets(entry, name.c_str(), movie)) continue;
        {
            std::lock_guard hold(g_injectLock);
            if (!g_loaded.emplace(name, i).second) continue;
        }
        Value parent;
        bool loaded = movie->GetVariable(&parent, entry.parent.c_str()) && parent.IsObject() &&
                      LoadChild(movie, parent, entry.movie.c_str());
        if (loaded) REX::DEBUG("{}: loading {} into {} ({})", name, entry.movie, entry.parent, entry.source);
        else REX::WARN("{}: could not load {} into {} ({})", name, entry.movie, entry.parent, entry.source);
    }
}

// Menus opening and closing: inject into each as it opens; forget what a closed one had.
class InjectSink : public RE::BSTEventSink<RE::MenuOpenCloseEvent> {
public:
    RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent &event,
                                          RE::BSTEventSource<RE::MenuOpenCloseEvent> *) override {
        std::string name = event.menuName.c_str() ? event.menuName.c_str() : "";
        if (name.empty()) return RE::BSEventNotifyControl::kContinue;
        if (event.opening) {
            if (Tasks()) Tasks()->AddUITask([name] { Inject(name); });
        } else {
            std::lock_guard hold(g_injectLock);
            std::erase_if(g_loaded, [&](const auto &loaded) { return loaded.first == name; });
        }
        return RE::BSEventNotifyControl::kContinue;
    }
};
InjectSink g_injectSink;

}  // namespace

void InstallInjections() {
    static std::once_flag once;
    std::call_once(once, [] {
        std::error_code error;
        for (const auto &entry : std::filesystem::directory_iterator(FOLDER, error))
            if (Lower(entry.path().extension().string()) == ".ini") ReadList(entry.path());
        RE::UI *ui = RE::UI::GetSingleton();
        if (!g_injections.empty() && ui) ui->GetEventSource<RE::MenuOpenCloseEvent>()->RegisterSink(&g_injectSink);
        REX::INFO("{} movies for other menus in {}", g_injections.size(), FOLDER);
    });
}

}  // namespace crui
