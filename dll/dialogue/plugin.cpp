// CleanroomDialogue.dll: the F4SE plugin for our dialogue menu alone (dialogue.cpp; docs/dialogue_menu_mod.md).
// It needs no other DLL or plugin of ours: the menu movie (Interface/DialogueMenu.swf) reaches it as
// f4se.plugins.CleanroomDialogue, its words come from its own translation file, and its links from
// Data\F4SE\Plugins\CleanroomDialogue\. Built on CommonLibF4 (xmake.lua), for every runtime with an Address Library
// database; its hooks run on the one its layouts were read in (base.h CheckedRuntime).
#include "dialogue.h"

namespace {

using namespace crui;

const char *NAME = "CleanroomDialogue";
// dialogue.cpp's three calls, 14 bytes each in CommonLibF4's trampoline.
const std::size_t TRAMPOLINE_SIZE = 64;

// Add our words (Interface\Translations\CleanroomDialogue_<sLanguage>.txt) to the game's translator, once: F4SE
// reads one file per active plugin, and this release ships none. False while the game has no translator yet.
bool LoadWords() {
    static bool loaded = false;
    auto *manager = RE::BSScaleformManager::GetSingleton();
    if (loaded || !manager || !manager->loader) return loaded;
    loaded = true;
    RE::BSScaleformTranslator *translator = manager->GetTranslator();
    if (!translator) {
        REX::WARN("no translator: the menu shows its $CRUI_ keys");
        return true;
    }
    translator->AddTranslationsMod(NAME);
    translator->Release();
    REX::INFO("loaded the words of {}", NAME);
    return true;
}

// F4SE messages: hook dialogue once every plugin has loaded, so XDI.dll shows; load our words once the game has
// made its translator (by the time input loads, else when the game has loaded).
void OnMessage(F4SE::MessagingInterface::Message *msg) {
    if (msg->type == F4SE::MessagingInterface::kPostLoad) HookDialogue();
    if (msg->type == F4SE::MessagingInterface::kInputLoaded || msg->type == F4SE::MessagingInterface::kGameLoaded)
        LoadWords();
}

}  // namespace

// Old-gen F4SE asks this before loading a plugin (next-gen reads F4SEPlugin_Version instead).
F4SE_PLUGIN_QUERY(const F4SE::QueryInterface *a_f4se, F4SE::PluginInfo *a_info) {
    if (const auto *data = F4SE::PluginVersionData::GetSingleton()) {
        a_info->infoVersion = F4SE::PluginInfo::kVersion;
        a_info->name = data->GetPluginName().data();
        a_info->version = data->GetPluginVersion().pack();
    }
    return a_f4se->RuntimeVersion() >= REL::Version(F4SE::RUNTIME_1_10_163);
}

F4SE_PLUGIN_LOAD(const F4SE::LoadInterface *a_f4se) {
    F4SE::Init(a_f4se, {.logLevel = LogLevel(NAME), .logName = NAME, .trampoline = true,
                        .trampolineSize = TRAMPOLINE_SIZE});
    REX::INFO("{} loaded on {}", NAME, RuntimeName());
    const F4SE::ScaleformInterface *scaleform = F4SE::GetScaleformInterface();
    if (!scaleform || !scaleform->Register(NAME, RegisterDialogueMenu)) REX::WARN("no dialogue menu natives");
    const F4SE::MessagingInterface *messaging = F4SE::GetMessagingInterface();
    if (!F4SE::GetTaskInterface() || !messaging || !messaging->RegisterListener(OnMessage))
        REX::WARN("dialogue menu off (no task or messaging interface)");
    return true;
}
