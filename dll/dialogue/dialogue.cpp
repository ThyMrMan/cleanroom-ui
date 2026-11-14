// CleanroomDialogue.dll: our dialogue menu in place of XDI (docs/fork/fo4_dialogue_menu_plan.md#test-build).
//
// An option is a player INFO that names an answer topic (the links in Data\F4SE\Plugins\CleanroomDialogue\*.txt); its
// answer is that topic's first passing INFO, chosen by the engine's own picker, so nothing pairs options and answers
// by position. The option shows that answer line's prompt (FNV shows the first passing line's) and skill check. The
// menu movie (Interface/DialogueMenu.swf) asks for the running Player Dialogue action's options and picks one as
// option index + 5. The engine then asks three call sites for the INFOs of the picked option; index 5 and up are ours,
// below 5 go to the engine's functions as before.
//
// Read in Fallout4.exe 1.11.240 (its RVAs in the comments); each address is its next-gen Address Library ID
// (docs/spin_offs.md#ids). The engine layouts below were read there only, so the hooks stay off on other runtimes.
#include "dialogue.h"

#include <atomic>
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace crui {

namespace {

using RE::TESForm;
using RE::TESObjectREFR;
using Scaleform::GFx::FunctionHandler;
using Scaleform::GFx::Value;

// ---- Engine functions ----
// The option's INFO for the player (target the NPC) or the NPC (target the player), reading the action's chosen option.
using OptionInfoFn = TESForm *(*)(void *action, void *scene, TESObjectREFR *target, std::int32_t option);
// An NPC Response action's INFO for the player's chosen response (player + PLAYER_RESPONSE).
using ResponseFn = TESForm *(*)(void *action, void *scene);
// A topic's first passing INFO for `speaker` talking to `target` (Random runs included).
using PickFn = TESForm *(*)(TESForm *topic, TESObjectREFR *speaker, TESObjectREFR *target);
// Whether an INFO passes for `speaker` and `target`: not deleted, not a said Say Once, its quest's dialogue
// conditions and its own.
using PassesFn = bool (*)(TESForm *info, bool *flag, TESObjectREFR *speaker, TESObjectREFR *target);
// An INFO's prompt (RNAM), through its shared INFO; "" without one.
using PromptFn = const char *(*)(TESForm *info);
// Mark an INFO said (its said byte, its change flag for the save, its reset timer), as speaking it does.
using SaidFn = void (*)(TESForm *info);
// A quest alias's reference handle (the option functions then look its reference up, as ActionActor does).
using AliasFn = std::uint32_t *(*)(TESForm *quest, std::uint32_t *handle, std::uint32_t alias);
// Pick dialogue option `option` (4: none) for the player.
using SelectFn = void (*)(RE::PlayerCharacter *player, std::uint32_t option);
// A Player Dialogue action's run function (its vtable slot 21; both option calls are in it).
using RunFn = std::uint64_t (*)(void *action, void *scene, void *third, void *fourth);
// Stop a scene: Papyrus Scene.Stop's native (0x1119D20) calls this, then sets SCENE_STOPPING in its flags.
using StopFn = void (*)(void *scene);
// An actor's "talking" test, the one the run function waits on before the NPC slot (vtable +0x270, 0x4159BF).
using TalkingFn = bool (*)(void *actor);
// An input handler's "should handle" test (BSInputEventUser slot 1); an input event's user-event name (its slot 2);
// the game's static ZoomIn / ZoomOut user-event names.
using ShouldFn = bool (*)(void *handler, void *event);
using EventNameFn = void **(*)(void *event);
using StaticNameFn = void **(*)();
// BSGFxShaderFXTarget::EnableShadedBackground(type, brightness): the UI shader's translucent quad behind a target.
using ShadeFn = void (*)(void *target, std::uint32_t type, float brightness);
// DialogueMenu's holder color (0x1043EF0): 0 the HUD color, 1-7 the speech-challenge tints (iDialogueSC_*_Tint).
using TintFn = void (*)(void *menu, void *target, std::uint32_t type, bool dim);

const Ids OPTION_INFO = Both(781358, 2196825);              // 0x416600
const Ids RESPONSE_INFO = Both(1369299, 2196806);           // 0x414180
const Ids PICK = Both(433331, 2208361);                     // 0x698DB0
const Ids PASSES = Both(244581, 2208464);                   // 0x69F490
const Ids PROMPT = Both(1435199, 2208446);                  // 0x69E740
const Ids MARK_SAID = Both(1160364, 2208467);               // 0x69F8D0
const Ids ALIAS_HANDLE = Both(688914, 2207810);             // 0x671FA0
const Ids SELECT_OPTION = Both(696117, 2233190);            // 0xD76130
const Ids STOP_SCENE = Both(904456, 2206866);               // 0x638D60
// The player-slot and NPC-slot calls to OptionInfo (in 0x415280), and the NPC Response call to ResponseInfo.
const Ids PLAYER_SLOT_CALL = Both(1439851, 2196817, 0x40A, 0x3E1); // 0x41568A
const Ids NPC_SLOT_CALL = Both(1439851, 2196817, 0x7A1, 0x777); // 0x415A21
const Ids RESPONSE_CALL = Both(574707, 2196800, 0x388);     // 0x413DA8
const Ids PLAYER_DIALOGUE_VTABLE = Both(388825, 388825);    // 0x2488978
const std::size_t RUN_SLOT = 21;
// The player starting to say the picked option (`mov rcx, rax` before the call at 0x4158E9); `jmp 0x4158F2` takes
// the said path without saying it (old-gen orders that block otherwise: its jump is 0x14 bytes, not 0x1E). The camera's switch to the player (its 1 at 0x832665 + 7; 2 keeps it on the NPC).
const Ids PLAYER_SPEAKS = Both(1439851, 2196817, 0x652, 0x62E); // 0x4158D2
const Ids CAMERA_TO_PLAYER = Both(24497, 2214898, 0xADC, 0x940); // 0x83266C
// The camera states that take the mouse wheel (first person: ZoomOut to third; third person: both), their vtables
// and "should handle" tests; the ZoomIn and ZoomOut name getters those tests call.
const Ids FIRST_PERSON_VTABLE = Both(246953, 246953);       // 0x25AAB08
const Ids FIRST_PERSON_SHOULD = Both(895531, 2248264);      // 0x10213E0
const Ids THIRD_PERSON_VTABLE = Both(728085, 728085);       // 0x2522A88
const Ids THIRD_PERSON_SHOULD = Both(908569, 2248466);      // 0x102DFC0
const Ids ZOOM_IN_NAME = Both(1282473, 2224896);            // 0xB25620
const Ids ZOOM_OUT_NAME = Both(750808, 2223332);            // 0xA7A080
// The shaded background, and a plain BSGFxShaderFXTarget's vtable (what DialogueMenu registers its holders as).
const Ids ENABLE_SHADED_BACKGROUND = Both(278402, 2287022); // 0x1B04350
const Ids SHADER_TARGET_VTABLE = Both(298683, 298683);      // 0x27236D0
const Ids TINT_CHALLENGE = Both(1432530, 2248685);          // 0x1043EF0
const std::uint8_t SPEAKS[] = {0x48, 0x8B, 0xC8}, SILENT[] = {0xEB, 0x1E, 0x90}, SILENT_OG[] = {0xEB, 0x14, 0x90};
const std::uint8_t CAMERA_PLAYER = 1, CAMERA_SPEAKER = 2;

// ---- Engine layouts (1.11.240) ----
// BGSScene: its quest. Scene action: its alias, its four player topics, its chosen option (4: none).
const std::size_t SCENE_QUEST = 0xA8, ACTION_ALIAS = 0x08, ACTION_TOPICS = 0x20, ACTION_OPTION = 0xE0;
// TESTopic: its INFOs, their order (u16 head at +2, next of INFO i at 4 * i + 4), their count. TESTopicInfo: said.
const std::size_t TOPIC_INFOS = 0x50, TOPIC_ORDER = 0x58, TOPIC_COUNT = 0x60, INFO_SAID = 0x43;
// PlayerCharacter: the chosen response an NPC Response action answers.
const std::size_t PLAYER_RESPONSE = 0xD3C;
// BGSScene flags (Scene.Stop sets STOPPING). Action flags: the player line done (the NPC slot runs next), the NPC
// line said. Player: the flag that skips the talking wait (0x4159B3). Actor: the "talking" vtable slot.
const std::size_t SCENE_FLAGS = 0x28, ACTION_FLAGS = 0x10, PLAYER_NO_WAIT = 0x440, TALKING_SLOT = 0x270 / 8;
// TESObjectREFR's model state (CommonLibF4's modelState); 1 while its 3D loads, when 0x415AC5 holds the NPC's line.
const std::size_t REFR_MODEL_STATE = 0x10A;
const std::uint8_t MODEL_LOADING = 1;
const std::uint32_t SCENE_STOPPING = 0x100000, PLAYER_DONE = 0x2000000, NPC_SAID = 0x4000;
const std::uint32_t NO_OPTION = 4, FIRST_OURS = 5, SLOTS = 4;
// DialogueMenu: the Question holder's shader target (registerObjects keeps the four at +0xE0, Question last). The
// shaded background as vanilla menus ask for it: MenuNoColorBackground at full size.
const std::size_t MENU_NEGATIVE_TARGET = 0xE8, MENU_QUESTION_TARGET = 0xF8, TARGET_FX_FLAGS = 0x8C;
// The speech-challenge tint 0x1043EF0 gives types 3 and 7: iDialogueSC_Hard_Tint (red).
const std::uint32_t CHALLENGE_HARD = 3;
const std::uint32_t FX_BACKGROUND_QUAD = 1;
const std::uint32_t MENU_NO_COLOR_BACKGROUND = 1;
const float FULL_QUAD = 1.0f;
const std::uint16_t END = 0xFFFF;
// Our links folder, one file per plugin that has options (a mod or the converter writes it).
const char *LINKS = "Data\\F4SE\\Plugins\\CleanroomDialogue\\";
// The converter's folder from before the dialogue menu was its own release: still read, logged DEPRECATED, until
// the converter writes ours.
const char *LEGACY_LINKS = "Data\\F4SE\\Plugins\\F3NV_Menus\\";
// The links of the XDI-era hub plugin (tools.dialog.menu_rebuild), from before each build wrote its own: still read,
// logged DEPRECATED. Builds and the deploy delete it.
const char *LEGACY_HUB = "f3nv_menuhub.txt";

// The engine functions, resolved by HookDialogue; null until then or when the runtime lacks one.
struct Engine {
    OptionInfoFn optionInfo = nullptr;
    ResponseFn responseInfo = nullptr;
    PickFn pick = nullptr;
    PassesFn passes = nullptr;
    PromptFn prompt = nullptr;
    SaidFn markSaid = nullptr;
    AliasFn aliasHandle = nullptr;
    SelectFn selectOption = nullptr;
    StopFn stopScene = nullptr;
    StaticNameFn zoomInName = nullptr, zoomOutName = nullptr;
    ShadeFn enableShadedBackground = nullptr;
    TintFn tintChallenge = nullptr;
    std::uintptr_t shaderTargetVtable = 0;
};
Engine g_engine;

struct Option {
    TESForm *info;
    TESForm *answer;
    std::string prompt;
    bool said;
    bool failed;  // its skill check fails for the player now
};

// An option's skill check from its link: the player's actor value and the number it needs.
struct Check {
    RE::ActorValueInfo *av;
    std::uint32_t needed;
};

// The options last listed, for which action; the option picked from them.
struct Menu {
    void *action = nullptr;
    std::vector<Option> options;
};
// The answer line, once picked, and the action that asked for it: the engine asks that action's NPC slot again each
// frame until the NPC can say it (0x415AC5 waits while the NPC's 3D loads), and each ask must get the same line.
struct Chosen {
    void *action = nullptr;
    std::uint32_t index = 0;
    Option option{};
    TESForm *line = nullptr;
    void *answeredBy = nullptr;
    std::uint32_t asks = 0;
};

std::mutex g_dialogueLock;
Menu g_menu;
Chosen g_chosen;
// The waiting action last logged as listing none of ours (the menu asks again each frame for a while).
void *g_noneLogged = nullptr;
// Option INFO FormID -> answer topic FormID, read once the data has loaded.
std::unordered_map<std::uint32_t, std::uint32_t> g_links;
// Answer INFO FormID (or a legacy link's option INFO) -> the skill check its prompt carries.
std::unordered_map<std::uint32_t, Check> g_checks;
bool g_linksRead = false;
// The Player Dialogue action the engine ran last, and its scene.
std::atomic<void *> g_running{nullptr}, g_runningScene{nullptr};
RunFn g_run = nullptr;
ShouldFn g_firstShould = nullptr, g_thirdShould = nullptr;
bool g_hooked = false, g_speechPatch = false, g_playerSpeaks = true;
// The wait trace: the last state logged for our picked option, and when it was picked (0: none).
std::uint32_t g_lastWaits = ~0u;
std::chrono::steady_clock::time_point g_pickedAt{};

// Milliseconds since our option was picked.
long long SincePick() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - g_pickedAt).count();
}

// `plugin`'s object id `id` (hex) as a loaded FormID; 0 when the plugin is not loaded.
std::uint32_t Resolve(const std::string &plugin, const std::string &id) {
    auto *data = RE::TESDataHandler::GetSingleton();
    const RE::TESFile *mod = data ? data->LookupModByName(plugin) : nullptr;
    if (!mod || !mod->IsActive()) return 0;
    auto local = std::uint32_t(std::strtoul(id.c_str(), nullptr, 16));
    if (mod->IsLight()) return 0xFE000000 | (std::uint32_t(mod->GetSmallFileCompileIndex()) << 12) | (local & 0xFFF);
    return (std::uint32_t(mod->GetCompileIndex()) << 24) | (local & 0xFFFFFF);
}

// A skill check on INFO `info`: columns `at` to `at` + 2 of `f`, the actor value's plugin and id and the number needed.
void ReadCheck(std::uint32_t info, const std::vector<std::string> &f, std::size_t at, std::uint32_t *checks) {
    if (!info || f.size() < at + 3) return;
    std::uint32_t id = Resolve(f[at], f[at + 1]);
    TESForm *av = id ? TESForm::GetFormByID(id) : nullptr;
    if (!av || av->GetFormType() != RE::ENUM_FORM_ID::kAVIF) return;
    g_checks[info] = {static_cast<RE::ActorValueInfo *>(av), std::uint32_t(std::strtoul(f[at + 2].c_str(), nullptr, 10))};
    (*checks)++;
}

// One links file, tab-separated, # starting a comment. Four columns link an option to its answer topic (option
// plugin, option id, answer plugin, answer id); five name an answer INFO's skill check (INFO plugin, INFO id, skill
// plugin, skill id, needed). Seven, from older builds, are a link with its option's check.
void ReadLinks(const std::string &path) {
    std::ifstream in(path);
    std::string line;
    std::uint32_t read = 0, kept = 0, checks = 0;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::vector<std::string> f;
        for (std::size_t at = 0, tab; at <= line.size(); at = tab + 1) {
            tab = line.find('\t', at);
            tab = tab == std::string::npos ? line.size() : tab;
            f.push_back(line.substr(at, tab - at));
        }
        if (f.size() < 4) continue;
        if (f.size() == 5) {
            ReadCheck(Resolve(f[0], f[1]), f, 2, &checks);
            continue;
        }
        read++;
        std::uint32_t option = Resolve(f[0], f[1]), answer = Resolve(f[2], f[3]);
        TESForm *info = option ? TESForm::GetFormByID(option) : nullptr;
        TESForm *topic = answer ? TESForm::GetFormByID(answer) : nullptr;
        if (!info || !topic || info->GetFormType() != RE::ENUM_FORM_ID::kINFO ||
            topic->GetFormType() != RE::ENUM_FORM_ID::kDIAL)
            continue;
        g_links[option] = answer, kept++;
        ReadCheck(option, f, 4, &checks);
    }
    REX::INFO("dialogue menu: {}: {} links, {} resolved, {} skill checks", path, read, kept, checks);
}

// Read every links file in `folder`; how many were read.
std::uint32_t ReadFolder(const char *folder) {
    std::uint32_t files = 0;
    std::error_code error;
    for (const auto &entry : std::filesystem::directory_iterator(folder, error)) {
        std::string name = entry.path().filename().string();
        for (char &c : name) c = char(std::tolower(std::uint8_t(c)));
        if (!name.ends_with(".txt")) continue;
        if (name == LEGACY_HUB)
            REX::WARN("dialogue menu: DEPRECATED {} (the XDI-era hub; each plugin now writes its own links)", name);
        ReadLinks(entry.path().string());
        files++;
    }
    return files;
}

// Read every links file once (the data has loaded by the first menu): ours, then the converter's old folder.
void ReadAllLinks() {
    if (g_linksRead) return;
    g_linksRead = true;
    std::uint32_t ours = ReadFolder(LINKS), legacy = ReadFolder(LEGACY_LINKS);
    if (legacy) REX::WARN("dialogue menu: DEPRECATED {} ({} files): links now go in {}", LEGACY_LINKS, legacy, LINKS);
    if (!ours && !legacy) REX::INFO("dialogue menu: no links in {}", LINKS);
}

// A topic's INFOs in the engine's order.
std::vector<TESForm *> Infos(TESForm *topic) {
    std::vector<TESForm *> out;
    auto *infos = topic ? Field<TESForm **>(topic, TOPIC_INFOS) : nullptr;
    auto *order = topic ? Field<std::uint16_t *>(topic, TOPIC_ORDER) : nullptr;
    std::uint32_t count = topic ? Field<std::uint32_t>(topic, TOPIC_COUNT) : 0;
    if (!infos || !order) return out;
    for (std::uint16_t i = order[1]; i != END && i < count && out.size() < count; i = order[2 * i + 2])
        if (infos[i]) out.push_back(infos[i]);
    return out;
}

// The reference in `action`'s alias of `scene`'s quest (the NPC who answers).
RE::NiPointer<TESObjectREFR> ActionActor(void *action, void *scene) {
    RE::NiPointer<TESObjectREFR> ref;
    TESForm *quest = scene ? Field<TESForm *>(scene, SCENE_QUEST) : nullptr;
    if (!quest) return ref;
    std::uint32_t handle = 0;
    std::uint32_t *at = g_engine.aliasHandle(quest, &handle, Field<std::uint32_t>(action, ACTION_ALIAS));
    static_assert(sizeof(RE::ObjectRefHandle) == sizeof(handle));
    return RE::BSPointerHandleManagerInterface<TESObjectREFR>::GetSmartPointer(
        *reinterpret_cast<const RE::ObjectRefHandle *>(at ? at : &handle));
}

// `prompt` with its build-time tag "[Speech 25] " written as FNV writes it (FalloutNV.exe's "[%s", "%s %d/%d",
// "%s]  %s"): "[Speech 25]  text" when the player's value passes `check`, "[Speech 17/25]  text" when it fails.
std::string Tagged(const std::string &text, const Check &check, bool *failed) {
    std::size_t open = text.find_first_not_of(' '), close = text.find(']');
    std::size_t space = close == std::string::npos ? close : text.rfind(' ', close);
    if (open == std::string::npos || text[open] != '[' || space == std::string::npos || space <= open) return text;
    int value = int(RE::PlayerCharacter::GetSingleton()->GetActorValue(*check.av));
    *failed = value < int(check.needed);
    std::string name = text.substr(open + 1, space - open - 1);
    std::size_t rest = text.find_first_not_of(' ', close + 1);
    std::string tag = *failed ? std::format("[{} {}/{}]  ", name, value, check.needed)
                              : std::format("[{} {}]  ", name, check.needed);
    return tag + (rest == std::string::npos ? std::string() : text.substr(rest));
}

// The value of FNV text macro `name` (lower case) for the player, as FalloutNV.exe 0x7070C0 gives it: PCName the
// player's name, PCRace their race's name; none for any other (the build turns control macros into FO4 tags).
std::optional<std::string> MacroValue(const std::string &name) {
    RE::PlayerCharacter *player = RE::PlayerCharacter::GetSingleton();
    if (!player) return std::nullopt;
    RE::TESNPC *npc = player->GetNPC();
    if (name == "pcname" && npc) return std::string(npc->GetFullName());
    if (name == "pcrace" && player->race) return std::string(player->race->GetFullName());
    return std::nullopt;
}

// The lower-case name between `from` and the next ';' of `text`, and that ';' in `end`; "" when there is none.
std::string MacroName(const std::string &text, std::size_t from, std::size_t *end) {
    *end = text.find(';', from);
    if (*end == std::string::npos) return "";
    std::string name = text.substr(from, *end - from);
    for (char &c : name) c = char(std::tolower(std::uint8_t(c)));
    return name;
}

// `text` with FNV's player macros (&PCName; and &PCRace;, any case) written out, as FNV writes them in a prompt.
// Plugins built for XDI wrote the & as &amp; (&amp;PCName;), and are read the same.
std::string Expanded(std::string text) {
    std::size_t at = 0, end = 0;
    while ((at = text.find('&', at)) != std::string::npos) {
        std::string name = MacroName(text, at + 1, &end);
        if (end == std::string::npos) break;
        if (name == "amp") name = MacroName(text, end + 1, &end);
        if (end == std::string::npos) break;
        std::optional<std::string> value = MacroValue(name);
        if (!value) {
            at++;
            continue;
        }
        text.replace(at, end - at + 1, *value);
        at += value->size();
    }
    return text;
}

// An option as listed: the prompt of `line` (the answer line that plays) when it has one, else the option's, with
// FNV's player macros written out; tagged live with the skill check of that line, else of the option. Said (shown
// dim) when `line` was said: FNV keeps said on its INFO, the answer line here, not on the one option per topic.
Option Listed(TESForm *info, TESForm *answer, TESForm *line) {
    const char *own = g_engine.prompt(line);
    Option option{info, answer, Expanded(own && *own ? own : g_engine.prompt(info)),
                  Field<std::uint8_t>(line, INFO_SAID) != 0, false};
    auto check = g_checks.find(line->GetFormID());
    check = check != g_checks.end() ? check : g_checks.find(info->GetFormID());
    if (check != g_checks.end()) option.prompt = Tagged(option.prompt, check->second, &option.failed);
    return option;
}

// How far each line of a menu got: lines, linked, passing their own conditions, answered (listed).
struct Census {
    std::uint32_t lines = 0, linked = 0, passing = 0, answered = 0;
    std::uint32_t failed = 0;  // the first linked line whose conditions fail, or whose answer has no passing line
};

// The linked options of `action` that pass for the player talking to `npc` and whose answer topic has a line.
std::vector<Option> ListOptions(void *action, TESObjectREFR *npc, Census *census) {
    std::vector<Option> out;
    TESObjectREFR *player = RE::PlayerCharacter::GetSingleton();
    for (std::uint32_t slot = 0; slot < SLOTS; slot++)
        for (TESForm *info : Infos(Field<TESForm *>(action, ACTION_TOPICS + slot * sizeof(void *)))) {
            census->lines++;
            auto link = g_links.find(info->GetFormID());
            TESForm *answer = link == g_links.end() ? nullptr : TESForm::GetFormByID(link->second);
            bool flag = false;
            if (!answer) continue;
            census->linked++;
            bool passes = g_engine.passes(info, &flag, player, npc);
            census->passing += passes;
            TESForm *line = passes ? g_engine.pick(answer, npc, player) : nullptr;
            if (!line) {
                census->failed = census->failed ? census->failed : info->GetFormID();
                continue;
            }
            census->answered++;
            out.push_back(Listed(info, answer, line));
        }
    return out;
}

// Why a waiting menu lists none of ours: its scene, its NPC and how far its lines got.
void LogNoOptions(void *scene, TESObjectREFR *npc, const Census &census) {
    REX::DEBUG("dialogue menu: none of ours in scene {:08X} with NPC {:08X}: {} lines, {} linked, {} pass, {} answered; "
               "first failing linked line {:08X}", static_cast<TESForm *>(scene)->GetFormID(),
               npc ? npc->GetFormID() : 0, census.lines, census.linked, census.passing, census.answered, census.failed);
}

// The running Player Dialogue action of the player's scene, waiting for a pick; null when there is none.
void *WaitingAction(void **scene) {
    RE::PlayerCharacter *player = RE::PlayerCharacter::GetSingleton();
    void *action = g_running.load();
    *scene = g_runningScene.load();
    if (!player || !action || !*scene || player->GetCurrentScene() != *scene) return nullptr;
    return Field<std::uint32_t>(action, ACTION_OPTION) == NO_OPTION ? action : nullptr;
}

// A wheel step (ZoomIn / ZoomOut) while a Player Dialogue choice waits: the menu scrolls with it, not the camera.
bool WheelWhileChoosing(void *event) {
    void **name = (*reinterpret_cast<EventNameFn **>(event))[2](event);
    void *scene = nullptr;
    return name && (*name == *g_engine.zoomInName() || *name == *g_engine.zoomOutName()) && WaitingAction(&scene);
}

bool FirstPersonShould(void *state, void *event) {
    return !WheelWhileChoosing(event) && g_firstShould(state, event);
}

bool ThirdPersonShould(void *state, void *event) {
    return !WheelWhileChoosing(event) && g_thirdShould(state, event);
}

// Route a camera state's "should handle" (vtable slot 1) through `hook` when the slot still holds `vanilla`.
bool HookShould(std::uintptr_t vtable, std::uintptr_t vanilla, ShouldFn hook, ShouldFn *original) {
    if (!vtable || !vanilla) return false;
    auto *slot = reinterpret_cast<ShouldFn *>(vtable + sizeof(void *));
    if (std::uintptr_t(*slot) != vanilla) return false;
    *original = *slot;
    return REL::WriteSafe(std::uintptr_t(slot), &hook, sizeof(hook));
}

// Keep the wheel from the first- and third-person cameras while a choice waits.
bool HookWheel() {
    return g_engine.zoomInName && g_engine.zoomOutName &&
           HookShould(Address(FIRST_PERSON_VTABLE), Address(FIRST_PERSON_SHOULD), FirstPersonShould, &g_firstShould) &&
           HookShould(Address(THIRD_PERSON_VTABLE), Address(THIRD_PERSON_SHOULD), ThirdPersonShould, &g_thirdShould);
}

// Let the player say the picked option (vanilla) or not (ours: the NPC answers at once, as in FNV).
void PlayerSpeaks(bool on) {
    if (!g_speechPatch || g_playerSpeaks == on) return;
    REL::WriteSafe(Address(PLAYER_SPEAKS), on ? SPEAKS : OldGen() ? SILENT_OG : SILENT, sizeof(SPEAKS));
    std::uint8_t camera = on ? CAMERA_PLAYER : CAMERA_SPEAKER;
    REL::WriteSafe(Address(CAMERA_TO_PLAYER), &camera, sizeof(camera));
    g_playerSpeaks = on;
}

// The option picked through SelectNative when option `chosen` is ours (of `action`, unless null); false otherwise.
bool PickedOption(void *action, std::uint32_t chosen, Chosen *out) {
    std::lock_guard<std::mutex> hold(g_dialogueLock);
    if (chosen < FIRST_OURS || !g_chosen.action || (action && g_chosen.action != action) ||
        g_chosen.index != chosen - FIRST_OURS)
        return false;
    *out = g_chosen;
    return true;
}

// The player slot: our option's own INFO, marked said, which the player does not say.
TESForm *PlayerSlot(void *action, void *scene, TESObjectREFR *target, std::int32_t option) {
    std::uint32_t chosen = Field<std::uint32_t>(action, ACTION_OPTION);
    PlayerSpeaks(chosen < FIRST_OURS);
    if (chosen < FIRST_OURS) return g_engine.optionInfo(action, scene, target, option);
    Chosen picked;
    if (!PickedOption(action, chosen, &picked)) {
        REX::DEBUG("dialogue menu: option {} was not picked through the menu; none taken", chosen);
        Field<std::uint32_t>(action, ACTION_OPTION) = NO_OPTION;
        return nullptr;
    }
    g_engine.markSaid(picked.option.info);
    REX::DEBUG("dialogue menu: picked {}, {:08X} '{}', answer topic {:08X}", picked.index,
               picked.option.info->GetFormID(), picked.option.prompt, picked.option.answer->GetFormID());
    return picked.option.info;
}

// Whether `actor`'s 3D is loading (TESObjectREFR model state 1), which holds its line back (0x2C6E60).
bool Loading3D(TESObjectREFR *actor) {
    return actor && Field<std::uint8_t>(actor, REFR_MODEL_STATE) == MODEL_LOADING;
}

// The answer line already picked for our option: again for the action it went to (the engine asks until the NPC
// can say it), none for another action. Logs the first ask again and every hundredth.
TESForm *Again(const Chosen &picked, void *action, void *scene) {
    if (picked.answeredBy != action) return nullptr;
    std::uint32_t asks;
    {
        std::lock_guard<std::mutex> hold(g_dialogueLock);
        asks = g_chosen.line == picked.line ? ++g_chosen.asks : 0;
    }
    RE::NiPointer<TESObjectREFR> npc = ActionActor(action, scene);
    if (asks == 1 || asks % 100 == 0)
        REX::DEBUG("dialogue menu: +{} ms: asked again ({}) for {:08X}; NPC 3D loading {}", SincePick(), asks,
                   picked.line->GetFormID(), Loading3D(npc.get()));
    return picked.line;
}

// The answer to our option `chosen` (of Player Dialogue action `owner`, unless null), said by `action`'s actor: its
// answer topic's first passing INFO, picked once and given to that action until it is said.
TESForm *Answer(void *owner, void *action, void *scene, std::uint32_t chosen, const char *where) {
    Chosen picked;
    if (!PickedOption(owner, chosen, &picked)) return nullptr;
    if (picked.line) return Again(picked, action, scene);
    RE::NiPointer<TESObjectREFR> npc = ActionActor(action, scene);
    TESForm *info = npc ? g_engine.pick(picked.option.answer, npc.get(), RE::PlayerCharacter::GetSingleton()) : nullptr;
    if (info) {
        std::lock_guard<std::mutex> hold(g_dialogueLock);
        if (g_chosen.action == picked.action) g_chosen.line = info, g_chosen.answeredBy = action;
    }
    REX::DEBUG("dialogue menu: {} answers {} from topic {:08X} with {:08X}", where, picked.index,
               picked.option.answer->GetFormID(), info ? info->GetFormID() : 0);
    return info;
}

// The NPC slot of a Player Dialogue action.
TESForm *NpcSlot(void *action, void *scene, TESObjectREFR *target, std::int32_t option) {
    std::uint32_t chosen = Field<std::uint32_t>(action, ACTION_OPTION);
    if (chosen < FIRST_OURS) return g_engine.optionInfo(action, scene, target, option);
    return Answer(action, action, scene, chosen, "NPC slot");
}

// An NPC Response action after a Player Dialogue one: our option's answer when its NPC slot gave none.
TESForm *Response(void *action, void *scene) {
    std::uint32_t chosen = Field<std::uint32_t>(RE::PlayerCharacter::GetSingleton(), PLAYER_RESPONSE);
    if (chosen < FIRST_OURS) return g_engine.responseInfo(action, scene);
    return Answer(nullptr, action, scene, chosen, "NPC Response");
}

// Whether `actor` is "talking" by the test the run function waits on.
bool Talking(void *actor) {
    return actor && (*reinterpret_cast<TalkingFn **>(actor))[TALKING_SLOT](actor);
}

// While our picked option's action runs, through the NPC's line: each change in what the NPC slot waits on
// (0x415954, 0x415AC5), with the time since the pick. The NPC talking turning 1 is when the answer starts.
void TraceWaits(void *action, void *scene) {
    RE::PlayerCharacter *player = RE::PlayerCharacter::GetSingleton();
    if (!player || g_pickedAt == std::chrono::steady_clock::time_point{} || g_chosen.action != action) return;
    RE::NiPointer<TESObjectREFR> npc = ActionActor(action, scene);
    std::uint32_t flags = Field<std::uint32_t>(action, ACTION_FLAGS);
    std::uint32_t waits = (flags & PLAYER_DONE ? 1 : 0) | (flags & NPC_SAID ? 2 : 0) | (Talking(player) ? 4 : 0) |
                          (Field<std::uint32_t>(player, PLAYER_NO_WAIT) & 2 ? 8 : 0) | (Talking(npc.get()) ? 16 : 0) |
                          (Loading3D(npc.get()) ? 32 : 0);
    if (waits == g_lastWaits) return;
    g_lastWaits = waits;
    REX::DEBUG("dialogue menu: +{} ms: player line {}, NPC line {}, player talking {} (no-wait flag {}), NPC talking {}"
               ", NPC 3D loading {}", SincePick(), waits & 1 ? "done" : "pending", waits & 2 ? "said" : "not said",
               (waits & 4) != 0, (waits & 8) != 0, (waits & 16) != 0, (waits & 32) != 0);
}

// A Player Dialogue action's run: remember it as the running one.
std::uint64_t Run(void *action, void *scene, void *third, void *fourth) {
    g_running = action;
    g_runningScene = scene;
    TraceWaits(action, scene);
    return g_run(action, scene, third, fourth);
}

// End the player's conversation as Papyrus Scene.Stop does (run on the game's thread).
void Leave() {
    RE::PlayerCharacter *player = RE::PlayerCharacter::GetSingleton();
    RE::BGSScene *scene = player ? player->GetCurrentScene() : nullptr;
    if (!scene) {
        REX::DEBUG("dialogue menu: leave: the player is in no scene");
        return;
    }
    g_engine.stopScene(scene);
    Field<std::uint32_t>(scene, SCENE_FLAGS) |= SCENE_STOPPING;
    REX::DEBUG("dialogue menu: left the talk (stopped scene {:08X})", reinterpret_cast<TESForm *>(scene)->GetFormID());
}

// The call at `site`'s target as `Fn` when it still calls `vanilla`; null otherwise.
template <class Fn>
Fn Target(std::uintptr_t site, std::uintptr_t vanilla) {
    return CallsTarget(site, vanilla) ? reinterpret_cast<Fn>(vanilla) : nullptr;
}

// Every engine function into g_engine; false when the runtime's database lacks one.
bool ResolveEngine() {
    g_engine = {At<OptionInfoFn>(OPTION_INFO), At<ResponseFn>(RESPONSE_INFO), At<PickFn>(PICK), At<PassesFn>(PASSES),
                At<PromptFn>(PROMPT), At<SaidFn>(MARK_SAID), At<AliasFn>(ALIAS_HANDLE),
                At<SelectFn>(SELECT_OPTION), At<StopFn>(STOP_SCENE), At<StaticNameFn>(ZOOM_IN_NAME),
                At<StaticNameFn>(ZOOM_OUT_NAME), At<ShadeFn>(ENABLE_SHADED_BACKGROUND), At<TintFn>(TINT_CHALLENGE),
                Address(SHADER_TARGET_VTABLE)};
    return g_engine.optionInfo && g_engine.responseInfo && g_engine.pick && g_engine.passes && g_engine.prompt &&
           g_engine.markSaid && g_engine.aliasHandle && g_engine.selectOption &&
           g_engine.stopScene;
}

// ---- The menu's natives (f4se.plugins.CleanroomDialogue in the menu's movie) ----

// GetDialogueOptions(): [{text, said, failed}] of the running action's options; [] when it has none of ours.
class GetOptionsNative : public FunctionHandler {
public:
    void Call(const Params &params) override {
        params.movie->CreateArray(params.retVal);
        void *scene = nullptr, *action = g_hooked ? WaitingAction(&scene) : nullptr;
        if (!action) return;
        std::lock_guard<std::mutex> hold(g_dialogueLock);
        ReadAllLinks();
        RE::NiPointer<TESObjectREFR> npc = ActionActor(action, scene);
        Census census;
        g_menu.action = action;
        g_menu.options = npc ? ListOptions(action, npc.get(), &census) : std::vector<Option>();
        if (g_menu.options.empty() && g_noneLogged != action) LogNoOptions(scene, npc.get(), census);
        g_noneLogged = g_menu.options.empty() ? action : nullptr;
        for (std::uint32_t i = 0; i < g_menu.options.size(); i++) {
            const Option &o = g_menu.options[i];
            Value entry;
            params.movie->CreateObject(&entry);
            entry.SetMember("text", String(params.movie, o.prompt.c_str()));
            entry.SetMember("said", Value(o.said));
            entry.SetMember("failed", Value(o.failed));
            params.retVal->PushBack(entry);
            REX::DEBUG("dialogue menu: option {}: {:08X} '{}' -> topic {:08X}{}", i, o.info->GetFormID(), o.prompt,
                       o.answer->GetFormID(), o.said ? " (said)" : "");
        }
        if (!g_menu.options.empty()) REX::DEBUG("dialogue menu: {} options listed", g_menu.options.size());
    }
};

// SelectDialogueOption(index): pick listed option `index`; false when it is not listed or the action has moved on.
class SelectNative : public FunctionHandler {
public:
    void Call(const Params &params) override {
        *params.retVal = false;
        if (params.argCount < 1 || !g_hooked) return;
        auto index = std::uint32_t(Num(params.args[0]));
        void *scene = nullptr, *action = WaitingAction(&scene);
        {
            std::lock_guard<std::mutex> hold(g_dialogueLock);
            if (!action || action != g_menu.action || index >= g_menu.options.size()) return;
            g_chosen = {action, index, g_menu.options[index]};
            g_pickedAt = std::chrono::steady_clock::now();
            g_lastWaits = ~0u;
        }
        g_engine.selectOption(RE::PlayerCharacter::GetSingleton(), index + FIRST_OURS);
        *params.retVal = true;
    }
};

// LeaveDialogue(): end the talk (the menu's Tab, Esc and [Leave] when no option of ours is up).
class LeaveNative : public FunctionHandler {
public:
    void Call(const Params &params) override {
        const F4SE::TaskInterface *tasks = g_engine.stopScene ? F4SE::GetTaskInterface() : nullptr;
        *params.retVal = tasks != nullptr;
        if (tasks) tasks->AddTask(Leave);
    }
};

// The DialogueMenu running `movie` and its holder's shader target at `slot`; nulls when either is missing.
void *HolderTarget(Scaleform::GFx::Movie *movie, std::size_t slot, RE::IMenu **out) {
    auto *ui = RE::UI::GetSingleton();
    Scaleform::Ptr<RE::IMenu> menu = ui ? ui->GetMenu("DialogueMenu"sv) : nullptr;
    *out = menu && menu->uiMovie.get() == movie ? menu.get() : nullptr;
    void *target = *out ? Field<void *>(*out, slot) : nullptr;
    return target && g_engine.shaderTargetVtable && *static_cast<std::uintptr_t *>(target) == g_engine.shaderTargetVtable
               ? target
               : nullptr;
}

// ShadeList(on): the game's translucent menu background behind the Question holder, where the list lives, on or
// off. A fill inside a HUD-colored holder draws opaque (its alpha only darkens it), so the backdrop comes from here.
class ShadeNative : public FunctionHandler {
public:
    void Call(const Params &params) override {
        bool on = params.argCount > 0 && params.args[0].IsBoolean() && params.args[0].GetBoolean();
        RE::IMenu *menu = nullptr;
        void *target = g_engine.enableShadedBackground ? HolderTarget(params.movie, MENU_QUESTION_TARGET, &menu) : nullptr;
        if (target && on) g_engine.enableShadedBackground(target, MENU_NO_COLOR_BACKGROUND, FULL_QUAD);
        if (target && !on) Field<std::uint32_t>(target, TARGET_FX_FLAGS) &= ~FX_BACKGROUND_QUAD;
        if (!target) REX::WARN("dialogue menu: no shader target for the list's background");
        *params.retVal = target != nullptr;
    }
};

// TintFailed(): the Negative holder, where the menu puts its failed skill checks, in the game's own speech-challenge
// Hard tint. The game re-tints the holders on each SetButtonText, so the menu calls this after them.
class TintFailedNative : public FunctionHandler {
public:
    void Call(const Params &params) override {
        RE::IMenu *menu = nullptr;
        void *target = g_engine.tintChallenge ? HolderTarget(params.movie, MENU_NEGATIVE_TARGET, &menu) : nullptr;
        if (target) g_engine.tintChallenge(menu, target, CHALLENGE_HARD, false);
        else REX::WARN("dialogue menu: no shader target for failed skill checks");
        *params.retVal = target != nullptr;
    }
};

// Log(text): a line from the menu in CleanroomDialogue.log, with the time since the pick once one is made.
class LogNative : public FunctionHandler {
public:
    void Call(const Params &params) override {
        if (params.argCount < 1 || !params.args[0].IsString()) return;
        if (g_pickedAt != std::chrono::steady_clock::time_point{})
            REX::DEBUG("dialogue menu: +{} ms: {}", SincePick(), params.args[0].GetString());
        else REX::DEBUG("dialogue menu: {}", params.args[0].GetString());
    }
};

}  // namespace

bool RegisterDialogueMenu(Scaleform::GFx::Movie *view, Value *plugin) {
    if (!view || !plugin) return false;
    RegisterSharedNatives(view, plugin);
    RegisterFunction<GetOptionsNative>(plugin, view, "GetDialogueOptions");
    RegisterFunction<SelectNative>(plugin, view, "SelectDialogueOption");
    RegisterFunction<LogNative>(plugin, view, "Log");
    RegisterFunction<LeaveNative>(plugin, view, "LeaveDialogue");
    RegisterFunction<ShadeNative>(plugin, view, "ShadeList");
    RegisterFunction<TintFailedNative>(plugin, view, "TintFailed");
    return true;
}

bool HookDialogue() {
    if (REX::W32::GetModuleHandleA("XDI.dll")) {
        REX::INFO("dialogue menu: XDI.dll is loaded; leaving dialogue to XDI");
        return false;
    }
    if (!CheckedRuntime()) {
        REX::WARN("dialogue menu: not hooked on {}: its engine layouts are read in 1.11.240 only", RuntimeName());
        return false;
    }
    if (!ResolveEngine()) {
        REX::WARN("dialogue menu: not hooked: the Address Library database lacks a function");
        return false;
    }
    std::uintptr_t playerCall = Address(PLAYER_SLOT_CALL), npcCall = Address(NPC_SLOT_CALL);
    std::uintptr_t responseCall = Address(RESPONSE_CALL), vtable = Address(PLAYER_DIALOGUE_VTABLE);
    bool player = Target<OptionInfoFn>(playerCall, std::uintptr_t(g_engine.optionInfo)) != nullptr;
    bool npc = CallsTarget(npcCall, std::uintptr_t(g_engine.optionInfo));
    bool response = CallsTarget(responseCall, std::uintptr_t(g_engine.responseInfo));
    if (!player || !npc || !response || !vtable) {
        REX::WARN("dialogue menu: a call site no longer calls the vanilla function (player {}, NPC {}, response {}); "
                  "not hooked", player, npc, response);
        return false;
    }
    std::uintptr_t speaks = Address(PLAYER_SPEAKS), camera = Address(CAMERA_TO_PLAYER);
    g_speechPatch = speaks && camera && !std::memcmp(reinterpret_cast<void *>(speaks), SPEAKS, sizeof(SPEAKS)) &&
                    *reinterpret_cast<std::uint8_t *>(camera) == CAMERA_PLAYER;
    auto &trampoline = REL::GetTrampoline();
    trampoline.write_call<5>(playerCall, PlayerSlot);
    trampoline.write_call<5>(npcCall, NpcSlot);
    trampoline.write_call<5>(responseCall, Response);
    auto *slot = reinterpret_cast<RunFn *>(vtable + RUN_SLOT * sizeof(void *));
    g_run = *slot;
    RunFn run = Run;
    g_hooked = REL::WriteSafe(std::uintptr_t(slot), &run, sizeof(run));
    REX::INFO("dialogue menu: {} on {}, player line skip {}, wheel zoom {}", g_hooked ? "hooked" : "not hooked",
              RuntimeName(), g_speechPatch ? "ready" : "unavailable (code differs)",
              g_hooked && HookWheel() ? "off the camera while choosing" : "unchanged (code differs)");
    return g_hooked;
}

}  // namespace crui
