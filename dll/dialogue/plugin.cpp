// CleanroomDialogue.dll: the F4SE plugin for our dialogue menu alone (dialogue.cpp; docs/dialogue_menu_mod.md).
// It needs no other DLL or plugin of ours: the menu movie (Interface/DialogueMenu.swf) reaches it as
// f4se.plugins.CleanroomDialogue, its words come from its own translation file, and its links from
// Data\F4SE\Plugins\CleanroomDialogue\. Built against F4SE 0.7.9 for Fallout4.exe 1.11.240.
#include "dialogue.h"

#include "f4se/PluginAPI.h"
#include "f4se/ScaleformLoader.h"
#include "f4se/ScaleformState.h"
#include "f4se/ScaleformTranslator.h"
#include "f4se/Translation.h"
#include "f4se_common/BranchTrampoline.h"

#include <string>
#include "f4se_common/f4se_version.h"

namespace {

using namespace crui;

const char *NAME = "CleanroomDialogue";
// dialogue.cpp's three calls.
const size_t TRAMPOLINE_SIZE = 64;
PluginHandle g_handle = kPluginHandle_Invalid;
void *g_tasks = nullptr;
bool g_trampoline = false;

// Take F4SE's branch pool for our calls; false when it cannot.
bool ReserveTrampoline(const F4SEInterface *f4se) {
    auto *trampolines = static_cast<F4SETrampolineInterface *>(f4se->QueryInterface(kInterface_Trampoline));
    void *pool = trampolines ? trampolines->AllocateFromBranchPool(g_handle, TRAMPOLINE_SIZE) : nullptr;
    if (!pool) return false;
    g_branchTrampoline.SetBase(TRAMPOLINE_SIZE, pool);
    return true;
}

// Add our words (Interface\Translations\CleanroomDialogue_<sLanguage>.txt) to the game's translator, once: F4SE
// reads one file per active plugin, and this release ships none. False while the game has no Scaleform manager.
bool LoadWords() {
    static bool loaded = false;
    BSScaleformManager *manager = *g_scaleformManager;
    if (loaded || !manager || !manager->stateBag) return loaded;
    loaded = true;
    auto *translator = static_cast<BSScaleformTranslator *>(
        manager->stateBag->GetStateAddRef(GFxState::kInterface_Translator));
    if (!translator) {
        _WARNING("no translator: the menu shows its $CRUI_ keys");
        return true;
    }
    std::string name = NAME;
    Translation::ParseTranslation(translator, name);
    _MESSAGE("loaded the words of %s", NAME);
    return true;
}

// F4SE messages: hook dialogue once every plugin has loaded, so XDI.dll shows; load our words once the game has
// made its translator (by the time input loads, else when the game has loaded).
void OnMessage(F4SEMessagingInterface::Message *msg) {
    if (msg->type == F4SEMessagingInterface::kMessage_PostLoad && g_trampoline) HookDialogue(g_tasks);
    if (msg->type == F4SEMessagingInterface::kMessage_InputLoaded ||
        msg->type == F4SEMessagingInterface::kMessage_GameLoaded)
        LoadWords();
}

}  // namespace

extern "C" {

__declspec(dllexport) F4SEPluginVersionData F4SEPlugin_Version = {
    F4SEPluginVersionData::kVersion,
    1,
    "CleanroomDialogue",
    "cleanroom-ui",
    0,
    0,
    {RUNTIME_VERSION_1_11_240, 0},
    0,
};

__declspec(dllexport) bool F4SEPlugin_Load(const F4SEInterface *f4se) {
    OpenLog(NAME);
    g_handle = f4se->GetPluginHandle();
    _MESSAGE("%s loaded, runtime %08X", NAME, f4se->runtimeVersion);
    g_tasks = f4se->QueryInterface(kInterface_Task);
    g_trampoline = ReserveTrampoline(f4se);
    auto *scaleform = static_cast<F4SEScaleformInterface *>(f4se->QueryInterface(kInterface_Scaleform));
    if (!scaleform || !scaleform->Register(NAME, RegisterDialogueMenu)) _WARNING("no dialogue menu natives");
    auto *messaging = static_cast<F4SEMessagingInterface *>(f4se->QueryInterface(kInterface_Messaging));
    if (!g_trampoline || !g_tasks || !messaging || !messaging->RegisterListener(g_handle, "F4SE", OnMessage))
        _WARNING("dialogue menu off (trampoline %d, task %p, messaging %p)", g_trampoline, g_tasks, messaging);
    return true;
}

}
