// FO3/FNV SexChange on Fallout 4 (docs/fork/fo4_pipeline.md#sex-change).
//
// FO4 Papyrus has no sex setter. The game's own SexChange script command (Fallout4.exe 1.11.240: opcode 0x11E6,
// handler 0x5C5B00) takes an optional sex and an optional integer; given no arguments it toggles the actor's sex
// (the base's female flag, the head rebuilt, the 3D reset), which is what the console's `sexchange` does. The
// native CRUI_Sex.ChangeSex queues that handler on the main thread with an argument count of 0; the caller
// (TES4Polyfill.SetSex) only calls it when the sex differs.
#include "condition.h"

namespace crui {

namespace {

const char *SEX_SCRIPT = "CRUI_Sex";

// Run SexChange with no arguments on the actor `formID` names, on the main thread.
void RunSexChange(std::uint32_t formID) {
    static RE::SCRIPT_FUNCTION *command = RE::SCRIPT_FUNCTION::LocateScriptCommand("SexChange");
    auto *actor = FormOf(formID) ? FormOf(formID)->As<RE::Actor>() : nullptr;
    if (!actor || !command || !command->executeFunction) {
        REX::WARN("SexChange on {:08X} not run (actor {}, command {})", formID, actor != nullptr, command != nullptr);
        return;
    }
    char data[8] = {};  // the command's compiled arguments: a count of 0 at offset 0
    std::uint32_t offset = 0;
    double result = 0.0;  // room for the double F4SE's own ObScript declaration writes; CommonLibF4 says float
    command->executeFunction(command->parameters, data, actor, nullptr, nullptr, nullptr,
                             *reinterpret_cast<float *>(&result), offset);
    REX::DEBUG("SexChange on {:08X} run", formID);
}

// Papyrus: toggle `actor`'s sex (the game's SexChange with no arguments), on the main thread's next task.
void ChangeSex(std::monostate, RE::Actor *actor) {
    if (!actor || !Tasks()) return;
    std::uint32_t formID = actor->GetFormID();
    Tasks()->AddTask([formID] { RunSexChange(formID); });
}

}  // namespace

void RegisterSex(RE::BSScript::IVirtualMachine *vm) {
    vm->BindNativeMethod(SEX_SCRIPT, "ChangeSex", ChangeSex);
}

}  // namespace crui
