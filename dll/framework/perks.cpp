// The game's own perk points, for a level-up of a mod's own (docs/level_up_plan.md#game-points).
//
// FO4 adds a perk point at each level and the Pip-Boy's LEVEL UP notice shows that count (the PlayerInfo
// PerkPoints of pipboymenu.swf); a level-up with points of its own never spends it. Papyrus can only add to it
// (Game.AddPerkPoints, Fallout4.exe 1.11.240 RVA 0x1121690, returns unless the amount is above 0). The game's
// setter, which that native ends in, is RVA 0xD75FF0 (PlayerCharacter *, UInt8): it writes the count at
// PlayerCharacter +0xCF1 and sends the game's perk points event, which the Pip-Boy reads. The native
// CRUI_LevelUpNative.SetGamePerkPoints queues that setter on the main thread.
#include "condition.h"

#include "f4se/GameReferences.h"
#include "f4se/GameThreads.h"
#include "f4se/PapyrusNativeFunctions.h"
#include "f4se/PluginAPI.h"

namespace crui {

namespace {

const char *LEVEL_UP_SCRIPT = "CRUI_LevelUpNative";
using SetPerkCount = void (*)(PlayerCharacter *player, UInt8 count);
RelocAddr<SetPerkCount> SetPlayerPerkCount(0xD75FF0);
F4SETaskInterface *g_perkTasks = nullptr;

// The game's setter run with `count`, on the main thread.
class PerkCountTask : public ITaskDelegate {
public:
    explicit PerkCountTask(UInt8 count) : count_(count) {}

    void Run() override {
        if (*g_player) SetPlayerPerkCount(*g_player, count_);
    }

private:
    UInt8 count_;
};

// Papyrus: the game's perk points set to `points` (0 to 255) on the main thread's next task.
void SetGamePerkPoints(StaticFunctionTag *, SInt32 points) {
    UInt8 count = static_cast<UInt8>(points < 0 ? 0 : points > 255 ? 255 : points);
    if (g_perkTasks) g_perkTasks->AddTask(new PerkCountTask(count));
}

}  // namespace

void RegisterPerks(VirtualMachine *vm, void *tasks) {
    g_perkTasks = static_cast<F4SETaskInterface *>(tasks);
    vm->RegisterFunction(new NativeFunction1<StaticFunctionTag, void, SInt32>("SetGamePerkPoints", LEVEL_UP_SCRIPT,
                                                                             SetGamePerkPoints, vm));
    vm->SetFunctionFlags(LEVEL_UP_SCRIPT, "SetGamePerkPoints", IFunction::kFunctionFlag_NoWait);
}

}  // namespace crui
